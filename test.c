
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define G(x) x * 2
#define F(x) G(x)

struct Foo {
    int f2;
} foo;

int main(void)
{
    int tmp = MAX(3, 4);

    foo.f2 = tmp;

    return F(1) + MAX(MAX(1, 2), 5) + tmp;
}
