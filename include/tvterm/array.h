#ifndef TVTERM_ARRAY_H
#define TVTERM_ARRAY_H

#include <stdlib.h>
#include <string.h>

#include <tvision/tv.h>

namespace tvterm
{

class ByteArray
{
    struct
    {
        char *data;
        size_t size;
        size_t capacity;
    } p {};

    void grow(size_t minCapacity) noexcept;

public:

    ByteArray() noexcept = default;
    ByteArray(ByteArray &&other) noexcept;
    ~ByteArray();

    ByteArray &operator=(ByteArray other) noexcept;

    char *data() noexcept;
    size_t size() noexcept;
    void push(const char *aData, size_t aSize) noexcept;
    void clear() noexcept;
};

inline ByteArray::ByteArray(ByteArray &&other) noexcept
{
    p = other.p;
    other.p = {};
}

inline ByteArray::~ByteArray()
{
    free(p.data);
}

inline ByteArray &ByteArray::operator=(ByteArray other) noexcept
{
    auto aux = p;
    p = other.p;
    other.p = aux;
    return *this;
}

inline char *ByteArray::data() noexcept
{
    return p.data;
}

inline size_t ByteArray::size() noexcept
{
    return p.size;
}

inline void ByteArray::push(const char *aData, size_t aSize) noexcept
{
    size_t newSize = p.size + aSize;
    if (newSize > p.capacity)
        grow(newSize);
    memcpy(&p.data[p.size], aData, aSize);
    p.size = newSize;
}

inline void ByteArray::clear() noexcept
{
    p.size = 0;
}

inline void ByteArray::grow(size_t minCapacity) noexcept
{
    size_t newCapacity = max<size_t>(minCapacity, 2*p.capacity);
    if (!(p.data = (char *) realloc(p.data, newCapacity)))
        abort();
    p.capacity = newCapacity;
}

} // namespace tvterm
#endif
