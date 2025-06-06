/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*${header:start}*/
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "app.h"
#include "fsl_clock.h"
#include "fsl_pca9422.h"
#include "pmic_support.h"
#include "core1_support.h"
#include "fsl_cache.h"
#include "fsl_debug_console.h"
#include "fsl_mu.h"
#include "power_demo_config.h"
#include "fsl_iopctl.h"
/*${header:end}*/

/*${macro:start}*/
#define DEMO_LOW_POWER_RUN_VOLT 700000U /* The VDD2 voltage during low power run. Used for CPU0 DS while CPU1 active.*/
#define DEMO_CORE1_BOOT_ADDRESS 0x205A0000U
/*${macro:end}*/

/*${function:start}*/
uint32_t g_runVolt = 1000000U; /* The VDD2 voltage during normal run, 1000mV, will be update per CPU clock. */

void BOARD_ClockLPPreConfig(void)
{
    POWER_DisablePD(kPDRUNCFG_PD_FRO1); /* Make sure FRO1 is enabled. */
    BOARD_BootClockRUN_InitClockModule(kClockModule_XTAL_OSC); /* Enable SOSC used for FRO trim. */
    BOARD_BootClockRUN_InitClockModule(kClockModule_FRO0);

    /* Move clock to FRO0_DIV1. Note, for A0, AHB clock frequency must >= Flash clock frequency(ERR052440). */
    BOARD_SetXspiClock(XSPI0, 2, 1);
    BOARD_BootClockRUN_InitClockModule(kClockModule_CLK_ROOT_COMPUTE_MAIN_CLK);
}

void BOARD_ClockLPPostConfig(void)
{
}

void BOARD_ConfigPMICModes(pca9422_modecfg_t *cfg, pca9422_power_mode_t mode)
{
    assert(cfg);

    switch (mode)
    {
        case kPCA9422_ActiveModeDVS0:
        case kPCA9422_ActiveModeDVS1:
        case kPCA9422_ActiveModeDVS2:
        case kPCA9422_ActiveModeDVS3:
        case kPCA9422_ActiveModeDVS4:
        case kPCA9422_ActiveModeDVS5:
        case kPCA9422_ActiveModeDVS6:
        case kPCA9422_ActiveModeDVS7:
            cfg[mode].sw1OutVolt  = 1000000U; /* VDD2 */
            cfg[mode].sw2OutVolt  = 1100000U; /* VDDN */
            cfg[mode].sw3OutVolt  = 1000000U; /* VDD1 */
            cfg[mode].sw4OutVolt  = 1800000U;
            cfg[mode].ldo1OutVolt = 1800000U; /* 1V8 AO */
            cfg[mode].ldo2OutVolt = 1800000U;
            cfg[mode].ldo3OutVolt = 1200000U;
            cfg[mode].ldo4OutVolt = 3300000U;
            break;

        /* PMIC_MODE[1:0] = 01b */
        case kPCA9422_SleepMode:
            cfg[mode].sw1OutVolt  = 630000U;
            cfg[mode].sw2OutVolt  = 1000000U;
            cfg[mode].sw3OutVolt  = 630000U;
            cfg[mode].sw4OutVolt  = 1800000U;
            cfg[mode].ldo1OutVolt = 1800000U;
            cfg[mode].ldo2OutVolt = 1800000U;
            cfg[mode].ldo3OutVolt = 1200000U;
            cfg[mode].ldo4OutVolt = 3300000U;
            break;

        /* Note: the StandbyMode and DPStandbyMode use same register for voltage configuration. */
        case kPCA9422_StandbyMode: /* PMIC_MODE[1:0] = 10b */
        case kPCA9422_DPStandbyMode: /* PMIC_MODE[1:0] = 11b */
            cfg[mode].sw1OutVolt  = 500000U;
            cfg[mode].sw2OutVolt  = 1000000U;
            cfg[mode].sw3OutVolt  = 500000U;
            cfg[mode].sw4OutVolt  = 1800000U;
            cfg[mode].ldo1OutVolt = 1800000U;
            cfg[mode].ldo2OutVolt = 1800000U;
            cfg[mode].ldo3OutVolt = 1200000U;
            cfg[mode].ldo4OutVolt = 3300000U;
            break;

        default:
            break;
    }
}

