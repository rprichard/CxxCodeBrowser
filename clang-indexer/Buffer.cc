#include "Buffer.h"
#include "FileIo.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <utility>

// UNIX headers
#include <sys/mman.h>
#include <unistd.h>

namespace indexdb {

Buffer::Buffer() : m_data(NULL), m_size(0), m_capacity(0), m_isMapped(false)
{
}

Buffer::Buffer(uint32_t size, int fillChar)
{
    if (size == 0) {
        m_data = NULL;
        m_size = 0;
        m_capacity = 0;
        m_isMapped = false;
    } else {
        m_data = malloc(size);
        assert(m_data != NULL);
        m_size = size;
        m_capacity = size;
        m_isMapped = false;
        memset(m_data, fillChar, size);
    }
}

Buffer::Buffer(Buffer &&other)
{
    *this = std::move(other);
}

Buffer &Buffer::operator=(Buffer &&other)
{
    m_data = other.m_data;
    m_size = other.m_size;
    m_capacity = other.m_capacity;
    m_isMapped = other.m_isMapped;
    other.m_data = NULL;
    other.m_size = 0;
    other.m_capacity = 0;
    other.m_isMapped = false;
    return *this;
}

Buffer Buffer::fromFile(int fd, uint32_t offset, uint32_t size)
{
    // I think the size is not guaranteed to be a multiple of page size.
    assert(offset == roundToPage(offset));
    Buffer b;
    b.m_data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, offset);
    assert(b.m_data != reinterpret_cast<void*>(-1));
    b.m_size = size;
    b.m_capacity = size;
    b.m_isMapped = true;
    return std::move(b);
}

Buffer::~Buffer()
{
    if (m_isMapped)
        munmap(m_data, m_capacity);
    else
        free(m_data);
}

void Buffer::write(int fd)
{
    assert(tell(fd) == roundToPage(tell(fd)));
    ssize_t ret = ::write(fd, m_data, m_size);
    assert(ret == m_size);
    padFile(fd);
}

uint32_t Buffer::size() const
{
    return m_size;
}

void *Buffer::data()
{
    return m_data;
}

const void *Buffer::data() const
{
    return m_data;
}

void Buffer::append(const void *data, uint32_t size)
{
    assert(!m_isMapped);
    if (m_size + size > m_capacity) {
        uint32_t newCapacity = std::max(m_capacity * 2, m_size + size);
        m_data = realloc(m_data, newCapacity);
        assert(m_data != NULL);
        m_capacity = newCapacity;
    }
    memcpy(static_cast<char*>(m_data) + m_size, data, size);
    m_size += size;
}

} // namespace indexdb
