#include "FileIo.h"

#include <cassert>
#include <cstring>

// UNIX headers
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "Buffer.h"
#include "FileIo64BitSupport.h"
#include "Util.h"

namespace indexdb {


///////////////////////////////////////////////////////////////////////////////
// Writer

Writer::Writer(const std::string &path) : m_sha256(NULL)
{
    const char *pathPtr = path.c_str();
#ifdef __unix__
    const int flags = O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC;
    int fd = EINTR_LOOP(open(pathPtr, flags, 0666));
    m_fp = fdopen(fd, "w");
#else
    m_fp = fopen(pathPtr, "w");
#endif
    assert(m_fp);
    m_writeOffset = 0;
}

Writer::~Writer()
{
    fclose(m_fp);
}

void Writer::writeUInt32(uint32_t val)
{
    static const char padding[sizeof(uint32_t) - 1] = { 0 };
    uint32_t padAmount = m_writeOffset & (sizeof(uint32_t) - 1);
    if (padAmount != 0) {
        padAmount = sizeof(uint32_t) - padAmount;
        fwrite(padding, 1, padAmount, m_fp);
        m_writeOffset += padAmount;
    }
    val = HostToLE32(val);
    writeData(&val, sizeof(val));
}

void Writer::writeString(const std::string &string)
{
    writeUInt32(string.size());
    writeData(string.data(), string.size());
}

void Writer::writeData(const void *data, size_t count)
{
    if (m_sha256 != NULL)
        sha256_update(m_sha256, static_cast<const unsigned char*>(data), count);
    fwrite(data, 1, count, m_fp);
    m_writeOffset += count;
}

void Writer::writeBuffer(const Buffer &buffer)
{
    writeUInt32(buffer.size());
    writeData(buffer.data(), buffer.size());
}

void Writer::writeSignature(const char *signature)
{
    writeData(signature, strlen(signature));
}

uint64_t Writer::tell()
{
    return m_writeOffset;
}

void Writer::seek(uint64_t offset)
{
    Seek64(m_fp, offset, SEEK_SET);
    assert(Tell64(m_fp) == offset);
    m_writeOffset = offset;
}

// Configure the Writer with a SHA-256 hash context.  If sha is non-NULL, then
// the hash context will be updated with every written byte.  The Writer does
// not take ownership of the hash context.
void Writer::setSha256Hash(sha256_ctx *sha256)
{
    m_sha256 = sha256;
}


///////////////////////////////////////////////////////////////////////////////
// Reader

Reader::Reader(const std::string &path)
{
    int fd = EINTR_LOOP(open(path.c_str(), O_RDONLY | O_CLOEXEC));
    assert(fd != -1);
    m_bufferPointer = 0;
    m_bufferSize = LSeek64(fd, 0, SEEK_END);
    m_buffer = static_cast<char*>(
                mmap(NULL, m_bufferSize, PROT_READ, MAP_PRIVATE, fd, 0));
    assert(m_buffer != reinterpret_cast<void*>(-1));
    close(fd);
}

Reader::~Reader()
{
    munmap(m_buffer, m_bufferSize);
}

uint32_t Reader::readUInt32()
{
    uint32_t padAmount = m_bufferPointer & (sizeof(uint32_t) - 1);
    if (padAmount != 0) {
        m_bufferPointer += sizeof(uint32_t) - padAmount;
    }
    // TODO: Is this type punning safe?
    assert(m_bufferPointer + sizeof(uint32_t) <= m_bufferSize);
    uint32_t result = *reinterpret_cast<uint32_t*>(m_buffer + m_bufferPointer);
    m_bufferPointer += sizeof(uint32_t);
    result = LEToHost32(result);
    return result;
}

std::string Reader::readString()
{
    uint32_t amount = readUInt32();
    return std::string(static_cast<char*>(readData(amount)), amount);
}

// The returned buffer is valid until the Reader is freed.
// (The entire file is mapped into memory.)
void *Reader::readData(size_t size)
{
    size_t originalPointer = m_bufferPointer;
    void *result = &m_buffer[m_bufferPointer];
    m_bufferPointer += size;
    assert(m_bufferPointer >= originalPointer &&
           m_bufferPointer <= m_bufferSize);
    return result;
}

// The returned Buffer has pointers into the Reader's memory-mapped buffer,
// so it should be freed before the Reader.
// TODO: Consider some way of automatically keeping the Reader or mmap region
// alive, like reference counting?
Buffer Reader::readBuffer()
{
    uint32_t size = readUInt32();
    return Buffer::fromMappedBuffer(readData(size), size);
}

void Reader::readSignature(const char *signature)
{
    const char *actual = static_cast<const char*>(readData(strlen(signature)));
    assert(memcmp(actual, signature, strlen(signature)) == 0);
}

// Look for the signature without advancing the buffer pointer.  Returns true
// if the signature exists.
bool Reader::peekSignature(const char *signature)
{
    size_t len = strlen(signature);
    if (m_bufferPointer + len > m_bufferSize)
        return false;
    return memcmp(m_buffer + m_bufferPointer, signature, len) == 0;
}

uint64_t Reader::tell()
{
    return m_bufferPointer;
}

void Reader::seek(uint64_t offset)
{
    assert(offset <= m_bufferSize);
    m_bufferPointer = offset;
}

} // namespace indexdb
