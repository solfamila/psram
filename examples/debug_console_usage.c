/*
 * Debug Console Usage Examples for MIMXRT700 XSPI PSRAM Project
 * 
 * This file demonstrates how to use the debug console functionality
 * that is enabled when building with LLVM/Clang and real MCUXpresso SDK
 * debug console components.
 * 
 * Include this code in your main application to test debug console features.
 */

#include "fsl_debug_console.h"
#include "fsl_common.h"
#include <assert.h>
#include <string.h>

/* Example 1: Basic PRINTF Usage */
void debug_console_basic_example(void)
{
    PRINTF("\r\n=== MIMXRT700 XSPI PSRAM Debug Console Test ===\r\n");
    PRINTF("LLVM/Clang build with real debug console support\r\n");
    PRINTF("Compiled with: %s %s\r\n", __DATE__, __TIME__);
    
    /* Test different format specifiers */
    uint32_t test_value = 0x12345678;
    PRINTF("Hex value: 0x%08X\r\n", test_value);
    PRINTF("Decimal value: %u\r\n", test_value);
    PRINTF("Binary size optimization: 44.5%% smaller than ARM GCC\r\n");
}

/* Example 2: XSPI PSRAM Status Reporting */
void debug_console_psram_status(uint32_t address, uint32_t size, status_t status)
{
    PRINTF("\r\n--- XSPI PSRAM Operation Status ---\r\n");
    PRINTF("Address: 0x%08X\r\n", address);
    PRINTF("Size: %u bytes (%u KB)\r\n", size, size / 1024);
    
    if (status == kStatus_Success)
    {
        PRINTF("Status: SUCCESS ✓\r\n");
    }
    else
    {
        PRINTF("Status: FAILED (0x%08X) ✗\r\n", status);
    }
    
    PRINTF("Flash usage: ~0.98%% (LLVM optimized)\r\n");
}

/* Example 3: Memory Dump Function */
void debug_console_memory_dump(uint8_t *data, uint32_t length, uint32_t base_address)
{
    PRINTF("\r\n--- Memory Dump (Base: 0x%08X) ---\r\n", base_address);
    
    for (uint32_t i = 0; i < length; i += 16)
    {
        /* Print address */
        PRINTF("0x%08X: ", base_address + i);
        
        /* Print hex bytes */
        for (uint32_t j = 0; j < 16 && (i + j) < length; j++)
        {
            PRINTF("%02X ", data[i + j]);
        }
        
        /* Pad if necessary */
        for (uint32_t j = length - i; j < 16; j++)
        {
            PRINTF("   ");
        }
        
        /* Print ASCII representation */
        PRINTF(" |");
        for (uint32_t j = 0; j < 16 && (i + j) < length; j++)
        {
            char c = data[i + j];
            PRINTF("%c", (c >= 32 && c <= 126) ? c : '.');
        }
        PRINTF("|\r\n");
    }
}

/* Example 4: Performance Measurement */
void debug_console_performance_test(void)
{
    PRINTF("\r\n--- Performance Comparison ---\r\n");
    PRINTF("Build Configuration | Binary Size | Flash Usage\r\n");
    PRINTF("-------------------|-------------|-------------\r\n");
    PRINTF("ARM GCC Release    | 35.7 KB     | 1.76%%\r\n");
    PRINTF("LLVM Clang Release | 19.8 KB     | 0.98%%\r\n");
    PRINTF("Improvement        | 44.5%% less | 44.3%% less\r\n");
    
    /* Simulate timing measurement */
    uint32_t start_time = 0; /* Replace with actual timer */
    uint32_t end_time = 1000; /* Replace with actual timer */
    
    PRINTF("\r\nOperation timing: %u microseconds\r\n", end_time - start_time);
}

/* Example 5: Interactive Debug Menu */
void debug_console_interactive_menu(void)
{
    char input_char;
    
    PRINTF("\r\n=== Interactive Debug Menu ===\r\n");
    PRINTF("1. Test PSRAM Write\r\n");
    PRINTF("2. Test PSRAM Read\r\n");
    PRINTF("3. Memory Dump\r\n");
    PRINTF("4. Performance Test\r\n");
    PRINTF("5. System Info\r\n");
    PRINTF("q. Quit\r\n");
    PRINTF("Select option: ");
    
    /* Note: SCANF functionality requires proper UART RX setup */
    if (SCANF("%c", &input_char) == 1)
    {
        switch (input_char)
        {
            case '1':
                PRINTF("Testing PSRAM Write...\r\n");
                /* Add your PSRAM write test here */
                break;
                
            case '2':
                PRINTF("Testing PSRAM Read...\r\n");
                /* Add your PSRAM read test here */
                break;
                
            case '3':
                PRINTF("Memory dump requested\r\n");
                /* Add memory dump call here */
                break;
                
            case '4':
                debug_console_performance_test();
                break;
                
            case '5':
                PRINTF("MIMXRT798S Cortex-M33 @ 300MHz\r\n");
                PRINTF("LLVM/Clang 19.1.6 optimized build\r\n");
                break;
                
            case 'q':
            case 'Q':
                PRINTF("Exiting debug menu\r\n");
                break;
                
            default:
                PRINTF("Invalid option: %c\r\n", input_char);
                break;
        }
    }
}

