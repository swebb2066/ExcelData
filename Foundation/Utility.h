// Utility functions

#ifndef FOUNDATION_UTILITY_HEADER_
#define FOUNDATION_UTILITY_HEADER_

#include <Foundation/Config.h>

#ifdef _MSC_VER
#pragma warning (disable : 4305) // truncation from 'double' to 'float'
#endif

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979
#endif

#define M_RADIANS_PER_DEGREE (M_PI / 180)
#define M_DEGREES_PER_RADIAN (180 / M_PI)
const int MillidegreesPerRadian = int(180000 / M_PI);
const int MM_PER_METRE = 1000;

/// The radians value equivalent to \c theta degrees
template <typename T>
    T
degs2rads(T theta)
{
    return T((theta) * M_RADIANS_PER_DEGREE);
}

/// The degrees value equivalent to \c theta radians
template <typename T>
    T
rads2degs(T theta)
{
    return T((theta) * M_DEGREES_PER_RADIAN);
}

/// The radians value equivalent to \c theta degrees
template <typename T>
    T
D2R(T theta)
{
    return T((theta) * M_RADIANS_PER_DEGREE);
}

/// The degrees value equivalent to \c theta radians
template <typename T>
    T
R2D(T theta)
{
    return T((theta) * M_DEGREES_PER_RADIAN);
}

/// The square of \c value
template <typename T>
    T
Squared(T value) { return value * value; }

/// Clamp \c value_in to be between \c value_min and \c value_max
inline double Clamp(double value_in, double value_min, double value_max)
{
    if (value_in < value_min)
        return value_min;
    else if (value_in > value_max)
        return value_max;
    else
        return value_in;
}

/// The integer nearest to \c d
inline int Round(double d) { return (int)floor(d + 0.5); }

/// The nearest smaller natural number
inline double Floor(double value)
{
    double result;
    double n, f;
    f = modf(value, &n);
    result = ((f < 0) ? n - 1 : n);
    return result;
}

/// The nearest larger natural number
inline double Ceil(double value)
{
    double result;
    double n, f;
    f = modf(value, &n);
    result = ((0 < f) ? n + 1 : n);
    return result;
}

#endif // FOUNDATION_UTILITY_HEADER_

