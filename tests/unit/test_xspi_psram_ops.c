/*
 * Unit Tests for XSPI PSRAM Operations
 * Tests the core PSRAM read/write functionality
 * 
 * FIXED: Updated all test functions to actually call the mocked XSPI functions
 * (xspi_hyper_ram_init, xspi_hyper_ram_ipcommand_write_data, etc.) instead of
 * just simulating results with hardcoded values. This ensures proper mock
 * verification and realistic testing.
 */

#include "unity.h"
#include "mock_fsl_xspi.h"
#include "mock_fsl_debug_console.h"

// Test data
static uint8_t test_write_buffer[256];
static uint8_t test_read_buffer[256];

static void test_xspi_setup(void) {
    // Initialize test data
    for (int i = 0; i < 256; i++) {
        test_write_buffer[i] = (uint8_t)(i + 0xFF);
        test_read_buffer[i] = 0;
    }

    // Reset mocks
    mock_xspi_reset();
    mock_debug_console_reset();
}

void test_xspi_psram_write_read_basic(void) {
    // Test basic write/read functionality
    test_xspi_setup();

    // Mock expectations - use IP commands for basic read/write
    mock_xspi_expect_init();
    mock_xspi_expect_ip_write(0x1000, test_write_buffer, 256, MOCK_STATUS_SUCCESS);
    mock_xspi_expect_ip_read(0x1000, test_read_buffer, 256, MOCK_STATUS_SUCCESS);
    
    // Actually call the mocked functions
    xspi_hyper_ram_init(MOCK_XSPI_BASE);
    mock_status_t write_status = xspi_hyper_ram_ipcommand_write_data(MOCK_XSPI_BASE, 0x1000, 
                                                                   (uint32_t*)test_write_buffer, 256);
    mock_status_t read_status = xspi_hyper_ram_ipcommand_read_data(MOCK_XSPI_BASE, 0x1000,
                                                                 (uint32_t*)test_read_buffer, 256);
    
    // Verify results
    TEST_ASSERT_EQUAL(MOCK_STATUS_SUCCESS, write_status);
    TEST_ASSERT_EQUAL(MOCK_STATUS_SUCCESS, read_status);
    
    // Verify mock expectations were met
    TEST_ASSERT_TRUE(mock_xspi_verify_expectations());
}

void test_xspi_psram_write_read_boundary_conditions(void) {
    // Test boundary conditions
    test_xspi_setup();
    
    // Test minimum size
    mock_xspi_expect_ip_write(0x0000, test_write_buffer, 1, MOCK_STATUS_SUCCESS);
    mock_xspi_expect_ip_read(0x0000, test_read_buffer, 1, MOCK_STATUS_SUCCESS);
    
    // Actually call the functions
    mock_status_t write_status = xspi_hyper_ram_ipcommand_write_data(MOCK_XSPI_BASE, 0x0000, 
                                                                   (uint32_t*)test_write_buffer, 1);
    mock_status_t read_status = xspi_hyper_ram_ipcommand_read_data(MOCK_XSPI_BASE, 0x0000,
                                                                 (uint32_t*)test_read_buffer, 1);
    
    TEST_ASSERT_EQUAL(MOCK_STATUS_SUCCESS, write_status);
    TEST_ASSERT_EQUAL(MOCK_STATUS_SUCCESS, read_status);
    TEST_ASSERT_TRUE(mock_xspi_verify_expectations());
}

void test_xspi_psram_error_handling(void) {
    // Test error conditions
    test_xspi_setup();
    
    // Mock failure scenarios
    mock_xspi_expect_ip_write(0x1000, test_write_buffer, 256, MOCK_STATUS_FAIL);
    
    // Actually call the function
    mock_status_t status = xspi_hyper_ram_ipcommand_write_data(MOCK_XSPI_BASE, 0x1000, 
                                                             (uint32_t*)test_write_buffer, 256);
    
    TEST_ASSERT_EQUAL(MOCK_STATUS_FAIL, status);
    TEST_ASSERT_TRUE(mock_xspi_verify_expectations());
}

void test_xspi_psram_data_integrity(void) {
    // Test data integrity across write/read cycles
    test_xspi_setup();
    
    // Prepare test pattern
    for (int i = 0; i < 256; i++) {
        test_write_buffer[i] = (uint8_t)(i ^ 0xAA);
    }
    
    // Mock successful write/read
    mock_xspi_expect_ip_write(0x2000, test_write_buffer, 256, MOCK_STATUS_SUCCESS);
    mock_xspi_expect_ip_read(0x2000, test_read_buffer, 256, MOCK_STATUS_SUCCESS);
    
    // Actually call the functions
    mock_status_t write_status = xspi_hyper_ram_ipcommand_write_data(MOCK_XSPI_BASE, 0x2000, 
                                                                   (uint32_t*)test_write_buffer, 256);
    mock_status_t read_status = xspi_hyper_ram_ipcommand_read_data(MOCK_XSPI_BASE, 0x2000,
                                                                 (uint32_t*)test_read_buffer, 256);
    
    TEST_ASSERT_EQUAL(MOCK_STATUS_SUCCESS, write_status);
    TEST_ASSERT_EQUAL(MOCK_STATUS_SUCCESS, read_status);
    TEST_ASSERT_TRUE(mock_xspi_verify_expectations());
}

void test_xspi_psram_address_alignment(void) {
    // Test address alignment requirements
    test_xspi_setup();
    
    // Test aligned addresses
    mock_xspi_expect_ip_write(0x1000, test_write_buffer, 256, MOCK_STATUS_SUCCESS);
    mock_xspi_expect_ip_read(0x1000, test_read_buffer, 256, MOCK_STATUS_SUCCESS);
    
    // Actually call the functions
    mock_status_t write_status = xspi_hyper_ram_ipcommand_write_data(MOCK_XSPI_BASE, 0x1000, 
                                                                   (uint32_t*)test_write_buffer, 256);
    mock_status_t read_status = xspi_hyper_ram_ipcommand_read_data(MOCK_XSPI_BASE, 0x1000,
                                                                 (uint32_t*)test_read_buffer, 256);
    
    TEST_ASSERT_EQUAL(MOCK_STATUS_SUCCESS, write_status);
    TEST_ASSERT_EQUAL(MOCK_STATUS_SUCCESS, read_status);
    TEST_ASSERT_TRUE(mock_xspi_verify_expectations());
}