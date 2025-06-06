/*********************************************************************
*                SEGGER Microcontroller GmbH                         *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*        (c) 1996 - 2023  SEGGER Microcontroller GmbH                *
*                                                                    *
*        Internet: www.segger.com    Support:  support@segger.com    *
*                                                                    *
**********************************************************************

** emWin V6.46 - Graphical user interface for embedded applications **
All  Intellectual Property rights  in the Software belongs to  SEGGER.
emWin is protected by  international copyright laws.  Knowledge of the
source code may not be used to write a similar product.  This file may
only be used in accordance with the following terms:

The software has been licensed to  NXP Semiconductors USA, Inc.  whose
registered  office  is  situated  at 411 E. Plumeria Drive, San  Jose,
CA 95134, USA  solely for  the  purposes  of  creating  libraries  for
NXPs M0, M3/M4 and  ARM7/9 processor-based  devices,  sublicensed  and
distributed under the terms and conditions of the NXP End User License
Agreement.
Full source code is available at: www.segger.com

We appreciate your understanding and fairness.
----------------------------------------------------------------------
Licensing information
Licensor:                 SEGGER Microcontroller Systems LLC
Licensed to:              NXP Semiconductors, 1109 McKay Dr, M/S 76, San Jose, CA 95131, USA
Licensed SEGGER software: emWin
License number:           GUI-00186
License model:            emWin License Agreement, dated August 20th 2011 and Amendment No. 1, dated October 17th 2017 and Amendment No. 2, dated December 18th 2018
Licensed platform:        NXP's ARM 7/9, Cortex-M0, M3, M4, M7, A7, M33
----------------------------------------------------------------------
Support and Update Agreement (SUA)
SUA period:               2011-08-19 - 2025-09-02
Contact to extend SUA:    sales@segger.com
----------------------------------------------------------------------
File        : WM_Intern.h
Purpose     : Windows manager internal include
----------------------------------------------------------------------
*/

#ifndef WM_INTERN_H            /* Make sure we only include it once */
#define WM_INTERN_H            /* Make sure we only include it once */

#include "GUI_Private.h"
#include "WM.h"

#if defined(__cplusplus)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

#if GUI_WINSUPPORT

/*********************************************************************
*
*       defines
*
**********************************************************************

  The following could be placed in a file of its own as it is not
  used outside of the window manager

*/
/* Basic Windows status flags.
   For module-internally use only !
*/
#define WM_SF_HASTRANS          WM_CF_HASTRANS
#define WM_SF_MEMDEV            WM_CF_MEMDEV
#define WM_SF_MEMDEV_ON_REDRAW  WM_CF_MEMDEV_ON_REDRAW
#define WM_SF_DISABLED          WM_CF_DISABLED  /* Disabled: Does not receive PID (mouse & touch) input */
#define WM_SF_ISVIS             WM_CF_SHOW      /* Is visible flag */

#define WM_SF_STAYONTOP         WM_CF_STAYONTOP
#define WM_SF_LATE_CLIP         WM_CF_LATE_CLIP
#define WM_SF_ANCHOR_RIGHT      WM_CF_ANCHOR_RIGHT
#define WM_SF_ANCHOR_BOTTOM     WM_CF_ANCHOR_BOTTOM
#define WM_SF_ANCHOR_LEFT       WM_CF_ANCHOR_LEFT
#define WM_SF_ANCHOR_TOP        WM_CF_ANCHOR_TOP

#define WM_SF_INVALID           WM_CF_ACTIVATE  /* We reuse this flag, as it is create only and Invalid is status only */

#define WM_SF_CONST_OUTLINE     WM_CF_CONST_OUTLINE       /* Constant outline.*/

#if WM_VALIDATE_HANDLE
  #define WM_H2P(hWin)            ((WM_Obj*)WM__GetValidPointer(hWin))
#else
  #define WM_H2P(hWin)            ((WM_Obj*)GUI_ALLOC_h2p(hWin))
#endif


#if GUI_DEBUG_LEVEL  >= GUI_DEBUG_LEVEL_LOG_WARNINGS
  #define WM_ASSERT_NOT_IN_PAINT() { if (WM__PaintCallbackCnt) \
                                       GUI_DEBUG_ERROROUT("Function may not be called from within a paint event"); \
                                   }
#else
  #define WM_ASSERT_NOT_IN_PAINT()
#endif

