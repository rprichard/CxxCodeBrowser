#include <vector>

void bool_func(bool x) {}

namespace NS {
    int ns_i;
    namespace SubNS {
        int subns_i;
    }
};

// BUG: the NS ref here should be the NS symbol.
// BUG: the SubNS ref here should be the NS::SubNS symbol.
namespace FooBar = NS::SubNS;

using NS::ns_i;

void inc_ns_i() {
    // NOTE: the ns_i ref here is NS::ns_i, which I suppose is OK.
    ns_i++;
    // NOTE: the FooBar ref here is the FooBar symbol.
    FooBar::subns_i++;
}

struct A {
    int i;
};

struct B : A {
    typedef A Base;

    // BUG: ref on Base should point at typedef above.
    // BUG: ref on i should point at A::i.
    using Base::i;

    void func() {
        // NOTE: the i ref here is A::i.  I suppose that's OK.
        i++;
        Base b;
    }
};

template <typename A, typename B>
struct Vector {
    struct Int { int i; };
    void clear();
};

// BUG: refs on A and B should refer to Vector::A and Vector::B.
template <typename A, typename B>
// BUG: refs on A and B should refer to Vector::A and Vector::B.
void Vector<A, B>::clear()
{
}

// BUG: ref on Vector should be Vector<bool, void>
template <>
struct Vector<bool, void>
{
    // BUG: ref on clear should be Vector<bool, void>::clear()
    void clear();

    struct Float { float f; };
};

// BUG: ref on Vector should be Vector<bool, void>
// BUG: ref on clear should be Vector<bool, void>::clear()
void Vector<bool, void>::clear()
{
}

// BUG: ref on second and third T1 should be add(T1, T2)::T1
// BUG: ref on second T2 should be add(T1, T2)::T2
template <typename T1, typename T2> T1 add(T1 x, T2 y) { return x + y; }
// NOTE: The add ref is add<A, float>(A, float)
template <> A add<A, float>(A x, float y) { A ret = { (int)y }; return ret; }

int main()
{
    A var;

    {
        // BUG: ref on Vector should be Vector
        Vector<int, int> v1;
        Vector<int, int>::Int v1int;
        // BUG: ref on Vector should be Vector<bool, void>
        Vector<bool, void> v2;
        // BUG: ref on Vector should be Vector<bool, void>
        // BUG: ref on Float should be Vector<bool, void>::Float
        Vector<bool, void>::Float v2float;
        v1.clear();
        v2.clear();
    }

    {
        // BUG: ref on vector should be std::vector
        std::vector<int> vi;
        // BUG: ref on vector should be std::vector
        std::vector<float> vf;
        // BUG: ref on vector should be std::vector<bool>  (I think?)
        std::vector<bool> vb;
        vi.push_back(1);
        vf.push_back(1);
        vb.push_back(true);
    }  
}
