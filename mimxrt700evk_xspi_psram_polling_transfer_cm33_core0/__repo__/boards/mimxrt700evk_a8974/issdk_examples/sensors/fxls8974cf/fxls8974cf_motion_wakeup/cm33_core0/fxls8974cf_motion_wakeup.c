/*
 * Copyright 2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file  fxls8974_motion_wakeup.c
 *  @brief The fxls8974_motion_wakeup.c file implements the ISSDK FXLS8974CF I2C sensor driver
 *         example demonstrating motion detection and Auto-Wake/Sleep features.
 */

//-----------------------------------------------------------------------
// SDK Includes
//-----------------------------------------------------------------------
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_debug_console.h"

//-----------------------------------------------------------------------
// ISSDK Includes
//-----------------------------------------------------------------------
#include "issdk_hal.h"
#include "gpio_driver.h"
#include "fxls8974_drv.h"
#include "systick_utils.h"

//-----------------------------------------------------------------------
// CMSIS Includes
//-----------------------------------------------------------------------
#include "Driver_I2C.h"

//-----------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------
#define FXLS8974_DATA_SIZE      6
#define FXLS8974_STANDBY_MODE   0
#define FXLS8974_ACTIVE_MODE    1
#define FXLS8974_INT1_GPIO     GPIO2
#define FXLS8974_INT1_PIN      14U
#define FXLS8974_INT1_IRQ      GPIO20_IRQn
#define FXLS8974_INT1_ISR      GPIO20_IRQHandler
#define BOARD_LED_GPIO         BOARD_LED_GREEN_GPIO
#define BOARD_LED_GPIO_PIN     BOARD_LED_GREEN_GPIO_PIN
//-----------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------
/*! @brief Register settings for configuring SDCD-OT for tap-detection and Auto-Wake/Sleep in interrupt mode. */
const registerwritelist_t cFxls8974AwsConfig[] = {
    /* Set Full-scale range as 4G. */
    {FXLS8974_SENS_CONFIG1, FXLS8974_SENS_CONFIG1_FSR_4G, FXLS8974_SENS_CONFIG1_FSR_MASK},
    /* Set Wake ODR as 400Hz & Sleep Mode ODR as 6.25Hz. */
    {FXLS8974_SENS_CONFIG3, FXLS8974_SENS_CONFIG3_WAKE_ODR_400HZ | FXLS8974_SENS_CONFIG3_SLEEP_ODR_6_25HZ, FXLS8974_SENS_CONFIG3_WAKE_ODR_MASK | FXLS8974_SENS_CONFIG3_SLEEP_ODR_MASK},
    /* Enable SDCD OT for all 3 axes X, Y & Z and within-thresholds event latch disabled. */
    {FXLS8974_SDCD_CONFIG1, FXLS8974_SDCD_CONFIG1_X_OT_EN_EN | FXLS8974_SDCD_CONFIG1_Y_OT_EN_EN | FXLS8974_SDCD_CONFIG1_Z_OT_EN_EN | FXLS8974_SDCD_CONFIG1_OT_ELE_DIS,
    		FXLS8974_SDCD_CONFIG1_X_OT_EN_MASK | FXLS8974_SDCD_CONFIG1_Y_OT_EN_MASK | FXLS8974_SDCD_CONFIG1_Z_OT_EN_MASK | FXLS8974_SDCD_CONFIG1_OT_ELE_MASK},
    /* Enabling SDCD and Relative Data (N) � Data (N-1) mode for transient detection */
    {FXLS8974_SDCD_CONFIG2, FXLS8974_SDCD_CONFIG2_SDCD_EN_EN | FXLS8974_SDCD_CONFIG2_REF_UPDM_SDCD_REF, FXLS8974_SDCD_CONFIG2_SDCD_EN_MASK | FXLS8974_SDCD_CONFIG2_REF_UPDM_MASK},
    /* Set the SDCD_OT debounce count to 0 */
    {FXLS8974_SDCD_OT_DBCNT, 0, 0},
    /* Set the SDCD lower and upper thresholds to +/-100mg*/
    {FXLS8974_SDCD_LTHS_LSB, 0xCC, 0},
    {FXLS8974_SDCD_LTHS_MSB, 0xFF, 0},
    {FXLS8974_SDCD_UTHS_LSB, 0x34, 0},
    {FXLS8974_SDCD_UTHS_MSB, 0x00, 0},
    /* Enable SDCD outside of thresholds event Auto-WAKE/SLEEP transition source enable. */
    {FXLS8974_SENS_CONFIG4, FXLS8974_SENS_CONFIG4_WK_SDCD_OT_EN | FXLS8974_SENS_CONFIG4_INT_POL_ACT_HIGH, FXLS8974_SENS_CONFIG4_WK_SDCD_OT_MASK | FXLS8974_SENS_CONFIG4_INT_POL_MASK},
    /* Set the ASLP count to 5sec */
    {FXLS8974_ASLP_COUNT_LSB, 0xD0, 0},
    {FXLS8974_ASLP_COUNT_MSB, 0x07, 0},
    /* Enable Interrupts for WAKE mode. */
    {FXLS8974_INT_EN, FXLS8974_INT_EN_WAKE_OUT_EN_EN, FXLS8974_INT_EN_WAKE_OUT_EN_MASK},
    {FXLS8974_INT_PIN_SEL, FXLS8974_INT_PIN_SEL_WK_OUT_INT2_DIS, FXLS8974_INT_PIN_SEL_WK_OUT_INT2_MASK},
    __END_WRITE_DATA__};

