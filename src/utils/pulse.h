#pragma once
#include <stdint.h>


int16_t *upsample(int16_t *symb, unsigned int size, unsigned int K);

int16_t *rectangle(int16_t *symb, unsigned int size, unsigned int K);

int16_t *QAM(int16_t *data, unsigned int size);
