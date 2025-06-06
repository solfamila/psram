/** @file
 * Copyright (c) 2020-2023, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
**/

#include "test_crypto_common.h"

uint32_t g_test_count = 1;
void crypto_common_exit_action(void);

const uint8_t key_data[] = {
 0x01, 0x03, 0x0E, 0x0F, 0x1F, 0x3F, 0xEF, 0XFF,
 0x02, 0x0A, 0x2A, 0xAA, 0x04, 0x24, 0x88, 0x03,
 0x33, 0x77, 0xFF, 0x80, 0xC0, 0xE0, 0xF0, 0xF8,
 0xFC, 0xFE, 0xFF, 0xC0, 0xF0, 0xFC, 0xE0, 0xEE,
 0x01, 0x03, 0x0E, 0x0F, 0x1F, 0x3F, 0xEF, 0XFF,
 0x02, 0x0A, 0x2A, 0xAA, 0x04, 0x24, 0x88, 0x03,
 0x33, 0x77, 0xFF, 0x80, 0xC0, 0xE0, 0xF0, 0xF8,
 0xFC, 0xFE, 0xFF, 0xC0, 0xF0, 0xFC, 0xE0, 0xEE};

const uint8_t rsa_128_key_pair[] = {
0x30, 0x82, 0x02, 0x5e, 0x02, 0x01, 0x00, 0x02, 0x81, 0x81, 0x00, 0xaf, 0x05,
0x7d, 0x39, 0x6e, 0xe8, 0x4f, 0xb7, 0x5f, 0xdb, 0xb5, 0xc2, 0xb1, 0x3c, 0x7f,
0xe5, 0xa6, 0x54, 0xaa, 0x8a, 0xa2, 0x47, 0x0b, 0x54, 0x1e, 0xe1, 0xfe, 0xb0,
0xb1, 0x2d, 0x25, 0xc7, 0x97, 0x11, 0x53, 0x12, 0x49, 0xe1, 0x12, 0x96, 0x28,
0x04, 0x2d, 0xbb, 0xb6, 0xc1, 0x20, 0xd1, 0x44, 0x35, 0x24, 0xef, 0x4c, 0x0e,
0x6e, 0x1d, 0x89, 0x56, 0xee, 0xb2, 0x07, 0x7a, 0xf1, 0x23, 0x49, 0xdd, 0xee,
0xe5, 0x44, 0x83, 0xbc, 0x06, 0xc2, 0xc6, 0x19, 0x48, 0xcd, 0x02, 0xb2, 0x02,
0xe7, 0x96, 0xae, 0xbd, 0x94, 0xd3, 0xa7, 0xcb, 0xf8, 0x59, 0xc2, 0xc1, 0x81,
0x9c, 0x32, 0x4c, 0xb8, 0x2b, 0x9c, 0xd3, 0x4e, 0xde, 0x26, 0x3a, 0x2a, 0xbf,
0xfe, 0x47, 0x33, 0xf0, 0x77, 0x86, 0x9e, 0x86, 0x60, 0xf7, 0xd6, 0x83, 0x4d,
0xa5, 0x3d, 0x69, 0x0e, 0xf7, 0x98, 0x5f, 0x6b, 0xc3, 0x02, 0x03, 0x01, 0x00,
0x01, 0x02, 0x81, 0x81, 0x00, 0x87, 0x4b, 0xf0, 0xff, 0xc2, 0xf2, 0xa7, 0x1d,
0x14, 0x67, 0x1d, 0xdd, 0x01, 0x71, 0xc9, 0x54, 0xd7, 0xfd, 0xbf, 0x50, 0x28,
0x1e, 0x4f, 0x6d, 0x99, 0xea, 0x0e, 0x1e, 0xbc, 0xf8, 0x2f, 0xaa, 0x58, 0xe7,
0xb5, 0x95, 0xff, 0xb2, 0x93, 0xd1, 0xab, 0xe1, 0x7f, 0x11, 0x0b, 0x37, 0xc4,
0x8c, 0xc0, 0xf3, 0x6c, 0x37, 0xe8, 0x4d, 0x87, 0x66, 0x21, 0xd3, 0x27, 0xf6,
0x4b, 0xbe, 0x08, 0x45, 0x7d, 0x3e, 0xc4, 0x09, 0x8b, 0xa2, 0xfa, 0x0a, 0x31,
0x9f, 0xba, 0x41, 0x1c, 0x28, 0x41, 0xed, 0x7b, 0xe8, 0x31, 0x96, 0xa8, 0xcd,
0xf9, 0xda, 0xa5, 0xd0, 0x06, 0x94, 0xbc, 0x33, 0x5f, 0xc4, 0xc3, 0x22, 0x17,
0xfe, 0x04, 0x88, 0xbc, 0xe9, 0xcb, 0x72, 0x02, 0xe5, 0x94, 0x68, 0xb1, 0xea,
0xd1, 0x19, 0x00, 0x04, 0x77, 0xdb, 0x2c, 0xa7, 0x97, 0xfa, 0xc1, 0x9e, 0xda,
0x3f, 0x58, 0xc1, 0x02, 0x41, 0x00, 0xe2, 0xab, 0x76, 0x08, 0x41, 0xbb, 0x9d,
0x30, 0xa8, 0x1d, 0x22, 0x2d, 0xe1, 0xeb, 0x73, 0x81, 0xd8, 0x22, 0x14, 0x40,
0x7f, 0x1b, 0x97, 0x5c, 0xbb, 0xfe, 0x4e, 0x1a, 0x94, 0x67, 0xfd, 0x98, 0xad,
0xbd, 0x78, 0xf6, 0x07, 0x83, 0x6c, 0xa5, 0xbe, 0x19, 0x28, 0xb9, 0xd1, 0x60,
0xd9, 0x7f, 0xd4, 0x5c, 0x12, 0xd6, 0xb5, 0x2e, 0x2c, 0x98, 0x71, 0xa1, 0x74,
0xc6, 0x6b, 0x48, 0x81, 0x13, 0x02, 0x41, 0x00, 0xc5, 0xab, 0x27, 0x60, 0x21,
0x59, 0xae, 0x7d, 0x6f, 0x20, 0xc3, 0xc2, 0xee, 0x85, 0x1e, 0x46, 0xdc, 0x11,
0x2e, 0x68, 0x9e, 0x28, 0xd5, 0xfc, 0xbb, 0xf9, 0x90, 0xa9, 0x9e, 0xf8, 0xa9,
0x0b, 0x8b, 0xb4, 0x4f, 0xd3, 0x64, 0x67, 0xe7, 0xfc, 0x17, 0x89, 0xce, 0xb6,
0x63, 0xab, 0xda, 0x33, 0x86, 0x52, 0xc3, 0xc7, 0x3f, 0x11, 0x17, 0x74, 0x90,
0x2e, 0x84, 0x05, 0x65, 0x92, 0x70, 0x91, 0x02, 0x41, 0x00, 0xb6, 0xcd, 0xbd,
0x35, 0x4f, 0x7d, 0xf5, 0x79, 0xa6, 0x3b, 0x48, 0xb3, 0x64, 0x3e, 0x35, 0x3b,
0x84, 0x89, 0x87, 0x77, 0xb4, 0x8b, 0x15, 0xf9, 0x4e, 0x0b, 0xfc, 0x05, 0x67,
0xa6, 0xae, 0x59, 0x11, 0xd5, 0x7a, 0xd6, 0x40, 0x9c, 0xf7, 0x64, 0x7b, 0xf9,
0x62, 0x64, 0xe9, 0xbd, 0x87, 0xeb, 0x95, 0xe2, 0x63, 0xb7, 0x11, 0x0b, 0x9a,
0x1f, 0x9f, 0x94, 0xac, 0xce, 0xd0, 0xfa, 0xfa, 0x4d, 0x02, 0x40, 0x71, 0x19,
0x5e, 0xec, 0x37, 0xe8, 0xd2, 0x57, 0xde, 0xcf, 0xc6, 0x72, 0xb0, 0x7a, 0xe6,
0x39, 0xf1, 0x0c, 0xbb, 0x9b, 0x0c, 0x73, 0x9d, 0x0c, 0x80, 0x99, 0x68, 0xd6,
0x44, 0xa9, 0x4e, 0x3f, 0xd6, 0xed, 0x92, 0x87, 0x07, 0x7a, 0x14, 0x58, 0x3f,
0x37, 0x90, 0x58, 0xf7, 0x6a, 0x8a, 0xec, 0xd4, 0x3c, 0x62, 0xdc, 0x8c, 0x0f,
0x41, 0x76, 0x66, 0x50, 0xd7, 0x25, 0x27, 0x5a, 0xc4, 0xa1, 0x02, 0x41, 0x00,
0xbb, 0x32, 0xd1, 0x33, 0xed, 0xc2, 0xe0, 0x48, 0xd4, 0x63, 0x38, 0x8b, 0x7b,
0xe9, 0xcb, 0x4b, 0xe2, 0x9f, 0x4b, 0x62, 0x50, 0xbe, 0x60, 0x3e, 0x70, 0xe3,
0x64, 0x75, 0x01, 0xc9, 0x7d, 0xdd, 0xe2, 0x0a, 0x4e, 0x71, 0xbe, 0x95, 0xfd,
0x5e, 0x71, 0x78, 0x4e, 0x25, 0xac, 0xa4, 0xba, 0xf2, 0x5b, 0xe5, 0x73, 0x8a,
0xae, 0x59, 0xbb, 0xfe, 0x1c, 0x99, 0x77, 0x81, 0x44, 0x7a, 0x2b, 0x24};

