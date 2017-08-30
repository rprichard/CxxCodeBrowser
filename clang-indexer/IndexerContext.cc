#include "IndexerContext.h"

#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>

#include "../libindexdb/IndexDb.h"
#include "../libindexdb/IndexArchiveBuilder.h"
#include "NameGenerator.h"
#include "Util.h"

namespace indexer {

///////////////////////////////////////////////////////////////////////////////
// Reference Types

static const char *refTypeNames[] = {
    "Address-Taken",
    "Assigned",
    "Base-Class",
    "Called",
    "Declaration",
    "Defined-Test",
    "Definition",
    "Expansion",
    "Included",
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

static const char *symbolTypeNames[] = {
    "Class",
    "Constructor",
    "Destructor",
    "Enum",
    "Enumerator",
    "Field",
    "Function",
    "GlobalVariable",
    "LocalVariable",
    "Macro",
    "Method",
    "Namespace",
    "Parameter",
    "Path",
    "Struct",
    "Typedef",
    "Union",
    "Interface"
};

static_assert(sizeof(symbolTypeNames) / sizeof(symbolTypeNames[0]) == ST_Max,
              "dimension of symbolTypeNames array does not equal ST_Max");


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
        const std::string &pathSymbolName) :
    m_context(context),
    m_clangFileID(fileID),
    m_index(new indexdb::Index),
    m_indexPathID(indexdb::kInvalidID),
    m_builder(*m_index, /*createIndexTables=*/false)
{
    std::fill(&m_refTypeIDs[0],
              &m_refTypeIDs[RT_Max],
              indexdb::kInvalidID);
    std::fill(&m_symbolTypeIDs[0],
              &m_symbolTypeIDs[ST_Max],
              indexdb::kInvalidID);
    m_indexPathID = m_builder.insertSymbol(pathSymbolName.c_str());
    m_builder.recordSymbol(m_indexPathID, getSymbolTypeID(ST_Path));
}

indexdb::ID IndexerFileContext::createRefTypeID(RefType refType)
{
    m_refTypeIDs[refType] = m_builder.insertRefType(refTypeNames[refType]);
    return m_refTypeIDs[refType];
}

indexdb::ID IndexerFileContext::createSymbolTypeID(SymbolType symbolType)
{
    m_symbolTypeIDs[symbolType] =
            m_builder.insertSymbolType(symbolTypeNames[symbolType]);
    return m_symbolTypeIDs[symbolType];
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
    std::string pathSymbolName = "@";
    {
        const clang::FileEntry *pFE =
                m_sourceManager.getFileEntryForID(fileID);
        if (pFE != NULL) {
            llvm::StringRef fname = pFE->getName();
            char* filename = portableRealPath(fname.data());
            if (filename != NULL) {
                pathSymbolName += filename;
                free(filename);
            }
        } else {
            pathSymbolName +=
                    m_sourceManager.getBuffer(fileID)->getBufferIdentifier();
        }
        if (pathSymbolName.size() == 1)
            pathSymbolName += "<blank>";
    }

    IndexerFileContext *ret = NULL;

    {
        auto it = m_fileNameMap.find(pathSymbolName);
        if (it != m_fileNameMap.end())
            ret = it->second;
    }

    if (ret == NULL) {
        ret = new IndexerFileContext(*this, fileID, pathSymbolName);
        m_fileNameMap[pathSymbolName] = ret;
        m_fileContextSet.insert(ret);
        assert(pathSymbolName[0] == '@');
        m_archive.insert(pathSymbolName.substr(1), ret->index());
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
