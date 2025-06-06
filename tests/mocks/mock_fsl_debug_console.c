/*
 * Mock Implementation for FSL Debug Console
 * Provides controllable mock behavior for debug output testing
 */

#include "mock_fsl_debug_console.h"
#include <string.h>
#include <stdio.h>

// Mock expectation structure
typedef struct {
    char expected_format[256];
} printf_expectation_t;

// Mock state
#define MAX_PRINTF_EXPECTATIONS 50
static printf_expectation_t printf_expectations[MAX_PRINTF_EXPECTATIONS];
static int printf_expectation_count = 0;
static int current_printf_expectation = 0;
static bool printf_mock_initialized = false;

// Mock control functions
void mock_debug_console_reset(void) {
    printf_expectation_count = 0;
    current_printf_expectation = 0;
    printf_mock_initialized = true;
    memset(printf_expectations, 0, sizeof(printf_expectations));
}

bool mock_debug_console_verify_expectations(void) {
    if (!printf_mock_initialized) {
        printf("Debug console mock not initialized\n");
        return false;
    }
    
    if (current_printf_expectation != printf_expectation_count) {
        printf("Expected %d printf calls, but got %d\n", 
               printf_expectation_count, current_printf_expectation);
        return false;
    }
    
    return true;
}

// Mock expectation setup
void mock_debug_console_expect_printf(const char* expected_format) {
    if (printf_expectation_count < MAX_PRINTF_EXPECTATIONS) {
        strncpy(printf_expectations[printf_expectation_count].expected_format, 
                expected_format, sizeof(printf_expectations[0].expected_format) - 1);
        printf_expectations[printf_expectation_count].expected_format[
            sizeof(printf_expectations[0].expected_format) - 1] = '\0';
        printf_expectation_count++;
    }
}

// Mock implementation of PRINTF
int PRINTF(const char* format, ...) {
    if (!printf_mock_initialized) {
        return -1;
    }
    
    if (current_printf_expectation >= printf_expectation_count) {
        printf("Unexpected PRINTF call: %s\n", format);
        return -1;
    }
    
    printf_expectation_t* exp = &printf_expectations[current_printf_expectation];
    
    // Simple format string comparison (in real implementation, might want more sophisticated matching)
    if (strstr(format, exp->expected_format) == NULL && 
        strstr(exp->expected_format, format) == NULL) {
        printf("Expected printf format containing '%s', got '%s'\n", 
               exp->expected_format, format);
        return -1;
    }
    
    current_printf_expectation++;
    
    // Return length of format string as a simple mock behavior
    return strlen(format);
}
