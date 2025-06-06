/*
 * Copyright 2021-2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef FSL_MU_H_
#define FSL_MU_H_

#include "fsl_common.h"

/*!
 * @addtogroup mu
 * @{
 */

/******************************************************************************
 * Definitions
 *****************************************************************************/

/*!
 * @name Driver version
 * @{
 */
/*! @brief MU driver version. */
#define FSL_MU_DRIVER_VERSION (MAKE_VERSION(2, 7, 0))
/*! @} */

#define MU_CORE_INTR(intr) ((uint32_t)(intr) << 0U)
#define MU_MISC_INTR(intr) ((uint32_t)(intr) << 8U)
#define MU_TX_INTR(intr)   ((uint32_t)(intr) << 20U)
#define MU_RX_INTR(intr)   ((uint32_t)(intr) << 24U)
#define MU_GI_INTR(intr)   ((uint32_t)(intr) << 28U)

#define MU_GET_CORE_INTR(intrs) (((uint32_t)(intrs) >> 0U) & 0xFFUL)
#define MU_GET_TX_INTR(intrs)   (((uint32_t)(intrs) >> 20U) & 0xFUL)
#define MU_GET_RX_INTR(intrs)   (((uint32_t)(intrs) >> 24U) & 0xFUL)
#define MU_GET_GI_INTR(intrs)   (((uint32_t)(intrs) >> 28U) & 0xFUL)

#define MU_CORE_FLAG(flag) ((uint32_t)(flag) << 0U)
#define MU_STAT_FLAG(flag) ((uint32_t)(flag) << 8U)
#define MU_TX_FLAG(flag)   ((uint32_t)(flag) << 20U)
#define MU_RX_FLAG(flag)   ((uint32_t)(flag) << 24U)
#define MU_GI_FLAG(flag)   ((uint32_t)(flag) << 28U)

#define MU_GET_CORE_FLAG(flags) (((uint32_t)(flags) >> 0U) & 0xFFUL)
#define MU_GET_STAT_FLAG(flags) (((uint32_t)(flags) >> 8U) & 0xFFUL)
#define MU_GET_TX_FLAG(flags)   (((uint32_t)(flags) >> 20U) & 0xFUL)
#define MU_GET_RX_FLAG(flags)   (((uint32_t)(flags) >> 24U) & 0xFUL)
#define MU_GET_GI_FLAG(flags)   (((uint32_t)(flags) >> 28U) & 0xFUL)

/* General Purpose Interrupts count. */
#ifndef FSL_FEATURE_MU_GPI_COUNT
#define FSL_FEATURE_MU_GPI_COUNT 4U
#endif

/*!
 * @brief MU status flags.
 */
enum _mu_status_flags
{
    kMU_Tx0EmptyFlag = MU_TX_FLAG(1UL << 0U), /*!< TX0 empty. */
    kMU_Tx1EmptyFlag = MU_TX_FLAG(1UL << 1U), /*!< TX1 empty. */
    kMU_Tx2EmptyFlag = MU_TX_FLAG(1UL << 2U), /*!< TX2 empty. */
    kMU_Tx3EmptyFlag = MU_TX_FLAG(1UL << 3U), /*!< TX3 empty. */

    kMU_Rx0FullFlag = MU_RX_FLAG(1UL << 0U), /*!< RX0 full. */
    kMU_Rx1FullFlag = MU_RX_FLAG(1UL << 1U), /*!< RX1 full. */
    kMU_Rx2FullFlag = MU_RX_FLAG(1UL << 2U), /*!< RX2 full. */
    kMU_Rx3FullFlag = MU_RX_FLAG(1UL << 3U), /*!< RX3 full. */

    kMU_GenInt0Flag = MU_GI_FLAG(1UL << 0U), /*!< General purpose interrupt 0 pending. */
    kMU_GenInt1Flag = MU_GI_FLAG(1UL << 1U), /*!< General purpose interrupt 1 pending. */
    kMU_GenInt2Flag = MU_GI_FLAG(1UL << 2U), /*!< General purpose interrupt 2 pending. */
    kMU_GenInt3Flag = MU_GI_FLAG(1UL << 3U), /*!< General purpose interrupt 3 pending. */

#if !(defined(FSL_FEATURE_MU_NO_CEP) && (0 != FSL_FEATURE_MU_NO_CEP))
    kMU_CoreEventPendingFlag = MU_STAT_FLAG(MU_SR_CEP_MASK), /*!< The other core mode entry event pending. */
#endif
    kMU_RxFullPendingFlag  = MU_STAT_FLAG(MU_SR_RFP_MASK),  /*!< Any RX full flag is pending. */
    kMU_TxEmptyPendingFlag = MU_STAT_FLAG(MU_SR_TEP_MASK),  /*!< Any TX empty flag is pending. */
    kMU_GenIntPendingFlag  = MU_STAT_FLAG(MU_SR_GIRP_MASK), /*!< Any general interrupt flag is pending. */
    kMU_EventPendingFlag   = MU_STAT_FLAG(MU_SR_EP_MASK),   /*!< MU event pending. */
    kMU_FlagsUpdatingFlag  = MU_STAT_FLAG(MU_SR_FUP_MASK),  /*!< MU flags update is on-going. */
    kMU_MuInResetFlag      = MU_STAT_FLAG(MU_SR_MURS_MASK), /*!< MU of any side is in reset. */

#if !(defined(FSL_FEATURE_MU_HAS_SR_MURIP) && (FSL_FEATURE_MU_HAS_SR_MURIP == 0))
    kMU_MuResetInterruptFlag = MU_STAT_FLAG(MU_SR_MURIP_MASK), /*!< The other side initializes MU reset. */
#endif

#if !(defined(FSL_FEATURE_MU_NO_CORE_STATUS) && (0 != FSL_FEATURE_MU_NO_CORE_STATUS))
#if !(defined(FSL_FEATURE_MU_HAS_RUN_INT) && (FSL_FEATURE_MU_HAS_RUN_INT == 0))
    kMU_OtherSideEnterRunInterruptFlag = MU_CORE_FLAG(MU_CSSR0_RUN_MASK), /*!< The other side enters run mode. */
#endif
#if !(defined(FSL_FEATURE_MU_HAS_HALT_INT) && (FSL_FEATURE_MU_HAS_HALT_INT == 0))
    kMU_OtherSideEnterHaltInterruptFlag = MU_CORE_FLAG(MU_CSSR0_HALT_MASK), /*!< The other side enters halt mode. */
#endif
#if !(defined(FSL_FEATURE_MU_HAS_WAIT_INT) && (FSL_FEATURE_MU_HAS_WAIT_INT == 0))
    kMU_OtherSideEnterWaitInterruptFlag = MU_CORE_FLAG(MU_CSSR0_WAIT_MASK), /*!< The other side enters wait mode. */
#endif
#if !(defined(FSL_FEATURE_MU_HAS_STOP_INT) && (FSL_FEATURE_MU_HAS_STOP_INT == 0))
    kMU_OtherSideEnterStopInterruptFlag = MU_CORE_FLAG(MU_CSSR0_STOP_MASK), /*!< The other side enters stop mode. */
#endif

#if !(defined(FSL_FEATURE_MU_HAS_PD_INT) && (FSL_FEATURE_MU_HAS_PD_INT == 0))
    /*! The other side enters power down mode. */
    kMU_OtherSideEnterPowerDownInterruptFlag = MU_CORE_FLAG(MU_CSSR0_PD_MASK),
#endif

#if !(defined(FSL_FEATURE_MU_HAS_RESET_ASSERT_INT) && (FSL_FEATURE_MU_HAS_RESET_ASSERT_INT == 0))
    /*! The other core reset assert interrupt. */
    kMU_ResetAssertInterruptFlag = MU_CORE_FLAG(MU_CSSR0_RAIP_MASK),
#endif

#if !(defined(FSL_FEATURE_MU_HAS_SR_HRIP) && (FSL_FEATURE_MU_HAS_SR_HRIP == 0))
    /*! Current side has been hardware reset by the other side. */
    kMU_HardwareResetInterruptFlag = MU_CORE_FLAG(MU_CSSR0_HRIP_MASK),
#endif
#endif /* FSL_FEATURE_MU_NO_CORE_STATUS */
};

