#ifdef _WIN32
#ifndef WINVER
#define WINVER 0x0500
#endif
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT WINVER
#endif // _WIN32

#include "FileIo.h"
#include "../shared_headers/host.h"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <memory>

#if defined(SOURCEWEB_UNIX)
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include <windows.h>
#endif

#include <sha2.h>
#include <snappy.h>

#include "Buffer.h"
#include "FileIo64BitSupport.h"
#include "Util.h"
#include "WriterSha256Context.h"

namespace indexdb {

static size_t mapGranularity()
{
#if defined(SOURCEWEB_UNIX)
    return sysconf(_SC_PAGESIZE);
#elif defined(_WIN32)
    SYSTEM_INFO systemInfo;
    memset(&systemInfo, 0, sizeof(systemInfo));
    GetSystemInfo(&systemInfo);
    return systemInfo.dwAllocationGranularity;
#else
#error "Not implemented"
#endif
}


///////////////////////////////////////////////////////////////////////////////
// Writer

Writer::Writer(const std::string &path) : m_sha256(NULL), m_compressed(false)
{
    const char *pathPtr = path.c_str();
#if defined(SOURCEWEB_UNIX)
    const int flags = O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC;
    int fd = EINTR_LOOP(open(pathPtr, flags, 0666));
    m_fp = fdopen(fd, "w");
#else
    m_fp = fopen(pathPtr, "wb");
#endif
    assert(m_fp != NULL);
    m_writeOffset = 0;
}

Writer::~Writer()
{
    fclose(m_fp);
}

// Write padding bytes until the output is aligned to the given power of 2.
// The power of 2 must be in the range [1..kMaxAlign].
void Writer::align(int multiple)
{
    assert(multiple >= 1 && multiple <= kMaxAlign &&
           (multiple & (multiple - 1)) == 0);
    static const char padding[7] = { 0 };
    uint32_t padAmount = m_writeOffset & (multiple - 1);
    if (padAmount != 0) {
        padAmount = multiple - padAmount;
        writeData(padding, padAmount);
    }
}

void Writer::writeUInt8(uint8_t val)
{
    writeData(&val, sizeof(val));
}

void Writer::writeUInt32(uint32_t val)
{
    align(sizeof(uint32_t));
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
    if (m_sha256 != NULL) {
        sha256_update(
                    &m_sha256->ctx,
                    static_cast<const unsigned char*>(data),
                    count);
    }
    fwrite(data, 1, count, m_fp);
    m_writeOffset += count;
}

void Writer::writeBuffer(const Buffer &buffer)
{
    writeUInt8(m_compressed);

    if (m_compressed) {
        size_t maxLength = snappy::MaxCompressedLength(buffer.size());
        if (m_tempCompressionBuffer.size() < maxLength)
            m_tempCompressionBuffer.resize(maxLength);
        size_t length = 0;
        snappy::RawCompress(
                    static_cast<const char*>(buffer.data()), buffer.size(),
                    &m_tempCompressionBuffer[0], &length);
        writeUInt32(length);
        writeData(m_tempCompressionBuffer.data(), length);
    } else {
        writeUInt32(buffer.size());
        align(kMaxAlign);
        writeData(buffer.data(), buffer.size());
    }
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
void Writer::setSha256Hash(WriterSha256Context *sha256)
{
    m_sha256 = sha256;
}

// Enable or disable compression.  Compression is done on a buffer-by-buffer
// basis, so this flag may be toggled while writing a file.
void Writer::setCompressed(bool compressed)
{
    m_compressed = compressed;
}


///////////////////////////////////////////////////////////////////////////////
// Reader

void Reader::readData(void *output, size_t size)
{
    Buffer tmp = readData(size);
    memcpy(output, tmp.data(), size);
}

void Reader::align(int multiple)
{
    assert(multiple >= 1 && multiple <= kMaxAlign &&
           (multiple & (multiple - 1)) == 0);
    uint32_t padAmount = tell() & (multiple - 1);
    if (padAmount != 0) {
        seek(tell() + (multiple - padAmount));
    }
}

uint8_t Reader::readUInt8()
{
    uint8_t ret;
    readData(&ret, sizeof(ret));
    return ret;
}

uint32_t Reader::readUInt32()
{
    align(sizeof(uint32_t));
    uint32_t ret;
    readData(&ret, sizeof(ret));
    return LEToHost32(ret);
}

std::string Reader::readString()
{
    uint32_t amount = readUInt32();
    std::unique_ptr<char[]> buf(new char[amount]);
    readData(buf.get(), amount);
    return std::string(buf.get(), amount);
}

// The returned Buffer has pointers into the Reader's memory-mapped buffer,
// so it must be freed before the Reader.
Buffer Reader::readBuffer()
{
    bool isCompressed = readUInt8();
    if (isCompressed) {
        size_t compressedLength = readUInt32(); // TODO: Make 64-bit
        Buffer compressedData = readData(compressedLength);
        size_t length = 0;
        bool success = snappy::GetUncompressedLength(
                    static_cast<char*>(compressedData.data()),
                    compressedLength,
                    &length);
        assert(success);
        Buffer buffer(length);
        success = snappy::RawUncompress(
                    static_cast<char*>(compressedData.data()),
                    compressedLength,
                    static_cast<char*>(buffer.data()));
        assert(success);
        return std::move(buffer);
    } else {
        uint32_t size = readUInt32();
        align(kMaxAlign);
        return readData(size);
    }
}

void Reader::readSignature(const char *signature)
{
    size_t len = strlen(signature);
    Buffer actual = readData(len);
    assert(actual == Buffer::fromMappedBuffer(
               const_cast<char*>(signature), len));
}

// Look for the signature without advancing the buffer pointer.  Returns true
// if the signature exists.
bool Reader::peekSignature(const char *signature)
{
    size_t len = strlen(signature);
    if (size() - tell() < len)
        return false;
    Buffer actual = readData(len);
    seek(tell() - len);
    return actual == Buffer::fromMappedBuffer(
                const_cast<char*>(signature), len);
}


///////////////////////////////////////////////////////////////////////////////
// MappedReader

// offset need not be page-aligned, but it must be no greater than the file
// size.  (offset + size) may exceed the file size -- the memory-mapped region
// is limited to the file size.
MappedReader::MappedReader(const std::string &path, size_t offset, size_t size)
{
    // The view offset must be aligned to at least kMaxAlign bytes, because the
    // align method operates upon the offset within the mapped view rather than
    // the offset within the mapped file.  Files within the archive are also
    // aligned to kMaxAlign bytes.
    assert((offset & (kMaxAlign - 1)) == 0);

    // XXX: What about a size of 0?
    // XXX: What about an offset equal to the file size?
    const size_t alignOffset = offset & (mapGranularity() - 1);
    const uint64_t mapOffset = offset - alignOffset;

#if defined(SOURCEWEB_UNIX)
    int fd = EINTR_LOOP(open(path.c_str(), O_RDONLY | O_CLOEXEC));
    assert(fd != -1);
    const uint64_t fileSize = LSeek64(fd, 0, SEEK_END);
#elif defined(_WIN32)
    HANDLE hfile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, 0, NULL);
    assert(hfile != (HANDLE)INVALID_HANDLE_VALUE);
    LARGE_INTEGER fileSizeLI;
    BOOL flag = GetFileSizeEx(hfile, &fileSizeLI);
    assert(flag);
    const uint64_t fileSize = fileSizeLI.QuadPart;
#else
#error "Not implemented"
#endif
    assert(offset <= fileSize);

    m_viewSize = std::min<size_t>(size, fileSize - offset);
    m_mapBufferSize = m_viewSize + alignOffset;

#if defined(SOURCEWEB_UNIX)
    m_mapBuffer = static_cast<char*>(
                mmap(NULL, m_mapBufferSize, PROT_READ,
                     MAP_PRIVATE, fd, mapOffset));
    assert(m_mapBuffer != reinterpret_cast<void*>(-1));
    close(fd);
#elif defined(_WIN32)
    HANDLE hmap = CreateFileMapping(hfile, NULL, PAGE_READONLY, 0, 0, NULL);
    assert(hmap != NULL);
    m_mapBuffer = static_cast<char*>(
                MapViewOfFile(hmap, FILE_MAP_READ,
                    mapOffset >> 32, static_cast<uint32_t>(mapOffset),
                    m_mapBufferSize));
    assert(m_mapBuffer != NULL);
    CloseHandle(hmap);
    CloseHandle(hfile);
#else
#error "Not implemented"
#endif

    m_viewBuffer = m_mapBuffer + alignOffset;
    m_offset = 0;
}

MappedReader::~MappedReader()
{
#if defined(SOURCEWEB_UNIX)
    munmap(m_mapBuffer, m_mapBufferSize);
#elif defined(_WIN32)
    UnmapViewOfFile(m_mapBuffer);
#else
#error "Not implemented"
#endif
}

void MappedReader::seek(uint64_t offset)
{
    assert(offset <= m_viewSize);
    m_offset = offset;
}

char *MappedReader::readDataInternal(size_t size)
{
    //rekao bi da je slucajna vrednost koju procita, pa se pogubi sa ofsetom
//    , nemoguce da dobije da je size=2573,
    //TODO proveri mapiranje fajla
    size_t newOffset = m_offset + size;
    assert(newOffset >= m_offset && newOffset <= m_viewSize);
    size_t origOffset = m_offset;
    m_offset += size;
    return m_viewBuffer + origOffset;
}

Buffer MappedReader::readData(size_t size)
{
    return Buffer::fromMappedBuffer(readDataInternal(size), size);
}

void MappedReader::readData(void *output, size_t size)
{
    memcpy(output, readDataInternal(size), size);
}


///////////////////////////////////////////////////////////////////////////////
// UnmappedReader

UnmappedReader::UnmappedReader(const std::string &path)
{
    const char *pathPtr = path.c_str();
#if defined(SOURCEWEB_UNIX)
    int fd = EINTR_LOOP(open(pathPtr, O_RDONLY | O_CLOEXEC));
    m_fp = fdopen(fd, "r");
#else
    m_fp = fopen(pathPtr, "rb");
#endif
    assert(m_fp != NULL);
    Seek64(m_fp, 0, SEEK_END);
    m_fileSize = Tell64(m_fp);
    Seek64(m_fp, 0, SEEK_SET);
    m_offset = 0;
}

UnmappedReader::~UnmappedReader()
{
    fclose(m_fp);
}

void UnmappedReader::seek(uint64_t offset)
{
    if (offset - m_offset < static_cast<uint64_t>(kMaxAlign)) {
        char padding[kMaxAlign];
        readData(padding, offset - m_offset);
    } else {
        Seek64(m_fp, offset, SEEK_SET);
        assert(Tell64(m_fp) == offset);
        m_offset = offset;
    }
}

Buffer UnmappedReader::readData(size_t size)
{
    Buffer tmp(size);
    readData(tmp.data(), size);
    return std::move(tmp);
}

void UnmappedReader::readData(void *output, size_t size)
{
    size_t amount = fread(output, 1, size, m_fp);
    assert(amount == size);
    m_offset += size;
}

} // namespace indexdb
