#pragma once

#include <stdint.h>

typedef int32_t fixed;

struct fixed_vec2d {
	fixed x, y;
};


#define F_(x) ((fixed)(((double)(x))*65536.0))

const fixed f1 = 0x00010000;

#ifdef __cplusplus
extern "C" {
#endif
fixed fsine(int8_t angle);
fixed fcosine(int8_t angle);
fixed f32_imult(fixed multpicand, fixed mutlipler);
fixed f32_idiv(fixed dividend, fixed divisor);
#ifdef __cplusplus
}
#endif