/* Configure regulator output enable in Run mode. */
void BOARD_ConfigPMICRegEnable(pca9422_handle_t *handle)
{
    pca9422_regulatoren_t cfg;

    /* Configure Regulator Enable */
    PCA9422_GetDefaultRegEnableConfig(&cfg);

    /* All regulators enable in RUN state. */
    cfg.sw2Enable = true;
#if (DEMO_POWER_SUPPLY_OPTION == DEMO_POWER_SUPPLY_PMIC)
    cfg.sw1Enable = true;
    cfg.sw3Enable = true;
#else /* VDD1, VDD2 are supplied by internal LDO. */
    cfg.sw1Enable    = false;
    cfg.sw3Enable    = false;
#endif
    cfg.sw4Enable  = true;
    cfg.ldo1Enable = true;
    cfg.ldo2Enable = true;
    cfg.ldo3Enable = true;
    cfg.ldo4Enable = true;

    PCA9422_WriteRegEnableConfig(handle, cfg);
}

void BOARD_ConfigPMICEnMode(pca9422_handle_t *handle)
{
    pca9422_enmodecfg_t cfg;
    /* Configure ENMODE */
    PCA9422_GetDefaultEnModeConfig(&cfg);

#if (DEMO_POWER_SUPPLY_OPTION == DEMO_POWER_SUPPLY_PMIC)
    cfg.sw1OutEnMode = kPCA9422_EnmodeOnActiveSleepStandby;
    cfg.sw3OutEnMode = kPCA9422_EnmodeOnActiveSleepStandby;
#else
    cfg.sw1OutEnMode = kPCA9422_EnmodeOnActive;
    cfg.sw3OutEnMode = kPCA9422_EnmodeOnActive;
#endif
    cfg.sw2OutEnMode  = kPCA9422_EnmodeOnActiveSleepStandby;
    cfg.sw4OutEnMode  = kPCA9422_EnmodeOnActiveSleepStandby;
    cfg.ldo1OutEnMode = kPCA9422_EnmodeOnAll;
    cfg.ldo2OutEnMode = kPCA9422_EnmodeOnActiveSleepStandby;
    cfg.ldo3OutEnMode = kPCA9422_EnmodeOnActiveSleepStandby;
    cfg.ldo4OutEnMode = kPCA9422_EnmodeOnActiveSleepStandby;

    PCA9422_WriteEnModeConfig(handle, cfg);
}

void BOARD_RestorePeripheralsAfterDSR(void)
{
    DEMO_InitDebugConsole();
}

void BOARD_WaitCPU1Booted(void)
{
    RESET_ClearPeripheralReset(kMU1_RST_SHIFT_RSTn);
    MU_Init(APP_MU);

    /* Wait Core 1 is Boot Up */
    while (BOOT_FLAG != MU_GetFlags(APP_MU))
    {
    }

    MU_Deinit(APP_MU);
}

void APP_BootCore1(void)
{
    BOARD_ReleaseCore1Power();
    BOARD_InitAHBSC();
#ifdef CORE1_IMAGE_COPY_TO_RAM
    BOARD_CopyCore1Image(DEMO_CORE1_BOOT_ADDRESS);
#endif
    BOARD_BootCore1(DEMO_CORE1_BOOT_ADDRESS, DEMO_CORE1_BOOT_ADDRESS);
    BOARD_WaitCPU1Booted();
}

