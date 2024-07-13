#include "ax25.h"
#include <stdio.h>

ax25_encode_status_t add(int *a)
{
    *a++;
    return AX25_ENC_OK;
}
int main()
{
    int a=5;
    int *x;
    //*x=;
    ax25_encode_status_t status= add(&a);

    printf("%d,%d",status,x);

    return 0;
}