/*! @brief Read register list to read SysMode Register. */
const registerreadlist_t cFxls8974ReadSysMode[] = {{.readFrom = FXLS8974_SYS_MODE, .numBytes = 1}, __END_READ_DATA__};

/*! @brief Read register list to read INT_STATUS Register. */
const registerreadlist_t cFxls8974ReadIntStatus[] = {{.readFrom = FXLS8974_INT_STATUS, .numBytes = 1}, __END_READ_DATA__};

/*! @brief FXLS8974 Interrupt Status Register. */
const registerreadlist_t cFxls8974IntEn[] = {{.readFrom = FXLS8974_INT_STATUS, .numBytes = 1},
                                                    __END_READ_DATA__};

//-----------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------
volatile bool gFxls8974DataReady = false;

ARM_DRIVER_I2C *I2Cdrv = &I2C_S_DRIVER;
fxls8974_i2c_sensorhandle_t fxls8974Driver;
GENERIC_DRIVER_GPIO *pGpioDriver = &Driver_GPIO_KSDK;
/* Define the init structure for the interrupt pin */
gpio_pin_config_t int1_config = {
    kGPIO_DigitalInput,
    0,
};
//-----------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------
/*! -----------------------------------------------------------------------
 *  @brief       This is the Sensor Data Ready ISR implementation.
 *  @details     This function sets the flag which indicates SDCD motion wake up event occurred.
 *  @return      void  There is no return value.
 *  -----------------------------------------------------------------------*/
void FXLS8974_INT1_ISR(void)
{
    /* Clear external interrupt flag. */
    GPIO_GpioClearInterruptFlags(FXLS8974_INT1_GPIO, 1U << FXLS8974_INT1_PIN);
    /* Change state of button. */
    gFxls8974DataReady = true;
    SDK_ISR_EXIT_BARRIER;
}

/*! -----------------------------------------------------------------------
 *  @brief       Initialize FXLS8974CF Interrupt Pin and Enable IRQ
 *  @details     This function initializs FXLS8974CF interrupt pin to wake up sensor
 *  @return      void  There is no return value.
 *  -----------------------------------------------------------------------*/