void BOARD_InitPowerConfig(void)
{
    pca9422_modecfg_t pca9422ModeCfg[12];
    uint32_t i;
    clock_osc32k_config_t config = {
        .bypass = false, .monitorEnable = false, .lowPowerMode = true, .cap = kCLOCK_Osc32kCapPf16};
    /* Configure OSC32K. */
    CLOCK_EnableOsc32K(&config);

#if defined(DEMO_POWER_USE_PLL) && (DEMO_POWER_USE_PLL == 0U)
    CLKCTL2->MAINPLL0PFDDOMAINEN  = 0;
    CLKCTL2->AUDIOPLL0PFDDOMAINEN = 0;
    /* Disable PLL. */
    CLOCK_DeinitMainPll();
    CLOCK_DeinitAudioPll();
#endif

    /* Disable the clock for unused modules. */
    CLOCK_DisableClock(kCLOCK_Mmu0);
    CLOCK_DisableClock(kCLOCK_Mmu1);
    CLOCK_DisableClock(kCLOCK_Pkc);
    CLOCK_DisableClock(kCLOCK_PkcRam);
    CLOCK_DisableClock(kCLOCK_Syspm0);
    CLOCK_DisableClock(kCLOCK_Syspm1);
    CLOCK_DisableClock(kCLOCK_PrinceExe);
    CLOCK_DisableClock(kCLOCK_Prince0);
    CLOCK_DisableClock(kCLOCK_Prince1);
    CLOCK_DisableClock(kCLOCK_Iopctl0);
    CLOCK_DisableClock(kCLOCK_Ocotp0);
    CLOCK_DisableClock(kCLOCK_Glikey3);
    CLOCK_DisableClock(kCLOCK_Glikey4);
    CLOCK_DisableClock(kCLOCK_Glikey5);
    CLOCK_DisableClock(kCLOCK_Hifi4AccessRamArbiter1);
    CLOCK_DisableClock(kCLOCK_MediaAccessRamArbiter0);
    CLOCK_DisableClock(kCLOCK_MediaAccessRamArbiter1);
    CLOCK_DisableClock(kCLOCK_Hifi4);
    CLOCK_DisableClock(kCLOCK_Romcp);

    CLOCK_AttachClk(kNONE_to_DSP);
    CLOCK_AttachClk(kNONE_to_TPIU);
    CLOCK_AttachClk(kNONE_to_SYSTICK);
    CLOCK_AttachClk(kNONE_to_FCCLK1);
    CLOCK_AttachClk(kNONE_to_FCCLK2);
    CLOCK_AttachClk(kNONE_to_FCCLK3);
    CLOCK_AttachClk(kNONE_to_TRNG);
    CLOCK_AttachClk(kNONE_to_SDIO0);
    CLOCK_AttachClk(kNONE_to_SDIO1);

    BOARD_InitPmic();
    for (i = 0; i < ARRAY_SIZE(pca9422ModeCfg); i++)
    {
        PCA9422_GetDefaultPowerModeConfig(&pca9422ModeCfg[i]);
    }

    for (i = 0; i < ARRAY_SIZE(pca9422ModeCfg); i++)
    {
        BOARD_ConfigPMICModes(pca9422ModeCfg, (pca9422_power_mode_t)i);
        PCA9422_WritePowerModeConfigs(&pca9422Handle, (pca9422_power_mode_t)i, pca9422ModeCfg[i]);
    }
    BOARD_ConfigPMICRegEnable(&pca9422Handle);
    BOARD_ConfigPMICEnMode(&pca9422Handle);
#if defined(DEMO_POWER_SUPPLY_OPTION) && (DEMO_POWER_SUPPLY_OPTION == DEMO_POWER_SUPPLY_PMIC)
    /* Switch to a new DVS mode before re-configuring the VDD1/VDD2 per CPU frequency. */
    BOARD_SetPmicDVSPinStatus(0x1);
    /* PMIC is used. When using On-Chip regulator, need to be changed to kVddSrc_PMC. */
    POWER_SetVddnSupplySrc(kVddSrc_PMIC);
    POWER_SetVdd1SupplySrc(kVddSrc_PMIC);
    POWER_SetVdd2SupplySrc(kVddSrc_PMIC);
    POWER_DisableRegulators(kPower_SCPC);

    POWER_SelectRunSetpoint(kRegulator_Vdd1LDO, 0U);
    POWER_SelectSleepSetpoint(kRegulator_Vdd1LDO, 0U);
#endif

    /* Keep the used resources on. */
    POWER_DisablePD(kPDRUNCFG_SHUT_SENSES_MAINCLK); /* Keep Sense shared parts clock on. */

    POWER_DisablePD(kPDRUNCFG_GATE_FRO0);           /* Just use PD bit to control FRO. */
    POWER_DisablePD(kPDRUNCFG_SHUT_RAM1_CLK);
    POWER_ApplyPD();
    POWER_DisableLPRequestMask(kPower_MaskAll); /* Let's compute control all the shared resources. */
}

static void BOARD_DisableCache(CACHE64_CTRL_Type *base)
{
    if ((base->CCR & CACHE64_CTRL_CCR_ENCACHE_MASK) == CACHE64_CTRL_CCR_ENCACHE_MASK)
    {
        /* First, push any modified contents. */
        base->CCR |= CACHE64_CTRL_CCR_PUSHW0_MASK | CACHE64_CTRL_CCR_PUSHW1_MASK | CACHE64_CTRL_CCR_GO_MASK;

        /* Wait until the cache command completes. */
        while ((base->CCR & CACHE64_CTRL_CCR_GO_MASK) != 0x00U)
        {
        }

        /* As a precaution clear the bits to avoid inadvertently re-running this command. */
        base->CCR &= ~(CACHE64_CTRL_CCR_PUSHW0_MASK | CACHE64_CTRL_CCR_PUSHW1_MASK);

        /* Now disable the cache. */
        base->CCR &= ~CACHE64_CTRL_CCR_ENCACHE_MASK;
    }
}

