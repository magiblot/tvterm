#include <tvterm/pty.h>

#define Uses_TPoint
#include <tvision/tv.h>

#if !defined(_WIN32)
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>

#if __has_include(<pty.h>)
#   include <pty.h>
#elif __has_include(<libutil.h>)
#   include <libutil.h>
#elif __has_include(<util.h>)
#   include <util.h>
#endif

namespace tvterm
{

static struct termios createTermios() noexcept;
static struct winsize createWinsize(TPoint size) noexcept;

bool createPty( PtyDescriptor &ptyDescriptor, TPoint size,
                TSpan<const EnvironmentVar> customEnvironment,
                void (&onError)(const char *) ) noexcept
{
    auto termios = createTermios();
    auto winsize = createWinsize(size);
    int masterFd;
    pid_t clientPid = forkpty(&masterFd, nullptr, &termios, &winsize);
    if (clientPid == 0)
    {
        // Use the default ISIG signal handlers.
        signal(SIGINT,  SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGSTOP, SIG_DFL);
        signal(SIGCONT, SIG_DFL);

        for (const auto &envVar : customEnvironment)
            setenv(envVar.name, envVar.value, 1);

        char *shell = getenv("SHELL");
        char *args[] = {shell, nullptr};
        execvp(shell, args);

        setbuf(stderr, nullptr); // Ensure 'stderr' is unbuffered.
        fprintf(
            stderr,
            "\x1B[1;31m" // Color attributes: bold and red.
            "Error: Failed to execute the program specified by the environment variable SHELL ('%s'): %s"
            "\x1B[0m", // Reset color attributes.
            (shell ? shell : ""),
            strerror(errno)
        );

        // Exit the subprocess without cleaning up resources.
        _Exit(EXIT_FAILURE);
    }
    else if (clientPid == -1)
    {
        char *str = fmtStr("forkpty failed: %s", strerror(errno));
        onError(str);
        delete[] str;
        return false;
    }
    ptyDescriptor = {masterFd, clientPid};
    return true;
}

bool PtyMaster::readFromClient(TSpan<char> data, size_t &bytesRead) noexcept
{
    bytesRead = 0;
    if (data.size() > 1)
    {
        ssize_t r = read(d.masterFd, &data[0], 1);
        if (r < 0)
            return false;
        else if (r > 0)
        {
            bytesRead += r;
            int availableBytes = 0;
            if ( ioctl(d.masterFd, FIONREAD, &availableBytes) != -1 &&
                 availableBytes > 0 )
            {
                size_t bytesToRead = min(availableBytes, data.size() - 1);
                r = read(d.masterFd, &data[1], bytesToRead);
                if (r < 0)
                    return false;
                bytesRead += r;
            }
        }
    }
    return true;
}

bool PtyMaster::writeToClient(TSpan<const char> data) noexcept
{
    size_t written = 0;
    while (written < data.size())
    {
        size_t bytesToWrite = data.size() - written;
        ssize_t r = write(d.masterFd, &data[written], bytesToWrite);
        if (r < 0)
            return false;
        written += r;
    }
    return true;
}

void PtyMaster::resizeClient(TPoint size) noexcept
{
    struct winsize w = {};
    w.ws_row = size.y;
    w.ws_col = size.x;
    int rr = ioctl(d.masterFd, TIOCSWINSZ, &w);
    (void) rr;
}

void PtyMaster::disconnect() noexcept
{
    close(d.masterFd);
    // Send a SIGHUP, then a SIGKILL after a while if the process is not yet
    // terminated, like most terminal emulators do.
    kill(d.clientPid, SIGHUP);
    sleep(1);
    if (waitpid(d.clientPid, nullptr, WNOHANG) != d.clientPid)
    {
        kill(d.clientPid, SIGKILL);
        while( waitpid(d.clientPid, nullptr, 0) != d.clientPid &&
               errno == EINTR );
    }
}

static struct winsize createWinsize(TPoint size) noexcept
{
    struct winsize w = {};
    w.ws_row = size.y;
    w.ws_col = size.x;
    return w;
}

static struct termios createTermios() noexcept
{
    // Initialization like in pangoterm.
    struct termios t = {};

    t.c_iflag = ICRNL | IXON;
    t.c_oflag = OPOST | ONLCR
#ifdef TAB0
        | TAB0
#endif
        ;
    t.c_cflag = CS8 | CREAD;
    t.c_lflag = ISIG | ICANON | IEXTEN | ECHO | ECHOE | ECHOK;

#ifdef IUTF8
    t.c_iflag |= IUTF8;
#endif
#ifdef NL0
    t.c_oflag |= NL0;
#endif
#ifdef CR0
    t.c_oflag |= CR0;
#endif
#ifdef BS0
    t.c_oflag |= BS0;
#endif
#ifdef VT0
    t.c_oflag |= VT0;
#endif
#ifdef FF0
    t.c_oflag |= FF0;
#endif
#ifdef ECHOCTL
    t.c_lflag |= ECHOCTL;
#endif
#ifdef ECHOKE
    t.c_lflag |= ECHOKE;
#endif

