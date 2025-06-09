/*
 * Debug Console Stub for Clang Build
 * 
 * This file provides minimal stub implementations of debug console functions
 * that are expected by the MIMXRT700 project but are disabled in the Clang build
 * to avoid dependencies on FreeRTOS and complex UART adapter components.
 * 
 * For a production build, you would want to implement these functions properly
 * or use the full MCUXpresso SDK debug console components.
 */

#include <stdarg.h>
#include <stdio.h>

/**
 * @brief Stub implementation for DbgConsole_Init
 * 
 * In a real implementation, this would initialize the debug console UART
 * with the specified parameters.
 * 
 * @param instance UART instance number
 * @param baudRate Baud rate for UART communication
 * @param device Device type (UART, USB, etc.)
 * @param clkSrcFreq Clock source frequency
 * @return int Always returns 0 (success) in this stub
 */
int DbgConsole_Init(unsigned int instance, unsigned int baudRate, unsigned int device, unsigned int clkSrcFreq)
{
    // Stub implementation - no actual console initialization
    // In a real implementation, this would:
    // 1. Configure the specified UART instance
    // 2. Set up the baud rate and other parameters
    // 3. Initialize any necessary buffers or state
    
    (void)instance;    // Suppress unused parameter warning
    (void)baudRate;    // Suppress unused parameter warning
    (void)device;      // Suppress unused parameter warning
    (void)clkSrcFreq;  // Suppress unused parameter warning
    
    return 0;  // Return success
}

/**
 * @brief Stub implementation for DbgConsole_Printf
 * 
 * In a real implementation, this would format and output text to the debug console.
 * 
 * @param fmt_s Format string (printf-style)
 * @param ... Variable arguments for formatting
 * @return int Always returns 0 in this stub
 */
int DbgConsole_Printf(const char *fmt_s, ...)
{
    // Stub implementation - no actual output
    // In a real implementation, this would:
    // 1. Process the format string and arguments
    // 2. Send formatted output to the UART
    // 3. Handle any buffering or flow control
    
    (void)fmt_s;  // Suppress unused parameter warning
    
    // If you want to see debug output during development, you could
    // uncomment the following code to output to a buffer or implement
    // a simple UART output function:
    
    /*
    va_list args;
    va_start(args, fmt_s);
    // vprintf(fmt_s, args);  // This would require stdout to be configured
    va_end(args);
    */
    
    return 0;  // Return success
}

/**
 * @brief Alternative PRINTF implementation
 * 
 * Some code may use PRINTF instead of DbgConsole_Printf.
 * This provides a stub for that as well.
 * 
 * @param fmt_s Format string (printf-style)
 * @param ... Variable arguments for formatting
 * @return int Always returns 0 in this stub
 */
int PRINTF(const char *fmt_s, ...)
{
    // Stub implementation - same as DbgConsole_Printf
    (void)fmt_s;  // Suppress unused parameter warning
    return 0;
}

/*
 * Additional notes for production use:
 * 
 * 1. To enable actual debug output, you would need to:
 *    - Configure a UART peripheral for output
 *    - Implement the low-level UART write functions
 *    - Handle any necessary clock configuration
 * 
 * 2. For a minimal implementation, you could:
 *    - Use a simple polling-based UART output
 *    - Implement just enough to output debug strings
 *    - Avoid the complexity of the full MCUXpresso debug console
 * 
 * 3. For advanced features, you might want:
 *    - Buffered output for better performance
 *    - Support for different output devices (UART, USB, etc.)
 *    - Integration with RTT (Real-Time Transfer) for debugging
 */
