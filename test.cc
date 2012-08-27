
template <int i>
int addK(int j) {
    return i + j;
}

template <int i>
int addM(int j) {
    return i + j;
}

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define G(x) x * 2
#define F(x) G(x)

namespace NS {
    struct Foo {
        int f2;
    } foo;

    struct Bar {
        Bar();
    };

    Bar::Bar() {

    }
}

int main(void)
{
    int tmp = MAX(3, 4);
    int tmp2 = addK<4>(tmp) + addK<6>(tmp);

    NS::foo.f2 = tmp;
    NS::Bar b1;
    NS::Bar b2;

    return F(1) + MAX(MAX(1, 2), 5) + tmp + tmp2;
}
