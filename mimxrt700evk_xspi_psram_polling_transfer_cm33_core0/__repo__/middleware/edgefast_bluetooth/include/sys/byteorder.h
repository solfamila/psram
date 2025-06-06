/** @file
 *  @brief Byte order helpers.
 */

/*
 * Copyright (c) 2015-2016, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_BYTEORDER_H_
#define ZEPHYR_INCLUDE_SYS_BYTEORDER_H_

#include "fsl_common.h"
#include <stddef.h>

#include <toolchain.h>

/** @brief Encode 16-bit value into array values in little-endian format.
 *
 *  Helper macro to encode 16-bit values into comma separated values.
 *
 *  @note @p _v is evaluated 2 times.
 *
 *  @param _v 16-bit integer in host endianness.
 *
 *  @return The comma separated values for the 16-bit value.
 */
#define BT_BYTES_LIST_LE16(_v)         \
	(((_v) >>  0) & 0xFFU),     \
	(((_v) >>  8) & 0xFFU)      \

/** @brief Encode 24-bit value into array values in little-endian format.
 *
 *  Helper macro to encode 24-bit values into comma separated values.
 *
 *  @note @p _v is evaluated 3 times.
 *
 *  @param _v 24-bit integer in host endianness.
 *
 *  @return The comma separated values for the 24-bit value.
 */
#define BT_BYTES_LIST_LE24(_v)         \
	BT_BYTES_LIST_LE16(_v),        \
	(((_v) >> 16) & 0xFFU)      \

/** @brief Encode 32-bit value into array values in little-endian format.
 *
 *  Helper macro to encode 32-bit values into comma separated values.
 *
 *  @note @p _v is evaluated 4 times.
 *
 *  @param _v 32-bit integer in host endianness.
 *
 *  @return The comma separated values for the 32-bit value.
 */
#define BT_BYTES_LIST_LE32(_v)         \
	BT_BYTES_LIST_LE24(_v),        \
	(((_v) >> 24) & 0xFFU)      \

/** @brief Encode 40-bit value into array values in little-endian format.
 *
 *  Helper macro to encode 40-bit values into comma separated values.
 *
 *  @note @p _v is evaluated 5 times.
 *
 *  @param _v 40-bit integer in host endianness.
 *
 *  @return The comma separated values for the 40-bit value.
 */
#define BT_BYTES_LIST_LE40(_v)         \
	BT_BYTES_LIST_LE24(_v),        \
	BT_BYTES_LIST_LE16((_v) >> 24) \

/** @brief Encode 48-bit value into array values in little-endian format.
 *
 *  Helper macro to encode 48-bit values into comma separated values.
 *
 *  @note @p _v is evaluated 6 times.
 *
 *  @param _v 48-bit integer in host endianness.
 *
 *  @return The comma separated values for the 48-bit value.
 */
#define BT_BYTES_LIST_LE48(_v)         \
	BT_BYTES_LIST_LE32(_v),        \
	BT_BYTES_LIST_LE16((_v) >> 32) \

/** @brief Encode 64-bit value into array values in little-endian format.
 *
 *  Helper macro to encode 64-bit values into comma separated values.
 *
 *  @note @p _v is evaluated 8 times.
 *
 *  @param _v 64-bit integer in host endianness.
 *
 *  @return The comma separated values for the 64-bit value.
 */
#define BT_BYTES_LIST_LE64(_v)         \
	BT_BYTES_LIST_LE32(_v),        \
	BT_BYTES_LIST_LE32((_v) >> 32) \

/* Internal helpers only used by the sys_* APIs further below */
#define __bswap_16(x) ((uint16_t) ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8)))
#define __bswap_24(x) ((uint32_t) ((((x) >> 16) & 0xff) | \
				   (((x)) & 0xff00) | \
				   (((x) & 0xff) << 16)))
