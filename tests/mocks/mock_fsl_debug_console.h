/*
 * Mock for FSL Debug Console
 * Provides mock implementations for debug console functions
 */

#ifndef MOCK_FSL_DEBUG_CONSOLE_H
#define MOCK_FSL_DEBUG_CONSOLE_H

#include <stdarg.h>
#include <stdbool.h>

// Mock control functions
void mock_debug_console_reset(void);
bool mock_debug_console_verify_expectations(void);

// Mock expectation functions
void mock_debug_console_expect_printf(const char* expected_format);

// Mock implementations
int PRINTF(const char* format, ...);

#endif // MOCK_FSL_DEBUG_CONSOLE_H
