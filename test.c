#include <stdio.h>
#include <errno.h>
#include "test.h"

#define G(x) x * 2
#define F(x) G(x)

/* BUGS:

   xref G
   ==> missing use of G
*/

int main(void)
{
    int tmp = MAX(3, 4);

    return errno + F(1) + MAX(MAX(1, 2), 5);
}
