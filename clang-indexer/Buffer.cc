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

Buffer Buffer::fromMappedBuffer(void *data, uint32_t size)
{
    Buffer result;
    result.m_data = data;
    result.m_size = size;
    result.m_capacity = size;
    result.m_isMapped = true;
    return result;
}

Buffer::~Buffer()
{
    if (!m_isMapped)
        free(m_data);
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
