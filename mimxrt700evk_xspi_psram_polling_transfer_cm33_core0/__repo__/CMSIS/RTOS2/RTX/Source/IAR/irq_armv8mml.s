;/*
; * Copyright (c) 2016-2023 Arm Limited. All rights reserved.
; *
; * SPDX-License-Identifier: Apache-2.0
; *
; * Licensed under the Apache License, Version 2.0 (the License); you may
; * not use this file except in compliance with the License.
; * You may obtain a copy of the License at
; *
; * www.apache.org/licenses/LICENSE-2.0
; *
; * Unless required by applicable law or agreed to in writing, software
; * distributed under the License is distributed on an AS IS BASIS, WITHOUT
; * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; * See the License for the specific language governing permissions and
; * limitations under the License.
; *
; * -----------------------------------------------------------------------------
; *
; * Project:     CMSIS-RTOS RTX
; * Title:       ARMv8-M Mainline Exception handlers
; *
; * -----------------------------------------------------------------------------
; */


                NAME     irq_armv8mml.s


                #include "rtx_def.h"

#ifdef __ARMVFP__
FPU_USED        EQU      1
#else
FPU_USED        EQU      0
#endif

#if (defined(__ARM_FEATURE_MVE) && (__ARM_FEATURE_MVE > 0))
MVE_USED        EQU      1
#else
MVE_USED        EQU      0
#endif

I_T_RUN_OFS     EQU      20                     ; osRtxInfo.thread.run offset
TCB_SM_OFS      EQU      48                     ; TCB.stack_mem offset
TCB_SP_OFS      EQU      56                     ; TCB.SP offset
TCB_SF_OFS      EQU      34                     ; TCB.stack_frame offset
TCB_TZM_OFS     EQU      64                     ; TCB.tz_memory offset
TCB_ZONE_OFS    EQU      68                     ; TCB.zone offset

FPCCR           EQU      0xE000EF34             ; FPCCR Address

osRtxErrorStackOverflow\
                EQU      1                      ; Stack overflow
osRtxErrorSVC   EQU      6                      ; Invalid SVC function called


                PRESERVE8


                SECTION  .rodata:DATA:NOROOT(2)
                EXPORT   irqRtxLib
irqRtxLib       DCB      0                      ; Non weak library reference


                SECTION  .text:CODE:NOROOT(2)
                THUMB


