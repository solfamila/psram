/* ------------------------------------------------------------------------ */
/* Copyright (c) 2018 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED.  */
/* These coded instructions, statements, and computer programs ("Cadence    */
/* Libraries") are the copyrighted works of Cadence Design Systems Inc.	    */
/* Cadence IP is licensed for use with Cadence processor cores only and     */
/* must not be used for any other processors and platforms. Your use of the */
/* Cadence Libraries is subject to the terms of the license agreement you   */
/* have entered into with Cadence Design Systems, or a sublicense granted   */
/* to you by a direct Cadence licensee.                                     */
/* ------------------------------------------------------------------------ */
/*  IntegrIT, Ltd.   www.integrIT.com, info@integrIT.com                    */
/*                                                                          */
/* DSP Library                                                              */
/*                                                                          */
/* This library contains copyrighted materials, trade secrets and other     */
/* proprietary information of IntegrIT, Ltd. This software is licensed for  */
/* use with Cadence processor cores only and must not be used for any other */
/* processors and platforms. The license to use these sources was given to  */
/* Cadence, Inc. under Terms and Condition of a Software License Agreement  */
/* between Cadence, Inc. and IntegrIT, Ltd.                                 */
/* ------------------------------------------------------------------------ */
/*          Copyright (C) 2015-2018 IntegrIT, Limited.                      */
/*                      All Rights Reserved.                                */
/* ------------------------------------------------------------------------ */
#ifndef __NATUREDSP_SIGNAL_VECTOR_H__
#define __NATUREDSP_SIGNAL_VECTOR_H__

#include "NatureDSP_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
  Vector operations:
  vec_dot              Vector Dot Product
  vec_add              Vector Sum
  vec_power            Power of a Vector
  vec_shift,vec_scale  Vector Scaling with Saturation
  vec_bexp             Common Exponent
  vec_min,vec_max      Vector Min/Max
  Operations with emulated floating point format:
  vec_add add
  vec_mul multiply
  vec_mac multiply accumulate
  vec_dot dot-product
  vec_dot_batch batch computation of dot-products for multiple inputs
  These routines make basic operations with emulated floating point data 
  representing in pairs 32-bit mantissa/16-bit exponent. All functions form 
  normalized output. Denormalized numbers on input may cause degraded accuracy. 
  Special numbers are represented as following:
  - zero: mantissa is 0, exponent does not care
  - positive infinity: mantissa 0x7fffffff. exponent 0x7fff
  - negative infinity: mantissa 0x80000000. exponent 0x7fff
===========================================================================*/
/*-------------------------------------------------------------------------
  Vector Dot product
  These routines take two vectors and calculates their dot product.
  Two versions of routines are available: regular versions (vec_dot64x32,
  vec_dot64x64, vec_dot64x64i, vec_dot32x16, vec_dot32x32,vec_dot16x16, 
  vec_dotf) work with arbitrary arguments, faster versions (vec_dot64x32_fast, 
  vec_dot64x64_fast, vec_dot64x64i_fast, vec_dot32x16_fast, vec_dot32x32_fast,
  vec_dot16x16_fast) apply some restrictions.  
  NOTE:
  vec_dot16x16_fast utilizes 32-bit saturating accumulator, so input data 
  should be scaled properly to avoid erroneous results.

  Precision: 
  64x32  64x32-bit data, 64-bit output (fractional multiply Q63xQ31->Q63)
  64x64  64x64-bit data, 64-bit output (fractional multiply Q63xQ63->Q63)
  64x64i 64x64-bit data, 64-bit output (low 64 bit of integer multiply)
  32x32  32x32-bit data, 64-bit output
  32x16  32x16-bit data, 64-bit output
  16x16  16x16-bit data, 64-bit output for regular version and 32-bit for 
                        fast version
  f      single precision floating point

  Input:
  x[N]  input data, Q15, Q31, Q63 or floating point
  y[N]  input data, Q15, Q31, Q63 or floating point
  N	    length of vectors
  Returns:
  dot product of all data pairs, Q31, Q63 or floating point

  Restrictions:
  Regular versions:
    none
  Faster versions:
    x,y - aligned on 8-byte boundary
    N   - multiple of 4
-------------------------------------------------------------------------*/
int64_t   vec_dot64x32 (const int64_t   * x,const int32_t   * y,int N);
int64_t   vec_dot64x64 (const int64_t   * x,const int64_t   * y,int N);
int64_t   vec_dot64x64i(const int64_t   * x,const int64_t   * y,int N);
int64_t   vec_dot32x32 (const int32_t   * x,const int32_t   * y,int N);
int64_t   vec_dot32x16 (const int32_t   * x,const int16_t   * y,int N);
int64_t   vec_dot16x16 (const int16_t   * x,const int16_t   * y,int N);
float32_t vec_dotf     (const float32_t * x,const float32_t * y,int N);

