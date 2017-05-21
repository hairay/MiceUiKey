//*****************************************************************************
// Copyright (C) 2012 Cambridge Silicon Radio Ltd.
// $Header: /mfp/MiceUiKey/MiceUiKey/arch.h,v 1.1 2016/05/30 03:48:51 AV00287 Exp $
// $Change: 205871 $ $Date: 2016/05/30 03:48:51 $
//
/// @file
/// Machine Architecture for 4xxx chips.
/// Includes proper sub-file arch_*.h based on TARGET
///
/// @ingroup BIOS Framework
///
/// @author Lots of People
//
//*****************************************************************************
#ifndef ARCH_H
#define ARCH_H 1

// these statisy some firestarter imports
#define ARCH_H_INCLUDED 1
#define DSPTYPES_INCLUDED 1

#ifndef ALIGN_2_BYTES
/*
** Alignment qualifiers.  These are typically used when dealing with hardware
** units that require strict alignment.  For example, to declare a byte array
** that will be passed to a hardware unit that requires 32-bit alignment, use:
** Uint8 ALIGN_4_BYTES(byte_array[N_ELEM]);
*/
#if defined(__armcc) || defined(__CC_ARM)

#  define ALIGN_2_BYTES(x)   __align(2)   x
#  define ALIGN_4_BYTES(x)   __align(4)   x
#  define ALIGN_8_BYTES(x)   __align(8)   x
#  define ALIGN_16_BYTES(x)  __align(16)  x
#  define ALIGN_32_BYTES(x)  __align(32)  x
#  define ALIGN_64_BYTES(x)  __align(64)  x
#  define ALIGN_128_BYTES(x) __align(128) x
#  define ALIGN_256_BYTES(x) __align(256) x
#  define ALIGN_512_BYTES(x) __align(512) x

#ifndef INLINE
#  define INLINE inline
#endif

#elif defined(__GNUC__)

#  define ALIGN_2_BYTES(x)   x __attribute__ ((aligned(2)))
#  define ALIGN_4_BYTES(x)   x __attribute__ ((aligned(4)))
#  define ALIGN_8_BYTES(x)   x __attribute__ ((aligned(8)))
#  define ALIGN_16_BYTES(x)  x __attribute__ ((aligned(16)))
#  define ALIGN_32_BYTES(x)  x __attribute__ ((aligned(32)))
#  define ALIGN_64_BYTES(x)  x __attribute__ ((aligned(64)))
#  define ALIGN_128_BYTES(x) x __attribute__ ((aligned(128)))
#  define ALIGN_256_BYTES(x) x __attribute__ ((aligned(256)))
#  define ALIGN_512_BYTES(x) x __attribute__ ((aligned(512)))

#ifndef INLINE
#  define INLINE inline
#endif

#elif defined(_MSC_VER)

#  define ALIGN_2_BYTES(x)   x
#  define ALIGN_4_BYTES(x)   x
#  define ALIGN_8_BYTES(x)   x

#ifndef INLINE
#  define INLINE __inline
#endif

/*
** #elif defined(YOUR_COMPILER)
** Add your compiler's alignment qualifier mechanism here...
*/

#else

# ifndef HOST_BASED
/* Use an undefined pragma here to try to force a warning */
#  pragma WARNING missing alignment clause
# endif

#  define ALIGN_2_BYTES(x)   x
#  define ALIGN_4_BYTES(x)   x
#  define ALIGN_8_BYTES(x)   x
#  define ALIGN_16_BYTES(x)  x
#  define ALIGN_32_BYTES(x)  x
#  define ALIGN_64_BYTES(x)  x
#  define ALIGN_128_BYTES(x) x
#  define ALIGN_256_BYTES(x) x
#  define ALIGN_512_BYTES(x) x

#endif /* ALIGN_n_BYTES definitions */
#endif /* ALIGN_2_BYTES defined */

#endif

