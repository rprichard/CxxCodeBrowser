#include "Location.h"

#include <clang/Basic/FileManager.h>
#include <cstdlib>
#include <climits>

#include "IndexBuilder.h"

namespace indexer {

std::string Location::toString(IndexBuilder &builder) const
{
    std::stringstream ss;
    ss << builder.lookupPath(fileID) << ":" << line << ":" << column;
    return ss.str();
}

LocationConverter::LocationConverter(
        clang::SourceManager &sourceManager,
        IndexBuilder &builder) :
    m_sourceManager(sourceManager), m_builder(builder)
{
}

Location LocationConverter::convert(clang::SourceLocation loc)
{
    Location result = { indexdb::kInvalidID, 0, 0 };
    const char *blankPath = "";
    if (loc.isInvalid()) {
        result.fileID = m_builder.insertPath(blankPath);
        return result;
    }
    clang::SourceLocation sloc = m_sourceManager.getSpellingLoc(loc);
    clang::FileID fid = m_sourceManager.getFileID(sloc);
    auto it = m_realpathCache.find(fid);
    if (it != m_realpathCache.end()) {
        result.fileID = it->second;
    } else {
        const clang::FileEntry *pFE = m_sourceManager.getFileEntryForID(fid);
        if (pFE != NULL) {
            char *filename = realpath(pFE->getName(), NULL);
            if (filename != NULL) {
                result.fileID = m_builder.insertPath(filename);
                free(filename);
            } else {
                result.fileID = m_builder.insertPath(blankPath);
            }
        } else {
            result.fileID = m_builder.insertPath(
                        m_sourceManager.getBuffer(fid)->getBufferIdentifier());
        }
        m_realpathCache[fid] = result.fileID;
    }
    // TODO: The comment for getLineNumber says it's slow.  Is this going to
    // be a problem?
    unsigned int offset = m_sourceManager.getFileOffset(sloc);
    result.line = m_sourceManager.getLineNumber(fid, offset);
    result.column = m_sourceManager.getColumnNumber(fid, offset);
    return result;
}

} // namespace indexer