int64_t vec_dot64x32_fast (const int64_t * x,const int32_t * y,int N);
int64_t vec_dot64x64_fast (const int64_t * x,const int64_t * y,int N);
int64_t vec_dot64x64i_fast(const int64_t * x,const int64_t * y,int N);
int64_t vec_dot32x32_fast (const int32_t * x,const int32_t * y,int N);
int64_t vec_dot32x16_fast (const int32_t * x,const int16_t * y,int N);
int32_t vec_dot16x16_fast (const int16_t * x,const int16_t * y,int N);

/*-------------------------------------------------------------------------
  Batch Computation of Vector Dot products
  These routines take a set of input vectors and compute their dot product 
  with specific reference data.
  Two versions of routines are available: 
  - regular versions (vec_dot_batch8x8, vec_dot_batch8x16, vec_dot_batch16x16, 
    vec_dot_batchf). They work with arbitratry arguments
  - fast versions (vec_dot_batch8x8_fast, vec_dot_batch8x16_fast, 
    vec_dot_batch16x16_fast, vec_dot_batchf_fast) apply some restrictions.  

  Precision: 
  8x8    8x8-bit data, 16-bit output (fractional multiply Q7xQ7->Q15)
  8x16   8x16-bit data, 16-bit output (fractional multiply Q7xQ15->Q15)
  16x16  16x16-bit data, 16-bit output (fractional multiply Q15xQ15->Q31)
  f      single precision floating point

  Input:
  x[N]     input (reference) data, Q7, Q15 or floating point
  y[M][N]  pointers to M input vectors, Q7, Q15 or floating point
  N        length of vectors
  M        number of vectors
  rsh      right shift for output (for fixed point API only!)
  Output:
  z[M]     dot products between references and M inputs, Q15, Q31 or 
           floating point

  Restrictions:
  Regular versions:
    none
  Faster versions:
    x,y[m] - aligned on 8-byte boundary
    N      - multiple of 8
    M        multiple of 4
-------------------------------------------------------------------------*/
typedef const int8_t   * cint8ptr_t ;
typedef const int16_t   * cint16ptr_t ;
typedef const float32_t * cfloat32ptr_t ;
void vec_dot_batch8x8  (int16_t   *z, const int8_t      * x,const cint8ptr_t    * y,int rsh, int N, int M);
void vec_dot_batch8x16 (int16_t   *z, const int8_t      * x,const cint16ptr_t   * y,int rsh, int N, int M);
void vec_dot_batch16x16(int32_t   *z, const int16_t     * x,const cint16ptr_t   * y,int rsh, int N, int M);
void vec_dot_batchf    (float32_t *z, const float32_t   * x,const cfloat32ptr_t * y        , int N, int M);
void vec_dot_batch8x8_fast  (int16_t   *z, const int8_t      * x,const cint8ptr_t    * y,int rsh, int N, int M);
void vec_dot_batch8x16_fast (int16_t   *z, const int8_t      * x,const cint16ptr_t   * y,int rsh, int N, int M);
void vec_dot_batch16x16_fast(int32_t   *z, const int16_t     * x,const cint16ptr_t   * y,int rsh, int N, int M);
void vec_dot_batchf_fast    (float32_t *z, const float32_t   * x,const cfloat32ptr_t * y        , int N, int M);

/*-------------------------------------------------------------------------
  Vector Sum
  This routine makes pair wise saturated summation of vectors.
  Two versions of routines are available: regular versions (vec_add32x32, 
  vec_add16x16, vec_addf) work with arbitrary arguments, faster versions 
  (vec_add32x32_fast, vec_add16x16_fast) apply some restrictions.

  Precision: 
  32x32 32-bit inputs, 32-bit output
  16x16 16-bit inputs, 16-bit output
  f     single precision floating point

  Input:
  x[N]   input data
  y[N]   input data
  N      length of vectors
  Output:
  z[N]   output data

  Restriction:
  Regular versions (vec_add32x32, vec_add16x16, vec_addf):
  x,y,z - should not be overlapped
  Faster versions (vec_add32x32_fast, vec_add16x16_fast):
  z,x,y - aligned on 8-byte boundary
  N     - multiple of 4
-------------------------------------------------------------------------*/
void vec_add32x32 ( int32_t *  z,
              const int32_t *  x,
              const int32_t *  y,
              int N);
