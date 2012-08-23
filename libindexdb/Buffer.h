#ifndef INDEXDB_BUFFER_H
#define INDEXDB_BUFFER_H

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
    uint32_t size() const;
    void *data();
    const void *data() const;
    void append(const void *data, uint32_t size);
    bool isMapped() const { return m_isMapped; }

private:
    void *m_data;
    uint32_t m_size;
    uint32_t m_capacity;
    bool m_isMapped;
};

} // namespace indexdb

#endif // INDEXDB_BUFFER_H
