#include "pulse.h"
#include "dummy_math.h"
#include <stdlib.h>
#include <stdint.h>



int16_t *upsample(int16_t *symb, unsigned int size, unsigned int K){
    int16_t *upsamled = calloc(sizeof(uint16_t), size*K);
    for(int i = 0; i<size; i++)
        upsamled[i*K] = symb[i];
    return upsamled;
}

int16_t *rectangle(int16_t *symb, unsigned int size, unsigned int K){
    int16_t *ones = malloc(sizeof(int16_t)*K);
    for(int i = 0; i<K; i++)
        ones[i] = 1;
    int16_t *r = upsample(symb, size,  K);
    int16_t *res = dummy_conv(r, size*K, ones, K);

    free(ones);
    free(r);

    return res;
}

int16_t *QAM(int16_t *data, unsigned int size){
    int16_t *qam_symb = malloc(sizeof(int16_t)*(size));
    for(int i = 0; i<size; i+=2){
        qam_symb[i]   = 1-2*data[i];
        qam_symb[i+1] = 1-2*data[i+1];
    }
    return  qam_symb;
}
