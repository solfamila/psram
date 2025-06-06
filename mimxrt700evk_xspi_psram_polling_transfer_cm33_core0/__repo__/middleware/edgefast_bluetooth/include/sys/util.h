/*
 * Copyright 2021-2024 NXP
 * Copyright (c) 2011-2014, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Misc utilities
 *
 * Misc utilities usable by the kernel and application code.
 */

#ifndef ZEPHYR_INCLUDE_SYS_UTIL_H_
#define ZEPHYR_INCLUDE_SYS_UTIL_H_

#include <stdint.h>

#include <sys/util_macro.h>

#include <stddef.h>

#include <errno/errno.h>
#include <sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Cast @p x, a pointer, to an unsigned integer. */
#define POINTER_TO_UINT(x) ((uintptr_t) (x))
/** @brief Cast @p x, an unsigned integer, to a <tt>void*</tt>. */
#define UINT_TO_POINTER(x) ((void *) (uintptr_t) (x))
/** @brief Cast @p x, a pointer, to a signed integer. */
#define POINTER_TO_INT(x)  ((intptr_t) (x))
/** @brief Cast @p x, a signed integer, to a <tt>void*</tt>. */
#define INT_TO_POINTER(x)  ((void *) (intptr_t) (x))

#ifndef BIT
#if defined(_ASMLANGUAGE)
#define BIT(n)  (1 << (n))
#else
#define BIT(n)  (1UL << (n))
#endif
#endif

#ifndef BIT_MASK
#define BIT_MASK(n) (BIT(n) - 1)
#endif

#define MSEC_PER_SEC   1000

#ifndef BT_SECONDS
#define BT_SECONDS(x) ((x) * MSEC_PER_SEC)
#endif

#ifndef BT_MSEC
#define BT_MSEC(x) (x)
#endif

/**
 * @brief Divide and round up.
 *
 * Example:
 * @code{.c}
 * DIV_ROUND_UP(1, 2); // 1
 * DIV_ROUND_UP(3, 2); // 2
 * @endcode
 *
 * @param n Numerator.
 * @param d Denominator.
 *
 * @return The result of @p n / @p d, rounded up.
 */
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

/**
 * @brief Ceiling function applied to @p numerator / @p divider as a fraction.
 * @deprecated Use DIV_ROUND_UP() instead.
 */
#define ceiling_fraction(numerator, divider) DIV_ROUND_UP(numerator, divider)

#ifndef MAX
/**
 * @brief Obtain the maximum of two values.
 *
 * @note Arguments are evaluated twice. Use Z_MAX for a GCC-only, single
 * evaluation version
 *
 * @param a First value.
 * @param b Second value.
 *
 * @returns Maximum value of @p a and @p b.
 */
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
/**
 * @brief Obtain the minimum of two values.
 *
 * @note Arguments are evaluated twice. Use Z_MIN for a GCC-only, single
 * evaluation version
 *
 * @param a First value.
 * @param b Second value.
 *
 * @returns Minimum value of @p a and @p b.
 */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

/**
 * @brief Check if a pointer @p ptr lies within @p array.
 *
 * In C but not C++, this causes a compile error if @p array is not an array
 * (e.g. if @p ptr and @p array are mixed up).
 *
 * @param array an array
 * @param ptr a pointer
 * @return 1 if @p ptr is part of @p array, 0 otherwise
 */
#define PART_OF_ARRAY(array, ptr)                                                                  \
	((ptr) && POINTER_TO_UINT(array) <= POINTER_TO_UINT(ptr) &&                                \
	 POINTER_TO_UINT(ptr) < POINTER_TO_UINT(&(array)[ARRAY_SIZE(array)]))

#ifndef CONTAINER_OF
#define CONTAINER_OF(ptr, type, field) \
	((type *)((void *)(((char *)(ptr)) - offsetof(type, field))))
#endif

#ifndef IS_ARRAY_ELEMENT
#define IS_ARRAY_ELEMENT(array, ptr)                                                               \
	((ptr) && POINTER_TO_UINT(array) <= POINTER_TO_UINT(ptr) &&                          \
	 POINTER_TO_UINT(ptr) < POINTER_TO_UINT(&(array)[ARRAY_SIZE(array)]) &&                    \
	 (POINTER_TO_UINT(ptr) - POINTER_TO_UINT(array)) % sizeof((array)[0]) == 0)
