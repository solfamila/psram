/** @file wm_utils.c
 *
 *  @brief This file provides random number and pattern APIs.
 *
 *  Copyright 2008-2020 NXP
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif /* __linux__ */

#include <stdlib.h>
//#include <crc32.h>
#include <wmtypes.h>
#include <osa.h>
#include <wm_utils.h>
#include <string.h>

#define WM_MAX_FLOAT_PRECISION 9

void fill_sequential_pattern(void *buffer, int size, uint8_t first_byte)
{
    uint8_t byte      = first_byte;
    uint8_t *l_buffer = buffer;
    int i;
    for (i = 0; i < size; i++, byte++)
        *(l_buffer + i) = byte;
}

bool verify_sequential_pattern(const void *buffer, int size, uint8_t first_byte)
{
    uint8_t byte            = first_byte;
    const uint8_t *l_buffer = buffer;
    int i;
    for (i = 0; i < size; i++, byte++)
        if (*(l_buffer + i) != byte)
            return false;

    /* Return verification success */
    return true;
}

char *strdup(const char *s)
{
    char *result = OSA_MemoryAllocate(strlen(s) + 1);
    if (result)
        strcpy(result, s);
    return result;
}

#define MAX_ENTROPY_HDLRS 4
static random_hdlr_t entropy_hdlrs[MAX_ENTROPY_HDLRS];

/* A single seed handler should be sufficient as of now
 * It can be expanded if required */
#define MAX_SEED_HDLRS 1
static random_hdlr_t seed_hdlrs[MAX_SEED_HDLRS];

int random_register_handler(random_hdlr_t func)
{
    int i;

    for (i = 0; i < MAX_ENTROPY_HDLRS; i++)
    {
        if (entropy_hdlrs[i] != NULL)
            continue;

        entropy_hdlrs[i] = func;
        return WM_SUCCESS;
    }

    return -WM_E_NOSPC;
}

int random_unregister_handler(random_hdlr_t func)
{
    int i;

    for (i = 0; i < MAX_ENTROPY_HDLRS; i++)
    {
        if (entropy_hdlrs[i] != func)
            continue;

        entropy_hdlrs[i] = NULL;
        return WM_SUCCESS;
    }

    return -WM_E_INVAL;
}

int random_register_seed_handler(random_hdlr_t func)
{
    int i;

    for (i = 0; i < MAX_SEED_HDLRS; i++)
    {
        if (seed_hdlrs[i] != NULL)
            continue;

        seed_hdlrs[i] = func;
        return WM_SUCCESS;
    }

    return -WM_E_NOSPC;
}

int random_unregister_seed_handler(random_hdlr_t func)
{
    int i;

    for (i = 0; i < MAX_SEED_HDLRS; i++)
    {
        if (seed_hdlrs[i] != func)
            continue;

        seed_hdlrs[i] = NULL;
        return WM_SUCCESS;
    }

    return -WM_E_INVAL;
}

static uint32_t seed;

void random_initialize_seed()
{
    int i;
    for (i = 0; i < MAX_SEED_HDLRS; i++)
    {
        if (seed_hdlrs[i])
        {
            seed ^= (*seed_hdlrs[i])();
        }
    }
    srand(seed);
}

