/*
 * Test for various required C++11 features.
 *
 * These compilers should pass:
 *  - G++ 4.6 and up.
 *  - Clang 3.1 and up.
 *  - No current version of MSVC.  (VC11 lacks deleted and defaulted functions,
 *    which is currently easy to work around if VC11 support is desired.)
 */

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename T>
int callFunc(T func) { return func(1); }

// Forward declarations for enums
enum E : int;
E i;

// rvalue references and defaulted functions
struct A {
    A();
    A(A &other) = delete;
    A(A &&other) {}
    A &operator=(A &other) = delete;
    A &operator=(A &&other) { return *this; }
};
struct B {
    B();
    B(B &other) = delete;
    B(B &&other) = default;
    B &operator=(B &other) = delete;
    B &operator=(B &&other) = default;
    A a;
};
void func(A other1, A other2) {
    A tmp1(std::move(other1));
    A tmp2 = std::move(other2);
}

// decltype
int dtTestVar1;
decltype(dtTestVar1) dtTestVar2;

int main()
{
    std::vector<int> vi;
    vi.push_back(1);
    vi.push_back(2);
    vi.push_back(3);

    // C++11 libraries
    std::unique_ptr<std::vector<int> > pvi(new std::vector<int>);
    std::unordered_map<int, int> umii;

    // For-range syntax
    for (auto i : vi) {}

    // Local type as template argument and lambdas
    struct IntCompare { bool operator()(int x, int y) { return x < y; } };
    std::sort(vi.begin(), vi.end(), IntCompare());
    std::sort(vi.begin(), vi.end(), [](int x, int y) { return x < y; });
    callFunc([](int x) { return x * x; });

    return 0;
}