/*!
 * @brief MU interrupt source to enable.
 */
enum _mu_interrupt_enable
{
    kMU_Tx0EmptyInterruptEnable = MU_TX_INTR(1UL << 0U), /*!< TX0 empty. */
    kMU_Tx1EmptyInterruptEnable = MU_TX_INTR(1UL << 1U), /*!< TX1 empty. */
    kMU_Tx2EmptyInterruptEnable = MU_TX_INTR(1UL << 2U), /*!< TX2 empty. */
    kMU_Tx3EmptyInterruptEnable = MU_TX_INTR(1UL << 3U), /*!< TX3 empty. */

    kMU_Rx0FullInterruptEnable = MU_RX_INTR(1UL << 0U), /*!< RX0 full. */
    kMU_Rx1FullInterruptEnable = MU_RX_INTR(1UL << 1U), /*!< RX1 full. */
    kMU_Rx2FullInterruptEnable = MU_RX_INTR(1UL << 2U), /*!< RX2 full. */
    kMU_Rx3FullInterruptEnable = MU_RX_INTR(1UL << 3U), /*!< RX3 full. */

    kMU_GenInt0InterruptEnable = MU_GI_INTR(1UL << 0U), /*!< General purpose interrupt 0. */
    kMU_GenInt1InterruptEnable = MU_GI_INTR(1UL << 1U), /*!< General purpose interrupt 1. */
    kMU_GenInt2InterruptEnable = MU_GI_INTR(1UL << 2U), /*!< General purpose interrupt 2. */
    kMU_GenInt3InterruptEnable = MU_GI_INTR(1UL << 3U), /*!< General purpose interrupt 3. */

#if !(defined(FSL_FEATURE_MU_NO_CORE_STATUS) && (0 != FSL_FEATURE_MU_NO_CORE_STATUS))
#if !(defined(FSL_FEATURE_MU_HAS_RUN_INT) && (FSL_FEATURE_MU_HAS_RUN_INT == 0))
    kMU_OtherSideEnterRunInterruptEnable = MU_CORE_INTR(MU_CIER0_RUNIE_MASK), /*!< The other side enters run mode. */
#endif
#if !(defined(FSL_FEATURE_MU_HAS_HALT_INT) && (FSL_FEATURE_MU_HAS_HALT_INT == 0))
    kMU_OtherSideEnterHaltInterruptEnable = MU_CORE_INTR(MU_CIER0_HALTIE_MASK), /*!< The other side enters halt mode. */
#endif
#if !(defined(FSL_FEATURE_MU_HAS_WAIT_INT) && (FSL_FEATURE_MU_HAS_WAIT_INT == 0))
    kMU_OtherSideEnterWaitInterruptEnable = MU_CORE_INTR(MU_CIER0_WAITIE_MASK), /*!< The other side enters wait mode. */
#endif
#if !(defined(FSL_FEATURE_MU_HAS_STOP_INT) && (FSL_FEATURE_MU_HAS_STOP_INT == 0))
    kMU_OtherSideEnterStopInterruptEnable = MU_CORE_INTR(MU_CIER0_STOPIE_MASK), /*!< The other side enters stop mode. */
#endif

#if !(defined(FSL_FEATURE_MU_HAS_PD_INT) && (FSL_FEATURE_MU_HAS_PD_INT == 0))
    /*! The other side enters power down mode. */
    kMU_OtherSideEnterPowerDownInterruptEnable = MU_CORE_INTR(MU_CIER0_PDIE_MASK),
#endif

#if !(defined(FSL_FEATURE_MU_HAS_RESET_ASSERT_INT) && (FSL_FEATURE_MU_HAS_RESET_ASSERT_INT == 0))
    /*! The other core reset assert interrupt. */
    kMU_ResetAssertInterruptEnable = MU_CORE_INTR(MU_CIER0_RAIE_MASK),
#endif

#if !(defined(FSL_FEATURE_MU_HAS_SR_HRIP) && (FSL_FEATURE_MU_HAS_SR_HRIP == 0))
    /*! Current side has been hardware reset by the other side. */
    kMU_HardwareResetInterruptEnable = MU_CORE_INTR(MU_CIER0_HRIE_MASK),
#endif
#endif /* FSL_FEATURE_MU_NO_CORE_STATUS */

#if !(defined(FSL_FEATURE_MU_HAS_SR_MURIP) && (FSL_FEATURE_MU_HAS_SR_MURIP == 0))
    kMU_MuResetInterruptEnable = MU_MISC_INTR(MU_CR_MURIE_MASK), /*!< The other side initializes MU reset. */
#endif
};

