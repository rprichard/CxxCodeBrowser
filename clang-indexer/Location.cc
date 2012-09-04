#include "Location.h"

#include <clang/Basic/FileManager.h>

namespace indexer {

Location convertLocation(
        clang::SourceManager *pSM,
        clang::SourceLocation loc)
{
    Location result = { "", 0, 0 };
    if (loc.isInvalid())
        return result;
    clang::SourceLocation sloc = pSM->getSpellingLoc(loc);
    clang::FileID fid = pSM->getFileID(sloc);
    unsigned int offset = pSM->getFileOffset(sloc);
    const clang::FileEntry *pFE = pSM->getFileEntryForID(fid);
    result.filename = pFE != NULL ?
                pFE->getName() : pSM->getBuffer(fid)->getBufferIdentifier();
    // TODO: The comment for getLineNumber says it's slow.  Is this going to
    // be a problem?
    result.line = pSM->getLineNumber(fid, offset);
    result.column = pSM->getColumnNumber(fid, offset);
    return result;
}

} // namespace indexer