/*********************************************************************
*
*       Data types & structures
*
**********************************************************************
*/
typedef struct {
  WM_HWIN hOld;
  WM_HWIN hNew;
} WM_NOTIFY_CHILD_HAS_FOCUS_INFO;

typedef struct WM_CRITICAL_HANDLE {
  struct  WM_CRITICAL_HANDLE * pNext;
  volatile WM_HWIN hWin;
} WM_CRITICAL_HANDLE;

/*********************************************************************
*
*       Data (extern)
*
**********************************************************************
*/
extern U32            WM__CreateFlags;
extern WM_HWIN        WM__ahCapture[GUI_NUM_LAYERS];
extern WM_HWIN        WM__ahWinFocus[GUI_NUM_LAYERS];
extern char           WM__CaptureReleaseAuto;
extern WM_tfPollPID * WM_pfPollPID;
extern U8             WM__PaintCallbackCnt;      /* Public for assertions only */
extern WM_HWIN        WM__hCreateStatic;

#if WM_SUPPORT_TRANSPARENCY
  extern int     WM__TransWindowCnt;
  extern WM_HWIN WM__hATransWindow;
#endif

#if WM_SUPPORT_DIAG
  extern void (*WM__pfShowInvalid)(WM_HWIN hWin);
#endif

extern WM_CRITICAL_HANDLE     WM__aCHWinModal[GUI_NUM_LAYERS];
extern WM_CRITICAL_HANDLE     WM__aCHWinLast[GUI_NUM_LAYERS];
extern int                    WM__ModalLayer;

#if GUI_SUPPORT_MOUSE
  extern WM_CRITICAL_HANDLE   WM__aCHWinMouseOver[GUI_NUM_LAYERS];
#endif

#ifdef WM_C
  #define GUI_EXTERN
#else
  #define GUI_EXTERN extern
#endif

#if (GUI_NUM_LAYERS > 1)
  extern U32                       WM__InvalidLayerMask;
  extern unsigned                  WM__TouchedLayer;
  #define WM__TOUCHED_LAYER            WM__TouchedLayer
#else
  #define WM__TOUCHED_LAYER            GUI_CURSOR_LAYER
#endif

extern U16     WM__NumWindows;
extern U16     WM__NumInvalidWindows;
extern WM_HWIN WM__FirstWin;
extern WM_CRITICAL_HANDLE * WM__pFirstCriticalHandle;

extern WM_HWIN   WM__ahDesktopWin[GUI_NUM_LAYERS];
extern GUI_COLOR WM__aBkColor[GUI_NUM_LAYERS];

extern U32 WM__DrawSprite;  // Required when using sprites in combination with the WM.