    t.c_cc[VINTR]    = 0x1f & 'C';
    t.c_cc[VQUIT]    = 0x1f & '\\';
    t.c_cc[VERASE]   = 0x7f;
    t.c_cc[VKILL]    = 0x1f & 'U';
    t.c_cc[VEOF]     = 0x1f & 'D';
    t.c_cc[VEOL]     = _POSIX_VDISABLE;
    t.c_cc[VEOL2]    = _POSIX_VDISABLE;
    t.c_cc[VSTART]   = 0x1f & 'Q';
    t.c_cc[VSTOP]    = 0x1f & 'S';
    t.c_cc[VSUSP]    = 0x1f & 'Z';
    t.c_cc[VREPRINT] = 0x1f & 'R';
    t.c_cc[VWERASE]  = 0x1f & 'W';
    t.c_cc[VLNEXT]   = 0x1f & 'V';
    t.c_cc[VMIN]     = 1;
    t.c_cc[VTIME]    = 0;

    cfsetispeed(&t, B38400);
    cfsetospeed(&t, B38400);

    return t;
}

} // namespace tvterm

#else

#include <vector>

namespace tvterm
{

// The OS-provided ConPTY is usually outdated. Try to overcome this by loading
// a user-installed conpty.dll.

struct ConPtyApi
{
    decltype(::CreatePseudoConsole) *CreatePseudoConsole {nullptr};
    decltype(::ResizePseudoConsole) *ResizePseudoConsole {nullptr};
    decltype(::ClosePseudoConsole) *ClosePseudoConsole {nullptr};

    void init() noexcept;
};

static ConPtyApi conPty;

void ConPtyApi::init() noexcept
{
    HMODULE mod = LoadLibraryA("conpty");
    if (!mod)
        mod = GetModuleHandleA("kernel32");

    CreatePseudoConsole =
        (decltype(CreatePseudoConsole)) GetProcAddress(mod, "CreatePseudoConsole");
    ResizePseudoConsole =
        (decltype(ResizePseudoConsole)) GetProcAddress(mod, "ResizePseudoConsole");
    ClosePseudoConsole =
        (decltype(ClosePseudoConsole)) GetProcAddress(mod, "ClosePseudoConsole");
}

static COORD toCoord(TPoint point) noexcept
{
    return {
        (short) point.x,
        (short) point.y,
    };
}

static std::vector<char> constructEnvironmentBlock(TSpan<const EnvironmentVar> customEnvironment) noexcept
{
    std::vector<char> result;

    if (const char *currentEnvironment = GetEnvironmentStrings())
    {
        size_t len = 1;
        // The environment block is terminated by two null characters.
        while (currentEnvironment[len - 1] != '\0' || currentEnvironment[len] != '\0')
            ++len;
        result.insert(result.end(), &currentEnvironment[0], &currentEnvironment[len]);
    }

    for (const auto &var : customEnvironment)
    {
        TStringView name(var.name),
                    value(var.value);
        result.insert(result.end(), name.begin(), name.end());
        result.push_back('=');
        result.insert(result.end(), value.begin(), value.end());
        result.push_back('\0');
    }

    result.push_back('\0');

    return result;
}

struct ProcessWaiter
{
    HANDLE hClientWrite;
    HANDLE hWait;

    ~ProcessWaiter()
    {
        CloseHandle(hClientWrite);
    }

    static void CALLBACK callback(PVOID ctx, BOOLEAN)
    {
        auto &self = *(ProcessWaiter *) ctx;
        // Wake up the thread waiting for client data.
        DWORD r = 0;
        WriteFile(self.hClientWrite, "", 1, &r, nullptr);
        UnregisterWait(self.hWait);
        delete &self;
    }
};

bool createPty( PtyDescriptor &ptyDescriptor,
                TPoint size,
                TSpan<const EnvironmentVar> customEnvironment,
                void (&onError)(const char *) ) noexcept
{
    static bool conPtyAvailable = [] ()
    {
        conPty.init();
        return conPty.CreatePseudoConsole &&
               conPty.ResizePseudoConsole &&
               conPty.ClosePseudoConsole;
    }();

    HANDLE hMasterRead {},
           hClientWrite {};
    HANDLE hMasterWrite {},
           hClientRead {};
    HPCON hPseudoConsole {};
    STARTUPINFOEXA siClient {};
    PROCESS_INFORMATION piClient {};

    size_t attrListSize = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attrListSize);
    std::vector<char> attributeList(attrListSize);

    const char *failedAction {};