/*!
 * @brief MU interrupt that could be triggered to the other core.
 */
enum _mu_interrupt_trigger
{
    kMU_GenInt0InterruptTrigger = MU_GI_INTR(1UL << 0U), /*!< General purpose interrupt 0. */
    kMU_GenInt1InterruptTrigger = MU_GI_INTR(1UL << 1U), /*!< General purpose interrupt 1. */
    kMU_GenInt2InterruptTrigger = MU_GI_INTR(1UL << 2U), /*!< General purpose interrupt 2. */
    kMU_GenInt3InterruptTrigger = MU_GI_INTR(1UL << 3U), /*!< General purpose interrupt 3. */
};

#if !(defined(FSL_FEATURE_MU_NO_CORE_STATUS) && (0 != FSL_FEATURE_MU_NO_CORE_STATUS))
/*!
 * @brief MU core status flags.
 */
enum _mu_core_status_flags
{
#if !(defined(FSL_FEATURE_MU_HAS_RUN_INT) && (FSL_FEATURE_MU_HAS_RUN_INT == 0))
    kMU_OtherSideEnterRunFlag = MU_CSSR0_RUN_MASK, /*!< The other side in run mode. */
#endif

#if !(defined(FSL_FEATURE_MU_HAS_HALT_INT) && (FSL_FEATURE_MU_HAS_HALT_INT == 0))
    kMU_OtherSideEnterHaltFlag = MU_CSSR0_HALT_MASK, /*!< The other side in halt mode. */
#endif

#if !(defined(FSL_FEATURE_MU_HAS_WAIT_INT) && (FSL_FEATURE_MU_HAS_WAIT_INT == 0))
    kMU_OtherSideEnterWaitFlag = MU_CSSR0_WAIT_MASK, /*!< The other side in wait mode. */
#endif

#if !(defined(FSL_FEATURE_MU_HAS_STOP_INT) && (FSL_FEATURE_MU_HAS_STOP_INT == 0))
    kMU_OtherSideEnterStopFlag = MU_CSSR0_STOP_MASK, /*!< The other side in stop mode. */
#endif

#if !(defined(FSL_FEATURE_MU_HAS_PD_INT) && (FSL_FEATURE_MU_HAS_PD_INT == 0))
    kMU_OtherSideEnterPowerDownFlag = MU_CSSR0_PD_MASK, /*!< The other side in power down mode. */
#endif

#if !(defined(FSL_FEATURE_MU_HAS_RESET_ASSERT_INT) && (FSL_FEATURE_MU_HAS_RESET_ASSERT_INT == 0))
    kMU_OtherSideEnterResetFlag = MU_CSSR0_RAIP_MASK, /*!< The other core entered reset. */
#endif

#if !(defined(FSL_FEATURE_MU_HAS_SR_HRIP) && (FSL_FEATURE_MU_HAS_SR_HRIP == 0))
    kMU_HardwareResetFlag = MU_CSSR0_HRIP_MASK, /*!< Current side has been hardware reset by the other side. */
#endif
};
#endif /* FSL_FEATURE_MU_NO_CORE_STATUS */

/*!
 * @brief MU message register index.
 */
typedef enum _mu_msg_reg_index
{
    kMU_MsgReg0 = 0, /*!< Message register 0. */
    kMU_MsgReg1,     /*!< Message register 1. */
    kMU_MsgReg2,     /*!< Message register 2. */
    kMU_MsgReg3,     /*!< Message register 3. */
} mu_msg_reg_index_t;

#if (defined(FSL_FEATURE_MU_HAS_BOOT) && (0 == FSL_FEATURE_MU_HAS_BOOT))
/*!
 * @brief The other core boot mode.
 */
typedef enum _mu_core_boot_mode
{
    kMU_CoreBootModeDummy = 0x0U, /*!< MU doesn't support boot mode selection, only for API compatibility. */
} mu_core_boot_mode_t;
#endif

/*!
 * @brief MU general purpose interrupts
 */
typedef enum _mu_general_purpose_interrupt
{
    kMU_GeneralPurposeInterrupt0  = 1UL << 0U,  /*!< General purpose interrupt 0  */
    kMU_GeneralPurposeInterrupt1  = 1UL << 1U,  /*!< General purpose interrupt 1  */
    kMU_GeneralPurposeInterrupt2  = 1UL << 2U,  /*!< General purpose interrupt 2  */
    kMU_GeneralPurposeInterrupt3  = 1UL << 3U,  /*!< General purpose interrupt 3  */
#if (FSL_FEATURE_MU_GPI_COUNT > 4U)
    kMU_GeneralPurposeInterrupt4  = 1UL << 4U,  /*!< General purpose interrupt 4  */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 5U)
    kMU_GeneralPurposeInterrupt5  = 1UL << 5U,  /*!< General purpose interrupt 5  */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 6U)
    kMU_GeneralPurposeInterrupt6  = 1UL << 6U,  /*!< General purpose interrupt 6  */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 7U)
    kMU_GeneralPurposeInterrupt7  = 1UL << 7U,  /*!< General purpose interrupt 7  */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 8U)
    kMU_GeneralPurposeInterrupt8  = 1UL << 8U,  /*!< General purpose interrupt 8  */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 9U)
    kMU_GeneralPurposeInterrupt9  = 1UL << 9U,  /*!< General purpose interrupt 9  */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 10U)
    kMU_GeneralPurposeInterrupt10 = 1UL << 10U, /*!< General purpose interrupt 10 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 11U)
    kMU_GeneralPurposeInterrupt11 = 1UL << 11U, /*!< General purpose interrupt 11 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 12U)
    kMU_GeneralPurposeInterrupt12 = 1UL << 12U, /*!< General purpose interrupt 12 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 13U)
    kMU_GeneralPurposeInterrupt13 = 1UL << 13U, /*!< General purpose interrupt 13 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 14U)
    kMU_GeneralPurposeInterrupt14 = 1UL << 14U, /*!< General purpose interrupt 14 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 15U)
    kMU_GeneralPurposeInterrupt15 = 1UL << 15U, /*!< General purpose interrupt 15 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 16U)
    kMU_GeneralPurposeInterrupt16 = 1UL << 16U, /*!< General purpose interrupt 16 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 17U)
    kMU_GeneralPurposeInterrupt17 = 1UL << 17U, /*!< General purpose interrupt 17 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 18U)
    kMU_GeneralPurposeInterrupt18 = 1UL << 18U, /*!< General purpose interrupt 18 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 19U)
    kMU_GeneralPurposeInterrupt19 = 1UL << 19U, /*!< General purpose interrupt 19 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 20U)
    kMU_GeneralPurposeInterrupt20 = 1UL << 20U, /*!< General purpose interrupt 20 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 21U)
    kMU_GeneralPurposeInterrupt21 = 1UL << 21U, /*!< General purpose interrupt 21 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 22U)
    kMU_GeneralPurposeInterrupt22 = 1UL << 22U, /*!< General purpose interrupt 22 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 23U)
    kMU_GeneralPurposeInterrupt23 = 1UL << 23U, /*!< General purpose interrupt 23 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 24U)
    kMU_GeneralPurposeInterrupt24 = 1UL << 24U, /*!< General purpose interrupt 24 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 25U)
    kMU_GeneralPurposeInterrupt25 = 1UL << 25U, /*!< General purpose interrupt 25 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 26U)
    kMU_GeneralPurposeInterrupt26 = 1UL << 26U, /*!< General purpose interrupt 26 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 27U)
    kMU_GeneralPurposeInterrupt27 = 1UL << 27U, /*!< General purpose interrupt 27 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 28U)
    kMU_GeneralPurposeInterrupt28 = 1UL << 28U, /*!< General purpose interrupt 28 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 29U)
    kMU_GeneralPurposeInterrupt29 = 1UL << 29U, /*!< General purpose interrupt 29 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 30U)
    kMU_GeneralPurposeInterrupt30 = 1UL << 30U, /*!< General purpose interrupt 30 */
