#include "IndexerContext.h"

#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>

#include "../libindexdb/IndexDb.h"
#include "../libindexdb/IndexArchiveBuilder.h"
#include "NameGenerator.h"

namespace indexer {

///////////////////////////////////////////////////////////////////////////////
// Reference Types

const char *refTypeNames[] = {
    "Address-Taken",
    "Assigned",
    "Base-Class",
    "Called",
    "Declaration",
    "Defined-Test",
    "Definition",
    "Expansion",
    "Initialized",
    "Modified",
    "Namespace-Alias",
    "Other",
    "Qualifier",
    "Read",
    "Reference",
    "Undefinition",
    "Using",
    "Using-Directive",
};

static_assert(sizeof(refTypeNames) / sizeof(refTypeNames[0]) == RT_Max,
              "dimension of refTypeNames array does not equal RT_Max");


///////////////////////////////////////////////////////////////////////////////
// IndexerFileContext

Location IndexerFileContext::location(clang::SourceLocation spellingLoc)
{
    Location ret;
    ret.fileID = m_indexPathID;
    clang::SourceManager &sourceManager = m_context.sourceManager();
    unsigned int offset = sourceManager.getFileOffset(spellingLoc);
    // TODO: The comment for getLineNumber says it's slow.  Is this going to
    // be a problem?
    ret.line = sourceManager.getLineNumber(m_clangFileID, offset);
    ret.column = sourceManager.getColumnNumber(m_clangFileID, offset);
    return ret;
}

indexdb::ID IndexerFileContext::getDeclSymbolID(clang::NamedDecl *decl)
{
    // Get the symbolID for the declaration.
    indexdb::ID symbolID;
    auto it = m_declNameCache.find(decl);
    if (it != m_declNameCache.end()) {
        symbolID = it->second;
    } else {
        m_tempSymbolName.clear();
        getDeclName(decl, m_tempSymbolName);
        symbolID = m_builder.insertSymbol(m_tempSymbolName.c_str());
        m_declNameCache[decl] = symbolID;
    }
    return symbolID;
}

IndexerFileContext::IndexerFileContext(
        IndexerContext &context,
        clang::FileID fileID,
        const std::string &path) :
    m_context(context),
    m_clangFileID(fileID),
    m_path(path),
    m_index(new indexdb::Index),
    m_indexPathID(indexdb::kInvalidID),
    m_builder(*m_index, /*createLocationTables=*/false)
{
    std::fill(&m_refTypeIDs[0], &m_refTypeIDs[RT_Max], indexdb::kInvalidID);
    m_indexPathID = m_builder.insertPath(m_path.c_str());
}

indexdb::ID IndexerFileContext::createRefTypeID(RefType refType)
{
    m_refTypeIDs[refType] = m_builder.insertRefType(refTypeNames[refType]);
    return m_refTypeIDs[refType];
}


///////////////////////////////////////////////////////////////////////////////
// IndexerContext

IndexerContext::IndexerContext(
        clang::SourceManager &sourceManager,
        clang::Preprocessor &preprocessor,
        indexdb::IndexArchiveBuilder &archive) :
    m_sourceManager(sourceManager),
    m_preprocessor(preprocessor),
    m_archive(archive)
{
}

// Get the IndexerFileContext associated with the given file ID.  Create a new
// IndexerFileContext if this is the first time fileContext has been called on
// this file.  It is possible for multiple file IDs to map to the same
// IndexerFileContext object.
IndexerFileContext &IndexerContext::fileContext(clang::FileID fileID)
{
    {
        auto it = m_fileIDMap.find(fileID);
        if (it != m_fileIDMap.end())
            return *it->second;
    }

    // Get the name for the file.
    std::string name;
    {
        const clang::FileEntry *pFE =
                m_sourceManager.getFileEntryForID(fileID);
        if (pFE != NULL) {
            char *filename = realpath(pFE->getName(), NULL);
            if (filename != NULL) {
                name = filename;
                free(filename);
            } else {
                name = "<invalid>";
            }
        } else {
            name = m_sourceManager.getBuffer(fileID)->getBufferIdentifier();
        }
    }

    IndexerFileContext *ret = NULL;

    {
        auto it = m_fileNameMap.find(name);
        if (it != m_fileNameMap.end())
            ret = it->second;
    }

    if (ret == NULL) {
        ret = new IndexerFileContext(*this, fileID, name);
        m_fileNameMap[name] = ret;
        m_fileContextSet.insert(ret);
        m_archive.insert(name, ret->index());
    }

    m_fileIDMap[fileID] = ret;
    return *ret;
}

IndexerContext::~IndexerContext()
{
    for (IndexerFileContext *fileContext : m_fileContextSet)
        delete fileContext;
}

} // namespace indexer