void BOARD_PowerConfigAfterCPU1Booted(void)
{
    /* Turn off unused resources. */
    CLOCK_DisableClock(kCLOCK_Glikey0);
    CLOCK_DisableClock(kCLOCK_Glikey1);
    CLOCK_DisableClock(kCLOCK_Glikey2);
    CLOCK_DisableClock(kCLOCK_CompAccessRamArbiter1);
    CLKCTL0->RAMCLKSEL = 0;          /* Sense access RAM arbiter0 clock. */
    CLOCK_DisableClock(kCLOCK_Cpu1); /*Let CPU1 control it's clock. */

#if (DEMO_POWER_ENABLE_DEBUG == 0U)
    CLOCK_DisableClock(kCLOCK_Dbg);
#endif

    if (!IS_XIP_XSPI0())
    {
        BOARD_DisableCache(CACHE64_CTRL0);
        CLOCK_DisableClock(kCLOCK_Cache64ctrl0);
        CLOCK_DisableClock(kCLOCK_Xspi0);
        CLOCK_AttachClk(kNONE_to_XSPI0);
        POWER_EnablePD(kPDRUNCFG_APD_XSPI0);
        POWER_EnablePD(kPDRUNCFG_PPD_XSPI0);
        POWER_ApplyPD();
    }

    if (!IS_XIP_XSPI1())
    {
        BOARD_DisableCache(CACHE64_CTRL1);
        CLOCK_DisableClock(kCLOCK_Cache64ctrl1);
        CLOCK_DisableClock(kCLOCK_Xspi1);
        CLOCK_AttachClk(kNONE_to_XSPI1);
        POWER_EnablePD(kPDRUNCFG_APD_XSPI1);
        POWER_EnablePD(kPDRUNCFG_PPD_XSPI1);
        POWER_ApplyPD();
    }
#if defined(DEMO_POWER_USE_PLL) && (DEMO_POWER_USE_PLL == 0U)
    POWER_EnablePD(kPDRUNCFG_PD_SYSXTAL);
#endif
    POWER_EnablePD(kPDRUNCFG_PD_ADC0);
    POWER_EnablePD(kPDRUNCFG_SHUT_MEDIA_MAINCLK);
    POWER_EnablePD(kPDRUNCFG_SHUT_RAM1_CLK);

    POWER_EnablePD(kPDRUNCFG_LP_DCDC);
    POWER_EnablePD(kPDRUNCFG_APD_XSPI2);
    POWER_EnablePD(kPDRUNCFG_PPD_XSPI2);
    POWER_EnablePD(kPDRUNCFG_APD_DMA0_1_PKC_ETF);
    POWER_EnablePD(kPDRUNCFG_PPD_DMA0_1_PKC_ETF);
    POWER_EnablePD(kPDRUNCFG_APD_USB0_SRAM);
    POWER_EnablePD(kPDRUNCFG_PPD_USB0_SRAM);
    POWER_EnablePD(kPDRUNCFG_APD_SDHC0_SRAM);
    POWER_EnablePD(kPDRUNCFG_PPD_SDHC0_SRAM);
    POWER_EnablePD(kPDRUNCFG_APD_SDHC1_SRAM);
    POWER_EnablePD(kPDRUNCFG_PPD_SDHC1_SRAM);
    POWER_EnablePD(kPDRUNCFG_PPD_OCOTP);

    SYSCON0->COMP_AUTOGATE_EN = 0x7U; /* MBUS_EN bit disabled to allow other master accessing RAM0. */

    POWER_EnablePD(kPDRUNCFG_DSR_VDD2N_MEDIA);
    POWER_ApplyPD();

    PMC0->PDRUNCFG1 = 0x7FFFFFFFU; /* Power down ROM, Power down or set low-power mode for HVD, LVD, GDET. 0x4020F0A4 */

    POWER_EnablePD(kPDRUNCFG_SHUT_SENSEP_MAINCLK); /* Let Sense control private parts clock. */
    POWER_EnablePD(kPDRUNCFG_PD_FRO1); /* Note: Sense boots using FRO1 and switchs to FRO2(Sense can't control FRO1). */
    POWER_EnablePD(kPDRUNCFG_PD_FRO2);

    POWER_EnablePD(kPDRUNCFG_SHUT_RAM0_CLK);                        /* Sense access RAM arbiter0 clock. */
    POWER_EnablePD(kPDRUNCFG_SHUT_RAM1_CLK);                        /* Compute access RAM arbiter1 clock. */

    POWER_EnableRunAFBB(kPower_BodyBiasVdd2 | kPower_BodyBiasVddn); /* Configure VDD2 AFBB mode during active.*/
    POWER_EnableRunNBB(kPower_BodyBiasVdd2Sram);
    POWER_EnableRunRBB(kPower_BodyBiasVdd1 | kPower_BodyBiasVdd1Sram);
    POWER_EnableSleepRBB(kPower_BodyBiasVddn | kPower_BodyBiasVdd2Sram | kPower_BodyBiasVdd2 | kPower_BodyBiasVdd1 |
                         kPower_BodyBiasVdd1Sram);

    POWER_ApplyPD();

    /* Set the four LDO setpoints per predefined CPU frequency, must in ascending order. */
    uint32_t freqs[4] = {0};
    freqs[0] = 0U; /* For DeepSleep. */
    freqs[1] = 64000000U;
    freqs[2] = SystemCoreClock; /* Only setpoint 2 and 0 are used. */
    freqs[3] = 325000000U;

    uint32_t miniVolts[4] = {0U};
    miniVolts[0] = 630000U; /* For DeepSleep. */

    POWER_ConfigRegulatorSetpointsForFreq(kRegulator_Vdd2LDO, freqs, miniVolts, 0U, 4U);

    g_runVolt = POWER_CalcVoltLevel(kRegulator_Vdd2LDO, SystemCoreClock, 0U); /* Calculate the voltage per frequency. */

#if defined(DEMO_POWER_SUPPLY_OPTION) && (DEMO_POWER_SUPPLY_OPTION == DEMO_POWER_SUPPLY_MIXED)
    /* VDDN use external PMIC supply, VDD1&VDD2 use internal LDO. */
    POWER_SetVddnSupplySrc(kVddSrc_PMIC);
    POWER_SetVdd1SupplySrc(kVddSrc_PMC);
    POWER_SetVdd2SupplySrc(kVddSrc_PMC);

    POWER_SelectRunSetpoint(kRegulator_Vdd2LDO, 2U);
    POWER_SelectSleepSetpoint(kRegulator_Vdd2LDO, 0U);
    POWER_SelectRunSetpoint(kRegulator_Vdd1LDO, 0U);
    POWER_SelectSleepSetpoint(kRegulator_Vdd1LDO, 0U);
    POWER_ApplyPD();
#elif defined(DEMO_POWER_SUPPLY_OPTION) && (DEMO_POWER_SUPPLY_OPTION == DEMO_POWER_SUPPLY_PMIC)
    POWER_SelectRunSetpoint(kRegulator_Vdd2LDO, 0U);
    POWER_SelectSleepSetpoint(kRegulator_Vdd2LDO, 0U);
    POWER_ApplyPD();

    BOARD_SetPmicVdd2Voltage(g_runVolt);
#endif
}

