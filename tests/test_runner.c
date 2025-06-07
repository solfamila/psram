/*
 * Test Runner for MIMXRT700 XSPI PSRAM Example
 * Unity testing framework integration
 * 
 * FIXED: Changed from calling test suite functions (test_xspi_psram_ops, test_main_functionality)
 * to calling individual test functions directly. This ensures proper test isolation and
 * correct mock setup/teardown for each test.
 */

#include "unity.h"
#include <stdio.h>

// Individual test function declarations from test_xspi_psram_ops.c
void test_xspi_psram_write_read_basic(void);
void test_xspi_psram_write_read_boundary_conditions(void);
void test_xspi_psram_error_handling(void);
void test_xspi_psram_data_integrity(void);
void test_xspi_psram_address_alignment(void);

// Individual test function declarations from test_main.c
void test_main_initialization(void);
void test_main_ahb_command_sequence(void);
void test_main_ip_command_sequence(void);
void test_main_error_handling_ahb_failure(void);
void test_main_error_handling_ip_failure(void);
void test_main_data_pattern_variations(void);
void test_main_memory_coverage(void);

// Unity setup and teardown
void setUp(void) {
    // Set up code for each test
}

void tearDown(void) {
    // Clean up code for each test
}

// Test suite runner
int main(void) {
    UNITY_BEGIN();
    
    printf("Running MIMXRT700 XSPI PSRAM Tests...\n");
    
    // Run XSPI PSRAM operation tests
    RUN_TEST(test_xspi_psram_write_read_basic);
    RUN_TEST(test_xspi_psram_write_read_boundary_conditions);
    RUN_TEST(test_xspi_psram_error_handling);
    RUN_TEST(test_xspi_psram_data_integrity);
    RUN_TEST(test_xspi_psram_address_alignment);
    
    // Run main functionality tests
    RUN_TEST(test_main_initialization);
    RUN_TEST(test_main_ahb_command_sequence);
    RUN_TEST(test_main_ip_command_sequence);
    RUN_TEST(test_main_error_handling_ahb_failure);
    RUN_TEST(test_main_error_handling_ip_failure);
    RUN_TEST(test_main_data_pattern_variations);
    RUN_TEST(test_main_memory_coverage);
    
    return UNITY_END();
}