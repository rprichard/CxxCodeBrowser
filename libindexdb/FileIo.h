#ifndef INDEXDB_FILEIO_H
#define INDEXDB_FILEIO_H

#include <string>
#include <vector>
#include <stdint.h>

#include "sha2.h"

namespace indexdb {

class Buffer;


///////////////////////////////////////////////////////////////////////////////
// Writer

class Writer {
public:
    Writer(const std::string &path);
    ~Writer();
    void align(int multiple);
    void writeUInt8(uint8_t val);
    void writeUInt32(uint32_t val);
    void writeString(const std::string &string);
    void writeData(const void *data, size_t count);
    void writeBuffer(const Buffer &buffer);
    void writeSignature(const char *signature);
    uint64_t tell();
    void seek(uint64_t offset);
    void setSha256Hash(sha256_ctx *sha256);
    void setCompressed(bool compressed);
private:
    sha256_ctx *m_sha256;
    bool m_compressed;
    FILE *m_fp;
    uint64_t m_writeOffset;
    std::vector<char> m_tempCompressionBuffer;
};


///////////////////////////////////////////////////////////////////////////////
// Reader

class Reader {
public:
    Reader(const std::string &path);
    ~Reader();
    void align(int multiple);
    uint8_t readUInt8();
    uint32_t readUInt32();
    std::string readString();
    void readLine(const char *&line, size_t &size);
    void *readData(size_t size);
    Buffer readBuffer();
    void readSignature(const char *signature);
    bool peekSignature(const char *signature);
    uint64_t tell();
    void seek(uint64_t offset);
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
