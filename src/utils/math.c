/**
 * @file math.c
 * @brief Generic Math Utilities.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/utils/math.h"
#include "math_private.h"
#include "measlib/boards/math_ops.h"
#include <math.h> // Generic C math for sqrt/fabs
#include <stdint.h>

// ===========================================
// Internal: Reference Float Optimizations
// ===========================================

// Fast Sqrt (Float)
// static float fast_sqrtf(float x) {
//   union {
//     float x;
//     uint32_t i;
//   } u = {x};
//   u.i = (1 << 29) + (u.i >> 1) - (1 << 22);
//   u.x = u.x + x / u.x;
//   u.x = 0.25f * u.x + x / u.x;
//   return u.x;
// }

// Fast Cbrt (Float)
static float fast_cbrtf(float x) {
  if (x == 0)
    return 0;
  float b = 1.0f;
  float last_b_1 = 0;
  float last_b_2 = 0;
  while (last_b_1 != b && last_b_2 != b) {
    last_b_1 = b;
    b = (2 * b + x / b / b) / 3;
    last_b_2 = b;
    b = (2 * b + x / b / b) / 3;
  }
  return b;
}

// Fast Log (Float)
static float fast_logf(float x) {
  const float MULTIPLIER = 0.69314718056f; // log(2)
  union {
    float f;
    uint32_t i;
  } vx = {x};
  union {
    uint32_t i;
    float f;
  } mx = {(vx.i & 0x007FFFFF) | 0x3f000000};
  if (vx.i <= 0)
    return -1.0f / (x * x); // Error handling (naive)
  return vx.i * (MULTIPLIER / (1 << 23)) - (124.22544637f * MULTIPLIER) -
         (1.498030302f * MULTIPLIER) * mx.f -
         (1.72587999f * MULTIPLIER) / (0.3520887068f + mx.f);
}

// Fast Atan2 (Float)
// NOTE: Maps to full circle.
static float fast_atan2f(float y, float x) {
  union {
    float f;
    int32_t i;
  } ux = {x};
  union {
    float f;
    int32_t i;
  } uy = {y};
  if (ux.i == 0 && uy.i == 0)
    return 0.0f;

  float ax, ay, r, s;
  ax = MEAS_MATH_ABS_IMPL(x);
  ay = MEAS_MATH_ABS_IMPL(y);
  r = (ay < ax) ? ay / ax : ax / ay;
  s = r * r;

  // Polynomial approximation (~0.005 deg error)
  r *= 0.999133448222780f -
       s * (0.320533292381664f -
            s * (0.144982490144465f - s * 0.038254464970299f));

  if (ay > ax)
    r = (MEAS_PI / 2) - r; // PI/2 - r
  if (ux.i < 0)
    r = MEAS_PI - r; // PI - r
  if (uy.i < 0)
    r = -r;
  return r;
}

static float fast_modff(float x, float *iptr) {
  union {
    float f;
    uint32_t i;
  } u = {x};
  int e = (int)((u.i >> 23) & 0xff) - 0x7f;
  if (e < 0) {
    if (iptr)
      *iptr = 0;
    return u.f;
  }
  if (e >= 23) {
    if (iptr)
      *iptr = x;
    return 0.0f;
  }
  x = u.f;
  u.i &= ~(0x007fffff >> e);
  x -= u.f;
  if (iptr)
    *iptr = u.f;
  return x;
}

static void fast_sincosf(float angle, float *pSinVal, float *pCosVal) {
#if !defined(F303) && !defined(AT32) && !defined(F072)
  *pSinVal = sinf(angle);
  *pCosVal = cosf(angle);
#else
  float fpart, ipart;
  float x = angle * (1.0f / (2.0f * MEAS_PI)); // angle / (2*PI)

  fpart = fast_modff(x, &ipart);
  if (fpart < 0.0f)
    fpart += 1.0f;

#if defined(F303) || defined(AT32)
  const float table_size_full_circle = 2048.0f;
  const uint16_t table_quarter = 512;
#else // F072
  const float table_size_full_circle = 1024.0f;
  const uint16_t table_quarter = 256;
#endif

  float scaled = fpart * table_size_full_circle;
  uint16_t full_index = (uint16_t)scaled;
  float fract = scaled - full_index;

  // Taylor Series Method (2nd Order):
  // sin(a + h) = sin(a) + h*cos(a) - 0.5*h^2*sin(a)
  // cos(a + h) = cos(a) - h*sin(a) - 0.5*h^2*cos(a)
  //
  // h = fract * step_size_rad
  // step_size_rad = (2*PI) / table_size_full_circle = (PI/2) / table_quarter

  float step_rad = (float)(MEAS_PI * 0.5) / (float)table_quarter;
  float h = fract * step_rad;
  float h2_05 = 0.5f * h * h;

  // Determine Quadrant and Base Index
  uint8_t quad = full_index / table_quarter;
  uint16_t idx = full_index % table_quarter;

  float s0, c0;

  // Fetch Base sin(a) and cos(a) from first quadrant LUT
  // LUT contains 0..PI/2.
  // sin(idx) = table[idx]
  // cos(idx) = sin(90 - idx) = table[quarter - idx]

  // Safety for table access
  float val_s = sin_table_qtr[idx];
  float val_c = sin_table_qtr[table_quarter - idx];

  // Apply Quadrant Rotation to base s0, c0
  switch (quad) {
  case 0: // 0..90
    s0 = val_s;
    c0 = val_c;
    break;
  case 1: // 90..180
    s0 = val_c;
    c0 = -val_s;
    break;
  case 2: // 180..270
    s0 = -val_s;
    c0 = -val_c;
    break;
  case 3: // 270..360
    s0 = -val_c;
    c0 = val_s;
    break;
  default:
    s0 = 0.0f;
    c0 = 1.0f;
  }

  // Taylor Expansion
  if (pSinVal)
    *pSinVal = s0 + h * c0 - h2_05 * s0;
  if (pCosVal)
    *pCosVal = c0 - h * s0 - h2_05 * c0;
#endif
}

// ===========================================
// Public Wrapper Implementations
// ===========================================

meas_real_t meas_math_interp_linear(meas_real_t x, meas_real_t x0,

                                    meas_real_t y0, meas_real_t x1,
                                    meas_real_t y1) {
  if ((x1 - x0) == 0.0)
    return y0;
  return y0 + (x - x0) * (y1 - y0) / (x1 - x0);
}

bool meas_math_is_close(meas_real_t a, meas_real_t b, meas_real_t epsilon) {
  return MEAS_MATH_ABS_IMPL(a - b) <= epsilon;
}

meas_real_t meas_math_sqrt(meas_real_t x) {
  if (sizeof(meas_real_t) == 4) {
    return (meas_real_t)MEAS_MATH_SQRT_IMPL((float)x);
  }
  return sqrt(x);
}

meas_real_t meas_math_cbrt(meas_real_t x) {
  if (sizeof(meas_real_t) == 4) {
    return (meas_real_t)fast_cbrtf((float)x);
  }
#if defined(_MSC_VER)
  return pow(x, 1.0 / 3.0);
#else
  return cbrt(x);
#endif
}

meas_real_t meas_math_log(meas_real_t x) {
  if (sizeof(meas_real_t) == 4)
    return (meas_real_t)fast_logf((float)x);
  return log(x);
}

meas_real_t meas_math_log10(meas_real_t x) {
  if (sizeof(meas_real_t) == 4) {
    // Reference vna_log10f_x_10 does 10*log10(x).
    // Here we implement standard log10(x).
    return (meas_real_t)(fast_logf((float)x) * 0.4342944819f);
  }
  return log10(x);
}

meas_real_t meas_math_exp(meas_real_t x) {
  if (sizeof(meas_real_t) == 4) {
    union {
      float f;
      int32_t i;
    } v;
    v.i = (int32_t)(12102203.0f * (float)x) + 0x3F800000;
    int32_t m = (v.i >> 7) & 0xFFFF; // copy mantissa
    // cubic spline approximation
    v.i += ((((((((1277 * m) >> 14) + 14825) * m) >> 14) - 79749) * m) >> 11) -
           626;
    return (meas_real_t)v.f;
  }
  return exp(x);
}

meas_real_t meas_math_atan(meas_real_t x) {
  if (sizeof(meas_real_t) == 4)
    return (meas_real_t)fast_atan2f((float)x, 1.0f);
  return atan(x);
}

meas_real_t meas_math_atan2(meas_real_t y, meas_real_t x) {
  if (sizeof(meas_real_t) == 4)
    return (meas_real_t)fast_atan2f((float)y, (float)x);
  return atan2(y, x);
}

meas_real_t meas_math_modf(meas_real_t x, meas_real_t *iptr) {
  if (sizeof(meas_real_t) == 4) {
    float i_val;
    float res = fast_modff((float)x, &i_val);
    if (iptr)
      *iptr = (meas_real_t)i_val;
    return (meas_real_t)res;
  }
  double i_val;
  double res = modf(x, &i_val); // standard double modf
  if (iptr)
    *iptr = (meas_real_t)i_val;
  return (meas_real_t)res;
}

void meas_math_stats(const meas_real_t *data, size_t count, meas_real_t *mean,
                     meas_real_t *std_dev, meas_real_t *min_val,
                     meas_real_t *max_val) {
  if (!data || count == 0)
    return;

  meas_real_t m = 0.0;
  meas_real_t S = 0.0;
  meas_real_t min_v = data[0];
  meas_real_t max_v = data[0];

  for (size_t i = 0; i < count; i++) {
    meas_real_t val = data[i];

    // Welford's algorithm
    meas_real_t old_m = m;
    m += (val - m) / (i + 1);
    S += (val - m) * (val - old_m);

    if (val < min_v)
      min_v = val;
    if (val > max_v)
      max_v = val;
  }

  if (mean)
    *mean = m;
  if (min_val)
    *min_val = min_v;
  if (max_val)
    *max_val = max_v;

  if (std_dev) {
    meas_real_t variance = S / count;
    *std_dev = meas_math_sqrt(variance > 0 ? variance : 0);
  }
}

// ===========================================
// Statistics & Signal Processing
// ===========================================

meas_real_t meas_math_rms(const meas_real_t *data, size_t count) {
  if (!data || count == 0)
    return 0.0;

  meas_real_t sum_sq = 0.0;
  for (size_t i = 0; i < count; i++) {
    sum_sq += data[i] * data[i];
  }
  return MEAS_MATH_SQRT_IMPL(sum_sq / count);
}

void meas_math_sma(const meas_real_t *input, size_t count, size_t window_size,
                   meas_real_t *output, size_t *out_count) {
  if (!input || !output || window_size == 0 || window_size > count) {
    if (out_count)
      *out_count = 0;
    return;
  }

  size_t valid_points = count - window_size + 1;
  meas_real_t current_sum = 0.0;

  // Initial window
  for (size_t i = 0; i < window_size; i++) {
    current_sum += input[i];
  }
  output[0] = current_sum / window_size;

  // Slide window
  for (size_t i = 1; i < valid_points; i++) {
    current_sum -= input[i - 1];
    current_sum += input[i + window_size - 1];
    output[i] = current_sum / window_size;
  }

  if (out_count)
    *out_count = valid_points;
}

meas_real_t meas_math_ema(meas_real_t current_avg, meas_real_t new_sample,
                          meas_real_t alpha) {
  return (alpha * new_sample) + ((1.0 - alpha) * current_avg);
}

meas_real_t meas_cabs(meas_complex_t z) {
  return meas_math_sqrt(z.re * z.re + z.im * z.im);
}

meas_real_t meas_carg(meas_complex_t z) { return meas_math_atan2(z.im, z.re); }

void meas_math_sincos(meas_real_t angle, meas_real_t *sin_val,
                      meas_real_t *cos_val) {
  if (sizeof(meas_real_t) == 4) {
    float s, c;
    fast_sincosf((float)angle, &s, &c);
    if (sin_val)
      *sin_val = (meas_real_t)s;
    if (cos_val)
      *cos_val = (meas_real_t)c;
    return;
  }
#if defined(_GNU_SOURCE)
  sincos(angle, sin_val, cos_val);
#else
  if (sin_val)
    *sin_val = sin(angle);
  if (cos_val)
    *cos_val = cos(angle);
#endif
}

meas_real_t meas_math_interp_parabolic(meas_real_t y1, meas_real_t y2,
                                       meas_real_t y3, meas_real_t x) {
  const meas_real_t a = 0.5 * (y1 + y3) - y2;
  const meas_real_t b = 0.5 * (y3 - y1);
  const meas_real_t c = y2;
  return a * x * x + b * x + c;
}

meas_real_t meas_math_interp_cosine(meas_real_t y1, meas_real_t y2,
                                    meas_real_t x) {
  meas_real_t cos_val;
  meas_math_sincos(x * MEAS_PI, NULL, &cos_val);
  const meas_real_t mu2 = (1.0 - cos_val) / 2.0;
  return (y1 * (1.0 - mu2) + y2 * mu2);
}

meas_real_t meas_math_extrap_linear(meas_real_t x, meas_real_t x0,
                                    meas_real_t y0, meas_real_t x1,
                                    meas_real_t y1) {
  if (meas_math_is_close(x1, x0, MEAS_EPSILON)) // DIV0 Guard
    return y0;
  return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
}
