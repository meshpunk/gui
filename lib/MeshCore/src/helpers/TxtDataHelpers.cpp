#include "TxtDataHelpers.h"

void StrHelper::strncpy(char* dest, const char* src, size_t buf_sz) {
  while (buf_sz > 1 && *src) {
    *dest++ = *src++;
    buf_sz--;
  }
  *dest = 0;  // truncates if needed
}

void StrHelper::strzcpy(char* dest, const char* src, size_t buf_sz) {
  while (buf_sz > 1 && *src) {
    *dest++ = *src++;
    buf_sz--;
  }
  while (buf_sz > 0) {  // pad remaining with nulls
    *dest++ = 0;
    buf_sz--;
  }
}

#include <Arduino.h>

union int32_Float_t 
{
  int32_t Long;
  float Float;
};

#ifndef FLT_MIN_EXP
#define FLT_MIN_EXP (-999)
#endif
 
#ifndef FLT_MAX_EXP
#define FLT_MAX_EXP (999)
#endif
 
#define _FTOA_TOO_LARGE -2  // |input| > 2147483520 
#define _FTOA_TOO_SMALL -1  // |input| < 0.0000001 
 
//precision 0-9
#define PRECISION 7
 
//_ftoa function 
static void _ftoa(float f, char *p, int *status) 
{
  int32_t mantissa, int_part, frac_part;
  int16_t exp2;
  int32_Float_t x;
 
  *status = 0;
  if (f == 0.0) 
  {
    *p++ = '0';
    *p++ = '.';
    *p++ = '0';
    *p = 0;
    return;
  }
 
  x.Float = f;
  exp2 = (unsigned char)(x.Long>>23) - 127;
  mantissa = (x.Long&0xFFFFFF) | 0x800000;
  frac_part = 0;
  int_part = 0;
 
  if (exp2 >= 31) 
  {
    *status = _FTOA_TOO_LARGE;
    return;
  } 
  else if (exp2 < -23) 
  {
    *status = _FTOA_TOO_SMALL;
    return;
  } 
  else if (exp2 >= 23)
  { 
    int_part = mantissa<<(exp2 - 23);
  }
  else if (exp2 >= 0) 
  {
    int_part = mantissa>>(23 - exp2);
    frac_part = (mantissa<<(exp2 + 1))&0xFFFFFF;
  } 
  else 
  {
    //if (exp2 < 0)
    frac_part = (mantissa&0xFFFFFF)>>-(exp2 + 1);
  }
 
  if (x.Long < 0)
      *p++ = '-';
  if (int_part == 0)
    *p++ = '0';
  else 
  {
    ltoa(int_part, p, 10);
    while (*p)
      p++;
  }
  *p++ = '.';
  if (frac_part == 0)
    *p++ = '0';
  else 
  {
    char m;
 
    for (m=0; m<PRECISION; m++) 
    {
      //frac_part *= 10;
      frac_part = (frac_part<<3) + (frac_part<<1); 
      *p++ = (frac_part>>24) + '0';
      frac_part &= 0xFFFFFF;
    }
 
    //delete ending zeroes
    for (--p; p[0] == '0' && p[-1] != '.'; --p)
      ;
    ++p;
  }
  *p = 0;
}

const char* StrHelper::ftoa(float f) {
  static char tmp[16];
  int status;
  _ftoa(f, tmp, &status);
  if (status) {
    tmp[0] = '0';  // fallback/error value
    tmp[1] = 0;
  }
  return tmp;
}
