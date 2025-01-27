/*
	Encode and decode algorithms for
	IMA ADPCM

	2019-2022 by superctr.
*/

//#define IMA_HIGHPASS // Enable highpass filtering. Useful for files containing multiple samples.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

static const uint16_t ima_step_table[89] = {
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 
	19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 
	130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
	337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
	876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 
	2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
	5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767 
};

static inline int16_t ima_step(uint8_t step, int16_t* history, uint8_t* step_hist)
{
	static const int8_t adjust_table[8] = {
		-1,-1,-1,-1,2,4,6,8
	};

	uint16_t step_size = ima_step_table[*step_hist];
	int32_t delta = step_size >> 3;
	if(step & 1)
		delta += step_size >> 2;
	if(step & 2)
		delta += step_size >> 1;
	if(step & 4)
		delta += step_size;
	if(step & 8)
		delta = -delta;

#ifdef IMA_HIGHPASS
	int32_t out = CLAMP(((delta << 8) + ((int32_t)(*history) * 245)) >> 8, -32768, 32767);
#else
	int32_t out = CLAMP((int32_t)(*history) + delta, -32768, 32767); // Saturate output
#endif
	*history = out;
	int8_t adjusted_step = *step_hist + adjust_table[step & 7];
	*step_hist = CLAMP(adjusted_step, 0, 88);

	return out;
}

static inline uint8_t ima_encode_step(int16_t input, int16_t* history, uint8_t *step_hist)
{
	int bit;
	uint16_t step_size = ima_step_table[*step_hist];
	int32_t delta = input - *history;
	uint8_t adpcm_sample = (delta < 0) ? 8 : 0;
	delta = abs(delta);
	for(bit=3; bit--; )
	{
		if(delta > step_size)
		{
			adpcm_sample |= (1 << bit);
			delta -= step_size;
		}
		step_size >>= 1;
	}
	ima_step(adpcm_sample, history, step_hist);
	return adpcm_sample;
}

void dvi_encode(int16_t *buffer,uint8_t *outbuffer,long len)
{
	long i;

	int16_t history = 0;
	uint8_t step_hist = 0;
	uint8_t buf_sample = 0, nibble = 0;

	for(i=0;i<len;i++)
	{
		int16_t sample = *buffer++;
		int step = ima_encode_step(sample, &history, &step_hist);
		if(nibble)
			*outbuffer++ = buf_sample | (step&15);
		else
			buf_sample = (step&15)<<4;
		nibble^=1;
	}
}

void dvi_decode(uint8_t *buffer,int16_t *outbuffer,long len)
{
	long i;

	int16_t history = 0;
	uint8_t step_hist = 0;
	uint8_t nibble = 0;

	for(i=0;i<len;i++)
	{
		int8_t step = (*(int8_t*)buffer) << nibble;
		step >>= 4;
		if(nibble)
			buffer++;
		nibble^=4;
		*outbuffer++ = ima_step(step, &history, &step_hist);
	}
}

void ima_encode(int16_t *buffer,uint8_t *outbuffer,long len)
{
	long i;

	int16_t history = 0;
	uint8_t step_hist = 0;
	uint8_t buf_sample = 0, nibble = 0;

	for(i=0;i<len;i++)
	{
		int16_t sample = *buffer++;
		int step = ima_encode_step(sample, &history, &step_hist);
		if(nibble)
			*outbuffer++ = buf_sample | ((step&15)<<4);
		else
			buf_sample = step&15;
		nibble^=1;
	}
}

void ima_decode(uint8_t *buffer,int16_t *outbuffer,long len)
{
	long i;

	int16_t history = 0;
	uint8_t step_hist = 0;
	uint8_t nibble = 4;

	for(i=0;i<len;i++)
	{
		int8_t step = (*(int8_t*)buffer) << nibble;
		step >>= 4;
		if(!nibble)
			buffer++;
		nibble^=4;
		*outbuffer++ = ima_step(step, &history, &step_hist);
	}
}
