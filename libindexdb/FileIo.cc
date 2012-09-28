#include "FileIo.h"

#include <cassert>

// UNIX headers
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "Buffer.h"
#include "Util.h"

namespace indexdb {

Writer::Writer(const std::string &path)
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
    fwrite(&val, 1, sizeof(val), m_fp);
}

void Writer::writeString(const std::string &string)
{
    writeUInt32(string.size());
    writeData(string.data(), string.size());
}

void Writer::writeData(const void *data, size_t count)
{
    fwrite(data, 1, count, m_fp);
    m_writeOffset += count;
}

void Writer::writeBuffer(const Buffer &buffer)
{
    writeUInt32(buffer.size());
    writeData(buffer.data(), buffer.size());
}

Reader::Reader(const std::string &path)
{
    int fd = EINTR_LOOP(open(path.c_str(), O_RDONLY | O_CLOEXEC));
    assert(fd != -1);
    m_bufferPointer = 0;
    m_bufferSize = lseek(fd, 0, SEEK_END); // TODO: allow 64-bit offset
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
    uint32_t result = *reinterpret_cast<uint32_t*>(m_buffer + m_bufferPointer);
    m_bufferPointer += sizeof(uint32_t);
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
    void *result = &m_buffer[m_bufferPointer];
    m_bufferPointer += size;
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

} // namespace indexdb
