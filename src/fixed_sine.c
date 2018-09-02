#include "fixed_math.h"
#include <math.h>

uint16_t sine_lookup_table[] =
{
	0x0000, 0x0648, 0x0c8f, 0x12d5, 0x1917, 0x1f56, 0x2590, 0x2bc4, 
	0x31f1, 0x3817, 0x3e33, 0x4447, 0x4a50, 0x504d, 0x563e, 0x5c22, 
	0x61f7, 0x67bd, 0x6d74, 0x7319, 0x78ad, 0x7e2e, 0x839c, 0x88f5, 
	0x8e39, 0x9368, 0x987f, 0x9d7f, 0xa267, 0xa736, 0xabeb, 0xb085, 
	0xb504, 0xb968, 0xbdae, 0xc1d8, 0xc5e4, 0xc9d1, 0xcd9f, 0xd14d, 
	0xd4db, 0xd848, 0xdb94, 0xdebe, 0xe1c5, 0xe4aa, 0xe76b, 0xea09, 
	0xec83, 0xeed8, 0xf109, 0xf314, 0xf4fa, 0xf6ba, 0xf853, 0xf9c7, 
	0xfb14, 0xfc3b, 0xfd3a, 0xfe13, 0xfec4, 0xff4e, 0xffb1, 0xffec
};

fixed fsine(int8_t angle){
	fixed result;

	if (angle < 0)
	{
		int8_t i = (-angle) & 0x3f;
		if (angle == (-128)) result = 0;
		else if (angle == (-64)) result = (-f1);
		else if (angle < (-64))
			result = -((fixed)sine_lookup_table[64 - i]);
		else
			result = -((fixed)sine_lookup_table[i]);
	}
	else
	{
		int8_t i = angle & 0x3f;
		if (angle == 64) result = f1;
		else if (angle < 64)
			result = sine_lookup_table[i];
		else
			result = sine_lookup_table[64 - i];
	}

	return result;
	/*
	int8_t i = abs(angle) & 0x3f;
	fixed result;
	if (angle == -128)
		result = 0;
	else if (angle < -64)
		result = -sine_lookup_table[64 - i];
	else if (angle == -64)
		result = -f1;
	else if (angle < 0)
		result = -sine_lookup_table[i];
	else if (angle < 63)
		result = sine_lookup_table[i];
	else if (angle == 64)
		result = f1;
	else
		result = sine_lookup_table[64 - i];
	return result;*/
}

fixed fcosine(int8_t angle){
	return fsine(angle+64);
}

