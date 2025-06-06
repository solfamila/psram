/*
 * Copyright 2023, 2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _S_H_
#define _S_H_

#include "vg_lite.h"

union opcode_coord;

int s_count = 6;

static union opcode_coord s_path1[] = {
    {.opcode=0x02}, {319.3}, {422.5},
    {.opcode=0x09}, {-1.5}, {-1}, {-3.1}, {-1.7}, {-4.9}, {-2.1},
    {.opcode=0x05}, {-0.1}, {-94.4},
    {.opcode=0x09}, {7}, {-1.5}, {12.1}, {-7.8}, {12}, {-15.2},
    {.opcode=0x09}, {-0.1}, {-7.2}, {-5.3}, {-13.2}, {-12}, {-14.6},
    {.opcode=0x05}, {0}, {0},
    {.opcode=0x05}, {-0.5}, {-278.5},
    {.opcode=0x05}, {-5}, {0},
    {.opcode=0x04}, {308.2}, {296},
    {.opcode=0x05}, {0}, {0},
    {.opcode=0x09}, {-7.2}, {1.3}, {-12.6}, {7.7}, {-12.5}, {15.3},
    {.opcode=0x09}, {0.1}, {7.4}, {5.5}, {13.4}, {12.4}, {14.7},
    {.opcode=0x05}, {-0.2}, {94.4},
    {.opcode=0x09}, {-0.4}, {0.1}, {-0.8}, {0.2}, {-1.2}, {0.4},
    {.opcode=0x09}, {-2.8}, {1}, {-5.1}, {2.8}, {-6.8}, {5.2},
    {.opcode=0x09}, {-2.1}, {3.1}, {-2.9}, {6.8}, {-2.2}, {10.4},
    {.opcode=0x09}, {0.7}, {3.7}, {2.7}, {6.9}, {5.8}, {9},
    {.opcode=0x09}, {3.7}, {2.5}, {8.4}, {3.1}, {12.6}, {1.7},
    {.opcode=0x09}, {2.8}, {-1}, {5.1}, {-2.8}, {6.8}, {-5.2},
    {.opcode=0x09}, {2.1}, {-3.1}, {2.9}, {-6.8}, {2.2}, {-10.4},
    {.opcode=0x08}, {324.5}, {427.8}, {322.4}, {424.6}, {319.3}, {422.5},

    {.opcode=0x02}, {316.5}, {437.4},
    {.opcode=0x09}, {-0.7}, {1.1}, {-1.8}, {1.9}, {-3}, {2.3},
    {.opcode=0x09}, {-1.9}, {0.7}, {-4}, {0.4}, {-5.6}, {-0.7},
    {.opcode=0x09}, {-2.8}, {-1.9}, {-3.5}, {-5.8}, {-1.6}, {-8.6},
    {.opcode=0x09}, {0.7}, {-1.1}, {1.8}, {-1.9}, {3}, {-2.3},
    {.opcode=0x09}, {1.9}, {-0.7}, {4}, {-0.4}, {5.6}, {0.7},
    {.opcode=0x08}, {317.7}, {430.8}, {318.5}, {434.6}, {316.5}, {437.4},
    {.opcode=0x0}, 
};

static union opcode_coord s_path2[] = {
    {.opcode=0x02}, {318.8}, {423.3},
    {.opcode=0x09}, {-1.6}, {-1.1}, {-3.4}, {-1.8}, {-5.3}, {-2.1},
    {.opcode=0x05}, {-0.1}, {-96.1},
    {.opcode=0x09}, {0}, {0}, {0}, {0}, {0}, {0},
    {.opcode=0x09}, {6.9}, {-1.1}, {12.1}, {-7.1}, {12}, {-14.3},
    {.opcode=0x09}, {-0.1}, {-7}, {-5.3}, {-12.8}, {-12}, {-13.8},
    {.opcode=0x05}, {0}, {0},
    {.opcode=0x05}, {-0.5}, {-278.4},
    {.opcode=0x05}, {-3}, {0},
    {.opcode=0x05}, {-0.6}, {278.3},
    {.opcode=0x09}, {-7.2}, {0.9}, {-12.6}, {7}, {-12.5}, {14.4},
    {.opcode=0x09}, {0.1}, {7.2}, {5.5}, {13}, {12.4}, {13.9},
    {.opcode=0x09}, {0}, {0}, {0}, {0}, {0}, {0},
    {.opcode=0x05}, {-0.2}, {96.1},
    {.opcode=0x09}, {-0.6}, {0.1}, {-1.3}, {0.3}, {-1.9}, {0.5},
    {.opcode=0x09}, {-2.6}, {0.9}, {-4.8}, {2.6}, {-6.3}, {4.8},
    {.opcode=0x09}, {-4}, {5.9}, {-2.6}, {13.9}, {3.3}, {18},
    {.opcode=0x09}, {3.4}, {2.3}, {7.8}, {2.9}, {11.7}, {1.5},
    {.opcode=0x09}, {2.6}, {-0.9}, {4.8}, {-2.6}, {6.3}, {-4.9},
    {.opcode=0x09}, {2}, {-2.8}, {2.7}, {-6.3}, {2.1}, {-9.7},
    {.opcode=0x08}, {323.5}, {428.2}, {321.6}, {425.2}, {318.8}, {423.3},

    {.opcode=0x02}, {317.3}, {438},
    {.opcode=0x09}, {-0.9}, {1.3}, {-2.1}, {2.2}, {-3.5}, {2.7},
    {.opcode=0x09}, {-2.2}, {0.8}, {-4.6}, {0.4}, {-6.5}, {-0.9},
    {.opcode=0x09}, {-3.3}, {-2.3}, {-4.1}, {-6.7}, {-1.8}, {-10},
    {.opcode=0x09}, {0.9}, {-1.3}, {2.1}, {-2.2}, {3.5}, {-2.7},
    {.opcode=0x09}, {2.2}, {-0.8}, {4.6}, {-0.4}, {6.5}, {0.9},
    {.opcode=0x09}, {1.6}, {1.1}, {2.6}, {2.7}, {3}, {4.6},
    {.opcode=0x08}, {318.8}, {434.5}, {318.4}, {436.4}, {317.3}, {438},
    {.opcode=0x0}, 
};

static union opcode_coord s_path3[] = {
    {.opcode=0x02}, {318.2}, {424.1},
    {.opcode=0x09}, {-1.8}, {-1.2}, {-3.8}, {-1.9}, {-5.8}, {-2.1},
    {.opcode=0x05}, {-0.2}, {-97.8},
    {.opcode=0x09}, {0}, {0}, {0}, {0}, {0}, {0},
    {.opcode=0x09}, {6.8}, {-0.7}, {12.1}, {-6.5}, {12}, {-13.4},
    {.opcode=0x09}, {-0.1}, {-6.8}, {-5.4}, {-12.4}, {-12}, {-13},
    {.opcode=0x05}, {-0.5}, {-278.3},
    {.opcode=0x05}, {-1}, {0},
    {.opcode=0x05}, {-0.6}, {278.2},
    {.opcode=0x05}, {0}, {0},
    {.opcode=0x09}, {-7.1}, {0.4}, {-12.6}, {6.3}, {-12.5}, {13.5},
    {.opcode=0x09}, {0.1}, {7}, {5.6}, {12.6}, {12.5}, {13},
    {.opcode=0x05}, {-0.2}, {97.9},
    {.opcode=0x09}, {-3.3}, {0.4}, {-6.4}, {2.1}, {-8.4}, {5},
    {.opcode=0x09}, {-3.7}, {5.4}, {-2.4}, {12.9}, {3}, {16.6},
    {.opcode=0x09}, {5.4}, {3.7}, {12.9}, {2.4}, {16.6}, {-3},
    {.opcode=0x08}, {325}, {435.3}, {323.6}, {427.8}, {318.2}, {424.1},

    {.opcode=0x02}, {318.2}, {438.6},
    {.opcode=0x09}, {-2.6}, {3.7}, {-7.7}, {4.6}, {-11.4}, {2.1},
    {.opcode=0x09}, {-3.7}, {-2.5}, {-4.6}, {-7.7}, {-2.1}, {-11.4},
    {.opcode=0x09}, {2.5}, {-3.7}, {7.7}, {-4.6}, {11.4}, {-2.1},
    {.opcode=0x08}, {319.8}, {429.8}, {320.7}, {434.9}, {318.2}, {438.6},
    {.opcode=0x0},
};

static union opcode_coord s_path4[] = {
    {.opcode=0x02}, {311}, {300.7},
    {.opcode=0x09}, {-2.9}, {0}, {-5.8}, {1.2}, {-7.8}, {3.6},
    {.opcode=0x09}, {-3.7}, {4.3}, {-3.3}, {10.8}, {1}, {14.5},
    {.opcode=0x09}, {2}, {1.7}, {4.4}, {2.5}, {6.7}, {2.5},
    {.opcode=0x09}, {2.9}, {0}, {5.8}, {-1.2}, {7.8}, {-3.6},
    {.opcode=0x09}, {3.7}, {-4.3}, {3.3}, {-10.8}, {-1}, {-14.5},
    {.opcode=0x08}, {315.8}, {301.5}, {313.4}, {300.7}, {311}, {300.7},
    {.opcode=0x04}, {311}, {300.7},
    {.opcode=0x0},
};

static union opcode_coord s_path5[] = {
    {.opcode=0x02}, {316.7}, {304.4},
    {.opcode=0x09}, {-1.6}, {-1.4}, {-3.6}, {-2.1}, {-5.8}, {-2.1},
    {.opcode=0x09}, {-2.6}, {0}, {-5}, {1.1}, {-6.7}, {3},
    {.opcode=0x09}, {-3.2}, {3.7}, {-2.8}, {9.2}, {0.9}, {12.4},
    {.opcode=0x09}, {1.6}, {1.4}, {3.6}, {2.1}, {5.8}, {2.1},
    {.opcode=0x09}, {2.6}, {0}, {5}, {-1.1}, {6.7}, {-3},
    {.opcode=0x08}, {320.8}, {313.1}, {320.4}, {307.5}, {316.7}, {304.4},
    {.opcode=0x0},
};

static union opcode_coord s_path6[] = {
    {.opcode=0x02}, {307.1}, {307.7},
    {.opcode=0x09}, {1.8}, {-2.1}, {5.1}, {-2.4}, {7.2}, {-0.5},
    {.opcode=0x09}, {2.1}, {1.9}, {2.4}, {5.1}, {0.5}, {7.2},
    {.opcode=0x09}, {-1.8}, {2.1}, {-5.1}, {2.4}, {-7.2}, {0.5},
    {.opcode=0x08}, {305.5}, {313}, {305.3}, {309.8}, {307.1}, {307.7},
    {.opcode=0x0}, 
};

vg_lite_path_t s_data[6] = {
    {{0, 0, 622, 622}, VG_LITE_HIGH, VG_LITE_FP32, {0}, sizeof(s_path1), s_path1, 1},
    {{0, 0, 622, 622}, VG_LITE_HIGH, VG_LITE_FP32, {0}, sizeof(s_path2), s_path2, 1},
    {{0, 0, 622, 622}, VG_LITE_HIGH, VG_LITE_FP32, {0}, sizeof(s_path3), s_path3, 1},
    {{0, 0, 622, 622}, VG_LITE_HIGH, VG_LITE_FP32, {0}, sizeof(s_path4), s_path4, 1},
    {{0, 0, 622, 622}, VG_LITE_HIGH, VG_LITE_FP32, {0}, sizeof(s_path5), s_path5, 1},
    {{0, 0, 622, 622}, VG_LITE_HIGH, VG_LITE_FP32, {0}, sizeof(s_path6), s_path6, 1},
};

uint32_t s_color[] = {
    0x21fa5720, /* s_path1 */
    0x21fa5720, /* s_path2 */
    0xFFfa5720, /* s_path3 */
    0xFFfa8532, /* s_path4 */
    0xFFfa9b3c, /* s_path5 */
    0xFFfad25a, /* s_path6 */
};

#endif
