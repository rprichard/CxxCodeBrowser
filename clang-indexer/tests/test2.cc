// Various interesting test cases related to the representation of parameter
// types in symbol names.  It's important to canonicalize the types and to
// strip qualifiers.

typedef int INT;
typedef int *PINT;

struct {
    int m;
} globvar1, globvar2;

struct B {};

void f1(INT *i);
void f1(const int *j);
void f1(INT i);
void f1(PINT pi);

void f1(int *i) {}
void f1(const INT *j) {}
void f1(int i) {}

void f2(B a) {}

void f3(B a);
void call_f3() { B tmp; i(tmp); }
void f3(::B a) {}

// The qualifier from f4's parameter type should be stripped.
void f4(const int p);
void f4(int p)
{
}

namespace {
    struct A {};
    typedef A B;
    void f(B a);
    void f(B a) {}
    void g(::B a) {}
}

int main() {
    globvar1 = globvar2;
}
