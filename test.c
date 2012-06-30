
#include <stdio.h>

#define G(x) x * 2

#define F(x) G(x)

#define G(x) x * 10

int main(void)
{
    return F(1);
}
