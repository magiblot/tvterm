#ifndef TVTERM_REFCNT_H
#define TVTERM_REFCNT_H

struct AtomicOps
{
    using counter_type = std::atomic<size_t>;

    static void add(std::atomic<size_t> &refCnt)
    {
        refCnt.fetch_add(1, std::memory_order_relaxed);
    }

    static bool sub(std::atomic<size_t> &refCnt)
    {
        return refCnt.fetch_sub(1, std::memory_order_acq_rel) == 1;
    }
};

struct NormalOps
{
    using counter_type = size_t;

    static void add(size_t &refCnt)
    {
        ++refCnt;
    }

    static bool sub(size_t &refCnt)
    {
        return --refCnt == 0;
    }
};

template <class T, class Ops>
struct RefCounterBlock
{
    typename Ops::counter_type refCnt;
    void (&deleter)(T *, RefCounterBlock &) noexcept;

    using deleter_type = decltype(deleter); // The only way to get 'noexcept' in.

    RefCounterBlock() = default;
    RefCounterBlock(void (&aDeleter)(T *, RefCounterBlock &)) :
        refCnt(1),
        deleter(reinterpret_cast<deleter_type>(aDeleter))
    {
    }

    void unref(T *elem) noexcept
    {
        if (Ops::sub(refCnt))
            deleter(elem, *this);
    }

    void ref() noexcept
    {
        Ops::add(refCnt);
    }

    static void defaultDeleter(T *pt, RefCounterBlock &blk) noexcept
    {
        delete &blk;
        delete pt;
    }

};

template <class T, class Ops>
class BasicRefCounter
{
    using Block = RefCounterBlock<T, Ops>;

    struct IntrusiveBlock : Block
    {
        T t;

        template <class... Args>
        IntrusiveBlock(Args&&... args) :
            Block(destroy),
            t(static_cast<Args&&>(args)...)
        {
        }

        static void destroy(T *, Block &blk) noexcept
        {
            delete (IntrusiveBlock *) &blk;
        }
    };

    struct Ptrs
    {
        T *t;
        Block *blk;
    } p {};

    BasicRefCounter(T *pt, Block *pblk) noexcept :
        p {pt, pblk}
    {
    }

public:

    template <class... Args>
    static BasicRefCounter make(Args&&... args)
    {
        auto *pblk = new IntrusiveBlock(static_cast<Args&&>(args)...);
        return BasicRefCounter(&pblk->t, pblk);
    }

    BasicRefCounter() = default;

    ~BasicRefCounter() noexcept
    {
        if (p.blk)
            p.blk->unref(p.t);
    }

    BasicRefCounter(const BasicRefCounter& other) noexcept
    {
        p = other.p;
        if (p.blk)
            p.blk->ref();
    }

    BasicRefCounter(BasicRefCounter&& other) noexcept
    {
        p = other.p;
        other.p = {};
    }

    BasicRefCounter(T *pt) noexcept :
        BasicRefCounter(pt, new Block(Block::defaultDeleter))
    {
    }

    template <class U>
    BasicRefCounter(BasicRefCounter<U, Ops> other, T *pt) noexcept
    // Pre: the object pointed to by 'pt' must stay alive at least until
    //      the deleter in 'other' is invoked.
    {
        p = {pt, other.p.blk};
        other.p = {};
    }

    BasicRefCounter& operator=(BasicRefCounter other) noexcept
    {
        auto aux = p;
        p = other.p;
        other.p = aux;
        return *this;
    }

    T* get() const noexcept { return p.t; }
    T* operator->() const noexcept { return get(); }
    T& operator*() const noexcept { return *get(); }
    operator bool() const noexcept { return get(); }

    template <class U>
    BasicRefCounter<U, Ops> static_cast_to() &&
    {
        auto *pt = static_cast<U*>(get());
        return {static_cast<BasicRefCounter&&>(*this), pt};
    }

};

template <class T>
using TRc = BasicRefCounter<T, NormalOps>;

template <class T>
using TArc = BasicRefCounter<T, AtomicOps>;

#endif // TVTERM_REFCNT_H