void DEMO_InitDebugConsole(void)
{
    uint32_t uartClkSrcFreq;

    CLOCK_AttachClk(kFRO0_DIV1_to_FCCLK0);
    CLOCK_SetClkDiv(kCLOCK_DivFcclk0Clk, 10U);

    /* Attach FC0 clock to LP_FLEXCOMM (debug console) */
    CLOCK_AttachClk(kFCCLK0_to_FLEXCOMM0);

    uartClkSrcFreq = BOARD_DEBUG_UART_CLK_FREQ;

    DbgConsole_Init(BOARD_DEBUG_UART_INSTANCE, BOARD_DEBUG_UART_BAUDRATE, BOARD_DEBUG_UART_TYPE, uartClkSrcFreq);
}

void DEMO_DeinitDebugConsole(void)
{
    DbgConsole_Deinit();
    CLOCK_AttachClk(kNONE_to_FCCLK0);
}

void BOARD_DisableIoPads()
{
    uint8_t port, pin;

    IOPCTL_PinMuxSet(4U, 11U, 0U);

    if (IS_XIP_XSPI0() == 0U)
    {
        port = 6U;
        for (pin = 0U; pin <= 12U; pin++)
        {
            IOPCTL_PinMuxSet(port, pin, 0U);
        }
    }

    if (IS_XIP_XSPI1() == 0U)
    {
        port = 5U;
        for (pin = 0U; pin <= 20U; pin++)
        {
            IOPCTL_PinMuxSet(port, pin, 0U);
        }
    }
}

