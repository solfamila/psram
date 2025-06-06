/*
 * Copyright (c) 2006-2023 Cadence Design Systems, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*****************************************************************************/
/*                                                                           */
/*  File Name        : xa_error_handler.h                                    */
/*                                                                           */
/*  Description      : Error related function prototypes and definitions     */
/*                                                                           */
/*  List of Functions: None                                                  */
/*                                                                           */
/*  Issues / Problems: None                                                  */
/*                                                                           */
/*  Revision History :                                                       */
/*                                                                           */
/*        DD MM YYYY       Author                Changes                     */
/*                                                                           */
/*****************************************************************************/

#ifndef __XA_ERROR_HANDLER_H__
#define __XA_ERROR_HANDLER_H__

/*****************************************************************************/
/* File includes                                                             */
/*  xa_type_def.h                                                            */
/*  xa_error_standards.h                                                     */
/*****************************************************************************/

/* these definitions are used by error handling function         */
/* the error handler will work on a structure which identifies a */
/* particular error with a module, context and error_code        */
/* within error_code, MSB identifies FATAL (1), NONFATAL (0)     */
/* next 4 ms_bs identify a class of error                        */

/*****************************************************************************/
/* Constant hash defines                                                     */
/*****************************************************************************/
#define XA_ERROR_NON_FATAL_IDX		0x0
#define XA_ERROR_FATAL_IDX			0x1

#define XA_ERROR_CLASS_0		0x0
#define XA_ERROR_CLASS_1		0x1
#define XA_ERROR_CLASS_2		0x2
#define XA_ERROR_CLASS_3		0x3
#define XA_ERROR_CLASS_4		0x4
#define XA_ERROR_CLASS_5		0x5
#define XA_ERROR_CLASS_6		0x6
#define XA_ERROR_CLASS_7		0x7
#define XA_ERROR_CLASS_8		0x8
#define XA_ERROR_CLASS_9		0x9
#define XA_ERROR_CLASS_A		0xA
#define XA_ERROR_CLASS_B		0xB
#define XA_ERROR_CLASS_C		0xC
#define XA_ERROR_CLASS_D		0xD
#define XA_ERROR_CLASS_E		0xE
#define XA_ERROR_CLASS_F		0xF

/* each module, hence, needs to copy the following structure          */
/* the first index is for FATAL/NONFATAL                              */
/* the second index is for the classes                                */
/* then in a module specific initialization, fill in the following    */
/* structure with the pointers to the particular error message arrays */

/*****************************************************************************/
/* Type definitions                                                          */
/*****************************************************************************/
typedef struct {
  const char  *pb_module_name;
  const char  *ppb_class_names[16];
  const char **ppppb_error_msg_pointers[2][16];
} xa_error_info_struct;

/*****************************************************************************/
/* Function prototypes                                                       */
/*****************************************************************************/
/* this error handler maps the code generated by a module to a error string  */
/* pb_context is a string to specify where the module broke                  */
/* Note that this function declaration logically belongs to the calling      */
/* program; it is not used in the codec library.                             */
XA_ERRORCODE xa_error_handler(xa_error_info_struct *p_mod_err_info,
			      const char *pb_context,
			      XA_ERRORCODE code);

/*****************************************************************************/
/* Macro functions                                                           */
/*****************************************************************************/
/* the following macro does a one-line job of returning back to the parent   */
/* in case a fatal error occurs after calling the error handler function     */
/* Note that this macro logically belongs to the calling program; it         */
/* is not used in the codec library.                                         */
#define _XA_HANDLE_ERROR(p_mod_err_info, context, e) \
	if((e) != XA_NO_ERROR) \
	{ \
		xa_error_handler((p_mod_err_info), (context), (e)); \
		if((e) & XA_FATAL_ERROR) \
			return (e); \
	}

#endif /* __XA_ERROR_HANDLER_H__ */
