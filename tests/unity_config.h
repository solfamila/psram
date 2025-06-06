/*
 * Unity Configuration for MIMXRT700 Tests
 * Custom configuration for Unity testing framework
 */

#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

#include <stdio.h>

// Basic Unity configuration
#define UNITY_INCLUDE_DOUBLE
#define UNITY_INCLUDE_FLOAT
#define UNITY_SUPPORT_64

// Output configuration
#define UNITY_OUTPUT_CHAR(a)    putchar(a)
#define UNITY_OUTPUT_FLUSH()    fflush(stdout)
#define UNITY_OUTPUT_START()
#define UNITY_OUTPUT_COMPLETE()

// Memory configuration
#define UNITY_EXCLUDE_SETJMP_H

// Test configuration
#define UNITY_SUPPORT_TEST_CASES

#endif // UNITY_CONFIG_H
