#include <tvterm/termctrl.h>

#define Uses_TEventQueue
#include <tvision/tv.h>

#include <condition_variable>
#include <chrono>
#include <thread>
#include <mutex>
#include <queue>

namespace tvterm
{

class GrowArrayWriter final : public Writer
{
public:

    GrowArray buffer;

    void write(TSpan<const char> data) noexcept override
    {
        buffer.push(&data[0], data.size());
    }
};

// In order to support Windows pseudoconsoles, we need different threads for
// reading and writing client data, since we are expected to use blocking pipes.
// In Unix, on the other hand, the most straightforward implementation is
// probably to use something like 'select' to coordinate reads and writes in a
// single thread. But the model required by Windows also works on Unix, so we
// just use it everywhere.

struct TerminalController::TerminalEventLoop
{
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    enum { maxReadTimeMs = 20, readWaitStepMs = 5 };
    enum { readBufSize = 4096 };

    TerminalController &ctrl;

    // Used for granting exclusive access to TerminalEventLoop's fields and the
    // TerminalEmulator.
    std::mutex mutex;

    // Used for indicating whether event processing should be stopped because
    // the terminal was shut down by the application.
    bool terminated {false};

    // Used for sending events from the main thread to the TerminalEventLoop's
    // threads. Has its own mutex to avoid blocking the main thread for long.
    Mutex<std::queue<TerminalEvent>> eventQueue;

    // Used for waking up the WriterLoop a short time after data was received
    // by the ReaderLoop thread.
    TimePoint currentTimeout {};
    TimePoint maxReadTimeout {};

    // Used for waking up the WriterLoop thread on demand, e.g. when there are
    // pending events or when the timeouts change.
    std::condition_variable condVar;

    // Used for storing data to be sent to the client.
    GrowArrayWriter clientDataWriter;

    // Used for handling ViewportResize events properly.
    bool viewportResized {false};
    TPoint viewportSize {};

    void runWriterLoop() noexcept;
    void runReaderLoop() noexcept;
    void processEvents() noexcept;
    void updateState(bool &) noexcept;
    void updateTimeouts() noexcept;

