#ifndef FUNCTIONSIMD_H
#define FUNCTIONSIMD_H

#include <cmath>
#include <limits>
#include <immintrin.h>
#include "BasicDataStructure.h"

// 点乘
static inline __m256 simd_dot_ps(const SimdVector3D& vec1, const SimdVector3D& vec2)
{
    __m256 x_mul = _mm256_mul_ps(vec1.x, vec2.x);
    __m256 y_mul = _mm256_mul_ps(vec1.y, vec2.y);
    __m256 z_mul = _mm256_mul_ps(vec1.z, vec2.z);
    return _mm256_add_ps(_mm256_add_ps(x_mul, y_mul), z_mul);
}

// 平方
static inline __m256 simd_length_squared_ps(const SimdVector3D& vec)
{
    return simd_dot_ps(vec, vec);
}

// 开方
static inline __m256 simd_length_ps(const SimdVector3D& vec)
{
    return _mm256_sqrt_ps(simd_length_squared_ps(vec));
}

// 标准化
static inline SimdVector3D simd_normalize_ps(const SimdVector3D& vec)
{
    __m256 length = simd_length_ps(vec);
    __m256 epsilon = _mm256_set1_ps(1e-6f);
    length = _mm256_max_ps(length, epsilon); // Ensure length is not zero or too small
    __m256 oneOverLength = _mm256_div_ps(_mm256_set1_ps(1.0f), length); // 1 / length
    SimdVector3D normalizedVec = {_mm256_mul_ps(vec.x, oneOverLength), _mm256_mul_ps(vec.y, oneOverLength), _mm256_mul_ps(vec.z, oneOverLength)};
    return normalizedVec;
}

// Placeholder SIMD log_ps (Natural Logarithm)
static inline __m256 simd_log_ps(__m256 val)
{
    __m256 xMinus1 = _mm256_sub_ps(val, _mm256_set1_ps(1.0f));
    __m256 xMinusSq = _mm256_mul_ps(xMinus1, xMinus1);
    __m256 xMinusCub = _mm256_mul_ps(xMinusSq, xMinus1);
    __m256 result = _mm256_add_ps(_mm256_mul_ps(_mm256_set1_ps(1.0f), xMinus1), // x-1
                                  _mm256_mul_ps(_mm256_set1_ps(-0.5f), xMinusSq)); // -0.5 * (x-1)^2
    result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_set1_ps(1.0f/3.0f), xMinusCub)); // + 1/3 * (x-1)^3
    // Basic handling for input <= 0 (log is undefined)
    __m256 leZeroMask = _mm256_cmp_ps(val, _mm256_setzero_ps(), _CMP_LE_OS);
    result = _mm256_blendv_ps(result, _mm256_set1_ps(std::numeric_limits<float>::quiet_NaN()), leZeroMask);
    return result;
}

// Placeholder SIMD exp_ps (e to the power of x)
static inline __m256 simd_exp_ps(__m256 val)
{
    __m256 onePs = _mm256_set1_ps(1.0f);
    __m256 x_sq = _mm256_mul_ps(val, val);
    __m256 x_cub = _mm256_mul_ps(x_sq, val);
    __m256 x_pow_4 = _mm256_mul_ps(x_cub, val);
    __m256 result = _mm256_add_ps(onePs, val); // 1 + x
    result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_set1_ps(0.5f), x_sq)); // + x^2 / 2!
    result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_set1_ps(1.0f/6.0f), x_cub)); // + x^3 / 3!
    result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_set1_ps(1.0f/24.0f), x_pow_4)); // + x^4 / 4!
    // Proper implementation requires range reduction and handling large/small results.
    return result;
}

inline __m256 simd_pow_ps(__m256 base, __m256 exponent)
{
    __m256 log_base = simd_log_ps(base);
    __m256 y_times_log_x = _mm256_mul_ps(exponent, log_base);
    __m256 result = simd_exp_ps(y_times_log_x);
    return result;
}

// SIMD Max function for two __m256
inline __m256 simd_max_ps(__m256 a, __m256 b)
{
    return _mm256_max_ps(a, b);
}

// SIMD Clamp function for __m256 (clamping between min and max)
inline __m256 simd_clamp_ps(__m256 value, __m256 min_val, __m256 max_val)
{
    return _mm256_min_ps(_mm256_max_ps(value, min_val), max_val);
}

// SIMD Vector Subtraction for SimdVector3D
inline SimdVector3D simd_sub_ps(const SimdVector3D& a, const SimdVector3D& b)
{
    SimdVector3D result;
    result.x = _mm256_sub_ps(a.x, b.x);
    result.y = _mm256_sub_ps(a.y, b.y);
    result.z = _mm256_sub_ps(a.z, b.z);
    return result;
}


inline SimdColor simd_mul_ps(const SimdColor& a, const SimdColor& b)
{
    SimdColor result;
    result.r = _mm256_mul_ps(a.r, b.r);
    result.g = _mm256_mul_ps(a.g, b.g);
    result.b = _mm256_mul_ps(a.b, b.b);
    return result;
}

// SIMD Vector Multiplication (element-wise) for SimdVector3D
inline SimdVector3D simd_mul_ps(const SimdVector3D& a, const SimdVector3D& b)
{
    SimdVector3D result;
    result.x = _mm256_mul_ps(a.x, b.x);
    result.y = _mm256_mul_ps(a.y, b.y);
    result.z = _mm256_mul_ps(a.z, b.z);
    return result;
}


// SIMD Vector Addition (element-wise) for SimdVector3D
inline SimdVector3D simd_add_ps(const SimdVector3D& a, const SimdVector3D& b)
{
    SimdVector3D result;
    result.x = _mm256_add_ps(a.x, b.x);
    result.y = _mm256_add_ps(a.y, b.y);
    result.z = _mm256_add_ps(a.z, b.z);
    return result;
}

inline SimdColor simd_add_ps(const SimdColor& a, const SimdColor& b)
{
    SimdColor result;
    result.r = _mm256_add_ps(a.r, b.r);
    result.g = _mm256_add_ps(a.g, b.g);
    result.b = _mm256_add_ps(a.b, b.b);
    return result;
}

// SIMD Scalar Multiplication for SimdVector3D
inline SimdColor simd_scalar_mul_ps(const SimdColor& v, __m256 scalar)
{
    SimdColor result;
    result.r = _mm256_mul_ps(v.r, scalar);
    result.g = _mm256_mul_ps(v.g, scalar);
    result.b = _mm256_mul_ps(v.b, scalar);
    return result;
}

inline SimdVector3D simd_scalar_mul_ps(const SimdVector3D& v, __m256 scalar)
{
    SimdVector3D result;
    result.x = _mm256_mul_ps(v.x, scalar);
    result.y = _mm256_mul_ps(v.y, scalar);
    result.z = _mm256_mul_ps(v.z, scalar);
    return result;
}




#endif // FUNCTIONSIMD_H
