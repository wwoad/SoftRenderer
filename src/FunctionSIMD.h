#ifndef FUNCTIONSIMD_H
#define FUNCTIONSIMD_H

#include <cmath>
#include <limits>
#include <immintrin.h>
#include "BasicDataStructure.h"


inline __m256 simd_dot_ps(const SimdVector3D& a, const SimdVector3D& b)
{
    __m256 x_mul = _mm256_mul_ps(a.x, b.x);
    __m256 y_mul = _mm256_mul_ps(a.y, b.y);
    __m256 z_mul = _mm256_mul_ps(a.z, b.z);
    return _mm256_add_ps(_mm256_add_ps(x_mul, y_mul), z_mul);
}

// SIMD Vector Length Squared for SimdVector3D
inline __m256 simd_length_squared_ps(const SimdVector3D& v)
{
    return simd_dot_ps(v, v);
}

// SIMD Vector Length for SimdVector3D
inline __m256 simd_length_ps(const SimdVector3D& v)
{
    return _mm256_sqrt_ps(simd_length_squared_ps(v));
}

// SIMD Vector Normalization for SimdVector3D
inline SimdVector3D simd_normalize_ps(const SimdVector3D& v)
{
    __m256 length = simd_length_ps(v);
    // Handle division by zero or very small lengths (optional but good practice)
    __m256 epsilon = _mm256_set1_ps(1e-6f);
    length = _mm256_max_ps(length, epsilon); // Ensure length is not zero or too small
    __m256 one_over_length = _mm256_div_ps(_mm256_set1_ps(1.0f), length); // 1 / length
    SimdVector3D normalized_v;
    normalized_v.x = _mm256_mul_ps(v.x, one_over_length);
    normalized_v.y = _mm256_mul_ps(v.y, one_over_length);
    normalized_v.z = _mm256_mul_ps(v.z, one_over_length);
    return normalized_v;
}

// Placeholder SIMD log_ps (Natural Logarithm)
inline __m256 simd_log_ps(__m256 x)
{
    // ** Placeholder Implementation: Replace with actual SIMD log **
    // Using a simple approximation for demonstration.
    // For proper implementation, search for "SIMD log polynomial approximation" or use VML.
    // Example using a Taylor series around 1 (very limited range of accuracy):
    __m256 x_minus_1 = _mm256_sub_ps(x, _mm256_set1_ps(1.0f));
    __m256 x_minus_1_sq = _mm256_mul_ps(x_minus_1, x_minus_1);
    __m256 x_minus_1_cub = _mm256_mul_ps(x_minus_1_sq, x_minus_1);
    __m256 result = _mm256_add_ps(_mm256_mul_ps(_mm256_set1_ps(1.0f), x_minus_1), // x-1
                                  _mm256_mul_ps(_mm256_set1_ps(-0.5f), x_minus_1_sq)); // -0.5 * (x-1)^2
    result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_set1_ps(1.0f/3.0f), x_minus_1_cub)); // + 1/3 * (x-1)^3
    // Basic handling for input <= 0 (log is undefined)
    __m256 le_zero_mask = _mm256_cmp_ps(x, _mm256_setzero_ps(), _CMP_LE_OS);
    result = _mm256_blendv_ps(result, _mm256_set1_ps(std::numeric_limits<float>::quiet_NaN()), le_zero_mask);
    return result;
}
// Placeholder SIMD exp_ps (e to the power of x)
inline __m256 simd_exp_ps(__m256 x)
{
    // ** Placeholder Implementation: Replace with actual SIMD exp **
    // Using a simple Taylor series around 0 for demonstration.
    // For proper implementation, search for "SIMD exp polynomial approximation" or use VML.
    // Example using Taylor series (very limited range of accuracy):
    __m256 one_ps = _mm256_set1_ps(1.0f);
    __m256 x_sq = _mm256_mul_ps(x, x);
    __m256 x_cub = _mm256_mul_ps(x_sq, x);
    __m256 x_pow_4 = _mm256_mul_ps(x_cub, x);
    __m256 result = _mm256_add_ps(one_ps, x); // 1 + x
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
    // Calculate exp(y * log(x)). exp of NaN is NaN.
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

// SIMD Scalar Multiplication for SimdVector3D
inline SimdVector3D simd_scalar_mul_ps(const SimdVector3D& v, __m256 scalar)
{
    SimdVector3D result;
    result.x = _mm256_mul_ps(v.x, scalar);
    result.y = _mm256_mul_ps(v.y, scalar);
    result.z = _mm256_mul_ps(v.z, scalar);
    return result;
}


#endif // FUNCTIONSIMD_H