    void writePendingData(GrowArray &, bool &) noexcept;
    void notifyMainThread() noexcept;
};

TerminalController *TerminalController::create( TPoint size,
                                                TerminalEmulatorFactory &terminalEmulatorFactory,
                                                void (&onError)(const char *) ) noexcept
{
    PtyDescriptor ptyDescriptor;
    if ( !createPty( ptyDescriptor, size,
                     terminalEmulatorFactory.getCustomEnvironment(), onError ) )
        return nullptr;

    auto &terminalController = *new TerminalController( size,
                                                        terminalEmulatorFactory,
                                                        ptyDescriptor );

    // 'this' will be deleted when:
    // 1. 'shutDown()' is invoked from the main thread.
    // 2. Both the WriterLoop and ReaderLoop threads exit.
    auto deleter = [] (TerminalController *ctrl) {
        delete ctrl;
    };
    terminalController.selfOwningPtr.reset(&terminalController, deleter);

    std::thread([owningPtr = terminalController.selfOwningPtr] {
        owningPtr->eventLoop.runWriterLoop();
    }).detach();

    std::thread([owningPtr = terminalController.selfOwningPtr] {
        owningPtr->eventLoop.runReaderLoop();
    }).detach();

    return &terminalController;
}

void TerminalController::shutDown() noexcept
{
    {
        std::lock_guard<std::mutex> lock(eventLoop.mutex);
        eventLoop.terminated = true;
    }
    eventLoop.condVar.notify_one();
    selfOwningPtr.reset(); // May delete 'this'.
}

TerminalController::TerminalController( TPoint size,
                                        TerminalEmulatorFactory &terminalEmulatorFactory,
                                        PtyDescriptor ptyDescriptor ) noexcept :
    ptyMaster(ptyDescriptor),
    eventLoop(*new TerminalEventLoop {*this}),
    terminalEmulator(terminalEmulatorFactory.create(size, eventLoop.clientDataWriter))
{
}

TerminalController::~TerminalController()
{
    delete &terminalEmulator;
    delete &eventLoop;
}

void TerminalController::sendEvent(const TerminalEvent &event) noexcept
{
    eventLoop.eventQueue.lock([&] (auto &eventQueue) {
        eventQueue.push(event);
    });
    eventLoop.condVar.notify_one();
}

void TerminalController::TerminalEventLoop::runWriterLoop() noexcept
{
    GrowArray outputBuffer;
    while (true)
    {
        bool updated = false;
        {
            std::unique_lock<std::mutex> lock(mutex);
            if (currentTimeout != TimePoint())
                condVar.wait_until(lock, currentTimeout);
            else
                // Waiting until 'TimePoint()' is not always supported,
                // so use a regular 'wait'.
                condVar.wait(lock);

            if (terminated)
            {
                ctrl.ptyMaster.disconnect();
                break;
            }

            processEvents();
            updateState(updated);

            outputBuffer = std::move(clientDataWriter.buffer);
        }

        writePendingData(outputBuffer, updated);

        if (updated)
            notifyMainThread();
    }
}

void TerminalController::TerminalEventLoop::runReaderLoop() noexcept
{
    static thread_local char inputBuffer alignas(4096) [readBufSize];
    while (true)
    {
        size_t bytesRead;
        bool readOk = ctrl.ptyMaster.readFromClient(inputBuffer, bytesRead);

        if (!readOk || bytesRead == 0)
        {
            ctrl.disconnected = true;
            notifyMainThread();
            break;
        }

        if (terminated)
            // We are expected to consume all of the client's data, so keep
            // reading while we can.
            continue;

        bool updated = false;
        {
            std::lock_guard<std::mutex> lock(mutex);

            TerminalEvent event;
            event.type = TerminalEventType::ClientDataRead;
            event.clientDataRead = {inputBuffer, bytesRead};
            ctrl.terminalEmulator.handleEvent(event);

            updateTimeouts();

            // We also do these in the ReaderLoop thread because locking may be
            // unfair. In a situation where we continously receive data from the
            // client, the ReaderLoop thread may be able to acquire the lock
            // several times before the WriterLoop wakes up.
            processEvents();
            updateState(updated);
        }

        if (updated)
            notifyMainThread();
        // Notify the WriterLoop so that it can use the new timeouts and write
        // any pending data.
        condVar.notify_one();
    }
}

void TerminalController::TerminalEventLoop::processEvents() noexcept
// Pre: 'this->mutex' is locked.
{
    while (true)
    {
        bool hasEvent;
        TerminalEvent event;

        eventQueue.lock([&] (auto &eventQueue) {
            if ((hasEvent = !eventQueue.empty()))
            {
                event = eventQueue.front();
                eventQueue.pop();
            }
        });

        if (!hasEvent)
            break;

        switch (event.type)
        {
            case TerminalEventType::ViewportResize:
                // Do not resize the client yet. We will handle this later.
                viewportSize = {event.viewportResize.x, event.viewportResize.y};
                viewportResized = true;
                break;

            default:
                ctrl.terminalEmulator.handleEvent(event);
                break;
        }
    }
}

void TerminalController::TerminalEventLoop::updateState(bool &updated) noexcept
// Pre: 'this->mutex' is locked.
{
    if (Clock::now() > currentTimeout)
    {
        updated = true;
        currentTimeout = TimePoint();
        maxReadTimeout = TimePoint();

        ctrl.lockState([&] (auto &state) {
            ctrl.terminalEmulator.updateState(state);
        });
    }

    if (currentTimeout == TimePoint() && viewportResized)
    {
        // Handle the resize only once we reach a timeout and after updating
        // the TerminalState, because the client is now likely drawn properly.
        viewportResized = false;

        TerminalEvent event;
        event.type = TerminalEventType::ViewportResize;
        event.viewportResize = {viewportSize.x, viewportSize.y};

        ctrl.terminalEmulator.handleEvent(event);
        ctrl.ptyMaster.resizeClient(viewportSize);
    }
}

void TerminalController::TerminalEventLoop::updateTimeouts() noexcept
// Pre: 'this->mutex' is locked.
{
    // When receiving data, we want to flush updates either:
    // - 'readWaitStepMs' after data was last received.
    // - 'maxReadTimeMs' after the first time data was received.
    auto now = Clock::now();
    if (maxReadTimeout == TimePoint())
        maxReadTimeout = now + std::chrono::milliseconds(maxReadTimeMs);

    currentTimeout = ::min( now + std::chrono::milliseconds(readWaitStepMs),
                            maxReadTimeout );
}

void TerminalController::TerminalEventLoop::writePendingData(GrowArray &outputBuffer, bool &updated) noexcept
// Pre: 'this->mutex' needs not be locked.
{
    if (outputBuffer.size() > 0)
    {
        if ( !ctrl.disconnected &&
             !ctrl.ptyMaster.writeToClient({outputBuffer.data(), outputBuffer.size()}) )
        {
            ctrl.disconnected = true;
            updated = true;
        }

        outputBuffer.clear();
    }
}

void TerminalController::TerminalEventLoop::notifyMainThread() noexcept
// Pre: 'this->mutex' needs not be locked.
{
    ctrl.updated = true;
    TEventQueue::wakeUp();
}

} // namespace tvterm
