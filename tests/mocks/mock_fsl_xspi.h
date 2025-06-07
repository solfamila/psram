/*
 * Mock for FSL XSPI Driver
 * Provides mock implementations for XSPI functions
 * 
 * FIXED: Added missing function declarations (mock_xspi_expect_write/read,
 * mock_xspi_expect_ip_write/read) and improved the mock interface to support
 * all the test scenarios we developed.
 */

#ifndef MOCK_FSL_XSPI_H
#define MOCK_FSL_XSPI_H

#include <stdint.h>
#include <stdbool.h>

// Mock status type
typedef enum {
    MOCK_STATUS_SUCCESS = 0,
    MOCK_STATUS_FAIL = -1
} mock_status_t;

// Mock XSPI base address
#define MOCK_XSPI_BASE ((void*)0x40000000)

// Mock DRAM size for testing
#define DRAM_SIZE 1024

// Mock function declarations
void mock_xspi_reset(void);
bool mock_xspi_verify_expectations(void);

// Initialization mocks
void mock_xspi_expect_init(void);

// AHB command mocks
void mock_xspi_expect_ahb_write(uint32_t address, const uint8_t* buffer, uint32_t length);
void mock_xspi_expect_ahb_read(uint32_t address, const uint8_t* buffer, uint32_t length);
void mock_xspi_expect_ahb_read_with_corruption(uint32_t address, const uint8_t* buffer, uint32_t length);

// IP command mocks
void mock_xspi_expect_ip_write(uint32_t address, const uint8_t* buffer, uint32_t length, mock_status_t expected_status);
void mock_xspi_expect_ip_read(uint32_t address, const uint8_t* buffer, uint32_t length, mock_status_t expected_status);

// Generic write/read mocks (for backward compatibility)
void mock_xspi_expect_write(uint32_t address, const uint8_t* buffer, uint32_t length, mock_status_t expected_status);
void mock_xspi_expect_read(uint32_t address, const uint8_t* buffer, uint32_t length, mock_status_t expected_status);

// Mock implementations of actual XSPI functions
void xspi_hyper_ram_init(void *base);
void xspi_hyper_ram_ahbcommand_write_data(void *base, uint32_t address, uint32_t *buffer, uint32_t length);
void xspi_hyper_ram_ahbcommand_read_data(void *base, uint32_t address, uint32_t *buffer, uint32_t length);
mock_status_t xspi_hyper_ram_ipcommand_write_data(void *base, uint32_t address, uint32_t *buffer, uint32_t length);
mock_status_t xspi_hyper_ram_ipcommand_read_data(void *base, uint32_t address, uint32_t *buffer, uint32_t length);

// Board initialization mock
void BOARD_InitHardware(void);

// Example XSPI instance
#define EXAMPLE_XSPI MOCK_XSPI_BASE

#endif // MOCK_FSL_XSPI_H