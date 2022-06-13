#ifndef TVTERM_ARRAY_H
#define TVTERM_ARRAY_H

#include <stdlib.h>
#include <string.h>

#include <tvision/tv.h>

namespace tvterm
{

class GrowArray
{
    struct
    {
        char *data;
        size_t size;
        size_t capacity;
    } p {};

    void grow(size_t minCapacity) noexcept;

public:

    GrowArray() noexcept = default;
    GrowArray(GrowArray &&other) noexcept;
    ~GrowArray();

    GrowArray &operator=(GrowArray other) noexcept;

    char *data() noexcept;
    size_t size() noexcept;
    void push(const char *aData, size_t aSize) noexcept;
    void clear() noexcept;
};

inline GrowArray::GrowArray(GrowArray &&other) noexcept
{
    p = other.p;
    other.p = {};
}

inline GrowArray::~GrowArray()
{
    free(p.data);
}

inline GrowArray &GrowArray::operator=(GrowArray other) noexcept
{
    auto aux = p;
    p = other.p;
    other.p = aux;
    return *this;
}

inline char *GrowArray::data() noexcept
{
    return p.data;
}

inline size_t GrowArray::size() noexcept
{
    return p.size;
}

inline void GrowArray::push(const char *aData, size_t aSize) noexcept
{
    size_t newSize = p.size + aSize;
    if (newSize > p.capacity)
        grow(newSize);
    memcpy(&p.data[p.size], aData, aSize);
    p.size = newSize;
}

inline void GrowArray::clear() noexcept
{
    p.size = 0;
}

inline void GrowArray::grow(size_t minCapacity) noexcept
{
    size_t newCapacity = max<size_t>(minCapacity, 2*p.capacity);
    if (!(p.data = (char *) realloc(p.data, newCapacity)))
        abort();
    p.capacity = newCapacity;
}

} // namespace tvterm
#endif
