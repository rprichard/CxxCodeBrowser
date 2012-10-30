
template <typename i>
struct Foo { // DEF of Foo
};

template <>
struct Foo<int> { // DEF of Foo<int>
    int special;
};

// DECL of Foo
extern template class Foo<long>;

// This currently appears as a Declaration of Foo.  Should it be Foo<int>?
extern template class Foo<int>;

int main()
{
    Foo<int> f1;  // REF of Foo<int>
    Foo<long> f2; // REF of Foo
}
