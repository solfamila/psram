/*
 * Test file for peripheral register access analysis
 * This file contains typical peripheral access patterns found in embedded systems
 */

#include <stdint.h>

// MIMXRT700 XSPI peripheral base addresses (from the project)
#define XSPI0_BASE      0x50184000U
#define XSPI0_NS_BASE   0x40184000U
#define XSPI1_BASE      0x40185000U
#define XSPI2_BASE      0x40411000U

// XSPI register offsets
#define XSPI_MCR_OFFSET     0x00U
#define XSPI_IPCR_OFFSET    0x08U
#define XSPI_FLSHCR_OFFSET  0x0CU
#define XSPI_BUFCR0_OFFSET  0x10U
#define XSPI_SFAR_OFFSET    0x100U
#define XSPI_LUTKEY_OFFSET  0x300U

// Register addresses
#define XSPI0_MCR       (XSPI0_BASE + XSPI_MCR_OFFSET)
#define XSPI0_IPCR      (XSPI0_BASE + XSPI_IPCR_OFFSET)
#define XSPI0_FLSHCR    (XSPI0_BASE + XSPI_FLSHCR_OFFSET)
#define XSPI0_BUFCR0    (XSPI0_BASE + XSPI_BUFCR0_OFFSET)
#define XSPI0_SFAR      (XSPI0_BASE + XSPI_SFAR_OFFSET)
#define XSPI0_LUTKEY    (XSPI0_BASE + XSPI_LUTKEY_OFFSET)

// GPIO peripheral
#define GPIO0_BASE      0x40100000U
#define GPIO0_PDOR      (GPIO0_BASE + 0x00U)
#define GPIO0_PSOR      (GPIO0_BASE + 0x04U)
#define GPIO0_PCOR      (GPIO0_BASE + 0x08U)
#define GPIO0_PDIR      (GPIO0_BASE + 0x10U)

// Bit manipulation macros
#define BIT(n)          (1U << (n))
#define MASK(width)     ((1U << (width)) - 1U)

// Function to initialize XSPI controller
void XSPI_Init(void) {
    volatile uint32_t *mcr_reg = (volatile uint32_t *)XSPI0_MCR;
    volatile uint32_t *ipcr_reg = (volatile uint32_t *)XSPI0_IPCR;
    
    // Module configuration - write operation
    *mcr_reg = 0x00000001U;  // Enable XSPI module
    
    // IP configuration - read-modify-write operation
    uint32_t ipcr_val = *ipcr_reg;
    ipcr_val |= BIT(0);  // Set bit 0
    ipcr_val &= ~BIT(1); // Clear bit 1
    *ipcr_reg = ipcr_val;
}

// Function to configure XSPI flash settings
void XSPI_ConfigFlash(uint32_t flash_size) {
    volatile uint32_t *flshcr_reg = (volatile uint32_t *)XSPI0_FLSHCR;
    volatile uint32_t *bufcr0_reg = (volatile uint32_t *)XSPI0_BUFCR0;
    
    // Configure flash size - bitfield operation
    uint32_t flshcr_val = *flshcr_reg;
    flshcr_val &= ~(MASK(8) << 16);  // Clear bits 16-23
    flshcr_val |= ((flash_size & MASK(8)) << 16);  // Set new flash size
    *flshcr_reg = flshcr_val;
    
    // Configure buffer settings
    *bufcr0_reg = 0x80000000U | (flash_size & 0xFFFFU);
}

// Function to read XSPI status
uint32_t XSPI_ReadStatus(void) {
    volatile uint32_t *mcr_reg = (volatile uint32_t *)XSPI0_MCR;
    
    // Simple read operation
    return *mcr_reg;
}

// Function to set flash address
void XSPI_SetFlashAddress(uint32_t address) {
    volatile uint32_t *sfar_reg = (volatile uint32_t *)XSPI0_SFAR;
    
    // Direct write operation
    *sfar_reg = address;
}

// Function to unlock LUT (Look-Up Table)
void XSPI_UnlockLUT(void) {
    volatile uint32_t *lutkey_reg = (volatile uint32_t *)XSPI0_LUTKEY;
    
    // Write unlock sequence
    *lutkey_reg = 0x5AF05AF0U;  // Magic unlock value
}

// GPIO operations
void GPIO_SetPin(uint32_t pin) {
    volatile uint32_t *psor_reg = (volatile uint32_t *)GPIO0_PSOR;
    
    // Set specific pin - bit manipulation
    *psor_reg = BIT(pin);
}

void GPIO_ClearPin(uint32_t pin) {
    volatile uint32_t *pcor_reg = (volatile uint32_t *)GPIO0_PCOR;
    
    // Clear specific pin
    *pcor_reg = BIT(pin);
}

uint32_t GPIO_ReadPin(uint32_t pin) {
    volatile uint32_t *pdir_reg = (volatile uint32_t *)GPIO0_PDIR;
    
    // Read pin state
    return (*pdir_reg & BIT(pin)) ? 1U : 0U;
}

// Complex register access pattern
void XSPI_ComplexOperation(void) {
    volatile uint32_t *mcr_reg = (volatile uint32_t *)XSPI0_MCR;
    volatile uint32_t *ipcr_reg = (volatile uint32_t *)XSPI0_IPCR;
    volatile uint32_t *flshcr_reg = (volatile uint32_t *)XSPI0_FLSHCR;
    
    // Multi-register configuration sequence
    uint32_t mcr_val = *mcr_reg;
    mcr_val |= BIT(2) | BIT(3);  // Set bits 2 and 3
    mcr_val &= ~(BIT(4) | BIT(5)); // Clear bits 4 and 5
    *mcr_reg = mcr_val;
    
    // Configure IP command register
    *ipcr_reg = 0x12345678U;
    
    // Read back for verification
    uint32_t verify = *flshcr_reg;
    (void)verify; // Suppress unused variable warning
}

// Main function to demonstrate usage
int main(void) {
    XSPI_Init();
    XSPI_ConfigFlash(0x1000000U);  // 16MB flash
    XSPI_SetFlashAddress(0x60000000U);
    XSPI_UnlockLUT();
    
    GPIO_SetPin(5);
    GPIO_ClearPin(10);
    uint32_t pin_state = GPIO_ReadPin(15);
    
    XSPI_ComplexOperation();
    
    uint32_t status = XSPI_ReadStatus();
    
    return (int)status;
}