void BOARD_InitHardware(void)
{
    BOARD_ConfigMPU();
    BOARD_DisableIoPads();
    BOARD_InitBootPins();
    BOARD_BootClockRUN();

    DEMO_InitDebugConsole();

    /* BE CAUTIOUS TO SET CORRECT VOLTAGE RANGE ACCORDING TO YOUR BOARD/APPLICATION. PAD SUPPLY BEYOND THE RANGE DO
       HARM TO THE SILICON. */
    POWER_SetPio2VoltRange(kPadVol_300_360);

    /* Initialze power/clock configuration. */
    BOARD_InitPowerConfig();

    /* Boot and wait CPU1 booted. */
    APP_BootCore1();

    /* After the CPU1 booted, CPU0 relinquish the domain's control over the modules, and give the other domain exclusive
     * control. */
    BOARD_PowerConfigAfterCPU1Booted();
}

#if DEMO_POWER_USE_PLL
void BOARD_DisablePll(void)
{
    /* Disable the PFD clock output first. */
    CLOCK_DeinitMainPfd(kCLOCK_Pfd0);
    CLOCK_DeinitAudioPfd(kCLOCK_Pfd3);
    /* Disable PLL. */
    CLOCK_DeinitMainPll();
    CLOCK_DeinitAudioPll();
}

void BOARD_RestorePll(void)
{
    /*Restore PLL*/
    BOARD_BootClockRUN_InitClockModule(kClockModule_MAIN_PLL0);
    BOARD_BootClockRUN_InitClockModule(kClockModule_AUDIO_PLL0);
}
#endif

/*! Disable clock for modules for cpu run only or sleep. */
static inline void BOARD_DisableClocks(void)
{
    CLOCK_DisableClock(kCLOCK_LPI2c15);

    CLOCK_DisableClock(kCLOCK_Gpio7);
    CLOCK_DisableClock(kCLOCK_Mu1);
    CLOCK_DisableClock(kCLOCK_Syscon0);
    CLOCK_DisableClock(kCLOCK_Syscon3);
    CLOCK_DisableClock(kCLOCK_Iopctl0);
    CLOCK_DisableClock(kCLOCK_Iopctl1);
    CLOCK_DisableClock(kCLOCK_Sema420);

#if (DEMO_POWER_USE_PLL == 0U) /* PLL located in VDDN. */
    if (!IS_XIP_XSPI0() && !IS_XIP_XSPI1())
    {
        CLOCK_EnableFro0ClkForDomain(kCLOCK_Vdd2CompDomainEnable);
    }
#endif
    CLOCK_EnableFroClkOutput(FRO0, kCLOCK_FroDiv1OutEn);
}

static inline void BOARD_RestoreClocks(void)
{
    /* Restore clock, power for used modules. */
    CLOCK_EnableFroClkOutput(FRO0, kCLOCK_FroDiv1OutEn | kCLOCK_FroDiv3OutEn | kCLOCK_FroDiv6OutEn);
    CLOCK_EnableFro0ClkForDomain(kCLOCK_Vdd2CompDomainEnable | kCLOCK_VddnComDomainEnable);

    CLOCK_EnableClock(kCLOCK_Syscon0);
    CLOCK_EnableClock(kCLOCK_Syscon3);
    CLOCK_EnableClock(kCLOCK_Iopctl0);
    CLOCK_EnableClock(kCLOCK_Iopctl1);
    CLOCK_EnableClock(kCLOCK_Mu1);
    CLOCK_EnableClock(kCLOCK_Sema420);
    CLOCK_EnableClock(kCLOCK_Gpio7);
    CLOCK_EnableClock(kCLOCK_LPI2c15);
}