const uint8_t rsa_128_key_data[] = {
 0x30, 0x81, 0x89, 0x02,
 0x81, 0x81, 0x00, 0xaf, 0x05, 0x7d, 0x39, 0x6e, 0xe8, 0x4f, 0xb7, 0x5f, 0xdb,
 0xb5, 0xc2, 0xb1, 0x3c, 0x7f, 0xe5, 0xa6, 0x54, 0xaa, 0x8a, 0xa2, 0x47, 0x0b,
 0x54, 0x1e, 0xe1, 0xfe, 0xb0, 0xb1, 0x2d, 0x25, 0xc7, 0x97, 0x11, 0x53, 0x12,
 0x49, 0xe1, 0x12, 0x96, 0x28, 0x04, 0x2d, 0xbb, 0xb6, 0xc1, 0x20, 0xd1, 0x44,
 0x35, 0x24, 0xef, 0x4c, 0x0e, 0x6e, 0x1d, 0x89, 0x56, 0xee, 0xb2, 0x07, 0x7a,
 0xf1, 0x23, 0x49, 0xdd, 0xee, 0xe5, 0x44, 0x83, 0xbc, 0x06, 0xc2, 0xc6, 0x19,
 0x48, 0xcd, 0x02, 0xb2, 0x02, 0xe7, 0x96, 0xae, 0xbd, 0x94, 0xd3, 0xa7, 0xcb,
 0xf8, 0x59, 0xc2, 0xc1, 0x81, 0x9c, 0x32, 0x4c, 0xb8, 0x2b, 0x9c, 0xd3, 0x4e,
 0xde, 0x26, 0x3a, 0x2a, 0xbf, 0xfe, 0x47, 0x33, 0xf0, 0x77, 0x86, 0x9e, 0x86,
 0x60, 0xf7, 0xd6, 0x83, 0x4d, 0xa5, 0x3d, 0x69, 0x0e, 0xf7, 0x98, 0x5f, 0x6b,
 0xc3, 0x02, 0x03, 0x01, 0x00, 0x01};