#define __bswap_32(x) ((uint32_t) ((((x) >> 24) & 0xff) | \
				   (((x) >> 8) & 0xff00) | \
				   (((x) & 0xff00) << 8) | \
				   (((x) & 0xff) << 24)))
#define __bswap_48(x) ((uint64_t) ((((x) >> 40) & 0xff) | \
				   (((x) >> 24) & 0xff00) | \
				   (((x) >> 8) & 0xff0000) | \
				   (((x) & 0xff0000) << 8) | \
				   (((x) & 0xff00) << 24) | \
				   (((x) & 0xff) << 40)))
#define __bswap_64(x) ((uint64_t) ((((x) >> 56) & 0xff) | \
				   (((x) >> 40) & 0xff00) | \
				   (((x) >> 24) & 0xff0000) | \
				   (((x) >> 8) & 0xff000000) | \
				   (((x) & 0xff000000) << 8) | \
				   (((x) & 0xff0000) << 24) | \
				   (((x) & 0xff00) << 40) | \
				   (((x) & 0xff) << 56)))

/** @def sys_le16_to_cpu
 *  @brief Convert 16-bit integer from little-endian to host endianness.
 *
 *  @param val 16-bit integer in little-endian format.
 *
 *  @return 16-bit integer in host endianness.
 */

/** @def sys_cpu_to_le16
 *  @brief Convert 16-bit integer from host endianness to little-endian.
 *
 *  @param val 16-bit integer in host endianness.
 *
 *  @return 16-bit integer in little-endian format.
 */

/** @def sys_le24_to_cpu
 *  @brief Convert 24-bit integer from little-endian to host endianness.
 *
 *  @param val 24-bit integer in little-endian format.
 *
 *  @return 24-bit integer in host endianness.
 */

/** @def sys_cpu_to_le24
 *  @brief Convert 24-bit integer from host endianness to little-endian.
 *
 *  @param val 24-bit integer in host endianness.
 *
 *  @return 24-bit integer in little-endian format.
 */

/** @def sys_le32_to_cpu
 *  @brief Convert 32-bit integer from little-endian to host endianness.
 *
 *  @param val 32-bit integer in little-endian format.
 *
 *  @return 32-bit integer in host endianness.
 */

/** @def sys_cpu_to_le32
 *  @brief Convert 32-bit integer from host endianness to little-endian.
 *
 *  @param val 32-bit integer in host endianness.
 *
 *  @return 32-bit integer in little-endian format.
 */

/** @def sys_le48_to_cpu
 *  @brief Convert 48-bit integer from little-endian to host endianness.
 *
 *  @param val 48-bit integer in little-endian format.
 *
 *  @return 48-bit integer in host endianness.
 */

/** @def sys_cpu_to_le48
 *  @brief Convert 48-bit integer from host endianness to little-endian.
 *
 *  @param val 48-bit integer in host endianness.
 *
 *  @return 48-bit integer in little-endian format.
 */

/** @def sys_be16_to_cpu
 *  @brief Convert 16-bit integer from big-endian to host endianness.
 *
 *  @param val 16-bit integer in big-endian format.
 *
 *  @return 16-bit integer in host endianness.
 */

/** @def sys_cpu_to_be16
 *  @brief Convert 16-bit integer from host endianness to big-endian.
 *
 *  @param val 16-bit integer in host endianness.
 *
 *  @return 16-bit integer in big-endian format.
 */

/** @def sys_be24_to_cpu
 *  @brief Convert 24-bit integer from big-endian to host endianness.
 *
 *  @param val 24-bit integer in big-endian format.
 *
 *  @return 24-bit integer in host endianness.
 */

/** @def sys_cpu_to_be24
 *  @brief Convert 24-bit integer from host endianness to big-endian.
 *
 *  @param val 24-bit integer in host endianness.
 *
 *  @return 24-bit integer in big-endian format.
 */