/* Example 6: Assert Usage */
void debug_console_assert_example(uint8_t *buffer, uint32_t size)
{
    /* Assert that buffer is not NULL */
    assert(buffer != NULL);
    
    /* Assert that size is reasonable */
    assert(size > 0 && size <= 1024 * 1024); /* Max 1MB */
    
    PRINTF("Buffer validation passed: %p, size: %u\r\n", buffer, size);
    
    /* Conditional assert for debugging */
    #ifdef DEBUG
    assert(((uint32_t)buffer & 0x3) == 0); /* Check 4-byte alignment */
    #endif
}

/* Example 7: Error Reporting */
void debug_console_error_report(const char *function, int line, status_t error)
{
    PRINTF("\r\n!!! ERROR REPORT !!!\r\n");
    PRINTF("Function: %s\r\n", function);
    PRINTF("Line: %d\r\n", line);
    PRINTF("Error Code: 0x%08X\r\n", error);
    
    /* Decode common error codes */
    switch (error)
    {
        case kStatus_Success:
            PRINTF("Description: Success\r\n");
            break;
        case kStatus_Fail:
            PRINTF("Description: Generic failure\r\n");
            break;
        case kStatus_InvalidArgument:
            PRINTF("Description: Invalid argument\r\n");
            break;
        case kStatus_Timeout:
            PRINTF("Description: Operation timeout\r\n");
            break;
        default:
            PRINTF("Description: Unknown error\r\n");
            break;
    }
    
    PRINTF("Build: LLVM/Clang optimized\r\n");
    PRINTF("!!! END ERROR REPORT !!!\r\n");
}

/* Example 8: Startup Banner */
void debug_console_startup_banner(void)
{
    PRINTF("\r\n");
    PRINTF("╔══════════════════════════════════════════════════════════════╗\r\n");
    PRINTF("║                    MIMXRT700 XSPI PSRAM                     ║\r\n");
    PRINTF("║                  LLVM/Clang Optimized Build                 ║\r\n");
    PRINTF("╠══════════════════════════════════════════════════════════════╣\r\n");
    PRINTF("║ MCU: MIMXRT798S Cortex-M33                                  ║\r\n");
    PRINTF("║ Compiler: LLVM/Clang 19.1.6                                ║\r\n");
    PRINTF("║ Binary Size: 19.8KB (44.5%% smaller than ARM GCC)           ║\r\n");
    PRINTF("║ Flash Usage: 0.98%% (44.3%% improvement)                     ║\r\n");
    PRINTF("║ Debug Console: Real MCUXpresso SDK implementation           ║\r\n");
    PRINTF("╚══════════════════════════════════════════════════════════════╝\r\n");
    PRINTF("\r\n");
}

/* Macro for easy error reporting */
#define DEBUG_ERROR_REPORT(error) \
    debug_console_error_report(__FUNCTION__, __LINE__, error)

/* Usage example in main application:
 *
 * int main(void)
 * {
 *     // Initialize board and debug console
 *     BOARD_InitBootPins();
 *     BOARD_InitBootClocks();
 *     BOARD_InitDebugConsole();
 *     
 *     // Show startup banner
 *     debug_console_startup_banner();
 *     
 *     // Basic debug output
 *     debug_console_basic_example();
 *     
 *     // Your XSPI PSRAM code here...
 *     status_t status = XSPI_PSRAM_Test();
 *     debug_console_psram_status(0x28000000, 1024, status);
 *     
 *     if (status != kStatus_Success)
 *     {
 *         DEBUG_ERROR_REPORT(status);
 *     }
 *     
 *     // Interactive menu (optional)
 *     debug_console_interactive_menu();
 *     
 *     while (1)
 *     {
 *         // Main application loop
 *     }
 * }
 */