const uint8_t rsa_256_key_pair[] = {
 0x30, 0x82, 0x04, 0xA5, 0x02, 0x01, 0x00, 0x02, 0x82, 0x01, 0x01, 0x00, 0xC0,
 0x95, 0x08, 0xE1, 0x57, 0x41, 0xF2, 0x71, 0x6D, 0xB7, 0xD2, 0x45, 0x41, 0x27,
 0x01, 0x65, 0xC6, 0x45, 0xAE, 0xF2, 0xBC, 0x24, 0x30, 0xB8, 0x95, 0xCE, 0x2F,
 0x4E, 0xD6, 0xF6, 0x1C, 0x88, 0xBC, 0x7C, 0x9F, 0xFB, 0xA8, 0x67, 0x7F, 0xFE,
 0x5C, 0x9C, 0x51, 0x75, 0xF7, 0x8A, 0xCA, 0x07, 0xE7, 0x35, 0x2F, 0x8F, 0xE1,
 0xBD, 0x7B, 0xC0, 0x2F, 0x7C, 0xAB, 0x64, 0xA8, 0x17, 0xFC, 0xCA, 0x5D, 0x7B,
 0xBA, 0xE0, 0x21, 0xE5, 0x72, 0x2E, 0x6F, 0x2E, 0x86, 0xD8, 0x95, 0x73, 0xDA,
 0xAC, 0x1B, 0x53, 0xB9, 0x5F, 0x3F, 0xD7, 0x19, 0x0D, 0x25, 0x4F, 0xE1, 0x63,
 0x63, 0x51, 0x8B, 0x0B, 0x64, 0x3F, 0xAD, 0x43, 0xB8, 0xA5, 0x1C, 0x5C, 0x34,
 0xB3, 0xAE, 0x00, 0xA0, 0x63, 0xC5, 0xF6, 0x7F, 0x0B, 0x59, 0x68, 0x78, 0x73,
 0xA6, 0x8C, 0x18, 0xA9, 0x02, 0x6D, 0xAF, 0xC3, 0x19, 0x01, 0x2E, 0xB8, 0x10,
 0xE3, 0xC6, 0xCC, 0x40, 0xB4, 0x69, 0xA3, 0x46, 0x33, 0x69, 0x87, 0x6E, 0xC4,
 0xBB, 0x17, 0xA6, 0xF3, 0xE8, 0xDD, 0xAD, 0x73, 0xBC, 0x7B, 0x2F, 0x21, 0xB5,
 0xFD, 0x66, 0x51, 0x0C, 0xBD, 0x54, 0xB3, 0xE1, 0x6D, 0x5F, 0x1C, 0xBC, 0x23,
 0x73, 0xD1, 0x09, 0x03, 0x89, 0x14, 0xD2, 0x10, 0xB9, 0x64, 0xC3, 0x2A, 0xD0,
 0xA1, 0x96, 0x4A, 0xBC, 0xE1, 0xD4, 0x1A, 0x5B, 0xC7, 0xA0, 0xC0, 0xC1, 0x63,
 0x78, 0x0F, 0x44, 0x37, 0x30, 0x32, 0x96, 0x80, 0x32, 0x23, 0x95, 0xA1, 0x77,
 0xBA, 0x13, 0xD2, 0x97, 0x73, 0xE2, 0x5D, 0x25, 0xC9, 0x6A, 0x0D, 0xC3, 0x39,
 0x60, 0xA4, 0xB4, 0xB0, 0x69, 0x42, 0x42, 0x09, 0xE9, 0xD8, 0x08, 0xBC, 0x33,
 0x20, 0xB3, 0x58, 0x22, 0xA7, 0xAA, 0xEB, 0xC4, 0xE1, 0xE6, 0x61, 0x83, 0xC5,
 0xD2, 0x96, 0xDF, 0xD9, 0xD0, 0x4F, 0xAD, 0xD7, 0x02, 0x03, 0x01, 0x00, 0x01,
 0x02, 0x82, 0x01, 0x01, 0x00, 0x9A, 0xD0, 0x34, 0x0F, 0x52, 0x62, 0x05, 0x50,
 0x01, 0xEF, 0x9F, 0xED, 0x64, 0x6E, 0xC2, 0xC4, 0xDA, 0x1A, 0xF2, 0x84, 0xD7,
 0x92, 0x10, 0x48, 0x92, 0xC4, 0xE9, 0x6A, 0xEB, 0x8B, 0x75, 0x6C, 0xC6, 0x79,
 0x38, 0xF2, 0xC9, 0x72, 0x4A, 0x86, 0x64, 0x54, 0x95, 0x77, 0xCB, 0xC3, 0x9A,
 0x9D, 0xB7, 0xD4, 0x1D, 0xA4, 0x00, 0xC8, 0x9E, 0x4E, 0xE4, 0xDD, 0xC7, 0xBA,
 0x67, 0x16, 0xC1, 0x74, 0xBC, 0xA9, 0xD6, 0x94, 0x8F, 0x2B, 0x30, 0x1A, 0xFB,
 0xED, 0xDF, 0x21, 0x05, 0x23, 0xD9, 0x4A, 0x39, 0xBD, 0x98, 0x6B, 0x65, 0x9A,
 0xB8, 0xDC, 0xC4, 0x7D, 0xEE, 0xA6, 0x43, 0x15, 0x2E, 0x3D, 0xBE, 0x1D, 0x22,
 0x60, 0x2A, 0x73, 0x30, 0xD5, 0x3E, 0xD8, 0xA2, 0xAC, 0x86, 0x43, 0x2E, 0xC4,
 0xF5, 0x64, 0x5E, 0x3F, 0x89, 0x75, 0x0F, 0x11, 0xD8, 0x51, 0x25, 0x4E, 0x9F,
 0xD8, 0xAA, 0xA3, 0xCE, 0x60, 0xB3, 0xE2, 0x8A, 0xD9, 0x7E, 0x1B, 0xF0, 0x64,
 0xCA, 0x9A, 0x5B, 0x05, 0x0B, 0x5B, 0xAA, 0xCB, 0xE5, 0xE3, 0x3F, 0x6E, 0x32,
 0x22, 0x05, 0xF3, 0xD0, 0xFA, 0xEF, 0x74, 0x52, 0x81, 0xE2, 0x5F, 0x74, 0xD3,
 0xBD, 0xFF, 0x31, 0x83, 0x45, 0x75, 0xFA, 0x63, 0x7A, 0x97, 0x2E, 0xD6, 0xB6,
 0x19, 0xC6, 0x92, 0x26, 0xE4, 0x28, 0x06, 0x50, 0x50, 0x0E, 0x78, 0x2E, 0xA9,
 0x78, 0x0D, 0x14, 0x97, 0xB4, 0x12, 0xD8, 0x31, 0x40, 0xAB, 0xA1, 0x01, 0x41,
 0xC2, 0x30, 0xF8, 0x07, 0x5F, 0x16, 0xE4, 0x61, 0x77, 0xD2, 0x60, 0xF2, 0x9F,
 0x8D, 0xE8, 0xF4, 0xBA, 0xEB, 0x63, 0xDE, 0x2A, 0x97, 0x81, 0xEF, 0x4C, 0x6C,
 0xE6, 0x55, 0x34, 0x51, 0x2B, 0x28, 0x34, 0xF4, 0x53, 0x1C, 0xC4, 0x58, 0x0A,
 0x3F, 0xBB, 0xAF, 0xB5, 0xF7, 0x4A, 0x85, 0x43, 0x2D, 0x3C, 0xF1, 0x58, 0x58,
 0x81, 0x02, 0x81, 0x81, 0x00, 0xF2, 0x2C, 0x54, 0x76, 0x39, 0x23, 0x63, 0xC9,
 0x10, 0x32, 0xB7, 0x93, 0xAD, 0xAF, 0xBE, 0x19, 0x75, 0x96, 0x81, 0x64, 0xE6,
 0xB5, 0xB8, 0x89, 0x42, 0x41, 0xD1, 0x6D, 0xD0, 0x1C, 0x1B, 0xF8, 0x1B, 0xAC,
 0x69, 0xCB, 0x36, 0x3C, 0x64, 0x7D, 0xDC, 0xF4, 0x19, 0xB8, 0xC3, 0x60, 0xB1,
 0x57, 0x48, 0x5F, 0x52, 0x4F, 0x59, 0x3A, 0x55, 0x7F, 0x32, 0xC0, 0x19, 0x43,
 0x50, 0x3F, 0xAE, 0xCE, 0x6F, 0x17, 0xF3, 0x0E, 0x9F, 0x40, 0xCA, 0x4E, 0xAD,
 0x15, 0x3B, 0xC9, 0x79, 0xE9, 0xC0, 0x59, 0x38, 0x73, 0x70, 0x9C, 0x0A, 0x7C,
 0xC9, 0x3A, 0x48, 0x32, 0xA7, 0xD8, 0x49, 0x75, 0x0A, 0x85, 0xC2, 0xC2, 0xFD,
 0x15, 0x73, 0xDA, 0x99, 0x09, 0x2A, 0x69, 0x9A, 0x9F, 0x0A, 0x71, 0xBF, 0xB0,
 0x04, 0xA6, 0x8C, 0x7A, 0x5A, 0x6F, 0x48, 0x5A, 0x54, 0x3B, 0xC6, 0xB1, 0x53,
 0x17, 0xDF, 0xE7, 0x02, 0x81, 0x81, 0x00, 0xCB, 0x93, 0xDE, 0x77, 0x15, 0x5D,
 0xB7, 0x5C, 0x5C, 0x7C, 0xD8, 0x90, 0xA9, 0x98, 0x2D, 0xD6, 0x69, 0x0E, 0x63,
 0xB3, 0xA3, 0xDC, 0xA6, 0xCC, 0x8B, 0x6A, 0xA4, 0xA2, 0x12, 0x8C, 0x8E, 0x7B,
 0x48, 0x2C, 0xB2, 0x4B, 0x37, 0xDC, 0x06, 0x18, 0x7D, 0xEA, 0xFE, 0x76, 0xA1,
 0xD4, 0xA1, 0xE9, 0x3F, 0x0D, 0xCD, 0x1B, 0x5F, 0xAF, 0x5F, 0x9E, 0x96, 0x5B,
 0x5B, 0x0F, 0xA1, 0x7C, 0xAF, 0xB3, 0x9B, 0x90, 0xDB, 0x57, 0x73, 0x3A, 0xED,
 0xB0, 0x23, 0x44, 0xAE, 0x41, 0x4F, 0x1F, 0x07, 0x42, 0x13, 0x23, 0x4C, 0xCB,
 0xFA, 0xF4, 0x14, 0xA4, 0xD5, 0xF7, 0x9E, 0x36, 0x7C, 0x5B, 0x9F, 0xA8, 0x3C,
 0xC1, 0x85, 0x5F, 0x74, 0xD2, 0x39, 0x2D, 0xFF, 0xD0, 0x84, 0xDF, 0xFB, 0xB3,
 0x20, 0x7A, 0x2E, 0x9B, 0x17, 0xAE, 0xE6, 0xBA, 0x0B, 0xAE, 0x5F, 0x53, 0xA4,
 0x52, 0xED, 0x1B, 0xC4, 0x91, 0x02, 0x81, 0x81, 0x00, 0xEC, 0x98, 0xDA, 0xBB,
 0xD5, 0xFE, 0xF9, 0x52, 0x4A, 0x7D, 0x02, 0x55, 0x49, 0x6F, 0x55, 0x6E, 0x52,
 0x2F, 0x84, 0xA3, 0x2B, 0xB3, 0x86, 0x62, 0xB3, 0x54, 0xD2, 0x63, 0x52, 0xDA,
 0xE3, 0x88, 0x76, 0xA0, 0xEF, 0x8B, 0x15, 0xA5, 0xD3, 0x18, 0x14, 0x72, 0x77,
 0x5E, 0xC7, 0xA3, 0x04, 0x1F, 0x9E, 0x19, 0x62, 0xB5, 0x1B, 0x1B, 0x9E, 0xC3,
 0xF2, 0xB5, 0x32, 0xF9, 0x4C, 0xC1, 0xAA, 0xEB, 0x0C, 0x26, 0x7D, 0xD4, 0x5F,
 0x4A, 0x51, 0x5C, 0xA4, 0x45, 0x06, 0x70, 0x44, 0xA7, 0x56, 0xC0, 0xD4, 0x22,
 0x14, 0x76, 0x9E, 0xD8, 0x63, 0x50, 0x89, 0x90, 0xD3, 0xE2, 0xBF, 0x81, 0x95,
 0x92, 0x31, 0x41, 0x87, 0x39, 0x1A, 0x43, 0x0B, 0x18, 0xA5, 0x53, 0x1F, 0x39,
 0x1A, 0x5F, 0x1F, 0x43, 0xBC, 0x87, 0x6A, 0xDF, 0x6E, 0xD3, 0x22, 0x00, 0xFE,
 0x22, 0x98, 0x70, 0x4E, 0x1A, 0x19, 0x29, 0x02, 0x81, 0x81, 0x00, 0x8A, 0x41,
 0x56, 0x28, 0x51, 0x9E, 0x5F, 0xD4, 0x9E, 0x0B, 0x3B, 0x98, 0xA3, 0x54, 0xF2,
 0x6C, 0x56, 0xD4, 0xAA, 0xE9, 0x69, 0x33, 0x85, 0x24, 0x0C, 0xDA, 0xD4, 0x0C,
 0x2D, 0xC4, 0xBF, 0x4F, 0x02, 0x69, 0x38, 0x7C, 0xD4, 0xE6, 0xDC, 0x4C, 0xED,
 0xD7, 0x16, 0x11, 0xC3, 0x3E, 0x00, 0xE7, 0xC3, 0x26, 0xC0, 0x51, 0x02, 0xDE,
 0xBB, 0x75, 0x9C, 0x6F, 0x56, 0x9C, 0x7A, 0xF3, 0x8E, 0xEF, 0xCF, 0x8A, 0xC5,
 0x2B, 0xD2, 0xDA, 0x06, 0x6A, 0x44, 0xC9, 0x73, 0xFE, 0x6E, 0x99, 0x87, 0xF8,
 0x5B, 0xBE, 0xF1, 0x7C, 0xE6, 0x65, 0xB5, 0x4F, 0x6C, 0xF0, 0xC9, 0xC5, 0xFF,
 0x16, 0xCA, 0x8B, 0x1B, 0x17, 0xE2, 0x58, 0x3D, 0xA2, 0x37, 0xAB, 0x01, 0xBC,
 0xBF, 0x40, 0xCE, 0x53, 0x8C, 0x8E, 0xED, 0xEF, 0xEE, 0x59, 0x9D, 0xE0, 0x63,
 0xE6, 0x7C, 0x5E, 0xF5, 0x8E, 0x4B, 0xF1, 0x3B, 0xC1, 0x02, 0x81, 0x80, 0x4D,
 0x45, 0xF9, 0x40, 0x8C, 0xC5, 0x5B, 0xF4, 0x2A, 0x1A, 0x8A, 0xB4, 0xF2, 0x1C,
 0xAC, 0x6B, 0xE9, 0x0C, 0x56, 0x36, 0xB7, 0x4E, 0x72, 0x96, 0xD5, 0xE5, 0x8A,
 0xD2, 0xE2, 0xFF, 0xF1, 0xF1, 0x18, 0x13, 0x3D, 0x86, 0x09, 0xB8, 0xD8, 0x76,
 0xA7, 0xC9, 0x1C, 0x71, 0x52, 0x94, 0x30, 0x43, 0xE0, 0xF1, 0x78, 0x74, 0xFD,
 0x61, 0x1B, 0x4C, 0x09, 0xCC, 0xE6, 0x68, 0x2A, 0x71, 0xAD, 0x1C, 0xDF, 0x43,
 0xBC, 0x56, 0xDB, 0xA5, 0xA4, 0xBE, 0x35, 0x70, 0xA4, 0x5E, 0xCF, 0x4F, 0xFC,
 0x00, 0x55, 0x99, 0x3A, 0x3D, 0x23, 0xCF, 0x67, 0x5A, 0xF5, 0x22, 0xF8, 0xB5,
 0x29, 0xD0, 0x44, 0x11, 0xEB, 0x35, 0x2E, 0x46, 0xBE, 0xFD, 0x8E, 0x18, 0xB2,
 0x5F, 0xA8, 0xBF, 0x19, 0x32, 0xA1, 0xF5, 0xDC, 0x03, 0xE6, 0x7C, 0x9A, 0x1F,
 0x0C, 0x7C, 0xA9, 0xB0, 0x0E, 0x21, 0x37, 0x3B, 0xF1, 0xB0};