#endif

/**
 * @brief Value of @p x rounded up to the next multiple of @p align.
 */
#ifndef ROUND_UP
#define ROUND_UP(x, align)                                   \
	((((unsigned long)(x) + ((unsigned long)(align) - 1)) / \
	  (unsigned long)(align)) * (unsigned long)(align))
#endif

/**
 * @brief Value of @p x rounded down to the previous multiple of @p align.
 */
#ifndef ROUND_DOWN
#define ROUND_DOWN(x, align)                                 \
	(((unsigned long)(x) / (unsigned long)(align)) * (unsigned long)(align))
#endif

/** @brief Value of @p x rounded up to the next word boundary. */
#define WB_UP(x) ROUND_UP(x, sizeof(void *))

/** @brief Value of @p x rounded down to the previous word boundary. */
#define WB_DN(x) ROUND_DOWN(x, sizeof(void *))

/**
 * @def CLAMP
 * @brief Clamp a value to a given range.
 * @note Arguments are evaluated multiple times.
 */
#ifndef CLAMP
/* Use Z_CLAMP for a GCC-only, single evaluation version */
#define CLAMP(val, low, high) (((val) <= (low)) ? (low) : MIN(val, high))
#endif

/**
 * @brief Checks if a value is within range.
 *
 * @note @p val is evaluated twice.
 *
 * @param val Value to be checked.
 * @param min Lower bound (inclusive).
 * @param max Upper bound (inclusive).
 *
 * @retval true If value is within range
 * @retval false If the value is not within range
 */
#define IN_RANGE(val, min, max) (((val) >= (min)) && ((val) <= (max)))

/**
 * @brief      Convert a single character into a hexadecimal nibble.
 *
 * @param[in]  c     The character to convert
 * @param      x     The address of storage for the converted number.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int char2hex(char c, uint8_t *x);

/**
 * @brief      Convert a uint8_t into decimal string representation.
 *
 * Convert a uint8_t value into ASCII decimal string representation.
 * The string is terminated if there is enough space in buf.
 *
 * @param[out] buf     Address of where to store the string representation.
 * @param[in]  buflen  Size of the storage area for string representation.
 * @param[in]  value   The value to convert to decimal string
 *
 * @return     The length of the converted string (excluding terminator if
 *             any), or 0 if an error occurred.
 */
uint8_t u8_to_dec(char *buf, uint8_t buflen, uint8_t value);

/**
 * @brief Properly truncate a NULL-terminated UTF-8 string
 *
 * Take a NULL-terminated UTF-8 string and ensure that if the string has been
 * truncated (by setting the NULL terminator) earlier by other means, that
 * the string ends with a properly formatted UTF-8 character (1-4 bytes).
 *
 * @htmlonly
 * Example:
 *      char test_str[] = "���";
 *      char trunc_utf8[8];
 *
 *      printf("Original : %s\n", test_str); // ���
 *      strncpy(trunc_utf8, test_str, sizeof(trunc_utf8));
 *      trunc_utf8[sizeof(trunc_utf8) - 1] = '\0';
 *      printf("Bad      : %s\n", trunc_utf8); // ��?
 *      utf8_trunc(trunc_utf8);
 *      printf("Truncated: %s\n", trunc_utf8); // ��
 * @endhtmlonly
 *
 * @param utf8_str NULL-terminated string
 *
 *  @return Pointer to the @p utf8_str
 */
char *utf8_trunc(char *utf8_str);

/**
 * @brief Copies a UTF-8 encoded string from @p src to @p dst
 *
 * The resulting @p dst will always be NULL terminated, and the @p dst string
 * will always be properly UTF-8 truncated.
 *
 * @param dst The destination of the UTF-8 string.
 * @param src The source string
 * @param n   The size of the @p dst buffer. Shall not be 0.
 *
 * return Pointer to the @p dst
 */
char *utf8_lcpy(char *dst, const char *src, size_t n);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_UTIL_H_ */
