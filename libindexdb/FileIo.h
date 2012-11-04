#ifndef INDEXDB_FILEIO_H
#define INDEXDB_FILEIO_H

#include <string>
#include <vector>
#include <stdint.h>

// XXX/TODO: Try to remove this include.  It requires users of libindexdb to
// also add libsha2 to their include path.
#include <sha2.h>

namespace indexdb {

class Buffer;
const int kMaxAlign = 8;


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
    virtual ~Reader() {}

    // Virtual pure methods.
    virtual uint64_t size() = 0;
    virtual uint64_t tell() = 0;
    virtual void seek(uint64_t offset) = 0;
    virtual Buffer readData(size_t size) = 0;

    // Overridable methods.
    virtual void readData(void *output, size_t size);

    // Shared method implementations.
    void align(int multiple);
    uint8_t readUInt8();
    uint32_t readUInt32();
    std::string readString();
    Buffer readBuffer();
    void readSignature(const char *signature);
    bool peekSignature(const char *signature);
};


///////////////////////////////////////////////////////////////////////////////
// MappedReader

class MappedReader : public Reader {
public:
    MappedReader(const std::string &path, size_t offset=0, size_t size=-1);
    virtual ~MappedReader();

    // Implementation of Reader methods.
    uint64_t size()                             { return m_viewSize; }
    uint64_t tell()                             { return m_offset; }
    void seek(uint64_t offset);
    Buffer readData(size_t size);
    void readData(void *output, size_t size);

private:
    inline char *readDataInternal(size_t size);

    char *m_mapBuffer;
    size_t m_mapBufferSize;
    char *m_viewBuffer;
    size_t m_viewSize;
    size_t m_offset;
};


///////////////////////////////////////////////////////////////////////////////
// UnmappedReader

class UnmappedReader : public Reader {
public:
    UnmappedReader(const std::string &path);
    virtual ~UnmappedReader();

    // Implementation of Reader methods.
    uint64_t size()                             { return m_fileSize; }
    uint64_t tell()                             { return m_offset; }
    void seek(uint64_t offset);
    Buffer readData(size_t size);
    void readData(void *output, size_t size);

private:
    FILE *m_fp;
    uint64_t m_fileSize;
    uint64_t m_offset;
};

} // namespace indexdb

#endif // INDEXDB_FILEIO_H