void BOARD_RunActiveTest(void)
{
    uint32_t pinCfg[3] = {0}; /* Used for IOPCTL configuration backup. */

    DEMO_LOG("\r\nThis test mode will keep CPU in run mode but close all other unused modules for a while.\n");
    DEMO_LOG("\r\nPlease don't input any charator until the mode finished.\n");

    /* Deinit unused modules. */
    BOARD_PMIC_I2C_Deinit();
    CLOCK_AttachClk(kNONE_to_LPI2C15);
    DEMO_DeinitDebugConsole();
    CLOCK_AttachClk(kNONE_to_FCCLK0);

    pinCfg[0] = IOPCTL0->PIO[0][31];
    pinCfg[1] = IOPCTL0->PIO[1][0];
    pinCfg[2] = IOPCTL0->PIO[0][9];

    CLOCK_DisableClock(kCLOCK_Rtc);
    /* Power down unused modules. */
#if (DEMO_POWER_USE_PLL == 0U) /* PLL located in VDDN. */
    if (!IS_XIP_XSPI0() && !IS_XIP_XSPI1())
    {
        /* XSPI0 and XSPI1 memory interface located in VDDN_COM. */
        POWER_EnablePD(kPDRUNCFG_DSR_VDDN_COM);
        POWER_EnableRunRBB(kPower_BodyBiasVddn);
        POWER_EnablePD(kPDRUNCFG_SHUT_COMNN_MAINCLK);
        POWER_ApplyPD();
    }
#endif
    POWER_EnablePD(kPDRUNCFG_APD_OCOTP);
    POWER_ApplyPD();

    BOARD_DisableClocks();

    /* Note, the debug will not work anymore when the sense shared mainclk is disabled. */
    POWER_EnablePD(kPDRUNCFG_PD_LPOSC);
    POWER_EnablePD(kPDRUNCFG_SHUT_SENSES_MAINCLK);

    CLOCK_DisableClock(kCLOCK_Sleepcon0);

    /* Simulate a task. */
    uint32_t i;
    for (i = 0U; i < 500U; i++)
    {
        SDK_DelayAtLeastUs(10000U, CLOCK_GetCoreSysClkFreq());
    }

    /* Restore clock, power for used modules. */
    CLOCK_EnableClock(kCLOCK_Sleepcon0);

    POWER_DisablePD(kPDRUNCFG_PD_LPOSC);
    POWER_DisablePD(kPDRUNCFG_SHUT_SENSES_MAINCLK);

    BOARD_RestoreClocks();
    CLOCK_EnableClock(kCLOCK_Rtc);

    IOPCTL0->PIO[0][31] = pinCfg[0];
    IOPCTL0->PIO[1][0]  = pinCfg[1];
    IOPCTL0->PIO[0][9]  = pinCfg[2];
#if (DEMO_POWER_USE_PLL == 0U) /* PLL located in VDDN. */
    if (!IS_XIP_XSPI0() && !IS_XIP_XSPI1())
    {
        /* XSPI0 and XSPI1 memory interface located in VDDN_COM. */
        POWER_DisablePD(kPDRUNCFG_DSR_VDDN_COM);
        POWER_EnableRunAFBB(kPower_BodyBiasVddn);
        POWER_DisablePD(kPDRUNCFG_SHUT_COMNN_MAINCLK);
        POWER_ApplyPD();
    }
#endif
    POWER_DisablePD(kPDRUNCFG_APD_OCOTP); /* Need keep OCOTP for warm reset boot. */
    POWER_ApplyPD();
    DEMO_InitDebugConsole();
    CLOCK_AttachClk(kSENSE_BASE_to_LPI2C15);
    BOARD_PMIC_I2C_Init();
}

void BOARD_EnterSleep(void)
{
    uint32_t irqMask;

    DEMO_DeinitDebugConsole();
    CLOCK_AttachClk(kNONE_to_FCCLK0);
    BOARD_DisableClocks();

#if (DEMO_POWER_USE_PLL == 0U) /* PLL located in VDDN. */
    if (!IS_XIP_XSPI0() && !IS_XIP_XSPI1())
    {
        /* XSPI0 and XSPI1 memory interface located in VDDN_COM. */
        POWER_EnablePD(kPDRUNCFG_DSR_VDDN_COM);
        POWER_EnableRunRBB(kPower_BodyBiasVddn);
        POWER_EnablePD(kPDRUNCFG_SHUT_COMNN_MAINCLK);
        POWER_ApplyPD();
    }
#endif

    /*LPOSC and Sense shared main clock are needed for RTC. */
    /* NOTE, debug and PMC registers access require sense shared main clock. */
    irqMask = DisableGlobalIRQ();

    POWER_EnablePD(kPDRUNCFG_PD_LPOSC);
    POWER_EnablePD(kPDRUNCFG_SHUT_SENSES_MAINCLK);

    POWER_EnterSleep();

    POWER_DisablePD(kPDRUNCFG_PD_LPOSC);
    POWER_DisablePD(kPDRUNCFG_SHUT_SENSES_MAINCLK);

    EnableGlobalIRQ(irqMask);
    __ISB();

#if (DEMO_POWER_USE_PLL == 0U) /* PLL located in VDDN. */
    if (!IS_XIP_XSPI0() && !IS_XIP_XSPI1())
    {
        /* XSPI0 and XSPI1 memory interface located in VDDN_COM. */
        POWER_DisablePD(kPDRUNCFG_DSR_VDDN_COM);
        POWER_EnableRunAFBB(kPower_BodyBiasVddn);
        POWER_DisablePD(kPDRUNCFG_SHUT_COMNN_MAINCLK);
        POWER_ApplyPD();
    }
#endif

    BOARD_RestoreClocks();
    DEMO_InitDebugConsole();
}