#undef GUI_EXTERN

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/
void    WM__ActivateClipRect        (void);
int     WM__ClipAtParentBorders     (GUI_RECT * pRect, WM_HWIN hWin);
void    WM__Client2Screen           (const WM_Obj * pWin, GUI_RECT * pRect);
void    WM__DeactivateEx            (void);
void    WM__DeleteAssocTimer        (WM_HWIN hWin);
void    WM__DetachWindow            (WM_HWIN hChild);
void    WM__ForEachDesc             (WM_HWIN hWin, WM_tfForEach * pcb, void * pData);
void    WM__GetClientRectWin        (const WM_Obj * pWin, GUI_RECT * pRect);
void    WM__GetClientRectEx         (WM_HWIN hWin, GUI_RECT * pRect);
WM_HWIN WM__GetFirstSibling         (WM_HWIN hWin);
WM_HWIN WM__GetFocusedChild         (WM_HWIN hWin);
WM_HWIN WM__GetLastSibling          (WM_HWIN hWin);
WM_HWIN WM__GetPrevSibling          (WM_HWIN hWin);
int     WM__GetTopLevelLayer        (WM_HWIN hWin);
int     WM__GetWindowSizeX          (const WM_Obj * pWin);
int     WM__GetWindowSizeY          (const WM_Obj * pWin);
void    WM__InsertWindowIntoList    (WM_HWIN hWin, WM_HWIN hParent);
void    WM__Invalidate1Abs          (WM_HWIN hWin, const GUI_RECT * pRect);
void    WM__InvalidateDraw          (WM_HWIN hWin);
void    WM__InvalidateRectEx        (const GUI_RECT * pInvalidRect, WM_HWIN hWin, WM_HWIN hStop);
int     WM__IsAncestor              (WM_HWIN hChild, WM_HWIN hParent);
int     WM__IsAncestorOrSelf        (WM_HWIN hChild, WM_HWIN hParent);
int     WM__IsChild                 (WM_HWIN hWin, WM_HWIN hParent);
int     WM__IsEnabled               (WM_HWIN hWin);
int     WM__IsInModalArea           (WM_HWIN hWin);
int     WM__IsInWindow              (WM_Obj * pWin, int x, int y);
int     WM__IsWindow                (WM_HWIN hWin);
void    WM__MoveTo                  (WM_HWIN hWin, int x, int y);
void    WM__MoveWindow              (WM_HWIN hWin, int dx, int dy);
void    WM__NotifyVisChanged        (WM_HWIN hWin, GUI_RECT * pRect);
int     WM__RectIsNZ                (const GUI_RECT * pr);
void    WM__RemoveWindowFromList    (WM_HWIN hWin);
void    WM__Screen2Client           (const WM_Obj * pWin, GUI_RECT * pRect);
void    WM__SelectTopLevelLayer     (WM_HWIN  hWin);
void    WM__SendMsgNoData           (WM_HWIN hWin, U8 MsgId);
void    WM__SendMessage             (WM_HWIN hWin, WM_MESSAGE * pm);
void    WM__SendMessageIfEnabled    (WM_HWIN hWin, WM_MESSAGE * pm);
void    WM__SendMessageNoPara       (WM_HWIN hWin, int MsgId);
void    WM__SendPIDMessage          (WM_HWIN hWin, WM_MESSAGE * pMsg);
int     WM__SetScrollbarH           (WM_HWIN hWin, int OnOff);
int     WM__SetScrollbarV           (WM_HWIN hWin, int OnOff);
void    WM__UpdateChildPositions    (WM_Obj * pObj, int dx0, int dy0, int dx1, int dy1);
void    WM_PID__GetPrevState        (GUI_PID_STATE * pPrevState, int Layer);
void    WM_PID__SetPrevState        (GUI_PID_STATE * pPrevState, int Layer);
void    WM__SendTouchMessage        (WM_HWIN hWin, WM_MESSAGE * pMsg);

U16     WM_GetFlags                 (WM_HWIN hWin);
int     WM__Paint                   (WM_HWIN hWin);
int     WM__Paint1                  (WM_HWIN hWin);
void    WM__AddCriticalHandle       (WM_CRITICAL_HANDLE * pCH);
void    WM__RemoveCriticalHandle    (WM_CRITICAL_HANDLE * pCH);
void    WM__SetLastTouched          (WM_HWIN hWin);

#if WM_SUPPORT_STATIC_MEMDEV
  void           WM__InvalidateDrawAndDescs(WM_HWIN hWin);
  void           WM__ClearSMDs             (void);
#else
  #define WM__InvalidateDrawAndDescs(hWin)
#endif

/*********************************************************************
*
*       Performance measurement
*/
#if GUI_SUPPORT_MEMDEV

void WM_FPS__Enable (int xPos, int yPos, GUI_COLOR ColorFG, GUI_COLOR ColorBG);
void WM_FPS__Disable(void);

#endif

/*********************************************************************
*
*       Validate WM handles
*/
#if WM_VALIDATE_HANDLE
  void   * WM__GetValidPointer(WM_HWIN hWin);
  WM_Obj * WM__LockValid      (WM_HWIN hWin);
#endif

/* Static memory devices */
#if (GUI_SUPPORT_MEMDEV)
  typedef struct {
    int xSize, ySize; // Size of bk window
  } EFFECT_CONTEXT;

  int  GUI_MEMDEV__CalcParaFadeIn    (int Period, int TimeUsed);
  void GUI_MEMDEV__ClipBK            (EFFECT_CONTEXT * pContext);
  void GUI_MEMDEV__RemoveStaticDevice(WM_HWIN hWin);
  void GUI_MEMDEV__UndoClipBK        (EFFECT_CONTEXT * pContext);
#endif

void WM__InvalidateRect(const GUI_RECT * pInvalidRect, WM_HWIN hWin, WM_HWIN hStop);

#endif   /* GUI_WINSUPPORT */

#if defined(__cplusplus)
  }
#endif

#endif   /* WM_INTERN_H */

/*************************** End of file ****************************/
