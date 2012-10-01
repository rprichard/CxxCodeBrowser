#ifndef INDEXER_NAMEGENERATOR_H
#define INDEXER_NAMEGENERATOR_H

#include <clang/AST/Decl.h>

namespace indexer {

// Returns true on success.
bool getDeclName(clang::NamedDecl *decl, std::string &output);

} // namespace indexer

#endif // INDEXER_NAMEGENERATOR_H
