#ifndef INDEXDB_FILEIO_H
#define INDEXDB_FILEIO_H

#include <string>
#include <stdint.h>

namespace indexdb {

class Buffer;

class Writer {
public:
    Writer(const std::string &path);
    ~Writer();
    void writeUInt32(uint32_t val);
    void writeString(const std::string &string);
    void writeData(const void *data, size_t count);
    void writeBuffer(const Buffer &buffer);
private:
    FILE *m_fp;
    uint64_t m_writeOffset;
};

class Reader {
public:
    Reader(const std::string &path);
    ~Reader();
    uint32_t readUInt32();
    std::string readString();
    void readLine(const char *&line, size_t &size);
    void *readData(size_t size);
    Buffer readBuffer();
private:
    inline char readChar();
    char readCharFull();
    void loadChunk();
private:
    char *m_buffer;
    size_t m_bufferSize;
    size_t m_bufferPointer;
};


} // namespace indexdb

#endif // INDEXDB_FILEIO_H