SVC_Handler
                EXPORT   SVC_Handler
                IMPORT   osRtxUserSVC
                IMPORT   osRtxInfo
            #ifdef RTX_STACK_CHECK
                IMPORT   osRtxThreadStackCheck
                IMPORT   osRtxKernelErrorNotify
            #endif
            #ifdef RTX_SVC_PTR_CHECK
                IMPORT   |Image$$RTX_SVC_VENEERS$$Base|
                IMPORT   |Image$$RTX_SVC_VENEERS$$Length|
                IMPORT   osRtxKernelErrorNotify
            #endif
            #ifdef RTX_EXECUTION_ZONE
                IMPORT   osZoneSetup_Callback
            #endif
            #ifdef RTX_TZ_CONTEXT
                IMPORT   TZ_LoadContext_S
                IMPORT   TZ_StoreContext_S
            #endif

                TST      LR,#0x04               ; Determine return stack from EXC_RETURN bit 2
                ITE      EQ
                MRSEQ    R0,MSP                 ; Get MSP if return stack is MSP
                MRSNE    R0,PSP                 ; Get PSP if return stack is PSP

                LDR      R1,[R0,#24]            ; Load saved PC from stack
                LDRB     R1,[R1,#-2]            ; Load SVC number
                CMP      R1,#0                  ; Check SVC number
                BNE      SVC_User               ; Branch if not SVC 0

            #ifdef RTX_SVC_PTR_CHECK

                LDR      R12,[R0,#16]           ; Load function address from stack
                SUB      R1,R12,#1              ; Clear T-bit of function address
                LSLS     R2,R1,#30              ; Check if 4-byte aligned
                BEQ      SVC_PtrBoundsCheck     ; Branch if address is aligned

SVC_PtrInvalid
                PUSH     {R0,LR}                ; Save SP and EXC_RETURN
                MOVS     R0,#osRtxErrorSVC      ; Parameter: code
                MOV      R1,R12                 ; Parameter: object_id
                BL       osRtxKernelErrorNotify ; Call osRtxKernelErrorNotify
                POP      {R12,LR}               ; Restore SP and EXC_RETURN
                B        SVC_Context            ; Branch to context handling

SVC_PtrBoundsCheck
                LDR      R2,=|Image$$RTX_SVC_VENEERS$$Base|
                LDR      R3,=|Image$$RTX_SVC_VENEERS$$Length|
                SUBS     R2,R1,R2               ; Subtract SVC table base address
                CMP      R2,R3                  ; Compare with SVC table boundaries
                BHS      SVC_PtrInvalid         ; Branch if address is out of bounds

              #endif

                PUSH     {R0,LR}                ; Save SP and EXC_RETURN
                LDM      R0,{R0-R3,R12}         ; Load function parameters and address from stack
                BLX      R12                    ; Call service function
                POP      {R12,LR}               ; Restore SP and EXC_RETURN
                STR      R0,[R12]               ; Store function return value

SVC_Context
                LDR      R3,=osRtxInfo+I_T_RUN_OFS; Load address of osRtxInfo.thread.run
                LDM      R3,{R1,R2}             ; Load osRtxInfo.thread.run: curr & next
                CMP      R1,R2                  ; Check if thread switch is required
                IT       EQ
                BXEQ     LR                     ; Exit when threads are the same

                STR      R2,[R3]                ; osRtxInfo.thread.run: curr = next

              #if ((FPU_USED != 0) || (MVE_USED != 0))
                CBNZ     R1,SVC_ContextSave     ; Branch if running thread is not deleted
SVC_FP_LazyState
                TST      LR,#0x10               ; Determine stack frame from EXC_RETURN bit 4
                BNE      SVC_ContextRestore     ; Branch if not extended stack frame
                LDR      R3,=FPCCR              ; FPCCR Address
                LDR      R0,[R3]                ; Load FPCCR
                BIC      R0,R0,#1               ; Clear LSPACT (Lazy state preservation)
                STR      R0,[R3]                ; Store FPCCR
                B        SVC_ContextRestore     ; Branch to context restore handling
              #else
                CBZ      R1,SVC_ContextRestore  ; Branch if running thread is deleted
              #endif

SVC_ContextSave
            #ifdef RTX_TZ_CONTEXT
                LDR      R0,[R1,#TCB_TZM_OFS]   ; Load TrustZone memory identifier
                CBZ      R0,SVC_ContextSave_NS  ; Branch if there is no secure context
                PUSH     {R1,R2,R12,LR}         ; Save registers and EXC_RETURN
                BL       TZ_StoreContext_S      ; Store secure context
                POP      {R1,R2,R12,LR}         ; Restore registers and EXC_RETURN
            #endif

SVC_ContextSave_NS
            #if (DOMAIN_NS != 0)
                TST      LR,#0x40               ; Check domain of interrupted thread
                BNE      SVC_ContextSaveSP      ; Branch if secure
            #endif

            #ifdef RTX_STACK_CHECK
                SUB      R12,R12,#32            ; Calculate SP: space for R4..R11
              #if ((FPU_USED != 0) || (MVE_USED != 0))
                TST      LR,#0x10               ; Determine stack frame from EXC_RETURN bit 4
                IT       EQ                     ; If extended stack frame
                SUBEQ    R12,R12,#64            ;  Additional space for S16..S31
              #endif

SVC_ContextSaveSP
                STR      R12,[R1,#TCB_SP_OFS]   ; Store SP
                STRB     LR, [R1,#TCB_SF_OFS]   ; Store stack frame information

                PUSH     {R1,R2}                ; Save osRtxInfo.thread.run: curr & next
                MOV      R0,R1                  ; Parameter: osRtxInfo.thread.run.curr
                BL       osRtxThreadStackCheck  ; Check if thread stack is overrun
                POP      {R1,R2}                ; Restore osRtxInfo.thread.run: curr & next
                CBNZ     R0,SVC_ContextSaveRegs ; Branch when stack check is ok

              #if ((FPU_USED != 0) || (MVE_USED != 0))
                MOV      R4,R1                  ; Assign osRtxInfo.thread.run.curr to R4
              #endif
                MOVS     R0,#osRtxErrorStackOverflow ; Parameter: r0=code, r1=object_id
                BL       osRtxKernelErrorNotify      ; Call osRtxKernelErrorNotify
                LDR      R3,=osRtxInfo+I_T_RUN_OFS   ; Load address of osRtxInfo.thread.run
                LDR      R2,[R3,#4]             ; Load osRtxInfo.thread.run: next
                STR      R2,[R3]                ; osRtxInfo.thread.run: curr = next
                MOVS     R1,#0                  ; Simulate deleted running thread
              #if ((FPU_USED != 0) || (MVE_USED != 0))
                LDRSB    LR,[R4,#TCB_SF_OFS]    ; Load stack frame information
                B        SVC_FP_LazyState       ; Branch to FP lazy state handling
              #else
                B        SVC_ContextRestore     ; Branch to context restore handling
              #endif

SVC_ContextSaveRegs
                LDRSB    LR,[R1,#TCB_SF_OFS]    ; Load stack frame information
              #if (DOMAIN_NS != 0)
                TST      LR,#0x40               ; Check domain of interrupted thread
                BNE      SVC_ContextRestore     ; Branch if secure
              #endif
                LDR      R12,[R1,#TCB_SP_OFS]   ; Load SP
              #if ((FPU_USED != 0) || (MVE_USED != 0))
                TST      LR,#0x10               ; Determine stack frame from EXC_RETURN bit 4
                IT       EQ                     ; If extended stack frame
                VSTMIAEQ R12!,{S16-S31}         ;  Save VFP S16..S31
              #endif
                STM      R12,{R4-R11}           ; Save R4..R11
            #else
                STMDB    R12!,{R4-R11}          ; Save R4..R11
              #if ((FPU_USED != 0) || (MVE_USED != 0))
                TST      LR,#0x10               ; Determine stack frame from EXC_RETURN bit 4
                IT       EQ                     ; If extended stack frame
                VSTMDBEQ R12!,{S16-S31}         ;  Save VFP S16.S31
              #endif

SVC_ContextSaveSP
                STR      R12,[R1,#TCB_SP_OFS]   ; Store SP
                STRB     LR, [R1,#TCB_SF_OFS]   ; Store stack frame information
            #endif

SVC_ContextRestore
                 MOVS     R4,R2                 ; Assign osRtxInfo.thread.run.next to R4, clear Z flag
            #ifdef RTX_EXECUTION_ZONE
                 LDRB     R0,[R2,#TCB_ZONE_OFS] ; Load osRtxInfo.thread.run.next: zone
                 CBZ      R1,SVC_ZoneSetup      ; Branch if running thread is deleted (Z flag unchanged)
                 LDRB     R1,[R1,#TCB_ZONE_OFS] ; Load osRtxInfo.thread.run.curr: zone
                 CMP      R0,R1                 ; Check if next:zone == curr:zone

SVC_ZoneSetup
                 IT       NE                    ; If zone has changed or running thread is deleted
                 BLNE     osZoneSetup_Callback  ;  Setup zone for next thread
            #endif

            #ifdef RTX_TZ_CONTEXT
                LDR      R0,[R4,#TCB_TZM_OFS]   ; Load TrustZone memory identifier
                CMP      R0,#0
                IT       NE                     ; If TrustZone memory allocated
                BLNE     TZ_LoadContext_S       ;  Load secure context
            #endif

                LDR      R0,[R4,#TCB_SP_OFS]    ; Load SP
                LDR      R1,[R4,#TCB_SM_OFS]    ; Load stack memory base
                MSR      PSPLIM,R1              ; Set PSPLIM
                LDRSB    LR,[R4,#TCB_SF_OFS]    ; Load stack frame information

            #if (DOMAIN_NS != 0)
                TST      LR,#0x40               ; Check domain of interrupted thread
                ITT      NE                     ; If secure
                MSRNE    PSP,R0                 ;  Set PSP
                BXNE     LR                     ;  Exit from handler
            #endif

              #if ((FPU_USED != 0) || (MVE_USED != 0))
                TST      LR,#0x10               ; Determine stack frame from EXC_RETURN bit 4
                IT       EQ                     ; If extended stack frame
                VLDMIAEQ R0!,{S16-S31}          ;  Restore VFP S16..S31
              #endif
                LDMIA    R0!,{R4-R11}           ; Restore R4..R11
                MSR      PSP,R0                 ; Set PSP

SVC_Exit
                BX       LR                     ; Exit from handler

SVC_User
                LDR      R2,=osRtxUserSVC       ; Load address of SVC table
                LDR      R3,[R2]                ; Load SVC maximum number
                CMP      R1,R3                  ; Check SVC number range
                BHI      SVC_Exit               ; Branch if out of range

                PUSH     {R0,LR}                ; Save SP and EXC_RETURN
                LDR      R12,[R2,R1,LSL #2]     ; Load address of SVC function
                LDM      R0,{R0-R3}             ; Load function parameters from stack
                BLX      R12                    ; Call service function
                POP      {R12,LR}               ; Restore SP and EXC_RETURN
                STR      R0,[R12]               ; Store function return value

                BX       LR                     ; Return from handler


PendSV_Handler
                EXPORT   PendSV_Handler
                IMPORT   osRtxPendSV_Handler

                PUSH     {R0,LR}                ; Save EXC_RETURN
                BL       osRtxPendSV_Handler    ; Call osRtxPendSV_Handler
                POP      {R0,LR}                ; Restore EXC_RETURN
                MRS      R12,PSP                ; Save PSP to R12
                B        SVC_Context            ; Branch to context handling


SysTick_Handler
                EXPORT   SysTick_Handler
                IMPORT   osRtxTick_Handler

                PUSH     {R0,LR}                ; Save EXC_RETURN
                BL       osRtxTick_Handler      ; Call osRtxTick_Handler
                POP      {R0,LR}                ; Restore EXC_RETURN
                MRS      R12,PSP                ; Save PSP to R12
                B        SVC_Context            ; Branch to context handling


            #ifdef RTX_SAFETY_FEATURES

osFaultResume   PROC
                EXPORT   osFaultResume

                MRS      R12,PSP                ; Save PSP to R12
                B        SVC_Context            ; Branch to context handling

                ALIGN
                ENDP

            #endif


                END