#endif
#if (FSL_FEATURE_MU_GPI_COUNT > 31U)
    kMU_GeneralPurposeInterrupt31 = 1UL << 31U, /*!< General purpose interrupt 31 */
#endif
} mu_general_purpose_interrupt_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name MU instance.
 * @{
 */

/*!
 * @brief Get the MU instance index.
 *
 * @param base MU peripheral base address.
 * @return MU instance index.
 */
uint32_t MU_GetInstance(MU_Type *base);
/*! @} */

/*!
 * @name MU initialization.
 * @{
 */
/*!
 * @brief Initializes the MU module.
 *
 * This function enables the MU clock only.
 *
 * @param base MU peripheral base address.
 */
void MU_Init(MU_Type *base);

/*!
 * @brief De-initializes the MU module.
 *
 * This function disables the MU clock only.
 *
 * @param base MU peripheral base address.
 */
void MU_Deinit(MU_Type *base);

/*! @} */

/*!
 * @name MU Message
 * @{
 */

/*!
 * @brief Writes a message to the TX register.
 *
 * This function writes a message to the specific TX register. It does not check
 * whether the TX register is empty or not. The upper layer should make sure the TX
 * register is empty before calling this function. This function can be used
 * in ISR for better performance.
 *
 * @code
 * while (!(kMU_Tx0EmptyFlag & MU_GetStatusFlags(base))) { }  Wait for TX0 register empty.
 * MU_SendMsgNonBlocking(base, kMU_MsgReg0, MSG_VAL);  Write message to the TX0 register.
 * @endcode
 *
 * @param base MU peripheral base address.
 * @param regIndex  TX register index, see @ref mu_msg_reg_index_t.
 * @param msg      Message to send.
 */
static inline void MU_SendMsgNonBlocking(MU_Type *base, uint32_t regIndex, uint32_t msg)
{
    assert(regIndex < MU_TR_COUNT);

    base->TR[regIndex] = msg;
}

/*!
 * @brief Blocks to send a message.
 *
 * This function waits until the TX register is empty and sends the message.
 *
 * @param base MU peripheral base address.
 * @param regIndex  MU message register, see @ref mu_msg_reg_index_t.
 * @param msg      Message to send.
 */
void MU_SendMsg(MU_Type *base, uint32_t regIndex, uint32_t msg);

/*!
 * @brief Reads a message from the RX register.
 *
 * This function reads a message from the specific RX register. It does not check
 * whether the RX register is full or not. The upper layer should make sure the RX
 * register is full before calling this function. This function can be used
 * in ISR for better performance.
 *
 * @code
 * uint32_t msg;
 * while (!(kMU_Rx0FullFlag & MU_GetStatusFlags(base)))
 * {
 * }  Wait for the RX0 register full.
 *
 * msg = MU_ReceiveMsgNonBlocking(base, kMU_MsgReg0);  Read message from RX0 register.
 * @endcode
 *
 * @param base MU peripheral base address.
 * @param regIndex  RX register index, see @ref mu_msg_reg_index_t.
 * @return The received message.
 */
static inline uint32_t MU_ReceiveMsgNonBlocking(MU_Type *base, uint32_t regIndex)
{
    assert(regIndex < MU_TR_COUNT);

    return base->RR[regIndex];
}

/*!
 * @brief Blocks to receive a message.
 *
 * This function waits until the RX register is full and receives the message.
 *
 * @param base MU peripheral base address.
 * @param regIndex  MU message register, see @ref mu_msg_reg_index_t
 * @return The received message.
 */
uint32_t MU_ReceiveMsg(MU_Type *base, uint32_t regIndex);

/*! @} */

/*!
 * @name MU Flags
 * @{
 */

/*!
 * @brief Sets the 3-bit MU flags reflect on the other MU side.
 *
 * This function sets the 3-bit MU flags directly. Every time the 3-bit MU flags are changed,
 * the status flag \c kMU_FlagsUpdatingFlag asserts indicating the 3-bit MU flags are
 * updating to the other side. After the 3-bit MU flags are updated, the status flag
 * \c kMU_FlagsUpdatingFlag is cleared by hardware. During the flags updating period,
 * the flags cannot be changed. The upper layer should make sure the status flag
 * \c kMU_FlagsUpdatingFlag is cleared before calling this function.
 *
 * @code
 * while (kMU_FlagsUpdatingFlag & MU_GetStatusFlags(base))
 * {
 * }  Wait for previous MU flags updating.
 *
 * MU_SetFlagsNonBlocking(base, 0U);  Set the mU flags.
 * @endcode
 *
 * @param base MU peripheral base address.
 * @param flags The 3-bit MU flags to set.
 */
static inline void MU_SetFlagsNonBlocking(MU_Type *base, uint32_t flags)
{
    base->FCR = flags;
}

/*!
 * @brief Blocks setting the 3-bit MU flags reflect on the other MU side.
 *
 * This function blocks setting the 3-bit MU flags. Every time the 3-bit MU flags are changed,
 * the status flag \c kMU_FlagsUpdatingFlag asserts indicating the 3-bit MU flags are
 * updating to the other side. After the 3-bit MU flags are updated, the status flag
 * \c kMU_FlagsUpdatingFlag is cleared by hardware. During the flags updating period,
 * the flags cannot be changed. This function waits for the MU status flag
 * \c kMU_FlagsUpdatingFlag cleared and sets the 3-bit MU flags.
 *
 * @param base MU peripheral base address.
 * @param flags The 3-bit MU flags to set.
 */
void MU_SetFlags(MU_Type *base, uint32_t flags);

/*!
 * @brief Gets the current value of the 3-bit MU flags set by the other side.
 *
 * This function gets the current 3-bit MU flags on the current side.
 *
 * @param base MU peripheral base address.
 * @return flags   Current value of the 3-bit flags.
 */
static inline uint32_t MU_GetFlags(MU_Type *base)
{
    return base->FSR;
}

/*! @} */

/*!
 * @name Status and Interrupt.
 * @{
 */

#if !(defined(FSL_FEATURE_MU_NO_CORE_STATUS) && (0 != FSL_FEATURE_MU_NO_CORE_STATUS))
/*!
 * @brief Gets the MU core status flags.
 *
 * @param base MU peripheral base address.
 * @return      Bit mask of the MU status flags, see @ref _mu_core_status_flags.
 */
static inline uint32_t MU_GetCoreStatusFlags(MU_Type *base)
{
    return base->CSR0;
}
#endif /* FSL_FEATURE_MU_NO_CORE_STATUS */

/*!
 * @brief Gets the MU status flags.
 *
 * This function returns the bit mask of the MU status flags. See _mu_status_flags.
 *
 * @code
 * uint32_t flags;
 * flags = MU_GetStatusFlags(base);  Get all status flags.
 * if (kMU_Tx0EmptyFlag & flags)
 * {
 *     The TX0 register is empty. Message can be sent.
 *     MU_SendMsgNonBlocking(base, kMU_MsgReg0, MSG0_VAL);
 * }
 * if (kMU_Tx1EmptyFlag & flags)
 * {
 *     The TX1 register is empty. Message can be sent.
 *     MU_SendMsgNonBlocking(base, kMU_MsgReg1, MSG1_VAL);
 * }
 * @endcode
 *
 * If there are more than 4 general purpose interrupts, use @ref MU_GetGeneralPurposeStatusFlags.
 *
 * @param base MU peripheral base address.
 * @return      Bit mask of the MU status flags, see _mu_status_flags.
 */
uint32_t MU_GetStatusFlags(MU_Type *base);

/*!
 * @brief Gets the MU IRQ pending status of enabled interrupts.
 *
 * This function returns the bit mask of the pending MU IRQs of enabled interrupts.
 * Only these flags are checked.
 *  kMU_Tx0EmptyFlag
 *  kMU_Tx1EmptyFlag
 *  kMU_Tx2EmptyFlag
 *  kMU_Tx3EmptyFlag
 *  kMU_Rx0FullFlag
 *  kMU_Rx1FullFlag
 *  kMU_Rx2FullFlag
 *  kMU_Rx3FullFlag
 *  kMU_GenInt0Flag
 *  kMU_GenInt1Flag
 *  kMU_GenInt2Flag
 *  kMU_GenInt3Flag
 *
 * @param base MU peripheral base address.
 * @return  Bit mask of the MU IRQs pending.
 */
static inline uint32_t MU_GetInterruptsPending(MU_Type *base)
{
    uint32_t flag = 0;
    uint32_t mask = 0;

    /* TX */
    flag |= MU_TX_FLAG(base->TSR);
    mask |= MU_TX_INTR(base->TCR);

    /* RX */
    flag |= MU_RX_FLAG(base->RSR);
    mask |= MU_RX_INTR(base->RCR);

    /* General purpose interrupt */
    flag |= MU_GI_FLAG(base->GSR);
    mask |= MU_GI_INTR(base->GCR);

    return (flag & mask);
}

/*!
 * @brief Clears the specific MU status flags.
 *
 * This function clears the specific MU status flags. The flags to clear should
 * be passed in as bit mask. See _mu_status_flags.
 *
 * @code
 * Clear general interrupt 0 and general interrupt 1 pending flags.
 * MU_ClearStatusFlags(base, kMU_GenInt0Flag | kMU_GenInt1Flag);
 * @endcode
 *
 * If there are more than 4 general purpose interrupts, use @ref MU_ClearGeneralPurposeStatusFlags.
 *
 * @param base MU peripheral base address.
 * @param flags Bit mask of the MU status flags. See _mu_status_flags. Only the
 * following flags can be cleared by software, other flags are cleared by hardware:
 * - #kMU_GenInt0Flag
 * - #kMU_GenInt1Flag
 * - #kMU_GenInt2Flag
 * - #kMU_GenInt3Flag
 * - #kMU_MuResetInterruptFlag
 * - #kMU_OtherSideEnterRunInterruptFlag
 * - #kMU_OtherSideEnterHaltInterruptFlag
 * - #kMU_OtherSideEnterWaitInterruptFlag
 * - #kMU_OtherSideEnterStopInterruptFlag
 * - #kMU_OtherSideEnterPowerDownInterruptFlag
 * - #kMU_ResetAssertInterruptFlag
 * - #kMU_HardwareResetInterruptFlag
 */
static inline void MU_ClearStatusFlags(MU_Type *base, uint32_t flags)
{
    uint32_t tmp;

    /* General interrupt tmp. */
    tmp = MU_GET_GI_FLAG(flags);
    if (0U != tmp)
    {
        base->GSR = tmp;
    }

#if !(defined(FSL_FEATURE_MU_NO_CORE_STATUS) && (0 != FSL_FEATURE_MU_NO_CORE_STATUS))
    /* Core interrupt. */
    tmp = MU_GET_CORE_FLAG(flags);
    if (0U != tmp)
    {
        base->CSSR0 = tmp;
    }
#endif /* FSL_FEATURE_MU_NO_CORE_STATUS */

#if !(defined(FSL_FEATURE_MU_HAS_SR_MURIP) && (FSL_FEATURE_MU_HAS_SR_MURIP == 0))
    /* kMU_MuResetInterruptFlag. */
    if (0U != ((uint32_t)kMU_MuResetInterruptFlag & flags))
    {
        base->SR = MU_SR_MURIP_MASK;
    }
#endif
}

/*!
 * @brief Enables the specific MU interrupts.
 *
 * This function enables the specific MU interrupts. The interrupts to enable
 * should be passed in as bit mask. See _mu_interrupt_enable.
 *
 * @code
 *    Enable general interrupt 0 and TX0 empty interrupt.
 * MU_EnableInterrupts(base, kMU_GenInt0InterruptEnable | kMU_Tx0EmptyInterruptEnable);
 * @endcode
 *
 * If there are more than 4 general purpose interrupts, use @ref MU_EnableGeneralPurposeInterrupts.
 *
 * @param base MU peripheral base address.
 * @param interrupts  Bit mask of the MU interrupts. See _mu_interrupt_enable.
 */
static inline void MU_EnableInterrupts(MU_Type *base, uint32_t interrupts)
{
    uint32_t tmp;

    /* TX message interrupts. */
    tmp = MU_GET_TX_INTR(interrupts);
    if (0U != tmp)
    {
        base->TCR |= tmp;
    }

    /* RX message interrupts. */
    tmp = MU_GET_RX_INTR(interrupts);
    if (0U != tmp)
    {
        base->RCR |= tmp;
    }

    /* General purpose interrupts. */
    tmp = MU_GET_GI_INTR(interrupts);
    if (0U != tmp)
    {
        base->GIER |= tmp;
    }

    /* Core interrupts. */
    tmp = MU_GET_CORE_INTR(interrupts);
    if (0U != tmp)
    {
        base->CIER0 |= tmp;
    }

#if !(defined(FSL_FEATURE_MU_HAS_SR_MURIP) && (FSL_FEATURE_MU_HAS_SR_MURIP == 0))
    if (0U != ((uint32_t)kMU_MuResetInterruptEnable & interrupts))
    {
        base->CR |= MU_CR_MURIE_MASK;
    }
#endif
}

/*!
 * @brief Disables the specific MU interrupts.
 *
 * This function disables the specific MU interrupts. The interrupts to disable
 * should be passed in as bit mask. See _mu_interrupt_enable.
 *
 * @code
 *    Disable general interrupt 0 and TX0 empty interrupt.
 * MU_DisableInterrupts(base, kMU_GenInt0InterruptEnable | kMU_Tx0EmptyInterruptEnable);
 * @endcode
 *
 * If there are more than 4 general purpose interrupts, use @ref MU_DisableGeneralPurposeInterrupts.
 *
 * @param base MU peripheral base address.
 * @param interrupts  Bit mask of the MU interrupts. See _mu_interrupt_enable.
 */
static inline void MU_DisableInterrupts(MU_Type *base, uint32_t interrupts)
{
    uint32_t tmp;

    /* TX message interrupts. */
    tmp = MU_GET_TX_INTR(interrupts);
    if (0U != tmp)
    {
        base->TCR &= ~tmp;
    }

    /* RX message interrupts. */
    tmp = MU_GET_RX_INTR(interrupts);
    if (0U != tmp)
    {
        base->RCR &= ~tmp;
    }

    /* General purpose interrupts. */
    tmp = MU_GET_GI_INTR(interrupts);
    if (0U != tmp)
    {
        base->GIER &= ~tmp;
    }

    /* Core interrupts. */
    tmp = MU_GET_CORE_INTR(interrupts);
    if (0U != tmp)
    {
        base->CIER0 &= ~tmp;
    }

#if !(defined(FSL_FEATURE_MU_HAS_SR_MURIP) && (FSL_FEATURE_MU_HAS_SR_MURIP == 0))
    if (0U != ((uint32_t)kMU_MuResetInterruptEnable & interrupts))
    {
        base->CR &= ~MU_CR_MURIE_MASK;
    }
#endif
}

/*!
 * @brief Triggers interrupts to the other core.
 *
 * This function triggers the specific interrupts to the other core. The interrupts
 * to trigger are passed in as bit mask. See \ref _mu_interrupt_trigger.
 * The MU should not trigger an interrupt to the other core when the previous interrupt
 * has not been processed by the other core. This function checks whether the
 * previous interrupts have been processed. If not, it returns an error.
 *
 * @code
 * if (kStatus_Success != MU_TriggerInterrupts(base, kMU_GenInt0InterruptTrigger | kMU_GenInt2InterruptTrigger))
 * {
 *      Previous general purpose interrupt 0 or general purpose interrupt 2
 *      has not been processed by the other core.
 * }
 * @endcode
 *
 * If there are more than 4 general purpose interrupts, use @ref MU_TriggerGeneralPurposeInterrupts.
 *
 * @param base MU peripheral base address.
 * @param interrupts Bit mask of the interrupts to trigger. See _mu_interrupt_trigger.
 * @retval kStatus_Success    Interrupts have been triggered successfully.
 * @retval kStatus_Fail       Previous interrupts have not been accepted.
 */
status_t MU_TriggerInterrupts(MU_Type *base, uint32_t interrupts);

#if !(defined(FSL_FEATURE_MU_NO_NMI) && (0 != FSL_FEATURE_MU_NO_NMI))
/*!
 * @brief Triggers NMI to the other core.
 *
 * This function triggers the NMI to the other core.
 * The MU should not trigger NMI to the other core when the previous interrupt
 * has not been processed by the other core. This function checks whether the
 * previous interrupts have been processed. If not, it returns an error.
 *
 * @param base MU peripheral base address.
 * @retval kStatus_Success    Interrupts have been triggered successfully.
 * @retval kStatus_Fail       Previous interrupts have not been accepted.
 */
status_t MU_TriggerNmi(MU_Type *base);

/*!
 * @brief Clear non-maskable interrupt (NMI) sent by the other core.
 *
 * This function clears non-maskable interrupt (NMI) sent by the other core.
 *
 * @param base MU peripheral base address.
 */
static inline void MU_ClearNmi(MU_Type *base)
{
    base->CSSR0 = MU_CSSR0_NMIC_MASK;
}
#endif /* FSL_FEATURE_MU_NO_NMI */

/*! @} */

/*!
 * @name MU general purpose interrupt
 * @{
 */

/*!
 * @brief Enables the MU general purpose interrupts.
 *
 * This function enables the MU general purpose interrupts. The interrupts
 * to enable should be passed in as bit mask of @ref mu_general_purpose_interrupt_t.
 * The function @ref MU_EnableInterrupts only support general interrupt 0~3,
 * this function supports all general interrupts.
 *
 * For example, to enable general purpose interrupt 0 and 3, use like this:
 * @code
   MU_EnableGeneralPurposeInterrupts(MU, kMU_GeneralPurposeInterrupt0 | kMU_GeneralPurposeInterrupt3);
   @endcode
 *
 * @param base MU peripheral base address.
 * @param interrupts  Bit mask of the MU general purpose interrupts,
 * see @ref mu_general_purpose_interrupt_t.
 */
static inline void MU_EnableGeneralPurposeInterrupts(MU_Type *base, uint32_t interrupts)
{
    base->GIER |= interrupts;
}

/*!
 * @brief Disables the MU general purpose interrupts.
 *
 * This function disables the MU general purpose interrupts. The interrupts
 * to disable should be passed in as bit mask of @ref mu_general_purpose_interrupt_t.
 * The function @ref MU_DisableInterrupts only support general interrupt 0~3,
 * this function supports all general interrupts.
 *
 * For example, to disable general purpose interrupt 0 and 3, use like this:
 * @code
   MU_EnableGeneralPurposeInterrupts(MU, kMU_GeneralPurposeInterrupt0 | kMU_GeneralPurposeInterrupt3);
   @endcode
 *
 * @param base MU peripheral base address.
 * @param interrupts  Bit mask of the MU general purpose interrupts.
 * see @ref mu_general_purpose_interrupt_t.
 */
static inline void MU_DisableGeneralPurposeInterrupts(MU_Type *base, uint32_t interrupts)
{
    base->GIER &= ~interrupts;
}

/*!
 * @brief Gets the MU general purpose interrupt status flags.
 *
 * This function returns the bit mask of the MU general purpose interrupt status flags.
 * @ref MU_GetStatusFlags can only get general purpose interrupt status 0~3,
 * this function can get all general purpose interrupts status.
 *
 * This example shows to check whether general purpose interrupt 0 and 3 happened.
 *
 * @code
   uint32_t flags;
   flags = MU_GetGeneralPurposeStatusFlags(base);
   if (kMU_GeneralPurposeInterrupt0 & flags)
   {
   }
   if (kMU_GeneralPurposeInterrupt3 & flags)
   {
   }
   @endcode
 *
 * @param base MU peripheral base address.
 * @return      Bit mask of the MU general purpose interrupt status flags.
 */
static inline uint32_t MU_GetGeneralPurposeStatusFlags(MU_Type *base)
{
    return base->GSR;
}

/*!
 * @brief Clear the MU general purpose interrupt status flags.
 *
 * This function clears the specific MU general purpose interrupt status flags. The flags to clear should
 * be passed in as bit mask. mu_general_purpose_interrupt_t_mu_status_flags.
 *
 * Example to clear general purpose interrupt 0 and general interrupt 1 pending flags.
 * @code
   MU_ClearGeneralPurposeStatusFlags(base, kMU_GeneralPurposeInterrupt0 | kMU_GeneralPurposeInterrupt1);
   @endcode
 *
 * @param base MU peripheral base address.
 * @param flags Bit mask of the MU general purpose interrupt status flags.
 * See @ref mu_general_purpose_interrupt_t.
 */
static inline void MU_ClearGeneralPurposeStatusFlags(MU_Type *base, uint32_t flags)
{
    base->GSR = flags;
}

/*!
 * @brief Return the RX status flags in reverse numerical order.
 *
 * This function return the RX status flags in reverse order.
 * Note: RFn bits of SR[3-0](mu status register) are
 * mapped in ascending numerical order:
 *      RF0 -> SR[0]
 *      RF1 -> SR[1]
 *      RF2 -> SR[2]
 *      RF3 -> SR[3]
 * This function will return these bits in reverse numerical
 * order(RF3->RF1) to comply with MU_GetRxStatusFlags() of
 * mu driver. See MU_GetRxStatusFlags() from drivers/mu/fsl_mu.h
 *
 * @code
 * status_reg = MU_GetRxStatusFlags(base);
 * @endcode
 *
 * @param base MU peripheral base address.
 * @return MU RX status flags in reverse order
 */

static inline uint32_t MU_GetRxStatusFlags(MU_Type *base)
{
    uint32_t flags = 0U;
    flags = MU_GET_RX_FLAG(MU_GetStatusFlags(base));
    flags = (((flags >> MU_RSR_RF3_SHIFT) << MU_RSR_RF0_SHIFT) |
             ((flags >> MU_RSR_RF2_SHIFT) << MU_RSR_RF1_SHIFT) |
             ((flags >> MU_RSR_RF1_SHIFT) << MU_RSR_RF2_SHIFT) |
             ((flags >> MU_RSR_RF0_SHIFT) << MU_RSR_RF3_SHIFT))
             & 0x0000000FU;

    return flags;
}

/*!
 * @brief Triggers general purpose interrupts to the other core.
 *
 * This function triggers the specific general purpose interrupts to the other core.
 * The interrupts to trigger are passed in as bit mask. See @ref mu_general_purpose_interrupt_t.
 * The MU should not trigger an interrupt to the other core when the previous interrupt
 * has not been processed by the other core. This function checks whether the
 * previous interrupts have been processed. If not, it returns an error.
 *
 * @code
   status_t status;
   status = MU_TriggerGeneralPurposeInterrupts(base, kMU_GeneralPurposeInterrupt0 | kMU_GeneralPurposeInterrupt2);

   if (kStatus_Success != status)
   {
        Previous general purpose interrupt 0 or general purpose interrupt 2
        has not been processed by the other core.
   }
   @endcode
 *
 * @param base MU peripheral base address.
 * @param interrupts Bit mask of the interrupts to trigger. See @ref mu_general_purpose_interrupt_t.
 * @retval kStatus_Success    Interrupts have been triggered successfully.
 * @retval kStatus_Fail       Previous interrupts have not been accepted.
 */
status_t MU_TriggerGeneralPurposeInterrupts(MU_Type *base, uint32_t interrupts);

/*! @} */

/*!
 * @name MU misc functions
 * @{
 */

#if !(defined(FSL_FEATURE_MU_HAS_BOOT) && (0 == FSL_FEATURE_MU_HAS_BOOT))
/*!
 * @brief Boots the other core.
 *
 * This function boots the other core with a boot configuration.
 *
 * @param base MU peripheral base address.
 * @param mode The other core boot mode.
 */
void MU_BootOtherCore(MU_Type *base, mu_core_boot_mode_t mode);
#endif /* FSL_FEATURE_MU_HAS_BOOT */

#if !(defined(FSL_FEATURE_MU_HAS_RSTH) && (0 == FSL_FEATURE_MU_HAS_RSTH))
/*!
 * @brief Holds the other core reset.
 *
 * This function causes the other core to be held in reset following any reset event.
 *
 * @param base MU peripheral base address.
 */
void MU_HoldOtherCoreReset(MU_Type *base);
#endif /* FSL_FEATURE_MU_HAS_RSTH */

/*!
 * @brief Resets the MU for both A side and B side.
 *
 * This function resets the MU for both A side and B side. Before reset, it is
 * recommended to interrupt processor B, because this function may affect the
 * ongoing processor B programs.
 *
 * @param base MU peripheral base address.
 */
static inline void MU_ResetBothSides(MU_Type *base)
{
    base->CR |= MU_CR_MUR_MASK;

    while (0U != (base->SR & MU_SR_MURS_MASK))
    {
    }
}

#if !(defined(FSL_FEATURE_MU_NO_CLKE) && (0 != FSL_FEATURE_MU_NO_CLKE))
/*!
 * @brief Enables or disables the clock on the other core.
 *
 * This function enables or disables the platform clock on the other core when
 * that core enters a stop mode. If disabled, the platform clock for the other
 * core is disabled when it enters stop mode. If enabled, the platform clock
 * keeps running on the other core in stop mode, until this core also enters
 * stop mode.
 *
 * @param base MU peripheral base address.
 * @param enable   Enable or disable the clock on the other core.
 */
static inline void MU_SetClockOnOtherCoreEnable(MU_Type *base, bool enable)
{
    if (enable)
    {
        base->CCR0 |= MU_CCR0_CLKE_MASK;
    }
    else
    {
        base->CCR0 &= ~MU_CCR0_CLKE_MASK;
    }
}
#endif

#if !(defined(FSL_FEATURE_MU_HAS_HR) && (FSL_FEATURE_MU_HAS_HR == 0))
/*!
 * @brief Hardware reset the other core.
 *
 * This function resets the other core, the other core could mask the
 * hardware reset by calling MU_MaskHardwareReset. The hardware reset
 * mask feature is only available for some platforms.
 * This function could be used together with MU_BootOtherCore to control the
 * other core reset workflow.
 *
 * Example 1: Reset the other core, and no hold reset
 * @code
 * MU_HardwareResetOtherCore(MU_A, true, false, bootMode);
 * @endcode
 * In this example, the core at MU side B will reset with the specified boot mode.
 *
 * Example 2: Reset the other core and hold it, then boot the other core later.
 * @code
 *  Here the other core enters reset, and the reset is hold
 * MU_HardwareResetOtherCore(MU_A, true, true, modeDontCare);
 *  Current core boot the other core when necessary.
 * MU_BootOtherCore(MU_A, bootMode);
 * @endcode
 *
 * @note The feature waitReset, holdReset, and bootMode might be not supported
 * for some platforms. waitReset is only available for platforms that
 * FSL_FEATURE_MU_NO_CORE_STATUS not defined as 1 and
 * FSL_FEATURE_MU_HAS_RESET_ASSERT_INT not defined as 0. holdReset is only available
 * for platforms that FSL_FEATURE_MU_HAS_RSTH not defined as 0.
 * bootMode is only available for platforms that FSL_FEATURE_MU_HAS_BOOT not
 * defined as 0.
 *
 * @param base MU peripheral base address.
 * @param waitReset Wait the other core enters reset. Only work when there is CSSR0[RAIP]
 *                    - true: Wait until the other core enters reset, if the other
 *                      core has masked the hardware reset, then this function will
 *                      be blocked.
 *                    - false: Don't wait the reset.
 * @param holdReset Hold the other core reset or not. Only work when there is CCR0[RSTH]
 *                    - true: Hold the other core in reset, this function returns
 *                      directly when the other core enters reset.
 *                    - false: Don't hold the other core in reset, this function
 *                      waits until the other core out of reset.
 * @param bootMode Boot mode of the other core, if @p holdReset is true, this
 *                 parameter is useless.
 */
void MU_HardwareResetOtherCore(MU_Type *base, bool waitReset, bool holdReset, mu_core_boot_mode_t bootMode);
#endif /* FSL_FEATURE_MU_HAS_HR */

#if (defined(FSL_FEATURE_MU_HAS_HRM) && FSL_FEATURE_MU_HAS_HRM)
/*!
 * @brief Mask hardware reset by the other core.
 *
 * The other core could call MU_HardwareResetOtherCore() to reset current core.
 * To mask the reset, call this function and pass in true.
 *
 * @param base MU peripheral base address.
 * @param mask Pass true to mask the hardware reset, pass false to unmask it.
 */
static inline void MU_MaskHardwareReset(MU_Type *base, bool mask)
{
#if !(defined(FSL_FEATURE_MU_NO_NMI) && (0 != FSL_FEATURE_MU_NO_NMI))
    if (mask)
    {
        base->CCR0 |= (base->CCR0 & ~(MU_CCR0_HR_MASK | MU_CCR0_NMI_MASK)) | MU_CCR0_HRM_MASK;
    }
    else
    {
        base->CCR0 |= (base->CCR0 & ~(MU_CCR0_HR_MASK | MU_CCR0_NMI_MASK | MU_CCR0_HRM_MASK));
    }
#else
    if (mask)
    {
        base->CCR0 |= (base->CCR0 & ~(MU_CCR0_HR_MASK)) | MU_CCR0_HRM_MASK;
    }
    else
    {
        base->CCR0 |= (base->CCR0 & ~(MU_CCR0_HR_MASK | MU_CCR0_HRM_MASK));
    }
#endif
}
#endif /* FSL_FEATURE_MU_HAS_HRM */

/*! @} */

#if defined(__cplusplus)
}
#endif /*_cplusplus*/
/*! @} */

#endif /* FSL_MU_H_*/
