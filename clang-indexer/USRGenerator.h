#ifndef INDEXER_USRGENERATOR_H
#define INDEXER_USRGENERATOR_H

#include <clang/AST/Decl.h>
#include <llvm/ADT/SmallString.h>

namespace indexer {

// Returns true on success.
bool getDeclCursorUSR(const clang::Decl *D, llvm::SmallVectorImpl<char> &Buf);

} // namespace indexer

#endif // INDEXER_USRGENERATOR_H
