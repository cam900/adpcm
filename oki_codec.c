/*
	Encode and decode algorithms for
	OKI ADPCM
	
	2019 by superctr.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

static const uint16_t step_table[49] = {
	16, 17, 19, 21, 23, 25, 28, 31,
	34, 37, 41, 45, 50, 55, 60, 66,
	73, 80, 88, 97, 107,118,130,143,
	157,173,190,209,230,253,279,307,
	337,371,408,449,494,544,598,658,
	724,796,876,963,1060,1166,1282,1411,1552
};

static const int8_t oki_adjust_table[8] = {
	-1,-1,-1,-1,2,4,6,8
};

static const int8_t yamaha_adjust_table[8] = {
	-1,-1,-1,-1,2,5,7,9
};

// Decode step
inline int16_t oki_step(uint8_t step, int16_t* history, uint8_t* step_hist, const int8_t* adjust_tab)
{
	uint16_t step_size = step_table[*step_hist];
	int16_t delta = step_size >> 3;
	if(step & 1)
		delta += step_size >> 2;
	if(step & 2)
		delta += step_size >> 1;
	if(step & 4)
		delta += step_size;
	if(step & 8)
		delta = -delta;
	int16_t out = *history + delta;
	*history = out = CLAMP(out, -2048, 2047);
	int8_t adjusted_step = *step_hist + adjust_tab[step & 7];
	*step_hist = CLAMP(adjusted_step, 0, 48);

	return out;
}

// Encode step
inline uint8_t oki_encode_step(int16_t input, int16_t* history, uint8_t *step_hist, const int8_t* adjust_tab)
{
	int bit;
	uint16_t step_size = step_table[*step_hist];
	int16_t delta = input - *history;
	uint8_t adpcm_sample = (delta < 0) ? 8 : 0;
	if(delta < 0)
		adpcm_sample = 8;
	delta = abs(delta);
	for(bit=3; bit--; )
	{
		if(delta >= step_size)
		{
			adpcm_sample |= (1<<bit);
			delta -= step_size;
		}
		step_size >>= 1;
	}
	oki_step(adpcm_sample,history,step_hist,adjust_tab);
	return adpcm_sample;
}

// *** OKI ***
void oki_encode(int16_t *buffer,uint8_t *outbuffer,long len)
{
	long i;

	int16_t history = 0;
	uint8_t step_hist = 0;
	uint8_t buf_sample = 0, nibble = 0;
	
	for(i=0;i<len;i++)
	{
		int16_t sample = *buffer++;
		if(sample < 0x7ff8) // round up
			sample += 8;
		sample >>= 4;
		int step = oki_encode_step(sample, &history, &step_hist, oki_adjust_table);
		if(nibble)
			*outbuffer++ = buf_sample | (step&15);
		else
			buf_sample = (step&15)<<4;
		nibble^=1;
	}
}

void oki_decode(uint8_t *buffer,int16_t *outbuffer,long len)
{
	long i;

	int16_t history = 0;
	uint8_t step_hist = 0;
	uint8_t nibble = 0;
	
	for(i=0;i<len;i++)
	{
		int8_t step = (*(int8_t*)buffer)<<nibble;
		step >>= 4;
		if(nibble)
			buffer++;
		nibble^=4;
		*outbuffer++ = oki_step(step, &history, &step_hist, oki_adjust_table) << 4;
	}
}

// *** Yamaha ADPCM-A ***
void yma_encode(int16_t *buffer,uint8_t *outbuffer,long len)
{
	long i;

	int16_t history = 0;
	uint8_t step_hist = 0;
	uint8_t buf_sample = 0, nibble = 0;
	
	for(i=0;i<len;i++)
	{
		int16_t sample = *buffer++;
		if(sample < 0x7ff8) // round up
			sample += 8;
		sample >>= 4;
		int step = oki_encode_step(sample, &history, &step_hist, yamaha_adjust_table);
		if(nibble)
			*outbuffer++ = buf_sample | (step&15);
		else
			buf_sample = (step&15)<<4;
		nibble^=1;
	}
}

void yma_decode(uint8_t *buffer,int16_t *outbuffer,long len)
{
	long i;

	int16_t history = 0;
	uint8_t step_hist = 0;
	uint8_t nibble = 0;
	
	for(i=0;i<len;i++)
	{
		int8_t step = (*(int8_t*)buffer)<<nibble;
		step >>= 4;
		if(nibble)
			buffer++;
		nibble^=4;
		*outbuffer++ = oki_step(step, &history, &step_hist, yamaha_adjust_table) << 4;
	}
}