const uint8_t rsa_256_key_data[] = {
 0x30, 0x82, 0x01, 0x0A,
 0x02, 0x82, 0x01, 0x01, 0x00, 0xDB, 0x1C, 0x7F, 0x2E, 0x0B, 0xCD, 0xBF, 0xCE, 0xD1,
 0x75, 0x10, 0xA0, 0xA2, 0xB8, 0xCE, 0x7D, 0xAA, 0xE2, 0x05, 0xE0, 0x7A, 0xD8, 0x44,
 0x63, 0x8F, 0xB5, 0xBD, 0xC0, 0xB0, 0x19, 0xB9, 0x37, 0xB8, 0x19, 0x4A, 0x0E, 0xF1,
 0x5D, 0x74, 0x80, 0x67, 0x46, 0x87, 0x06, 0xDE, 0x5B, 0x7F, 0x06, 0x03, 0xBD, 0xC1,
 0x8D, 0x5E, 0x07, 0x15, 0xD4, 0x5B, 0xF4, 0xDC, 0xE5, 0xCF, 0x3D, 0xF9, 0xC1, 0x11,
 0x2C, 0xAE, 0x6A, 0xB9, 0x8A, 0xBD, 0x1D, 0x67, 0x66, 0x17, 0xEA, 0x4E, 0xBD, 0xDB,
 0x15, 0x9A, 0x82, 0x87, 0xE4, 0xF0, 0x78, 0xC3, 0xA3, 0x85, 0x87, 0xB0, 0xFD, 0x9F,
 0xA9, 0x99, 0x5F, 0xE3, 0x33, 0xEC, 0xCC, 0xEA, 0x0B, 0xB5, 0x61, 0x5E, 0xF1, 0x49,
 0x7E, 0x3F, 0xA3, 0x2D, 0xEA, 0x01, 0x0C, 0xCC, 0x42, 0x9A, 0x76, 0x9B, 0xC4, 0xD0,
 0x37, 0xD3, 0xB1, 0x17, 0x01, 0x61, 0x01, 0x16, 0x59, 0x7E, 0x1C, 0x17, 0xC3, 0x53,
 0xFD, 0xD1, 0x72, 0xCB, 0x4C, 0x60, 0x15, 0xDA, 0x7D, 0xE2, 0xEA, 0xAD, 0x50, 0xEF,
 0x8E, 0xE2, 0x8B, 0xD4, 0x6A, 0x77, 0x55, 0xD6, 0x70, 0xD9, 0x6B, 0xBB, 0xF1, 0xEE,
 0x39, 0x04, 0x38, 0xA3, 0xBD, 0xE2, 0xD1, 0xE0, 0x66, 0x6B, 0xE2, 0x9C, 0x47, 0x99,
 0xE9, 0x28, 0xE6, 0xB6, 0xFC, 0x2E, 0xCA, 0x67, 0x43, 0x84, 0xE8, 0xD5, 0x83, 0xD6,
 0x9D, 0x98, 0x6B, 0x01, 0x3E, 0x81, 0xDC, 0x3C, 0x7A, 0xCA, 0xF9, 0xF3, 0x9C, 0xF7,
 0xD6, 0x28, 0x1B, 0x27, 0x78, 0x7C, 0xC3, 0xD0, 0xD5, 0x63, 0xA7, 0x81, 0x34, 0x89,
 0xAD, 0x25, 0x6A, 0xBD, 0xF2, 0xEA, 0xED, 0xFA, 0x57, 0xFC, 0xE5, 0x34, 0xC6, 0xC1,
 0x0F, 0x71, 0x2D, 0xD2, 0x08, 0x10, 0x1B, 0xAD, 0x44, 0x41, 0xE0, 0xFE, 0x79, 0xA0,
 0x63, 0x93, 0x8A, 0xB1, 0x5D, 0xE9, 0xB0, 0xEE, 0x6F, 0x02, 0x03, 0x01, 0x00, 0x01};

const uint8_t ec_key_data[] = {
 0x04, 0xde, 0xa5, 0xe4, 0x5d, 0x0e, 0xa3, 0x7f, 0xc5, 0x66, 0x23, 0x2a, 0x50, 0x8f,
 0x4a, 0xd2, 0x0e, 0xa1, 0x3d, 0x47, 0xe4, 0xbf, 0x5f, 0xa4, 0xd5, 0x4a, 0x57, 0xa0,
 0xba, 0x01, 0x20, 0x42, 0x08, 0x70, 0x97, 0x49, 0x6e, 0xfc, 0x58, 0x3f, 0xed, 0x8b,
 0x24, 0xa5, 0xb9, 0xbe, 0x9a, 0x51, 0xde, 0x06, 0x3f, 0x5a, 0x00, 0xa8, 0xb6, 0x98,
 0xa1, 0x6f, 0xd7, 0xf2, 0x9b, 0x54, 0x85, 0xf3, 0x20};

const uint8_t ec_key_pair[] = {
 0x68, 0x49, 0xf9, 0x7d, 0x10, 0x66, 0xf6, 0x99, 0x77, 0x59, 0x63, 0x7c, 0x7e, 0x38,
 0x99, 0x46, 0x4c, 0xee, 0x3e, 0xc7, 0xac, 0x97, 0x06, 0x53, 0xa0, 0xbe, 0x07, 0x42};

/* Deterministic EC */
const uint8_t ec_keypair_deterministic[] = {
 0xab, 0x45, 0x43, 0x57, 0x12, 0x64, 0x9c, 0xb3, 0x0b, 0xbd, 0xda, 0xc4, 0x91, 0x97, 0xee, 0xbf,
 0x27, 0x40, 0xff, 0xc7, 0xf8, 0x74, 0xd9, 0x24, 0x4c, 0x34, 0x60, 0xf5, 0x4f, 0x32, 0x2d, 0x3a};

const uint8_t md2_hash[] = {
 0x8c, 0x9c, 0x17, 0x66, 0x5d, 0x25, 0xb3, 0x5f, 0xc4, 0x13, 0xc4, 0x18, 0x05, 0xc6, 0x79, 0xcf};

const uint8_t md4_hash[] = {
 0x18, 0xc3, 0x3f, 0x97, 0x29, 0x7e, 0xfe, 0x5f, 0x8a, 0x73, 0x22, 0x58, 0x28, 0x9f, 0xda, 0x25};

const uint8_t md5_hash[] = {
 0xab, 0xae, 0x57, 0xcb, 0x56, 0x2e, 0xcf, 0x29, 0x5b, 0x4a, 0x37, 0xa7, 0x6e, 0xfe, 0x61, 0xfb};

const uint8_t ripemd_160_hash[] = {
 0x50, 0x89, 0x26, 0x5e, 0xe5, 0xd9, 0xaf, 0x75, 0xd1, 0x2d,
 0xbf, 0x7e, 0xa2, 0xf2, 0x7d, 0xbd, 0xee, 0x43, 0x5b, 0x37};