void vec_add32x32_fast
            ( int32_t * z,
              const int32_t *  x,
              const int32_t *  y,
              int N );
void vec_addf ( float32_t *  z,
              const float32_t *  x,
              const float32_t *  y,
              int N);
void vec_add16x16 ( int16_t *  z,
              const int16_t *  x,
              const int16_t *  y,
              int N);
void vec_add16x16_fast
            ( int16_t *  z,
              const int16_t *  x,
              const int16_t *  y,
              int N );

/*-------------------------------------------------------------------------
  Power of a Vector
  These routines compute power of vector with scaling output result by rsh 
  bits. Fixed point rountines make accumulation in the 64-bit wide 
  accumulator and output may scaled down with saturation by rsh bits. 
  So, if representation of x input is Qx, result will be represented in 
  Q(2x-rsh) format.
  Two versions of routines are available: regular versions (vec_power32x32, 
  vec_power16x16, vec_powerf) work with arbitrary arguments, faster versions 
  (vec_power32x32_fast, vec_power16x16_fast) apply some restrictions.

  Precision: 
  32x32 32x32-bit data, 64-bit output
  16x16 16x16-bit data, 64-bit output
  f     single precision floating point

  Input:
  x[N]  input data, Q31, Q15 or floating point
  rsh   right shift of result
  N     length of vector
  Returns: 
  Sum of squares of a vector, Q(2x-rsh)

  Restrictions:
  for vec_power32x32(): rsh in range 31...62
  for vec_power16x16(): rsh in range 0...31
  For regular versions (vec_power32x32, vec_power16x16, vec_powerf):
  none
  For faster versions (vec_power32x32_fast, vec_power16x16_fast ):
  x - aligned on 8-byte boundary
  N - multiple of 4
-------------------------------------------------------------------------*/
int64_t     vec_power32x32 ( const int32_t *  x,int rsh,int N);
int64_t     vec_power16x16 ( const int16_t *  x,int rsh,int N);
float32_t   vec_powerf     ( const float32_t *  x,int N);

int64_t     vec_power32x32_fast ( const int32_t *  x,int rsh,int N);
int64_t     vec_power16x16_fast ( const int16_t *  x,int rsh,int N);

/*-------------------------------------------------------------------------
  Vector Scaling with Saturation
  These routines make shift with saturation of data values in the vector 
  by given scale factor (degree of 2).
  Functions vec_scale() make multiplication of a vector to a coefficient 
  which is not a power of 2 forming Q31, Q15 or floating-point result.
  Two versions of routines are available: regular versions (vec_shift32x32, 
  vec_shift16x16, vec_shiftf, vec_scale32x32, vec_scale16x16, vec_scalef, 
  vec_scale_sf) work with arbitrary arguments, faster versions 
  (vec_shift32x32_fast, vec_shift16x16_fast, vec_scale32x32_fast, 
  vec_scale16x16_fast) apply some restrictions

  For floating point:
  Fuction vec_shiftf() makes scaling without saturation of data values by given 
  scale factor (degree of 2). 
  Functions vec_scalef() and vec_scale_sf() make multiplication of input vector
  to coefficient which is not a power of 2.
  Two versions of routines are available: 
    without saturation - vec_scalef;
    with saturation - vec_scale_sf; 

  Precision:
  32x32 32-bit input, 32-bit output
  16x16 16-bit input, 16-bit output
  f     single precision floating point

  Input:
  x[N]    input data, Q31, Q15 or floating point
  t       shift count. If positive, it shifts left with saturation, if
          negative it shifts right
  s       scale factor, Q31, Q15 or floating point
  N       length of vector
  fmin    minimum output value (only for vec_scale_sf)
  fmax    maximum output value (only for vec_scale_sf)
  Output:
  y[N]    output data, Q31, Q15 or floating point

  Restrictions:
  For regular versions (vec_shift32x32, vec_shift16x16, vec_shiftf, 
  vec_scale32x32, vec_scale16x16, vec_scalef, vec_scale_sf):
  x,y should not overlap
  t   should be in range -31...31 for fixed-point functions and -129...146 
      for floating point
  For vec_scale_sf:
  fmin<=fmax;

  For faster versions (vec_shift32x32_fast, vec_shift16x16_fast, 
  vec_scale32x32_fast,vec_scale16x16_fast):
  x,y should not overlap
  t should be in range -31...31 
  x,y - aligned on 8-byte boundary
  N   - multiple of 4 
-------------------------------------------------------------------------*/
void vec_shift32x32 (     int32_t *  y,
                    const int32_t *  x,
                    int t,
                    int N);
