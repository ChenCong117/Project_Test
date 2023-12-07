
#ifndef __TYPEDEF_H__
#define __TYPEDEF_H__

typedef unsigned long 			uint32;
typedef unsigned short          uint16;
typedef unsigned char           uint8;

typedef signed long             int32;
typedef signed short            int16;
typedef signed char             int8;

typedef unsigned long 			uint32_t;
typedef unsigned short			uint16_t;
typedef unsigned char			uint8_t;

typedef signed long 			int32_t;
typedef signed short			int16_t;
typedef signed char				int8_t;

typedef unsigned char 			bool;
typedef unsigned char 			bool_t;

#define true					1
#define false 					0

#ifndef null
#define null ((void *) 0L)
#endif


// set 8/16/32-bit value bits:
// n: target variable
// s: start bit (high bit)
// e: end bit (low bit)
// v: value to be set
// note: not support "s = 31/15/7, e = 0", you can set the value by "=" directly...
#define SET_V32_BITS(n, s, e, v)	do{(n) = ((n) & ~(((uint32)(1UL << ((s) - (e) + 1)) - 1) << (e))) | (((uint32)(v)) << (e));}while(0)
#define SET_V16_BITS(n, s, e, v)	do{(n) = ((n) & ~(((uint16)(1 << ((s) - (e) + 1)) - 1) << (e))) | (((uint32)(v)) << (e));}while(0)
#define SET_V8_BITS(n, s, e, v)		do{(n) = ((n) & ~(((uint8)(1 << ((s) - (e) + 1)) - 1) << (e))) | (((uint32)(v)) << (e));}while(0)
// for easy use:
#define SET_BITS(n, s, e, v)	SET_V8_BITS((n), (s), (e), (v))

// endian swap:
#define V32_ENDIAN_SWAP(n)	((((((uint32)(n)) >>  0) & 0xFF) << 24) | \
							 (((((uint32)(n)) >>  8) & 0xFF) << 16) | \ 
							 (((((uint32)(n)) >> 16) & 0xFF) <<  8) | \
							 (((((uint32)(n)) >> 24) & 0xFF) <<  0) )
#define V16_ENDIAN_SWAP(n)	((((((uint16)(n)) >>  0) & 0xFF) <<  8) | \
							 (((((uint16)(n)) >>  8) & 0xFF) <<  0) )




#endif	/* __TYPEDEF_H__ */