/** @def sys_be32_to_cpu
 *  @brief Convert 32-bit integer from big-endian to host endianness.
 *
 *  @param val 32-bit integer in big-endian format.
 *
 *  @return 32-bit integer in host endianness.
 */

/** @def sys_cpu_to_be32
 *  @brief Convert 32-bit integer from host endianness to big-endian.
 *
 *  @param val 32-bit integer in host endianness.
 *
 *  @return 32-bit integer in big-endian format.
 */

/** @def sys_be48_to_cpu
 *  @brief Convert 48-bit integer from big-endian to host endianness.
 *
 *  @param val 48-bit integer in big-endian format.
 *
 *  @return 48-bit integer in host endianness.
 */

/** @def sys_cpu_to_be48
 *  @brief Convert 48-bit integer from host endianness to big-endian.
 *
 *  @param val 48-bit integer in host endianness.
 *
 *  @return 48-bit integer in big-endian format.
 */

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define sys_le16_to_cpu(val) (val)
#define sys_cpu_to_le16(val) (val)
#define sys_le24_to_cpu(val) (val)
#define sys_cpu_to_le24(val) (val)
#define sys_le32_to_cpu(val) (val)
#define sys_cpu_to_le32(val) (val)
#define sys_le48_to_cpu(val) (val)
#define sys_cpu_to_le48(val) (val)
#define sys_le64_to_cpu(val) (val)
#define sys_cpu_to_le64(val) (val)
#define sys_be16_to_cpu(val) __bswap_16(val)
#define sys_cpu_to_be16(val) __bswap_16(val)
#define sys_be24_to_cpu(val) __bswap_24(val)
#define sys_cpu_to_be24(val) __bswap_24(val)
#define sys_be32_to_cpu(val) __bswap_32(val)
#define sys_cpu_to_be32(val) __bswap_32(val)
#define sys_be48_to_cpu(val) __bswap_48(val)
#define sys_cpu_to_be48(val) __bswap_48(val)
#define sys_be64_to_cpu(val) __bswap_64(val)
#define sys_cpu_to_be64(val) __bswap_64(val)
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define sys_le16_to_cpu(val) __bswap_16(val)
#define sys_cpu_to_le16(val) __bswap_16(val)
#define sys_le24_to_cpu(val) __bswap_24(val)
#define sys_cpu_to_le24(val) __bswap_24(val)
#define sys_le32_to_cpu(val) __bswap_32(val)
#define sys_cpu_to_le32(val) __bswap_32(val)
#define sys_le48_to_cpu(val) __bswap_48(val)
#define sys_cpu_to_le48(val) __bswap_48(val)
#define sys_le64_to_cpu(val) __bswap_64(val)
#define sys_cpu_to_le64(val) __bswap_64(val)
#define sys_be16_to_cpu(val) (val)
#define sys_cpu_to_be16(val) (val)
#define sys_be24_to_cpu(val) (val)
#define sys_cpu_to_be24(val) (val)
#define sys_be32_to_cpu(val) (val)
#define sys_cpu_to_be32(val) (val)
#define sys_be48_to_cpu(val) (val)
#define sys_cpu_to_be48(val) (val)
#define sys_be64_to_cpu(val) (val)
#define sys_cpu_to_be64(val) (val)
#else
#error "Unknown byte order"
#endif

/**
 *  @brief Put a 16-bit integer as big-endian to arbitrary location.
 *
 *  Put a 16-bit integer, originally in host endianness, to a
 *  potentially unaligned memory location in big-endian format.
 *
 *  @param val 16-bit integer in host endianness.
 *  @param dst Destination memory address to store the result.
 */
static inline void sys_put_be16(uint16_t val, uint8_t dst[2])
{
	dst[0] = (uint8_t)(val >> 8);
	dst[1] = (uint8_t)val;
}

