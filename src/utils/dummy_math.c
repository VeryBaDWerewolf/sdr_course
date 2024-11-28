#include "dummy_math.h"
#include <stdlib.h>
#include <stdint.h>

int16_t *dummy_conv(int16_t *u, unsigned int n, int16_t *v, unsigned int m){
    int16_t *res = calloc(sizeof(int16_t), (n+m-1));
    for(int i = 0; i<(n+m-1); i++)
        for(int j = 0; j<i; j++)
            res[i] += u[i-j]*v[j];

    return res;
}