    do
    {
        if (!conPtyAvailable)
        {
            failedAction = "Loading ConPTY";
            break;
        }

        if (!CreatePipe(&hMasterRead, &hClientWrite, nullptr, 0))
        {
            failedAction = "CreatePipe";
            break;
        }

        if (!CreatePipe(&hClientRead, &hMasterWrite, nullptr, 0))
        {
            failedAction = "CreatePipe";
            break;
        }

        if (FAILED(conPty.CreatePseudoConsole(toCoord(size), hClientRead, hClientWrite, 0, &hPseudoConsole)))
        {
            failedAction = "CreatePseudoConsole";
            break;
        }

        siClient.StartupInfo.cb = sizeof(STARTUPINFOEX);
        siClient.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST) &attributeList[0];
        if (!InitializeProcThreadAttributeList(siClient.lpAttributeList, 1, 0, &attrListSize))
        {
            failedAction = "InitializeProcThreadAttributeList";
            break;
        }

        if ( !UpdateProcThreadAttribute( siClient.lpAttributeList,
                                         0,
                                         PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                                         hPseudoConsole,
                                         sizeof(HPCON),
                                         nullptr,
                                         nullptr ) )
        {
            failedAction = "UpdateProcThreadAttribute";
            break;
        }

        char *comspec = getenv("COMSPEC");
        if (!comspec)
        {
            failedAction = "Retrieving 'COMSPEC' environment variable";
            break;
        }

        std::vector<char> environmentBlock = constructEnvironmentBlock(customEnvironment);
        if ( !CreateProcessA( nullptr,
                              comspec,
                              nullptr,
                              nullptr,
                              false,
                              EXTENDED_STARTUPINFO_PRESENT,
                              &environmentBlock[0],
                              nullptr,
                              (LPSTARTUPINFOA) &siClient.StartupInfo,
                              &piClient ) )
        {
            failedAction = "CreateProcessA";
            break;
        }

        ProcessWaiter &procWaiter = *new ProcessWaiter {hClientWrite};
        if ( !RegisterWaitForSingleObject( &procWaiter.hWait,
                                           piClient.hProcess,
                                           &ProcessWaiter::callback,
                                           &procWaiter,
                                           INFINITE,
                                           WT_EXECUTEINWAITTHREAD | WT_EXECUTEONLYONCE ) )
        {
            failedAction = "RegisterWaitForSingleObject";
            delete &procWaiter;
            break;
        }
    } while (false);

    if (!failedAction)
    {
        ptyDescriptor = {
            hMasterRead,
            hMasterWrite,
            hPseudoConsole,
            piClient.hProcess,
        };
    }
    else
    {
        char *msg = fmtStr("%s failed with error code %d", failedAction, (int) GetLastError());
        onError(msg);
        delete[] msg;

        for (HANDLE handle : {hMasterRead, hMasterWrite, hClientWrite, piClient.hProcess})
            if (handle)
                CloseHandle(handle);
        if (hPseudoConsole)
            conPty.ClosePseudoConsole(hPseudoConsole);
    }

    for (HANDLE handle : {hClientRead, piClient.hThread})
        if (handle)
            CloseHandle(handle);
    if (siClient.lpAttributeList)
        DeleteProcThreadAttributeList(siClient.lpAttributeList);

    return !failedAction;
}

static bool processIsNotRunning(HANDLE hProcess)
{
    DWORD exitCode;
    return !GetExitCodeProcess(hProcess, &exitCode) ||
           exitCode != STILL_ACTIVE;
}

bool PtyMaster::readFromClient(TSpan<char> data, size_t &bytesRead) noexcept
{
    bytesRead = 0;

    if (processIsNotRunning(d.hClientProcess))
        return false;

    if (data.size() > 1)
    {
        DWORD r;
        if (!ReadFile(d.hMasterRead, &data[0], 1, &r, nullptr))
            return false;
        else if (r > 0)
        {
            bytesRead += r;
            DWORD availableBytes = 0;
            if ( PeekNamedPipe(d.hMasterRead, nullptr, 0, nullptr, &availableBytes, nullptr)  &&
                 availableBytes > 0 )
            {
                DWORD bytesToRead = min(availableBytes, data.size() - 1);
                if (!ReadFile(d.hMasterRead, &data[1], bytesToRead, &r, nullptr))
                    return false;
                bytesRead += r;
            }
        }
    }
    return true;
}

bool PtyMaster::writeToClient(TSpan<const char> data) noexcept
{
    if (processIsNotRunning(d.hClientProcess))
        return false;

    size_t written = 0;
    while (written < data.size())
    {
        DWORD bytesToWrite = data.size() - written;
        DWORD r;
        if (!WriteFile(d.hMasterWrite, &data[written], bytesToWrite, &r, nullptr))
            return false;
        written += r;
    }
    return true;
}

void PtyMaster::resizeClient(TPoint size) noexcept
{
    conPty.ResizePseudoConsole(d.hPseudoConsole, toCoord(size));
}

void PtyMaster::disconnect() noexcept
{
    conPty.ClosePseudoConsole(d.hPseudoConsole);
    CloseHandle(d.hMasterRead);
    CloseHandle(d.hMasterWrite);
    CloseHandle(d.hClientProcess);
}

} // namespace tvterm

#endif // _WIN32
