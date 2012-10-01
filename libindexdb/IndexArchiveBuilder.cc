#include "IndexArchiveBuilder.h"

#include <cassert>

#include "FileIo.h"
#include "IndexDb.h"
#include "sha2.h"

namespace indexdb {

IndexArchiveBuilder::~IndexArchiveBuilder()
{
    for (const auto &pair : m_indices)
        delete pair.second;
}

void IndexArchiveBuilder::insert(const std::string &entryName, Index *index)
{
    assert(m_indices.find(entryName) == m_indices.end());
    m_indices[entryName] = index;
}

Index *IndexArchiveBuilder::lookup(const std::string &entryName)
{
    auto it = m_indices.find(entryName);
    return (it != m_indices.end()) ? it->second : NULL;
}

void IndexArchiveBuilder::setReadOnly()
{
    for (const auto &pair : m_indices)
        pair.second->setReadOnly();
}

void IndexArchiveBuilder::write(const std::string &path)
{
    const int kHashByteSize = 256 / 8;
    std::string zeroHash;
    zeroHash.resize(kHashByteSize);

    Writer writer(path);
    writer.writeSignature(kIndexArchiveSignature);
    writer.writeUInt32(m_indices.size());

    std::vector<std::string> entryHashes;
    std::vector<uint64_t> entryOffsets;
    std::vector<uint64_t> entryLengths;

    // Write table-of-contents.
    uint64_t tocOffset = writer.tell();
    for (const auto &pair : m_indices) {
        writer.writeString(pair.first);
        writer.writeString(zeroHash);
        writer.writeUInt32(0); // Entry offset
        writer.writeUInt32(0); // Entry length
    }

    unsigned char hashDigest[kHashByteSize];

    // Write each file.
    for (const auto &pair : m_indices) {
        sha256_ctx hashContext;
        sha256_init(&hashContext);

        entryOffsets.push_back(writer.tell());
        writer.setSha256Hash(&hashContext);
        pair.second->write(writer);
        writer.setSha256Hash(NULL);
        const uint64_t length = writer.tell() - entryOffsets.back();
        entryLengths.push_back(length);

        // Compute a SHA-256 hash.
        sha256_final(&hashContext, hashDigest);
        entryHashes.push_back(
                    std::string(reinterpret_cast<char*>(hashDigest),
                                kHashByteSize));
    }

    // Rewrite the table-of-contents.
    writer.seek(tocOffset);
    int index = 0;
    for (const auto &pair : m_indices) {
        writer.writeString(pair.first);
        writer.writeString(entryHashes[index]);
        writer.writeUInt32(entryOffsets[index]);
        writer.writeUInt32(entryLengths[index]);
        index++;
    }
}

} // namespace indexdb