void get_random_sequence(void *buf, unsigned int size)
{
#ifdef __linux__
    int fd;
    fd = open("/dev/urandom", O_RDONLY);
    if (fd)
    {
        read(fd, buf, size);
        close(fd);
        return;
    }
#endif /* __linux__ */
    if (!seed)
        random_initialize_seed();

    int32_t i;
    uint32_t feed_data = 0, curr_time;

    curr_time = OSA_TicksGet();

    for (i = 0; i < MAX_ENTROPY_HDLRS; i++)
    {
        if (entropy_hdlrs[i])
        {
            feed_data ^= (*entropy_hdlrs[i])();
        }
    }

    /* In the beginning, the MSBs are mostly the same, hence XOR with all
     * the bytes in the feed_data to get greater randomness.
     */
    for (i = 0; i < 4; i++)
    {
        feed_data ^= (curr_time << (i * 8));
    }
    /* If the seed is 0 even at this location, it means that there
     * were no seed handlers registered. So, seed the random number
     * generator with feed_data.
     * We will keep the value of seed unchanged as a handler may be
     * registered later to set the seed.
     */
    if (!seed)
        srand(feed_data);

    uint32_t random_num = 0;
    uint8_t *lbuf       = (uint8_t *)buf;
    for (i = 0; i < size; i++)
    {
        /* Get a new random number after every 4 bytes */
        if ((i & 3) == 0)
            random_num = rand() ^ feed_data;
        lbuf[i] = random_num & 0xff;
        random_num >>= 8;
    }
}

/* Function to convert a string to float.
 * \param[in] str Pointer to a string to be addressed
 * \param[out] endptr Pointer pointing to next character in a string */
float wm_strtof(const char *str, char **endptr)
{
    char *start_ptr = (char *)str;
    int sign        = 1;
    char *end_ptr   = NULL;
    if (endptr == NULL)
    {
        endptr = &end_ptr;
    }

    if (*start_ptr == '-')
    {
        sign = -1;
        start_ptr++;
    }
    uint32_t dec_val = 0;
    uint32_t powten  = 1;
    uint32_t int_val = strtoul(start_ptr, endptr, 10);

    /* For Overload Handling because of iOS 11*/
    char temp_buf[WM_MAX_FLOAT_PRECISION + 1];

    if (**endptr == '.')
    {
        start_ptr = *endptr + 1;
        char *temp;
        temp         = start_ptr;
        uint32_t len = 0;
        while (isdigit((unsigned char)temp[len]))
        {
            len++;
            (*endptr)++;
        }
        /* If digits after decimal are greater than maximum float
         * precision*/
        if (len > WM_MAX_FLOAT_PRECISION)
        {
            (void)memcpy(temp_buf, start_ptr, (WM_MAX_FLOAT_PRECISION) * sizeof(char));
            temp_buf[WM_MAX_FLOAT_PRECISION] = '\0';
            dec_val                          = strtoul(temp_buf, NULL, 10);
            len                              = WM_MAX_FLOAT_PRECISION;
            while (len--)
                powten *= 10;
            /* endptr is right now pointing just before '}'*/
            (*endptr)++;
        }
        else
        {
            dec_val = strtoul(start_ptr, endptr, 10);
            while (start_ptr++ != *endptr)
                powten *= 10;
        }
    }
    else
        return sign * (float)int_val;

    float dec_frac = (float)dec_val / powten;
    float result   = (float)(int_val + dec_frac);

    /* Below part is done in order to improve the accuracy of the result.
     * Since addition above results in float value being drifted from
     * the actual value by narrow margin. e.g 50.10 results in float
     * equivalent of 50.09.*/
    /* TODO: Visit again to see if the below code really helps.
     * Sometimes, reporting values differently is a result of the
     * way float values are stored in memory. In that case, the below
     * code will not improve anything. Eg. If 50.1 is stored as 50.09
     * in memory, we cannot do much about it.
     */
    uint32_t result_int_value = wm_int_part_of(result);
    float result_frac_value   = (float)(result)-result_int_value;

    /* Generally difference between two float values comes out to be in
     * the order of 1/powten. e.g 0.10-0.09 comes out to be 0.00...1.
     * Hence we multiply the result of subtraction to achieve the accuracy
     * within desired float precision. */
    if (wm_frac_part_of(dec_frac, powten / 10) > wm_frac_part_of(result_frac_value, powten / 10))
        result += ((dec_frac - result_frac_value) * powten);
    if (wm_frac_part_of(result_frac_value, powten / 10) > wm_frac_part_of(dec_frac, powten / 10))
        result += ((result_frac_value - dec_frac) * powten);

    return sign * result;
}