/**
 *  @brief Put a 24-bit integer as big-endian to arbitrary location.
 *
 *  Put a 24-bit integer, originally in host endianness, to a
 *  potentially unaligned memory location in big-endian format.
 *
 *  @param val 24-bit integer in host endianness.
 *  @param dst Destination memory address to store the result.
 */
static inline void sys_put_be24(uint32_t val, uint8_t dst[3])
{
	dst[0] = (uint8_t)(val >> 16);
	sys_put_be16((uint16_t)val, &dst[1]);
}

/**
 *  @brief Put a 32-bit integer as big-endian to arbitrary location.
 *
 *  Put a 32-bit integer, originally in host endianness, to a
 *  potentially unaligned memory location in big-endian format.
 *
 *  @param val 32-bit integer in host endianness.
 *  @param dst Destination memory address to store the result.
 */
static inline void sys_put_be32(uint32_t val, uint8_t dst[4])
{
	sys_put_be16((uint16_t)(val >> 16), dst);
	sys_put_be16((uint16_t)val, &dst[2]);
}

/**
 *  @brief Put a 48-bit integer as big-endian to arbitrary location.
 *
 *  Put a 48-bit integer, originally in host endianness, to a
 *  potentially unaligned memory location in big-endian format.
 *
 *  @param val 48-bit integer in host endianness.
 *  @param dst Destination memory address to store the result.
 */
static inline void sys_put_be48(uint64_t val, uint8_t dst[6])
{
	sys_put_be16((uint16_t)(val >> 32), dst);
	sys_put_be32((uint32_t)val, &dst[2]);
}

/**
 *  @brief Put a 64-bit integer as big-endian to arbitrary location.
 *
 *  Put a 64-bit integer, originally in host endianness, to a
 *  potentially unaligned memory location in big-endian format.
 *
 *  @param val 64-bit integer in host endianness.
 *  @param dst Destination memory address to store the result.
 */
static inline void sys_put_be64(uint64_t val, uint8_t dst[8])
{
	sys_put_be32((uint32_t)(val >> 32), dst);
	sys_put_be32((uint32_t)val, &dst[4]);
}

/**
 *  @brief Put a 16-bit integer as little-endian to arbitrary location.
 *
 *  Put a 16-bit integer, originally in host endianness, to a
 *  potentially unaligned memory location in little-endian format.
 *
 *  @param val 16-bit integer in host endianness.
 *  @param dst Destination memory address to store the result.
 */
static inline void sys_put_le16(uint16_t val, uint8_t dst[2])
{
	dst[0] = (uint8_t)val;
	dst[1] = (uint8_t)(val >> 8);
}

/**
 *  @brief Put a 24-bit integer as little-endian to arbitrary location.
 *
 *  Put a 24-bit integer, originally in host endianness, to a
 *  potentially unaligned memory location in littel-endian format.
 *
 *  @param val 24-bit integer in host endianness.
 *  @param dst Destination memory address to store the result.
 */
static inline void sys_put_le24(uint32_t val, uint8_t dst[3])
{
	sys_put_le16((uint16_t)val, dst);
	dst[2] = (uint8_t)(val >> 16);
}

/**
 *  @brief Put a 32-bit integer as little-endian to arbitrary location.
 *
 *  Put a 32-bit integer, originally in host endianness, to a
 *  potentially unaligned memory location in little-endian format.
 *
 *  @param val 32-bit integer in host endianness.
 *  @param dst Destination memory address to store the result.
 */
static inline void sys_put_le32(uint32_t val, uint8_t dst[4])
{
	sys_put_le16((uint16_t)val, dst);
	sys_put_le16((uint16_t)(val >> 16), &dst[2]);
}

/**
 *  @brief Put a 48-bit integer as little-endian to arbitrary location.
 *
 *  Put a 48-bit integer, originally in host endianness, to a
 *  potentially unaligned memory location in little-endian format.
 *
 *  @param val 48-bit integer in host endianness.
 *  @param dst Destination memory address to store the result.
 */