const uint8_t sha_1_hash[] = {
 0x90, 0x34, 0xaa, 0xf4, 0x51, 0x43, 0x99, 0x6a, 0x2b, 0x14,
 0x46, 0x5c, 0x35, 0x2a, 0xb0, 0xc6, 0xfa, 0x26, 0xb2, 0x21};

const uint8_t sha_224_hash[] = {
 0xb1, 0xe4, 0x6b, 0xb9, 0xef, 0xe4, 0x5a, 0xf5, 0x54, 0x36, 0x34, 0x49, 0xc6, 0x94,
 0x5a, 0x0d, 0x61, 0x69, 0xfc, 0x3a, 0x5a, 0x39, 0x6a, 0x56, 0xcb, 0x97, 0xcb, 0x57};

const uint8_t sha_256_hash[] = {
 0x68, 0x32, 0x57, 0x20, 0xaa, 0xbd, 0x7c, 0x82, 0xf3, 0x0f, 0x55, 0x4b, 0x31, 0x3d, 0x05, 0x70,
 0xc9, 0x5a, 0xcc, 0xbb, 0x7d, 0xc4, 0xb5, 0xaa, 0xe1, 0x12, 0x04, 0xc0, 0x8f, 0xfe, 0x73, 0x2b};

const uint8_t sha_256_incorrect_hash[] = {
 0x68, 0x32, 0x57, 0x20, 0xab, 0xbd, 0x7c, 0x82, 0xf3, 0x0f, 0x55, 0x4b, 0x31, 0x3d, 0x05, 0x70,
 0xc9, 0x5a, 0xcc, 0xbb, 0x7d, 0xc4, 0xb5, 0xaa, 0xe1, 0x12, 0x04, 0xc0, 0x8f, 0xfe, 0x73, 0x78};

const uint8_t sha_384_hash[] = {
 0x43, 0x72, 0xe3, 0x8a, 0x92, 0xa2, 0x8b, 0x5d, 0x2c, 0x39, 0x1e, 0x62, 0x45, 0x2a, 0x86, 0xd5,
 0x0e, 0x02, 0x67, 0x22, 0x8b, 0xe1, 0x76, 0xc7, 0x7d, 0x24, 0x02, 0xef, 0xfe, 0x9f, 0xa5, 0x0d,
 0xe4, 0x07, 0xbb, 0xb8, 0x51, 0xb3, 0x7d, 0x59, 0x04, 0xab, 0xa2, 0xde, 0xde, 0x74, 0xda, 0x2a};

const uint8_t sha_512_hash[] = {
 0x29, 0x6e, 0x22, 0x67, 0xd7, 0x4c, 0x27, 0x8d, 0xaa, 0xaa, 0x94, 0x0d, 0x17, 0xb0, 0xcf, 0xb7,
 0x4a, 0x50, 0x83, 0xf8, 0xe0, 0x69, 0x72, 0x6d, 0x8c, 0x84, 0x1c, 0xbe, 0x59, 0x6e, 0x04, 0x31,
 0xcb, 0x77, 0x41, 0xa5, 0xb5, 0x0f, 0x71, 0x66, 0x6c, 0xfd, 0x54, 0xba, 0xcb, 0x7b, 0x00, 0xae,
 0xa8, 0x91, 0x49, 0x9c, 0xf4, 0xef, 0x6a, 0x03, 0xc8, 0xa8, 0x3f, 0xe3, 0x7c, 0x3f, 0x7b, 0xaf};

const uint8_t ecdh_secp_256_r1_prv_key[ECDH_SECP_256_R1_PRV_KEY_LEN] = {
 0xc8, 0x8f, 0x01, 0xf5, 0x10, 0xd9, 0xac, 0x3f, 0x70, 0xa2, 0x92, 0xda, 0xa2, 0x31, 0x6d, 0xe5,
 0x44, 0xe9, 0xaa, 0xb8, 0xaf, 0xe8, 0x40, 0x49, 0xc6, 0x2a, 0x9c, 0x57, 0x86, 0x2d, 0x14, 0x33};

const uint8_t ecdh_secp_256_r1_pub_key[ECDH_SECP_256_R1_PUB_KEY_LEN] = {
 0x04, 0xd1, 0x2d, 0xfb, 0x52, 0x89, 0xc8, 0xd4, 0xf8, 0x12, 0x08, 0xb7, 0x02,
 0x70, 0x39, 0x8c, 0x34, 0x22, 0x96, 0x97, 0x0a, 0x0b, 0xcc, 0xb7, 0x4c, 0x73,
 0x6f, 0xc7, 0x55, 0x44, 0x94, 0xbf, 0x63, 0x56, 0xfb, 0xf3, 0xca, 0x36, 0x6c,
 0xc2, 0x3e, 0x81, 0x57, 0x85, 0x4c, 0x13, 0xc5, 0x8d, 0x6a, 0xac, 0x23, 0xf0,
 0x46, 0xad, 0xa3, 0x0f, 0x83, 0x53, 0xe7, 0x4f, 0x33, 0x03, 0x98, 0x72, 0xab};

const uint8_t ecdh_secp_384_r1_prv_key[ECDH_SECP_384_R1_PRV_KEY_LEN] = {
 0x09, 0x9f, 0x3c, 0x70, 0x34, 0xd4, 0xa2, 0xc6, 0x99, 0x88, 0x4d, 0x73, 0xa3, 0x75, 0xa6, 0x7f,
 0x76, 0x24, 0xef, 0x7c, 0x6b, 0x3c, 0x0f, 0x16, 0x06, 0x47, 0xb6, 0x74, 0x14, 0xdc, 0xe6, 0x55,
 0xe3, 0x5b, 0x53, 0x80, 0x41, 0xe6, 0x49, 0xee, 0x3f, 0xae, 0xf8, 0x96, 0x78, 0x3a, 0xb1, 0x94};

const uint8_t ecdh_secp_384_r1_pub_key[ECDH_SECP_384_R1_PUB_KEY_LEN] = {
 0x04, 0xe5, 0x58, 0xdb, 0xef, 0x53, 0xee, 0xcd, 0xe3, 0xd3, 0xfc, 0xcf, 0xc1, 0xae, 0xa0, 0x8a,
 0x89, 0xa9, 0x87, 0x47, 0x5d, 0x12, 0xfd, 0x95, 0x0d, 0x83, 0xcf, 0xa4, 0x17, 0x32, 0xbc, 0x50,
 0x9d, 0x0d, 0x1a, 0xc4, 0x3a, 0x03, 0x36, 0xde, 0xf9, 0x6f, 0xda, 0x41, 0xd0, 0x77, 0x4a, 0x35,
 0x71, 0xdc, 0xfb, 0xec, 0x7a, 0xac, 0xf3, 0x19, 0x64, 0x72, 0x16, 0x9e, 0x83, 0x84, 0x30, 0x36,
 0x7f, 0x66, 0xee, 0xbe, 0x3c, 0x6e, 0x70, 0xc4, 0x16, 0xdd, 0x5f, 0x0c, 0x68, 0x75, 0x9d, 0xd1,
 0xff, 0xf8, 0x3f, 0xa4, 0x01, 0x42, 0x20, 0x9d, 0xff, 0x5e, 0xaa, 0xd9, 0x6d, 0xb9, 0xe6, 0x38,
 0x6c};

const uint8_t nonce[] = {
 0x00, 0x41, 0x2B, 0x4E, 0xA9, 0xCD, 0xBE, 0x3C, 0x96, 0x96, 0x76, 0x6C, 0xFA};

const uint8_t additional_data[] = {
 0x40, 0xa2, 0x7c, 0x1d, 0x1e, 0x23, 0xea, 0x3d, 0xbe, 0x80, 0x56, 0xb2, 0x77, 0x48, 0x61, 0xa4,
 0xa2, 0x01, 0xcc, 0xe4, 0x9f, 0x19, 0x99, 0x7d, 0x19, 0x20, 0x6d, 0x8c, 0x8a, 0x34, 0x39, 0x51};

const uint8_t plaintext[] = {
 0x45, 0x35, 0xd1, 0x2b, 0x43, 0x77, 0x92, 0x8a,
 0x7c, 0x0a, 0x61, 0xc9, 0xf8, 0x25, 0xa4, 0x86,
 0x71, 0xea, 0x05, 0x91, 0x07, 0x48, 0xc8, 0xef};