void vec_shift16x16 (     int16_t *  y,
                    const int16_t *  x,
                    int t,
                    int N);
void vec_shiftf     (     float32_t *  y,
                    const float32_t *  x,
                    int t,
                    int N);
void vec_scale32x32 (     int32_t *  y,
                    const int32_t *  x,
                    int32_t s,
                    int N);
void vec_scale16x16 (     int16_t *  y,
                    const int16_t *  x,
                    int16_t s,
                    int N);
void vec_scalef     (     float32_t *  y,
                    const float32_t *  x,
                    float32_t s,
                    int N);
void vec_scale_sf   (     float32_t *  y,
                    const float32_t *  x,
                    float32_t s, float32_t fmin, float32_t fmax,
                    int N);
void vec_shift32x32_fast
                  ( int32_t *  y,
                    const int32_t *  x,
                    int t,
                    int N );
void vec_shift16x16_fast
                  ( int16_t *  y,
                    const int16_t *  x,
                    int t,
                    int N );
void vec_scale32x32_fast
                  ( int32_t *  y,
                    const int32_t *  x,
                    int32_t s,
                    int N );
void vec_scale16x16_fast
                  ( int16_t *  y,
                    const int16_t *  x,
                    int16_t s,
                    int N );

/*-------------------------------------------------------------------------
  Common Exponent 
  These functions determine the number of redundant sign bits for each value 
  (as if it was loaded in a 32-bit register) and returns the minimum number 
  over the whole vector. This may be useful for a FFT implementation to 
  normalize data.  
  NOTES:
  Faster version of functions make the same task but in a different manner - 
  they compute exponent of maximum absolute value in the array. It allows 
  faster computations but not bitexact results - if minimum value in the 
  array will be -2^n , fast function returns max(0,30-n) while non-fast 
  function returns (31-n).
  Floating point function returns 0-floor(log2(max(abs(x)))). Returned 
  result will be always in range [-129...146]. 
  Special cases
  x       | result
  --------+-------
  0       |    0
  +/-Inf  | -129
  NaN     |    0

  If dimension N<=0 functions return 0

  Precision: 
  32 32-bit inputs 
  16 16-bit inputs 
  f  single precision floating point

  Input:
  x[N]    input data
  N       length of vector
  Returned value:
  minimum exponent

  Restriction:
  Regular versions (vec_bexp16, vec_bexp32, vec_bexpf):
  none
  Faster versions (vec_bexp16_fast, vec_bexp32_fast):
  x   - aligned on 8-byte boundary
  N   - a multiple of 4
-------------------------------------------------------------------------*/
int vec_bexp32 (const int32_t   * x, int N);
int vec_bexp16 (const int16_t   * x, int N);
int vec_bexpf  (const float32_t * x, int N);
int scl_bexp32 (int32_t x);
int scl_bexp16 (int16_t x);
int scl_bexpf  (float32_t x);
int vec_bexp32_fast (const int32_t * x, int N);
int vec_bexp16_fast (const int16_t * x, int N);

