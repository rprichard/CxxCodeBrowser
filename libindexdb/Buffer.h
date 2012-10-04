#ifndef INDEXDB_BUFFER_H
#define INDEXDB_BUFFER_H

#include <cstring>

#include <stdint.h>

namespace indexdb {

class Buffer {
public:
    Buffer();
    Buffer(uint32_t size, int fillChar=0);
    Buffer(Buffer &other) = delete;
    Buffer(Buffer &&other);
    Buffer &operator=(Buffer &&other);
    Buffer &operator=(Buffer &other) = delete;
    static Buffer fromMappedBuffer(void *data, uint32_t size);
    ~Buffer();
    uint32_t size() const       { return m_size; }
    void *data()                { return m_data; }
    const void *data() const    { return m_data; }
    void append(const void *data, uint32_t size);
    bool isMapped() const { return m_isMapped; }

private:
    void *m_data;
    uint32_t m_size;
    uint32_t m_capacity;
    bool m_isMapped;
};

inline bool operator==(const Buffer &x, const Buffer &y) {
    return x.size() == y.size() && !memcmp(x.data(), y.data(), x.size());
}

} // namespace indexdb

#endif // INDEXDB_BUFFER_H