const uint8_t aead_ciphertext_1[AEAD_CIPHERTEXT_LEN_1] = {
 0xFA, 0xBF, 0x93, 0x49, 0xE7, 0x24, 0xA5, 0x33, 0x4B, 0xC0, 0xBF, 0x01, 0x17, 0xAF, 0x04, 0x3D,
 0xC0, 0xC9, 0xDE, 0x51, 0xD9, 0xC7, 0xA5, 0x77, 0xDD, 0x30, 0x36, 0x55, 0xCD, 0xE6, 0xB3, 0x16,
 0xB7, 0x0A, 0xC0, 0x8C, 0xBA, 0x78, 0xD6};

const uint8_t aead_ciphertext_2[AEAD_CIPHERTEXT_LEN_2] = {
 0xFA, 0xBF, 0x93, 0x49, 0xE7, 0x24, 0xA5, 0x33, 0x4B, 0xC0, 0xBF, 0x01, 0x17, 0xAF, 0x04, 0x3D,
 0xC0, 0xC9, 0xDE, 0x51, 0xD9, 0xC7, 0xA5, 0x96, 0x7E, 0x15, 0x73, 0xF2, 0x0B, 0xD2, 0x8A, 0x59,
 0x4A, 0x9C, 0x95, 0x82, 0x60, 0x56, 0x24, 0x18};

const uint8_t aead_invalid_ciphertext_2[AEAD_CIPHERTEXT_LEN_2] = {
 0x93, 0x40, 0xD4, 0x54, 0xAF, 0xCB, 0xD1, 0xBF, 0x05, 0xCA, 0x56, 0x94, 0xAD, 0x47, 0xC1, 0x0D,
 0x13, 0x05, 0x96, 0xF4, 0x2F, 0x69, 0xAC, 0xD8, 0x20, 0x7C, 0x67, 0x53, 0x73, 0x88, 0x6B, 0x2B,
 0x3C, 0x0F, 0x8A, 0x6C, 0x01, 0x2E, 0x51, 0x77};

const uint8_t aead_ciphertext_3[AEAD_CIPHERTEXT_LEN_3] = {
 0xFA, 0xBF, 0x93, 0x49, 0xE7, 0x24, 0xA5, 0x33, 0x4B, 0xC0, 0xBF, 0x01, 0x17, 0xAF, 0x04, 0x3D,
 0xC0, 0xC9, 0xDE, 0x51, 0xD9, 0xC7, 0xA5, 0x96, 0xE5, 0x2B, 0x6F, 0x9B};

const uint8_t aead_ciphertext_4[AEAD_CIPHERTEXT_LEN_4] = {
 0xFA, 0xBF, 0x93, 0x49, 0xE7, 0x24, 0xA5, 0x33, 0x4B, 0xC0, 0xBF, 0x01, 0x17, 0xAF, 0x04, 0x3D,
 0xC0, 0xC9, 0xDE, 0x51, 0xD9, 0xC7, 0xA5, 0x96, 0xB1, 0x36, 0x17, 0xD4, 0xF7, 0x0A, 0xFB, 0xA5,
 0xED, 0x83, 0x08, 0xC1, 0xB2, 0x1D, 0x19, 0x27};

const uint8_t aead_ciphertext_5[AEAD_CIPHERTEXT_LEN_5] = {
 0x65, 0x46, 0x6B, 0xB0, 0xEA, 0x01, 0xE4, 0x8F, 0x94, 0xA2, 0x3C, 0xA6, 0x5C, 0x15, 0x56, 0x17};

const uint8_t aead_ciphertext_6[AEAD_CIPHERTEXT_LEN_6] = {
 0x1D, 0xEC, 0xC1, 0xED, 0x4B, 0x1A, 0x0F, 0x94, 0x64, 0x3C, 0xBB, 0x22, 0x89, 0xB5, 0xE3, 0x46,
 0x21, 0x11, 0x43, 0x20, 0x2D, 0xEA, 0x2B, 0x66, 0xB7, 0x27, 0x77, 0x27, 0x67, 0xDF, 0x44, 0xEC,
 0xD8, 0xDA, 0xA8, 0x7C, 0xB4, 0x9B, 0xB3, 0x2B};

