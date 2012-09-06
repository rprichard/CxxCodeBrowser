#ifndef INDEXER_USRGENERATOR_H
#define INDEXER_USRGENERATOR_H

#include <clang/AST/Decl.h>
#include <llvm/ADT/SmallString.h>

namespace indexer {

// Returns false on error.  The Buf is not guaranteed to be NUL-terminated.
bool getDeclCursorUSR(const clang::Decl *D, llvm::SmallVectorImpl<char> &Buf);

} // namespace indexer

#endif // INDEXER_USRGENERATOR_H
