#!/bin/sh

set -e

directives() {
cat <<EOF
if
ifdef
ifndef
elif
else
endif
include
define
undef
line
error
pragma
warning
include_next
import
EOF
}

keywords() {
cat <<EOF
alignas
alignof
asm
auto
bool
break
case
catch
char
char16_t
char32_t
class
const
constexpr
const_cast
continue
decltype
default
delete
double
do
dynamic_cast
else
enum
explicit
export
extern
false
float
for
friend
goto
if
inline
int
long
mutable
namespace
new
noexcept
nullptr
operator
private
protected
public
register
reinterpret_cast
return
short
signed
sizeof
static
static_assert
static_cast
struct
switch
template
this
thread_local
throw
true
try
typedef
typeid
typename
union
unsigned
using
virtual
void
volatile
wchar_t
while
and
not_eq
and_eq
or
bitand
or_eq
bitor
xor
compl
xor_eq
not
EOF
}

rm -f CXXSyntaxHighlighterDirectives.h
rm -f CXXSyntaxHighlighterKeywords.h

echo -n "Regenerating CXXSyntaxHighlighterDirectives.h... "
directives | gperf --language=C++ --enum --class-name=Directives --output-file=CXXSyntaxHighlighterDirectives.h
echo success

echo -n "Regenerating CXXSyntaxHighlighterKeywords.h... "
keywords   | gperf --language=C++ --enum --class-name=Keywords   --output-file=CXXSyntaxHighlighterKeywords.h
echo success