static inline void sys_put_le48(uint64_t val, uint8_t dst[6])
{
	sys_put_le32((uint32_t)val, dst);
	sys_put_le16((uint16_t)(val >> 32), &dst[4]);
}

/**
 *  @brief Put a 64-bit integer as little-endian to arbitrary location.
 *
 *  Put a 64-bit integer, originally in host endianness, to a
 *  potentially unaligned memory location in little-endian format.
 *
 *  @param val 64-bit integer in host endianness.
 *  @param dst Destination memory address to store the result.
 */
static inline void sys_put_le64(uint64_t val, uint8_t dst[8])
{
	sys_put_le32((uint32_t)val, dst);
	sys_put_le32((uint32_t)(val >> 32), &dst[4]);
}

/**
 *  @brief Get a 16-bit integer stored in big-endian format.
 *
 *  Get a 16-bit integer, stored in big-endian format in a potentially
 *  unaligned memory location, and convert it to the host endianness.
 *
 *  @param src Location of the big-endian 16-bit integer to get.
 *
 *  @return 16-bit integer in host endianness.
 */
static inline uint16_t sys_get_be16(const uint8_t src[2])
{
	return ((uint16_t)src[0] << 8) | src[1];
}

/**
 *  @brief Get a 24-bit integer stored in big-endian format.
 *
 *  Get a 24-bit integer, stored in big-endian format in a potentially
 *  unaligned memory location, and convert it to the host endianness.
 *
 *  @param src Location of the big-endian 24-bit integer to get.
 *
 *  @return 24-bit integer in host endianness.
 */
static inline uint32_t sys_get_be24(const uint8_t src[3])
{
	return ((uint32_t)src[0] << 16) | sys_get_be16(&src[1]);
}

/**
 *  @brief Get a 32-bit integer stored in big-endian format.
 *
 *  Get a 32-bit integer, stored in big-endian format in a potentially
 *  unaligned memory location, and convert it to the host endianness.
 *
 *  @param src Location of the big-endian 32-bit integer to get.
 *
 *  @return 32-bit integer in host endianness.
 */
static inline uint32_t sys_get_be32(const uint8_t src[4])
{
	return ((uint32_t)sys_get_be16(&src[0]) << 16) | sys_get_be16(&src[2]);
}

/**
 *  @brief Get a 48-bit integer stored in big-endian format.
 *
 *  Get a 48-bit integer, stored in big-endian format in a potentially
 *  unaligned memory location, and convert it to the host endianness.
 *
 *  @param src Location of the big-endian 48-bit integer to get.
 *
 *  @return 48-bit integer in host endianness.
 */
static inline uint64_t sys_get_be48(const uint8_t src[6])
{
	return ((uint64_t)sys_get_be32(&src[0]) << 16) | sys_get_be16(&src[4]);
}

/**
 *  @brief Get a 64-bit integer stored in big-endian format.
 *
 *  Get a 64-bit integer, stored in big-endian format in a potentially
 *  unaligned memory location, and convert it to the host endianness.
 *
 *  @param src Location of the big-endian 64-bit integer to get.
 *
 *  @return 64-bit integer in host endianness.
 */
static inline uint64_t sys_get_be64(const uint8_t src[8])
{
	return ((uint64_t)sys_get_be32(&src[0]) << 32) | sys_get_be32(&src[4]);
}

/**
 *  @brief Get a 16-bit integer stored in little-endian format.
 *
 *  Get a 16-bit integer, stored in little-endian format in a potentially
 *  unaligned memory location, and convert it to the host endianness.
 *
 *  @param src Location of the little-endian 16-bit integer to get.
 *
 *  @return 16-bit integer in host endianness.
 */
static inline uint16_t sys_get_le16(const uint8_t src[2])
{
	return ((uint16_t)src[1] << 8) | src[0];
}