/*-------------------------------------------------------------------------
  Vector Min/Max
  These routines find maximum/minimum value in a vector.
  Two versions of functions available: regular version (vec_min32x32, 
  vec_max32x32, vec_min16x16, vec_max16x16, vec_minf, vec_maxf) with 
  arbitrary arguments and faster version (vec_min32x32_fast, vec_max32x32_fast, 
  vec_min16x16_fast, vec_max16x16_fast) that apply some restrictions
  NOTE: functions return zero if N is less or equal to zero

  Precision: 
  32x32 32-bit data, 32-bit output
  16x16 16-bit data, 16-bit output
  f     single precision floating point
  
  Input:
  x[N]  input data
  N     length of vector
  Returned value:
  Minimum or maximum value correspondingly

  Restriction:
  For regular routines:
  none
  For faster routines:
  x aligned on 8-byte boundary
  N   - multiple of 4
-------------------------------------------------------------------------*/
int32_t   vec_min32x32 (const int32_t  *  x, int N);
int16_t   vec_min16x16 (const int16_t  *  x, int N);
float32_t vec_minf     (const float32_t*  x, int N);
int32_t vec_max32x32   (const int32_t  *  x, int N);
int16_t vec_max16x16   (const int16_t  *  x, int N);
float32_t vec_maxf     (const float32_t*  x, int N);
int32_t vec_min32x32_fast (const int32_t* x, int N);
int16_t vec_min16x16_fast (const int16_t* x, int N);
int32_t vec_max32x32_fast (const int32_t* x, int N);
int16_t vec_max16x16_fast (const int16_t* x, int N);


/*-------------------------------------------------------------------------
  Vector Addition for Emulated Floating Point
  routines add two vectors represented in emulated floating point format

  Input:
  xmant[N]  mantissa of input data
  ymant[N]  mantissa of input data
  xexp[N]   exponent of input data
  yexp[N]   exponent of input data
  N         length of vectors
  Output:
  zmant[N]  mantissa of output data
  zexp[N]   exponent of output data

  Restriction:
  xmant,ymant,xexp,yexp,zmant,zexp should not overlap
-------------------------------------------------------------------------*/
void   vec_add_32x16ef (      int32_t  *  zmant,       int16_t  *  zexp, 
                        const int32_t  *  xmant, const int16_t  *  xexp, 
                        const int32_t  *  ymant, const int16_t  *  yexp, 
                        int N);
void   scl_add_32x16ef (      int32_t  * zmant, int16_t *  zexp, 
                              int32_t    xmant, int16_t    xexp, 
                              int32_t    ymant, int16_t    yexp);

/*-------------------------------------------------------------------------
  Vector Multiply for Emulated Floating Point
  routines multiply two vectors represented in emulated floating point format

  Input:
  xmant[N]  mantissa of input data
  ymant[N]  mantissa of input data
  xexp[N]   exponent of input data
  yexp[N]   exponent of input data
  N         length of vectors
  Output:
  zmant[N]  mantissa of output data
  zexp[N]   exponent of output data

  Restriction:
  xmant,ymant,xexp,yexp,zmant,zexp should not overlap
-------------------------------------------------------------------------*/
void   vec_mul_32x16ef (      int32_t  *  zmant,       int16_t  *  zexp, 
                        const int32_t  *  xmant, const int16_t  *  xexp, 
                        const int32_t  *  ymant, const int16_t  *  yexp, 
                        int N);
void   scl_mul_32x16ef (      int32_t  * zmant, int16_t *  zexp, 
                              int32_t    xmant, int16_t    xexp, 
                              int32_t    ymant, int16_t    yexp);

/*-------------------------------------------------------------------------
  Vector Multiply-Accumulate for Emulated Floating Point
  routines multiply-accumulate vectors by scalar represented in emulated 
  floating point format

  Input:
  xmant[N]  mantissa of input data
  xexp[N]   exponent of input data
  ymant     mantissa of scalar
  yexp      exponent of scalar
  N         length of vectors
  Output:
  zmant[N]  mantissa of output data
  zexp[N]   exponent of output data

  Restriction:
  xmant,xexp,zmant,zexp should not overlap
-------------------------------------------------------------------------*/
void   vec_mac_32x16ef (      int32_t  *  zmant,       int16_t  *  zexp, 
                        const int32_t  *  xmant, const int16_t  *  xexp, 
                        const int32_t     ymant, const int16_t     yexp, 
                        int N);
void   scl_mac_32x16ef (      int32_t  * zmant, int16_t *  zexp, 
                              int32_t    xmant, int16_t    xexp, 
                              int32_t    ymant, int16_t    yexp);

/*-------------------------------------------------------------------------
  Vector Dot Product for Emulated Floating Point
  routines compute dot product of vectors represented in emulated floating 
  point format

  Input:
  xmant[N]  mantissa of input data
  ymant[N]  mantissa of input data
  xexp[N]   exponent of input data
  yexp[N]   exponent of input data
  N         length of vectors
  Output:
  zmant[1]  mantissa of output data
  zexp[1]   exponent of output data

  Restriction:
  xmant,ymant,xexp,yexp,zmant,zexp should not overlap
-------------------------------------------------------------------------*/
void   vec_dot_32x16ef (      int32_t  *  zmant,       int16_t  *  zexp, 
                        const int32_t  *  xmant, const int16_t  *  xexp, 
                        const int32_t  *  ymant, const int16_t  *  yexp, 
                        int N);

