/*
 * Mock Implementation for FSL XSPI Driver
 * Provides controllable mock behavior for testing
 */

#include "mock_fsl_xspi.h"
#include <string.h>
#include <stdio.h>

// Mock expectation structure
typedef struct {
    enum {
        MOCK_EXPECT_INIT,
        MOCK_EXPECT_AHB_WRITE,
        MOCK_EXPECT_AHB_READ,
        MOCK_EXPECT_AHB_READ_CORRUPT,
        MOCK_EXPECT_IP_WRITE,
        MOCK_EXPECT_IP_READ,
        MOCK_EXPECT_WRITE,
        MOCK_EXPECT_READ
    } type;
    uint32_t address;
    const uint8_t* buffer;
    uint32_t length;
    mock_status_t expected_status;
    bool corrupt_data;
} mock_expectation_t;

// Mock state
#define MAX_EXPECTATIONS 100
static mock_expectation_t expectations[MAX_EXPECTATIONS];
static int expectation_count = 0;
static int current_expectation = 0;
static bool mock_initialized = false;

// Mock control functions
void mock_xspi_reset(void) {
    expectation_count = 0;
    current_expectation = 0;
    mock_initialized = true;
    memset(expectations, 0, sizeof(expectations));
}

bool mock_xspi_verify_expectations(void) {
    if (!mock_initialized) {
        printf("Mock not initialized\n");
        return false;
    }
    
    if (current_expectation != expectation_count) {
        printf("Expected %d calls, but got %d\n", expectation_count, current_expectation);
        return false;
    }
    
    return true;
}

// Helper function to add expectation
static void add_expectation(int type, uint32_t address, const uint8_t* buffer, 
                           uint32_t length, mock_status_t status, bool corrupt) {
    if (expectation_count < MAX_EXPECTATIONS) {
        expectations[expectation_count].type = type;
        expectations[expectation_count].address = address;
        expectations[expectation_count].buffer = buffer;
        expectations[expectation_count].length = length;
        expectations[expectation_count].expected_status = status;
        expectations[expectation_count].corrupt_data = corrupt;
        expectation_count++;
    }
}

// Mock expectation setup functions
void mock_xspi_expect_init(void) {
    add_expectation(MOCK_EXPECT_INIT, 0, NULL, 0, MOCK_STATUS_SUCCESS, false);
}

void mock_xspi_expect_ahb_write(uint32_t address, const uint8_t* buffer, uint32_t length) {
    add_expectation(MOCK_EXPECT_AHB_WRITE, address, buffer, length, MOCK_STATUS_SUCCESS, false);
}

void mock_xspi_expect_ahb_read(uint32_t address, const uint8_t* buffer, uint32_t length) {
    add_expectation(MOCK_EXPECT_AHB_READ, address, buffer, length, MOCK_STATUS_SUCCESS, false);
}

void mock_xspi_expect_ahb_read_with_corruption(uint32_t address, const uint8_t* buffer, uint32_t length) {
    add_expectation(MOCK_EXPECT_AHB_READ_CORRUPT, address, buffer, length, MOCK_STATUS_SUCCESS, true);
}

void mock_xspi_expect_ip_write(uint32_t address, const uint8_t* buffer, uint32_t length, mock_status_t expected_status) {
    add_expectation(MOCK_EXPECT_IP_WRITE, address, buffer, length, expected_status, false);
}

void mock_xspi_expect_ip_read(uint32_t address, const uint8_t* buffer, uint32_t length, mock_status_t expected_status) {
    add_expectation(MOCK_EXPECT_IP_READ, address, buffer, length, expected_status, false);
}

void mock_xspi_expect_write(uint32_t address, const uint8_t* buffer, uint32_t length, mock_status_t expected_status) {
    add_expectation(MOCK_EXPECT_WRITE, address, buffer, length, expected_status, false);
}

void mock_xspi_expect_read(uint32_t address, const uint8_t* buffer, uint32_t length, mock_status_t expected_status) {
    add_expectation(MOCK_EXPECT_READ, address, buffer, length, expected_status, false);
}

// Helper function to verify current expectation
static bool verify_current_expectation(int type, uint32_t address, uint32_t length) {
    if (current_expectation >= expectation_count) {
        printf("Unexpected call: no more expectations\n");
        return false;
    }
    
    mock_expectation_t* exp = &expectations[current_expectation];
    
    if (exp->type != type) {
        printf("Expected type %d, got %d\n", exp->type, type);
        return false;
    }
    
    if (exp->address != address) {
        printf("Expected address 0x%x, got 0x%x\n", exp->address, address);
        return false;
    }
    
    if (exp->length != length) {
        printf("Expected length %u, got %u\n", exp->length, length);
        return false;
    }
    
    current_expectation++;
    return true;
}

// Mock implementations of XSPI functions
void xspi_hyper_ram_init(void *base) {
    verify_current_expectation(MOCK_EXPECT_INIT, 0, 0);
}

void xspi_hyper_ram_ahbcommand_write_data(void *base, uint32_t address, uint32_t *buffer, uint32_t length) {
    verify_current_expectation(MOCK_EXPECT_AHB_WRITE, address, length);
}

void xspi_hyper_ram_ahbcommand_read_data(void *base, uint32_t address, uint32_t *buffer, uint32_t length) {
    bool is_corrupt = false;
    
    if (current_expectation < expectation_count) {
        mock_expectation_t* exp = &expectations[current_expectation];
        is_corrupt = exp->corrupt_data;
    }
    
    if (is_corrupt) {
        verify_current_expectation(MOCK_EXPECT_AHB_READ_CORRUPT, address, length);
        // Simulate data corruption by modifying the buffer
        if (buffer && length > 0) {
            ((uint8_t*)buffer)[0] ^= 0xFF;
        }
    } else {
        verify_current_expectation(MOCK_EXPECT_AHB_READ, address, length);
    }
}

mock_status_t xspi_hyper_ram_ipcommand_write_data(void *base, uint32_t address, uint32_t *buffer, uint32_t length) {
    if (verify_current_expectation(MOCK_EXPECT_IP_WRITE, address, length)) {
        mock_expectation_t* exp = &expectations[current_expectation - 1];
        return exp->expected_status;
    }
    return MOCK_STATUS_FAIL;
}

mock_status_t xspi_hyper_ram_ipcommand_read_data(void *base, uint32_t address, uint32_t *buffer, uint32_t length) {
    if (verify_current_expectation(MOCK_EXPECT_IP_READ, address, length)) {
        mock_expectation_t* exp = &expectations[current_expectation - 1];
        return exp->expected_status;
    }
    return MOCK_STATUS_FAIL;
}

void BOARD_InitHardware(void) {
    // Mock board initialization - no operation needed
}