/**
 *  @brief Get a 24-bit integer stored in big-endian format.
 *
 *  Get a 24-bit integer, stored in big-endian format in a potentially
 *  unaligned memory location, and convert it to the host endianness.
 *
 *  @param src Location of the big-endian 24-bit integer to get.
 *
 *  @return 24-bit integer in host endianness.
 */
static inline uint32_t sys_get_le24(const uint8_t src[3])
{
	return ((uint32_t)src[2] << 16) | sys_get_le16(&src[0]);
}

/**
 *  @brief Get a 32-bit integer stored in little-endian format.
 *
 *  Get a 32-bit integer, stored in little-endian format in a potentially
 *  unaligned memory location, and convert it to the host endianness.
 *
 *  @param src Location of the little-endian 32-bit integer to get.
 *
 *  @return 32-bit integer in host endianness.
 */
static inline uint32_t sys_get_le32(const uint8_t src[4])
{
	return ((uint32_t)sys_get_le16(&src[2]) << 16) | sys_get_le16(&src[0]);
}

/**
 *  @brief Get a 48-bit integer stored in little-endian format.
 *
 *  Get a 48-bit integer, stored in little-endian format in a potentially
 *  unaligned memory location, and convert it to the host endianness.
 *
 *  @param src Location of the little-endian 48-bit integer to get.
 *
 *  @return 48-bit integer in host endianness.
 */
static inline uint64_t sys_get_le48(const uint8_t src[6])
{
	return ((uint64_t)sys_get_le32(&src[2]) << 16) | sys_get_le16(&src[0]);
}

/**
 *  @brief Get a 64-bit integer stored in little-endian format.
 *
 *  Get a 64-bit integer, stored in little-endian format in a potentially
 *  unaligned memory location, and convert it to the host endianness.
 *
 *  @param src Location of the little-endian 64-bit integer to get.
 *
 *  @return 64-bit integer in host endianness.
 */
static inline uint64_t sys_get_le64(const uint8_t src[8])
{
	return ((uint64_t)sys_get_le32(&src[4]) << 32) | sys_get_le32(&src[0]);
}

/**
 * @brief Swap one buffer content into another
 *
 * Copy the content of src buffer into dst buffer in reversed order,
 * i.e.: src[n] will be put in dst[end-n]
 * Where n is an index and 'end' the last index in both arrays.
 * The 2 memory pointers must be pointing to different areas, and have
 * a minimum size of given length.
 *
 * @param dst A valid pointer on a memory area where to copy the data in
 * @param src A valid pointer on a memory area where to copy the data from
 * @param length Size of both dst and src memory areas
 */
static inline void sys_memcpy_swap(void *dst, const void *src, size_t length)
{
	uint8_t *pdst = (uint8_t *)dst;
	const uint8_t *psrc = (const uint8_t *)src;

	__ASSERT(((psrc < pdst && (psrc + length) <= pdst) ||
		  (psrc > pdst && (pdst + length) <= psrc)),
		 "Source and destination buffers must not overlap");

	psrc += length - 1U;

	for (; length > 0U; length--) {
		*pdst++ = *psrc--;
	}
}

/**
 * @brief Swap buffer content
 *
 * In-place memory swap, where final content will be reversed.
 * I.e.: buf[n] will be put in buf[end-n]
 * Where n is an index and 'end' the last index of buf.
 *
 * @param buf A valid pointer on a memory area to swap
 * @param length Size of buf memory area
 */
static inline void sys_mem_swap(void *buf, size_t length)
{
	size_t i;

	for (i = 0U; i < (length/2U); i++) {
		uint8_t tmp = ((uint8_t *)buf)[i];

		((uint8_t *)buf)[i] = ((uint8_t *)buf)[length - 1U - i];
		((uint8_t *)buf)[length - 1U - i] = tmp;
	}
}

#endif /* ZEPHYR_INCLUDE_SYS_BYTEORDER_H_ */
