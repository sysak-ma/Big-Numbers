#include <stdio.h>
#include <stdlib.h>
#include "bn.h"



int main()
{
    bn *a = bn_factorial(228);
    bn_print(a);
    bn_delete(a);
    return 0;
}
