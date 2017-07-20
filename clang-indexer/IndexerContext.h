#ifndef INDEXER_INDEXERCONTEXT_H
#define INDEXER_INDEXERCONTEXT_H

#include <string>
#include <unordered_map>
#include <unordered_set>

#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>

#include "../libindexdb/IndexDb.h"
#include "../libindexdb/IndexArchiveBuilder.h"
#include "Location.h"
#include "IndexBuilder.h"

namespace clang {
    class NamedDecl;
    class Preprocessor;
    class SourceManager;
}

namespace indexer {

class IndexerContext;


///////////////////////////////////////////////////////////////////////////////
// Reference Types

// This list of references is C++-specific, so it belongs in IndexerContext and
// not in IndexBuilder.
enum RefType : int {
    RT_AddressTaken,
    RT_Assigned,
    RT_BaseClass,
    RT_Called,
    RT_Declaration,
    RT_DefinedTest,
    RT_Definition,
    RT_Expansion,
    RT_Included,
    RT_Initialized,
    RT_Modified,
    RT_NamespaceAlias,
    RT_Other,
    RT_Qualifier,
    RT_Read,
    RT_Reference,
    RT_Undefinition,
    RT_Using,
    RT_UsingDirective,
    RT_Max
};

enum SymbolType : int {
    ST_Class,
    ST_Constructor,
    ST_Destructor,
    ST_Enum,
    ST_Enumerator,
    ST_Field,
    ST_Function,
    ST_GlobalVariable,
    ST_LocalVariable,
    ST_Macro,
    ST_Method,
    ST_Namespace,
    ST_Parameter,
    ST_Path,
    ST_Struct,
    ST_Typedef,
    ST_Union,
    ST_Interface,
    ST_Max
};


///////////////////////////////////////////////////////////////////////////////
// IndexerFileContext

class IndexerFileContext
{
    friend class IndexerContext;
public:
    indexdb::Index *index() { return m_index; }
    IndexBuilder &builder() { return m_builder; }
    indexdb::ID getRefTypeID(RefType refType) {
        indexdb::ID id = m_refTypeIDs[refType];
        if (id != indexdb::kInvalidID)
            return id;
        return createRefTypeID(refType);
    }
    indexdb::ID getSymbolTypeID(SymbolType symbolType) {
        indexdb::ID id = m_symbolTypeIDs[symbolType];
        if (id != indexdb::kInvalidID)
            return id;
        return createSymbolTypeID(symbolType);
    }
    Location location(clang::SourceLocation spellingLoc);
    indexdb::ID getDeclSymbolID(clang::NamedDecl *decl);

    // Disallow copying of this class.
    IndexerFileContext(IndexerFileContext &other) = delete;
    IndexerFileContext &operator=(IndexerFileContext &other) = delete;

private:
    IndexerFileContext(
            IndexerContext &context,
            clang::FileID fileID,
            const std::string &pathSymbolName);
    indexdb::ID createRefTypeID(RefType refType);
    indexdb::ID createSymbolTypeID(SymbolType symbolType);

    IndexerContext &m_context;
    clang::FileID m_clangFileID;
    indexdb::Index *m_index;
    indexdb::ID m_indexPathID;
    IndexBuilder m_builder;

    std::string m_tempSymbolName;
    std::unordered_map<clang::NamedDecl*, indexdb::ID> m_declNameCache;
    indexdb::ID m_refTypeIDs[RT_Max];
    indexdb::ID m_symbolTypeIDs[ST_Max];
};


///////////////////////////////////////////////////////////////////////////////
// IndexerContext

class IndexerContext
{
public:
    IndexerContext(
            clang::SourceManager &sourceManager,
            clang::Preprocessor &preprocessor,
            indexdb::IndexArchiveBuilder &archive);
    ~IndexerContext();
    clang::SourceManager &sourceManager() { return m_sourceManager; }
    clang::Preprocessor &preprocessor() { return m_preprocessor; }
    indexdb::IndexArchiveBuilder &archive() { return m_archive; }
    IndexerFileContext &fileContext(clang::FileID fileID);

    // Disallow copying of this class.
    IndexerContext(IndexerContext &other) = delete;
    IndexerContext operator=(IndexerContext &other) = delete;

private:
    struct FileIDHash {
        size_t operator()(clang::FileID fileID) const {
            return fileID.getHashValue();
        }
    };

    clang::SourceManager &m_sourceManager;
    clang::Preprocessor &m_preprocessor;
    indexdb::IndexArchiveBuilder &m_archive;
    std::unordered_map<clang::FileID, IndexerFileContext*, FileIDHash> m_fileIDMap;
    std::unordered_map<std::string, IndexerFileContext*> m_fileNameMap;
    std::unordered_set<IndexerFileContext*> m_fileContextSet;
};

} // namespace indexer

#endif // INDEXER_INDEXERCONTEXT_H