/****  Matlab Code Gen  ********/

void vec_eleabsf (const float32_t*  x, float32_t*  z,  int N);
void vec_eleabs32x32 (const int32_t*  x, int32_t*  z,  int N);
void vec_eleabs16x16 (const int16_t*  x, int16_t*  z,  int N);

void vec_elemaxf (float32_t*  z, float32_t*  x, float32_t*  y, int N);
void vec_elemax32x32 (int32_t*  z, int32_t*  x, int32_t*  y, int N);
void vec_elemax16x16 (int16_t*  z, int16_t*  x, int16_t*  y, int N);

void vec_eleminf (float32_t*  z, float32_t*  x, float32_t*  y, int N);
void vec_elemin32x32 (int32_t*  z, int32_t*  x, int32_t*  y, int N);
void vec_elemin16x16 (int16_t*  z, int16_t*  x, int16_t*  y, int N);

void vec_elesubf (float32_t*  z, float32_t*  x, float32_t*  y, int N);
void vec_elesub32x32 (int32_t*  z, int32_t*  x, int32_t*  y, int N);
void vec_elesub16x16 (int16_t*  z, int16_t*  x, int16_t*  y, int N);

void vec_elemultf (float32_t*  z, float32_t*  x, float32_t*  y, int N);
void vec_elemult32x32 (int32_t*  z, int32_t*  x, int32_t*  y, int N);
void vec_elemult16x16 (int16_t*  z, int16_t*  x, int16_t*  y, int N);

float32_t vec_sumf (const float32_t *  x, int N);
int32_t vec_sum32x32 (const int32_t*  x, int N);
int16_t vec_sum16x16 (const int16_t*  x, int N);

float32_t 	vec_meanf (const float32_t *  x, int N);
int32_t 	vec_mean32x32 (const int32_t*  x, int N);
int16_t 	vec_mean16x16 (const int16_t*  x, int N);

float32_t 	vec_rmsf (const float32_t *  x, int N);
int32_t 	vec_rms32x32(const int32_t*  x, int N);
int16_t 	vec_rms16x16(const int16_t*  x, int N);

float32_t 	vec_varf (const float32_t *  x, int N);
int32_t 	vec_var32x32 (const int32_t*  x, int N);
int16_t 	vec_var16x16 (const int16_t*  x, int N);

float32_t 	vec_stddevf (const float32_t *  x, int N);
int32_t 	vec_stddev32x32 (const int32_t*  x, int N);
int16_t 	vec_stddev16x16 (const int16_t*  x, int N);

void vec_cplx2cplx_multf (complex_float*  z, complex_float*  x, complex_float*  y, int N);
void vec_cplx2cplx_mult32x32 (complex32_t*  z, complex32_t*  x, complex32_t*  y, int N);
void vec_cplx2cplx_mult16x16 (complex16_t*  z, complex16_t*  x, complex16_t*  y, int N);

void vec_cplx2real_multvf (complex_float*  z, complex_float*  x, float32_t*  y, int N);
void vec_cplx2real_multv32x32 (complex32_t*  z, complex32_t*  x, int32_t*  y, int N);
void vec_cplx2real_multv16x16 (complex16_t*  z, complex16_t*  x, int16_t*  y, int N);

void vec_cplx2real_multsf (complex_float* restrict z, complex_float* restrict x, float32_t y, int N);
void vec_cplx2real_mults32x32 (complex32_t*  z, complex32_t*  x, int32_t  y, int N);
void vec_cplx2real_mults16x16 (complex16_t*  z, complex16_t*  x, int16_t  y, int N);

void vec_cplx_Conjf(complex_float * r, const complex_float * x, int N);
void vec_cplxconj32x32(complex32_t*  z, const complex32_t*  x, int N);
void vec_cplxconj16x16(complex16_t*  z, const complex16_t*  x, int N);

void vec_complex2mag32x32 (int32_t* z, const complex32_t* x, int N);
void vec_complex2mag16x16 (int16_t* z, const complex16_t* x, int N);

#ifdef __cplusplus
}
#endif

#endif/* __NATUREDSP_SIGNAL_VECTOR_H__ */