void init_fxls8974_wakeup_int(void)
{
    CLOCK_EnableClock(kCLOCK_Gpio2);
    CLOCK_EnableClock(kCLOCK_Gpio0);
    GPIO_SetPinInterruptConfig(FXLS8974_INT1_GPIO, FXLS8974_INT1_PIN, kGPIO_InterruptRisingEdge);

    EnableIRQ(FXLS8974_INT1_IRQ);
    GPIO_PinInit(FXLS8974_INT1_GPIO, FXLS8974_INT1_PIN, &int1_config);
}

/*! -----------------------------------------------------------------------
 *  @brief       This is the The main function implementation.
 *  @details     This function invokes board initializes routines, then then brings up the sensor and
 *               finally enters an endless loop to continuously read available samples.
 *  @param[in]   void This is no input parameter.
 *  @return      void  There is no return value.
 *  -----------------------------------------------------------------------*/
int main(void)
{
    int32_t status;
    uint8_t whoami;
    uint8_t intStatus, eventStatus = 0;
    uint8_t sleeptowake = 0;
    uint8_t waketosleep = 0;
    uint8_t firsttransition = 1;
    uint8_t onetime_modetransition = 1;
    uint8_t int_en;

    /*! Initialize the MCU hardware. */
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_SystickEnable();
    BOARD_InitDebugConsole();

    PRINTF("\r\n ISSDK FXLS8974CF sensor driver example to detect motion event & AWS\r\n");
    /*! Initialize FXLS8974 wakeup pin used by MCX board */
    init_fxls8974_wakeup_int();

    /*! Initialize RGB LED pin used by FRDM board */
    pGpioDriver->pin_init(&GREEN_LED, GPIO_DIRECTION_OUT, NULL, NULL, NULL);
    pGpioDriver->pin_init(&RED_LED, GPIO_DIRECTION_OUT, NULL, NULL, NULL);

    /*! Initialize the I2C driver. */
    status = I2Cdrv->Initialize(I2C_S_SIGNAL_EVENT);
    if (ARM_DRIVER_OK != status)
    {
        PRINTF("\r\n I2C Initialization Failed\r\n");
        return -1;
    }

    /*! Set the I2C Power mode. */
    status = I2Cdrv->PowerControl(ARM_POWER_FULL);
    if (ARM_DRIVER_OK != status)
    {
        PRINTF("\r\n I2C Power Mode setting Failed\r\n");
        return -1;
    }

    /*! Set the I2C bus speed. */
    status = I2Cdrv->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_FAST);
    if (ARM_DRIVER_OK != status)
    {
        PRINTF("\r\n I2C Control Mode setting Failed\r\n");
        return -1;
    }

    /*! Initialize FXLS8974 sensor driver. */
    status = FXLS8974_I2C_Initialize(&fxls8974Driver, &I2C_S_DRIVER, I2C_S_DEVICE_INDEX, FXLS8974_I2C_ADDR,
                                     &whoami);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("\r\n Sensor Initialization Failed\r\n");
        return -1;
    }
    if ((FXLS8964_WHOAMI_VALUE == whoami) || (FXLS8967_WHOAMI_VALUE == whoami))
    {
    	PRINTF("\r\n Successfully Initialized Gemini with WHO_AM_I = 0x%X\r\n", whoami);
    }
    else if ((FXLS8974_WHOAMI_VALUE == whoami) || (FXLS8968_WHOAMI_VALUE == whoami))
    {
    	PRINTF("\r\n Successfully Initialized Timandra with WHO_AM_I = 0x%X\r\n", whoami);
    }
    else if ((FXLS8971_WHOAMI_VALUE == whoami) || (FXLS8961_WHOAMI_VALUE == whoami))
    {
    	PRINTF("\r\n Successfully Initialized Chiron with WHO_AM_I = 0x%X\r\n", whoami);
    }
    else if (FXLS8962_WHOAMI_VALUE == whoami)
    {
    	PRINTF("\r\n Successfully Initialized Newstein with WHO_AM_I = 0x%X\r\n", whoami);
    }
    else
    {
    	PRINTF("\r\n Bad WHO_AM_I = 0x%X\r\n", whoami);
        return -1;
    }

    /*!  Set the task to be executed while waiting for I2C transactions to complete. */
    //FXLS8974_I2C_SetIdleTask(&fxls8974Driver, (registeridlefunction_t)SMC_SetPowerModeVlpr, SMC);

    /*! Configure the FXLS8974 sensor. */
    status = FXLS8974_I2C_Configure(&fxls8974Driver, cFxls8974AwsConfig);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("\r\n FXLS8974 Sensor Configuration Failed, Err = %d\r\n", status);
        return -1;
    }
    PRINTF("\r\n Successfully Applied FXLS8974 Sensor Configuration\r\n");

    for (;;) /* Forever loop */
    {
    	eventStatus = 0;
        /*! Read new raw sensor data from the FXLS8974. */
        status = FXLS8974_I2C_ReadData(&fxls8974Driver, cFxls8974ReadSysMode, &eventStatus);
        if (ARM_DRIVER_OK != status)
        {
            return status;
        }

        if (eventStatus == FXLS8974_SYS_MODE_SYS_MODE_WAKE)
        {
#if 0 //Waiting for D2 Interrupt to occur
          if (true == gFxls8974IntFlag)
          {
            if (sleeptowake == 1)
            {
              /*! Wake Mode Detected. */
              PRINTF("\r\n Motion Detected....\r\n");
              PRINTF("\r\n Motion Wake Mode Detected....SYSMODE = %d\r\n", eventStatus);
              PRINTF("\r\n MCU woke-up on sensor motion event\r\n");
              PRINTF("\r\n Will enter sleep mode after expiration of ASLP counter = ~5sec\r\n\r\n");
              sleeptowake = 0;
            }
              pGpioDriver->set_pin(&RED_LED);
              pGpioDriver->clr_pin(&GREEN_LED);
              waketosleep = 1;
          }
#else
          /*! Read INT Status from the FXLS8974. */
          status = FXLS8974_I2C_ReadData(&fxls8974Driver, cFxls8974IntEn, &int_en);
          if (ARM_DRIVER_OK != status)
          {
              PRINTF("\r\n Read Failed. \r\n");
              return -1;
          }
          /* Check whether DRDY Interrupt was raised */
          if ((int_en & FXLS8974_INT_STATUS_SRC_DRDY_MASK) == 0x80)
          {
              if (sleeptowake == 1)
              {
                /*! Wake Mode Detected. */
                PRINTF("\r\n Motion Detected....\r\n");
                PRINTF("\r\n Motion Wake Mode Detected....SYSMODE = %d\r\n", eventStatus);
                PRINTF("\r\n MCU woke-up on sensor motion event\r\n");
                PRINTF("\r\n Will enter sleep mode after expiration of ASLP counter = ~5sec\r\n\r\n");
                sleeptowake = 0;
              }
                pGpioDriver->clr_pin(&RED_LED);
                pGpioDriver->set_pin(&GREEN_LED);
                waketosleep = 1;
            }
#endif
        }
        else
        {
           if ((waketosleep == 1) || (firsttransition == 1))
           {
             if (1 == onetime_modetransition)
             {
               onetime_modetransition = 0;
             }
             
             status = FXLS8974_I2C_ReadData(&fxls8974Driver, cFxls8974ReadIntStatus, &intStatus);
             if (ARM_DRIVER_OK != status)
             {
               return status;
             }
             PRINTF("\r\n ASLP counter expired....\r\n");
             PRINTF("\r\n Going to Sleep Mode....SYSMODE = %d\r\n", eventStatus);
             PRINTF("\r\n Putting MCU in low power sleep\r\n\r\n");
             waketosleep = 0;
             firsttransition = 0;
           }
           pGpioDriver->clr_pin(&GREEN_LED);
           pGpioDriver->set_pin(&RED_LED);
           sleeptowake = 1;
           //SMC_SetPowerModeWait(SMC);
          continue;
        }
    }
}
