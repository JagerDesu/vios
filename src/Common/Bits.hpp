#pragma once

/* shift and rotate helpers */
#define LSL(val, shift) \
    (((shift) >= 32) ? 0 : ((val) << (shift)))
#define LSR(val, shift) \
    (((shift) >= 32) ? 0 : ((val) >> (shift)))
#define ASR_SIMPLE(val, shift) \
    (((int)(val)) >> (shift))
#define ASR(val, shift) \
    (((shift) >= 32) ? (BIT(val, 31) ? (int)-1 : 0) : (((int)(val)) >> (shift)))
#define ROR(val, shift) \
    (((val) >> (shift)) | ((val) << (32 - (shift))))

/* bit manipulation macros */
#define BIT(x, bit) ((x) & (1 << (bit)))
#define BIT_SHIFT(x, bit) (((x) >> (bit)) & 1)
#define BITS(x, high, low) ((x) & (((1<<((high)-(low)+1))-1) << (low)))
#define BITS_SHIFT(x, high, low) (((x) >> (low)) & ((1<<((high)-(low)+1))-1))
#define BIT_SET(x, bit) (((x) & (1 << (bit))) ? 1 : 0)

#define ISNEG(x) BIT((x), 31)
#define ISPOS(x) (!(BIT(x, 31)))

#define CHANGE_BIT(val, num, bitval) (val = (val & ~(1 << num)) | ((!!bitval) << num))

/* 32-bit sign extension */
//#define SIGN_EXTEND(val, topbit) (BIT(val, topbit) ? ((val) | (0xffffffff << (topbit))) : (val))
#define SIGN_EXTEND(val, topbit) (ASR_SIMPLE(LSL(val, 32-(topbit)), 32-(topbit)))