/*! @brief Decrease the CPU frequency and supply voltage for lower power consumption.
 * @return the origin mainclk divider.
 */
static inline uint32_t BOARD_PrepareForDS(void)
{
    uint32_t mainDiv = 0U;
#if DEMO_POWER_USE_PLL
    /*
     * Special sequence is needed for the PLL power up/initialization. The application should manually handle the states
     * changes for the PLL if the PLL power states configuration are different in Active mode and Deep Sleep mode. To
     * save power and to be simple, keep the PLL on only when Compute domain is active and sense domain will not use the
     * PLL.
     */
    /* Disable Pll before enter deep sleep mode */
    BOARD_DisablePll();
#endif
#if defined(DEMO_POWER_SUPPLY_OPTION) && (DEMO_POWER_SUPPLY_OPTION == DEMO_POWER_SUPPLY_PMIC)
    /* Decrease CPU clock to decrease VDD2 supply in case sense is active. */
    mainDiv = (CLKCTL0->MAINCLKDIV & CLKCTL0_MAINCLKDIV_DIV_MASK) + 1U;

    if (!IS_XIP_XSPI0() && !IS_XIP_XSPI1())
    {
        CLOCK_SetClkDiv(kCLOCK_DivCmptMainClk, (SystemCoreClock + 31999999U) / 32000000U);
        BOARD_SetPmicVdd2Voltage(DEMO_LOW_POWER_RUN_VOLT);
    }
#endif /* DEMO_POWER_SUPPLY_OPTION */

#if (defined(BOARD_PMIC_CONFIG_USE_SEMA4) && (BOARD_PMIC_CONFIG_USE_SEMA4 != 0U))
    CLOCK_DisableClock(kCLOCK_Sema420);
#endif
    CLOCK_DisableClock(kCLOCK_LPI2c15);

    return mainDiv;
}

/*! @brief Decrease the CPU frequency and supply voltage for lower power consumption.
 * @param mainDiv the mainclk divider value.
 */
static inline void BOARD_RestoreAfterDS(uint32_t mainDiv)
{
#if (defined(BOARD_PMIC_CONFIG_USE_SEMA4) && (BOARD_PMIC_CONFIG_USE_SEMA4 != 0U))
    CLOCK_EnableClock(kCLOCK_Sema420);
#endif
    CLOCK_EnableClock(kCLOCK_LPI2c15);

#if defined(DEMO_POWER_SUPPLY_OPTION) && (DEMO_POWER_SUPPLY_OPTION == DEMO_POWER_SUPPLY_PMIC)
    if (!IS_XIP_XSPI0() && !IS_XIP_XSPI1())
    {
        /* Restore VDD2 supply and CPU clock. */
        BOARD_SetPmicVdd2Voltage(g_runVolt); /* Restore VDD2 supply. */
        CLOCK_SetClkDiv(kCLOCK_DivCmptMainClk, mainDiv);
    }

#endif /* DEMO_POWER_SUPPLY_OPTION */
#if DEMO_POWER_USE_PLL
    /* Restore Pll before enter deep sleep mode */
    BOARD_RestorePll();
#endif
}

void BOARD_EnterDeepSleep(const uint32_t exclude_from_pd[7])
{
    uint32_t mainDiv;
    mainDiv = BOARD_PrepareForDS();
    POWER_EnterDeepSleep(exclude_from_pd);
    BOARD_RestoreAfterDS(mainDiv);
}

void BOARD_RequestDSR(const uint32_t exclude_from_pd[7])
{
    uint32_t mainDiv;
    mainDiv = BOARD_PrepareForDS();
    POWER_EnterDSR(exclude_from_pd);
    BOARD_RestoreAfterDS(mainDiv);
}

void BOARD_RequestDPD(const uint32_t exclude_from_pd[7])
{
#if DEMO_POWER_USE_PLL
    /* Disable Pll. */
    BOARD_DisablePll();
#endif
    POWER_RequestDeepPowerDown(exclude_from_pd);
#if DEMO_POWER_USE_PLL
    /* Restore Pll. The code will not executed if the chip goes into DPD. */
    BOARD_RestorePll();
#endif
}

void BOARD_RequestFDPD(const uint32_t exclude_from_pd[7])
{
#if DEMO_POWER_USE_PLL
    BOARD_DisablePll();
#endif
    POWER_RequestFullDeepPowerDown(exclude_from_pd);
    /* The code will not executed if the chip goes into FDPD. */
#if DEMO_POWER_USE_PLL
    BOARD_RestorePll();
#endif
}
/*${function:end}*/