const unsigned char iv[] = {
 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

const unsigned char ciphertext_1[] = {
 0xDB, 0x03, 0x77, 0xF5, 0x59, 0x9F, 0x22, 0xB8, 0x5A, 0xFC, 0x13, 0xC3, 0x73, 0x01, 0xD7, 0x8D,
 0xC7, 0x2D, 0x52, 0x0C, 0x24, 0x8C, 0xDD, 0x61, 0x45, 0xDD, 0x05, 0xF0, 0xD3, 0xEE, 0x0D, 0xCD};

const unsigned char ciphertext_2[] = {
 0xE1, 0x98, 0x2E, 0x59, 0xAC, 0x61, 0x75, 0x7B, 0x83, 0xBE, 0xB4, 0x03, 0xF0, 0xDA, 0x7F, 0xCC};

const unsigned char ciphertext_3[] = {
 0x85, 0x6C, 0xF7, 0xD8, 0x1F, 0xA6, 0xA6, 0xEE};

const unsigned char ciphertext_4[] = {
 0x7A, 0x88, 0x56, 0xF2, 0xCA, 0xF5, 0xF8, 0x95};

const unsigned char ciphertext_5[] = {
 0xE3, 0xB7, 0xD2, 0x70, 0xDF, 0x41, 0x1A, 0x86};

const unsigned char ciphertext_6[] = {
 0xC3, 0x32, 0x1D, 0x6B, 0x81, 0x44, 0x6D, 0xF4, 0xA6, 0x5D, 0xBF, 0x6A, 0xB5, 0x0A, 0x55, 0xBC};

const uint8_t salt[] = {
 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

const uint8_t ciphertext_7[] = {
 0x0B, 0x1B, 0x3F, 0xC1, 0xA3, 0x94, 0x9A, 0x5E, 0x1F, 0xE2, 0x28, 0x06, 0x9E, 0xDE, 0x60, 0xB5,
 0x86, 0x12, 0x52, 0x5C, 0x78, 0xD2, 0xBE, 0x62, 0xDD, 0x82, 0x33, 0xD7, 0x82, 0x56, 0xC6, 0x23,
 0xA0, 0x86, 0x3F, 0x90, 0x9A, 0xFE, 0xF3, 0xA1, 0x0A, 0xD2, 0x47, 0x73, 0xF8, 0x48, 0x3D, 0x53,
 0x96, 0x60, 0x37, 0x06, 0x8F, 0xF4, 0x79, 0xC6, 0xC3, 0x93, 0x85, 0xBF, 0x4B, 0x4E, 0xB3, 0x34,
 0x3C, 0x53, 0x8D, 0x95, 0xCC, 0x8B, 0x2F, 0xB9, 0x1D, 0x99, 0x7B, 0xF2, 0x29, 0xAB, 0x87, 0x82,
 0x17, 0xC6, 0x4B, 0xF3, 0xF3, 0x92, 0xC4, 0x76, 0xAA, 0x32, 0x80, 0x74, 0x63, 0x43, 0xBB, 0xC2,
 0xD7, 0xC0, 0x9C, 0x4C, 0x55, 0x71, 0x96, 0x26, 0x36, 0x61, 0xA5, 0x59, 0x52, 0x8F, 0xB2, 0xDA,
 0x18, 0xF1, 0xA4, 0x54, 0x68, 0x8A, 0x9D, 0x19, 0x32, 0xC5, 0xEA, 0xF4, 0x4C, 0xAB, 0x09, 0x5A};

const uint8_t ciphertext_8[] = {
 0x95, 0x7D, 0xB0, 0xF1, 0x8D, 0xDE, 0x31, 0x65, 0x59, 0x51, 0x4B, 0xFB, 0xE2, 0x4F, 0xCB, 0xB0,
 0xBB, 0x62, 0x60, 0xB4, 0x8C, 0x0B, 0x96, 0xE2, 0xD1, 0x37, 0xF2, 0x8B, 0x2E, 0x5A, 0x10, 0x37,
 0x6A, 0xBE, 0xBD, 0xA6, 0x84, 0xC2, 0x7F, 0xF9, 0x4A, 0xBB, 0xA9, 0xF9, 0x14, 0xEC, 0xE2, 0x33,
 0xED, 0x6B, 0x71, 0xF9, 0x66, 0x50, 0x4D, 0x90, 0xDC, 0x5F, 0x11, 0xBE, 0x43, 0x96, 0xBB, 0x82,
 0x3B, 0x68, 0xA2, 0x81, 0xA7, 0x1F, 0xF7, 0x46, 0xA6, 0x41, 0xC7, 0x99, 0x89, 0x83, 0xFD, 0xA5,
 0x35, 0x08, 0xDE, 0xF4, 0x37, 0x3D, 0xB1, 0x54, 0x86, 0xC5, 0x68, 0x82, 0xE0, 0x5C, 0x02, 0xFA,
 0x06, 0x91, 0xCF, 0xE8, 0x96, 0x39, 0x68, 0x7E, 0x62, 0x42, 0xAF, 0x5F, 0x21, 0x08, 0x2C, 0x39,
 0x2E, 0x83, 0x53, 0x0F, 0xAF, 0x97, 0xA8, 0x2B, 0xA0, 0xC3, 0x0E, 0xCA, 0xF8, 0x32, 0x04, 0x72};

const uint8_t ciphertext_9[] = {
 0x28, 0x73, 0xC2, 0x96, 0x05, 0x5B, 0x8F, 0x47, 0x0F, 0x99, 0xE7, 0x0A, 0x9A, 0xA0, 0x53, 0x79,
 0xDC, 0x4A, 0x8B, 0x97, 0x7C, 0x67, 0xC3, 0x9A, 0xE6, 0x61, 0xC5, 0x33, 0x55, 0x46, 0xCE, 0xB9,
 0xCF, 0xB7, 0x50, 0x7B, 0x9C, 0x21, 0x92, 0x31, 0x62, 0xB3, 0xA3, 0x56, 0x2D, 0x8B, 0x03, 0xCB,
 0x4D, 0x7A, 0x29, 0xD7, 0xEF, 0x62, 0xAE, 0x36, 0x89, 0x25, 0xF4, 0x31, 0x37, 0x64, 0xF0, 0xE3,
 0x2F, 0xA3, 0x5A, 0xF6, 0x10, 0xC6, 0x00, 0x96, 0x56, 0x75, 0xA8, 0x3C, 0xBA, 0x50, 0x4E, 0x58,
 0x14, 0x05, 0x47, 0x1C, 0xDB, 0xDB, 0x08, 0xD7, 0x48, 0xEC, 0x20, 0x81, 0x28, 0xFD, 0xE5, 0x8E,
 0xB8, 0xFC, 0x7D, 0x60, 0x27, 0xC8, 0x0A, 0x07, 0x65, 0xB2, 0x43, 0xA9, 0x08, 0x42, 0xCF, 0xDC,
 0x24, 0xEE, 0xB0, 0x2A, 0xB8, 0x4A, 0xDC, 0x52, 0x12, 0x78, 0x62, 0x77, 0x0E, 0x8B, 0x76, 0xFE};

const uint8_t signature_1[] = {
 0x8C, 0x4F, 0x46, 0x69, 0x84, 0x86, 0xE6, 0x51, 0x55, 0x7A, 0xC4, 0xEB, 0x2B, 0x77, 0x73, 0x5F,
 0x98, 0x54, 0x63, 0x94, 0x0D, 0xDA, 0x36, 0xC0, 0x3F, 0x1B, 0x96, 0x1D, 0xAA, 0x96, 0xDC, 0xE7,
 0x7C, 0x3A, 0x89, 0x90, 0x54, 0xDB, 0xA9, 0x1C, 0xFA, 0x02, 0xEF, 0xC7, 0x4B, 0x2A, 0x6F, 0x70,
 0xDB, 0xE3, 0x4A, 0x62, 0x21, 0x87, 0xD6, 0xF0, 0x0B, 0x88, 0x85, 0x5F, 0x64, 0xFF, 0xA3, 0x26,
 0x12, 0xA2, 0xB2, 0x11, 0x37, 0x0B, 0xD6, 0x5E, 0x1F, 0xF8, 0x2A, 0xE9, 0xA3, 0x19, 0x76, 0x89,
 0x53, 0xB6, 0x43, 0xCE, 0xD6, 0x4A, 0x83, 0x83, 0xE2, 0x94, 0xA1, 0x0E, 0x01, 0x45, 0x61, 0x44,
 0x8F, 0x94, 0x6A, 0xF3, 0x07, 0x63, 0x51, 0xEE, 0x05, 0xDB, 0x60, 0x14, 0x33, 0x4F, 0x3C, 0x9F,
 0xF5, 0x8C, 0xF5, 0xC3, 0x71, 0xDA, 0x9E, 0x1C, 0x3B, 0xDE, 0x03, 0x14, 0x45, 0x3C, 0xCB, 0xD0};

const uint8_t signature_2[] = {
 0x62, 0xF7, 0xCB, 0xDC, 0x60, 0x2B, 0x9F, 0xC2, 0x7D, 0x96, 0x4F, 0xD1, 0xB3, 0x59, 0x52, 0x68,
 0xCB, 0x51, 0xEF, 0xDF, 0x05, 0xDB, 0xB8, 0x3D, 0x7B, 0x87, 0x3C, 0x08, 0x2D, 0x4A, 0x93, 0xA1,
 0xD3, 0x7B, 0xA5, 0x4F, 0x42, 0x46, 0x85, 0x9E, 0x16, 0xBD, 0x9B, 0xF4, 0xAF, 0x97, 0x68, 0x62,
 0xF4, 0xEC, 0x7F, 0x72, 0x2B, 0x4F, 0x95, 0xEB, 0x7D, 0xA5, 0xF5, 0xE2, 0x0E, 0x7B, 0x43, 0x93,
 0x6F, 0xFD, 0x7E, 0xFC, 0x5D, 0x74, 0x88, 0x2B, 0xEA, 0x02, 0x64, 0x0D, 0x12, 0x76, 0x98, 0x43,
 0xE7, 0xD5, 0xFF, 0x94, 0x81, 0xAD, 0x58, 0x87, 0x13, 0xB4, 0xEE, 0x98, 0x41, 0xA4, 0xDE, 0x30,
 0x84, 0xC1, 0xB1, 0x04, 0x85, 0x5B, 0xCA, 0x86, 0xA4, 0xF3, 0x64, 0xE5, 0x20, 0x21, 0x1B, 0xE0,
 0x55, 0x40, 0x24, 0x74, 0x95, 0x2A, 0x34, 0xF1, 0xC2, 0x79, 0x6A, 0x6D, 0x3C, 0xAB, 0xBD, 0xA0,

/* Valid hash length is 128, add extra byte to keep valid memory for PSA
 * functionional test: "TEST: 242 Test psa_verify_hash - Wrong signature size"
 */
 0x00};

const uint8_t signature_2_invalid[] = {
 0x62, 0xF7, 0xCB, 0xDC, 0x60, 0x2B, 0x9F, 0xC2, 0x7D, 0x96, 0x4F, 0xD1, 0xB3, 0x59, 0x52, 0x68,
 0xCB, 0x51, 0xEF, 0xDF, 0x05, 0xDB, 0xB8, 0x3D, 0x7B, 0x87, 0x3C, 0x08, 0x2D, 0x4A, 0x93, 0xA1,
 0xD3, 0x7B, 0xA5, 0x4F, 0x42, 0x47, 0x85, 0x9E, 0x16, 0xBD, 0x9B, 0xF4, 0xAF, 0x97, 0x68, 0x62,
 0xF4, 0xEC, 0x7F, 0x72, 0x2B, 0x4F, 0x95, 0xEB, 0x7D, 0xA5, 0xF5, 0xE2, 0x0E, 0x7B, 0x43, 0x93,
 0x6F, 0xFD, 0x7E, 0xFC, 0x5D, 0x74, 0x88, 0x2B, 0xEA, 0x02, 0x64, 0x0D, 0x12, 0x76, 0x98, 0x43,
 0xE7, 0xD5, 0xFF, 0x94, 0x81, 0xAD, 0x59, 0x87, 0x13, 0xB4, 0xEE, 0x98, 0x41, 0xA4, 0xDE, 0x30,
 0x84, 0xC3, 0xB1, 0x04, 0x85, 0x5B, 0xCA, 0x86, 0xA4, 0xF3, 0x64, 0xE5, 0x20, 0x21, 0x1B, 0xE0,
 0x55, 0x40, 0x24, 0x74, 0x95, 0x2A, 0x34, 0xF1, 0xC2, 0x79, 0x6A, 0x6D, 0x3C, 0xAB, 0xBD, 0xA0};

const uint8_t signature_3[] = {
 0xC6, 0x54, 0x59, 0xB1, 0xC9, 0x50, 0x8F, 0xE6, 0xBD, 0x97, 0x3F, 0x43, 0xAA, 0xBF, 0x58, 0x12,
 0xAF, 0xF6, 0xA8, 0xDB, 0x40, 0x2F, 0x77, 0x3D, 0x74, 0x0E, 0xF1, 0x41, 0x8D, 0xDC, 0x54, 0xA8,
 0xD6, 0xFE, 0x52, 0x4C, 0xD3, 0x78, 0xEB, 0xE2, 0xC2, 0xAF, 0x80, 0xF6, 0xAF, 0xFC, 0x0F, 0xB0,
 0x4B, 0xEA, 0x36, 0x0C, 0x92, 0xE5, 0x64, 0x4A, 0x60, 0x41, 0xB2, 0xF4, 0x53, 0x49, 0xD4, 0x22};

const uint8_t signature_4[] = {
 0x42, 0x4A, 0xC6, 0x17, 0x69, 0x09, 0x5F, 0xD5, 0x31, 0x89, 0xF8, 0x33, 0x6E, 0x03, 0x66, 0x1B,
 0x24, 0x3F, 0xE8, 0xBA, 0x84, 0x61, 0x07, 0x3F, 0xF1, 0x2E, 0x6C, 0x53, 0xF2, 0x3F, 0x0F, 0x14,
 0xFC, 0xD6, 0x38, 0x93, 0x01, 0x8A, 0x8A, 0x96, 0xF6, 0x92, 0xE7, 0x8D, 0x25, 0x6C, 0xD5, 0x4D,
 0xD1, 0xCC, 0x86, 0xBD, 0x1D, 0xD4, 0xFC, 0xB2, 0xC3, 0x39, 0x7E, 0x9C, 0x4C, 0xDC, 0x99, 0x18,
 0x5C, 0xCE, 0x7D, 0xF7, 0x1A, 0xE3, 0x31, 0x95, 0xE1, 0x50, 0xFF, 0x13, 0xB1, 0x68, 0x9B, 0x0B,
 0x4A, 0x5C, 0x1F, 0x13, 0x2A, 0x29, 0x53, 0x86, 0x5E, 0xE4, 0x5F, 0xC6, 0x1C, 0xD4, 0x22, 0xF7,
 0x1F, 0x25, 0x4A, 0xDA, 0x31, 0xB9, 0xFF, 0x70, 0x35, 0x08, 0x3C, 0x40, 0x5B, 0xED, 0x10, 0xAC,
 0x2A, 0x85, 0xBE, 0x64, 0x33, 0xEC, 0xC5, 0x3C, 0x2D, 0x6E, 0xEA, 0xE5, 0x18, 0xD0, 0xB7, 0x6A};

const uint8_t signature_4_invalid[] = {
 0x42, 0x4A, 0xC6, 0x17, 0x69, 0x09, 0x5F, 0xD5, 0x31, 0x89, 0xF8, 0x33, 0x6E, 0x03, 0x66, 0x1B,
 0x24, 0x3F, 0xE8, 0xBA, 0x84, 0x63, 0x07, 0x3F, 0xF1, 0x2E, 0x6C, 0x53, 0xF2, 0x3F, 0x0F, 0x14,
 0xFC, 0xD6, 0x38, 0x93, 0x01, 0x8A, 0x8A, 0x96, 0xF6, 0x92, 0xE7, 0x8D, 0x25, 0x6C, 0xD5, 0x4D,
 0xD1, 0xCC, 0x86, 0xBD, 0x1D, 0xD4, 0xFC, 0xB2, 0xC3, 0x39, 0x7E, 0x9C, 0x4C, 0xDC, 0x99, 0x18,
 0x5C, 0xCE, 0x7D, 0xF7, 0x1A, 0xE3, 0x31, 0x95, 0xE1, 0x50, 0xFF, 0x13, 0xB1, 0x68, 0x9B, 0x0B,
 0x4A, 0x5C, 0x1F, 0x13, 0x2A, 0x29, 0x53, 0x86, 0x5E, 0xE4, 0x5F, 0xC6, 0x1C, 0xD4, 0x22, 0xF7,
 0x1F, 0x25, 0x4A, 0xDA, 0x31, 0xB9, 0xFF, 0x70, 0x35, 0x08, 0x3C, 0x40, 0x5B, 0xED, 0x10, 0xAC,
 0x2A, 0x85, 0xBE, 0x64, 0x33, 0xEC, 0xC5, 0x3C, 0x2D, 0x6E, 0xEA, 0xE5, 0x18, 0xD0, 0xB7, 0x6A};

const uint8_t signature_5[] = {
 0xA8, 0x86, 0x15, 0x1A, 0x0A, 0xB6, 0xCF, 0x29, 0x9C, 0xE7, 0x11, 0x67, 0x91, 0xF3, 0x87, 0x6D,
 0x40, 0x88, 0x37, 0x14, 0x9B, 0xC0, 0x1F, 0x75, 0x77, 0xD3, 0xA7, 0x3A, 0xCF, 0x9C, 0xBE, 0x30,
 0xC2, 0xE2, 0x19, 0xF8, 0xAD, 0xF8, 0x6F, 0x7D, 0xC6, 0x4C, 0xA8, 0x03, 0xBC, 0x18, 0x5B, 0x4E,
 0x60, 0xD0, 0xB9, 0xFC, 0x23, 0xB0, 0x20, 0x7A, 0x84, 0x83, 0x38, 0xB8, 0xA1, 0x15, 0x81, 0xA1};

const uint8_t signature_6[] = {0xC6, 0x54, 0x59, 0xB1, 0xC9, 0x50, 0x8F, 0xE6, 0xBD, 0x97, 0x3F,
0x43, 0xAA, 0xBF, 0x58, 0x12, 0xAF, 0xF6, 0xA8, 0xDB, 0x40, 0x2F, 0x77, 0x3D, 0x74, 0x0E, 0xF1,
0x41, 0x8D, 0xDC, 0x54, 0xA8, 0xD6, 0xFE, 0x52, 0x4C, 0xD3, 0x78, 0xEB, 0xE2, 0xC2, 0xAF, 0x80,
0xF6, 0xAF, 0xFC, 0x0F, 0xB0, 0x4B, 0xEA, 0x36, 0x0C, 0x92, 0xE5, 0x64, 0x4A, 0x60, 0x41, 0xB2,
0xF4, 0x53, 0x49, 0xD4, 0x22};

const uint8_t hmac_sha224[] = {
 0x2D, 0x39, 0x37, 0x90, 0xE1, 0xDA, 0x9A, 0x86, 0xEC, 0x0D, 0x1B, 0x17, 0x59, 0xC1, 0x23, 0xF9,
 0xBA, 0xA4, 0x38, 0xF9, 0x11, 0xA7, 0x3F, 0xEA, 0x1E, 0x9E, 0xA1, 0x7D};

const uint8_t hmac_sha224_invalid[] = {
 0x2D, 0x39, 0x37, 0x90, 0xE1, 0xDA, 0x9A, 0x86, 0xEC, 0x0D, 0x1B, 0x17, 0x59, 0xC1, 0x23, 0xF9,
 0xBA, 0xA4, 0x38, 0xF9, 0x11, 0xA5, 0x3F, 0xEA, 0x1E, 0x9E, 0xA1, 0x7D};

const uint8_t hmac_sha256[] = {
 0xFA, 0xEB, 0x2B, 0x64, 0x77, 0x79, 0xEE, 0x1B, 0x5C, 0x97, 0x0B, 0xBC, 0x28, 0x0E, 0xE5, 0x1D,
 0xE9, 0x79, 0x22, 0x9D, 0xE8, 0x4A, 0x94, 0xA1, 0xA4, 0x2C, 0x84, 0x76, 0xD1, 0xBF, 0xE9, 0x3F};

const uint8_t hmac_sha512[] = {
 0x46, 0x31, 0x33, 0x41, 0x39, 0xCB, 0x97, 0x3A, 0xCD, 0x7C, 0xF2, 0x2C, 0xA0, 0x89, 0x1B, 0x55,
 0x0F, 0x58, 0x76, 0x25, 0x9E, 0x72, 0x7C, 0x91, 0xF7, 0xEC, 0x78, 0x75, 0x2A, 0xF9, 0x52, 0x13,
 0x91, 0xD1, 0x35, 0x52, 0x7F, 0xF2, 0xC3, 0x67, 0x43, 0x99, 0xDC, 0x20, 0x2A, 0xC4, 0x77, 0xB4,
 0x4C, 0x51, 0xD0, 0xFE, 0x1D, 0xB6, 0xC8, 0x28, 0x34, 0x02, 0x6A, 0x6D, 0x8D, 0xD3, 0x20, 0xDB};

const uint8_t cmac_aes_128[] = {0x7F, 0x97, 0x9B, 0xA0, 0xFF, 0xF1, 0x35, 0x98, 0x61, 0x38, 0xF2,
                                0xAB, 0x05, 0x4B, 0x28, 0x4B};

/* test inputs */
const uint8_t hash_input                               = 0xbd;
const uint8_t input_bytes_data[INPUT_BYTES_DATA_LEN]   = "abcdefghijklmnop";
const uint8_t input_salt[INPUT_SALT_LEN]               = "Salt";
const uint8_t input_info[INPUT_INFO_LEN]               = "Info";
const uint8_t input_seed[INPUT_SEED_LEN]               = "Seed";
const uint8_t input_label[INPUT_LABEL_LEN]             = "Label";

/* output buffers */
uint8_t expected_output[BUFFER_SIZE];

/* crypto common testcase exit action */
void crypto_common_exit_action(void)
{
	g_test_count = 1;
}
