// System specific configuration options for Foundation
#ifndef FOUNDATION_CONFIG_INCLUDED_HEADER
#define FOUNDATION_CONFIG_INCLUDED_HEADER

/* Version number of package */
#define FOUNDATION_VERSION "0.0.1"

// Micosoft compiler settings
#ifdef _MSC_VER
#pragma warning (disable : 4996) // unsafe copy warning

#if _MSC_VER > 1300
#define USE_SSE_INSTRUCTIONS 1
#define HAS_TEMPLATE_MEMBER_FUNCTION_INSTANTIATION 1
#define HAS_PARTIAL_TEMPLATE_SPECIALIZATION 1
#define HAS_CONDITIONAL_TEMPLATE_MEMBER_FUNCTION_INSTANTIATION 1
#endif

#endif // _MSC_VER

// gcc compiler settings
#ifdef __GNUC__
#if __GNUC__ > 3
#define HAS_TEMPLATE_MEMBER_FUNCTION_INSTANTIATION 1
#define HAS_PARTIAL_TEMPLATE_SPECIALIZATION 1
//#define HAS_CONDITIONAL_TEMPLATE_MEMBER_FUNCTION_INSTANTIATION 1
#endif

#endif

#define UNUSED( x ) ( &reinterpret_cast< const int& >( x ) )

#endif
