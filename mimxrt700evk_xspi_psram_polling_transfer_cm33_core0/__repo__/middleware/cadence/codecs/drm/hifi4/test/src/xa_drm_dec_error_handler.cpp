/*
 * Copyright (c) 2014-2022 Cadence Design Systems, Inc.
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


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "xa_type_def.h"
#include "xa_error_standards.h"
#include "xa_error_handler.h"

/*****************************************************************************/
/* Class 0: API Errors                                                       */
/*****************************************************************************/
/* Nonfatal Errors */
const char *ppb_xa_drm_dec_api_non_fatal[] = {
    "API Command type not supported",
    "Invalid API Sequence "
};
/* Fatal Errors */
const char *ppb_xa_drm_dec_api_fatal[] = {
    "Memory Allocation Error: NULL Pointer ",
    "Memory Allocation Error: Alignment requirement not met",
    "Invalid Command",
    "Invalid Command Type/Index",
    "Invalid API Sequence "
};

/*****************************************************************************/
/* Class 1: Configuration Errors                                             */
/*****************************************************************************/
/* Non Fatal Errors */
const char *ppb_xa_drm_dec_config_non_fatal[] = {
    "Config parameters not yet set",
    "Invalid config param",
    "Warning! The stream position is invalid",
    "Note: Support to this format is limited and provided only for conformance testing;"
};

/* Fatal Errors */
const char *ppb_xa_drm_dec_config_fatal[] = {
    "Invalid config param",
    "Unsupported Bitstream format",
    "unsupported bitrate",
    "Decoder can't queue more than 4 config changes"
};

/*****************************************************************************/
/* Class 2: Execution Class Errors                                           */
/*****************************************************************************/
/* Nonfatal Errors */
const char *ppb_xa_drm_dec_execute_non_fatal[] = {
    "Insufficient Frame Data"
        ,"Non-Fatal DRM Frame Parsing Error"
        ,"Warning: decode frame error"
        ,"Warning!: Empty Input Buffer Supplied"
    ,"CRC Error detected; frame concealed"
    ,"stream change detected"
    ,"xHE-AAC sync failure:corrupted input super frame"
};
/* Fatal Errors */
const char *ppb_xa_drm_dec_execute_fatal[] = {
    "Fatal Parsing Error"
    ,"Fatal Init Error"
    ,"Fatal stream changed"
    ,"Fatal unsupported feature"
 
};

/*****************************************************************************/
/* error info structure                                                      */
/*****************************************************************************/
/* The Module's Error Info Structure */
xa_error_info_struct xa_drm_dec_error_info =
{
    /* The Module Name	*/
    "Tensilica DRM+ Decoder",
    {
        /* The Class Names	*/
        "API",
        "Configuration",
        "Execution Class",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        ""
    },
    {
        /* The Message Pointers	*/
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
    }
};

/*****************************************************************************/
/*                                                                           */
/*  Function name : xa_drm_dec_error_handler_init                         */
/*                                                                           */
/*  Description   : Initialize the error struct with string pointers         */
/*                                                                           */
/*  Inputs        : none                                                     */
/*                                                                           */
/*  Globals       : xa_error_info_struct xa_drm_dec_error_info               */
/*                  const char *ppb_xa_drm_dec_api_non_fatal              */
/*                  const char *ppb_xa_drm_dec_api_fatal                  */
/*                  const char *ppb_xa_drm_dec_config_non_fatal           */
/*                  const char *ppb_xa_drm_dec_config_fatal               */
/*                  const char *ppb_xa_drm_dec_execute_non_fatal          */
/*                  const char *ppb_xa_drm_dec_execute_fatal              */
/*                                                                           */
/*  Processing    : Init the struct with error string pointers               */
/*                                                                           */
/*  Outputs       : none                                                     */
/*                                                                           */
/*  Returns       : none                                                     */
/*                                                                           */
/*  Issues        : none                                                     */
/*                                                                           */
/*  Revision history :                                                       */
/*                                                                           */
/*        DD MM YYYY       Author                Changes                     */
/*                                                                           */
/*****************************************************************************/

VOID xa_drm_dec_error_handler_init()
{
    /* The Message Pointers	*/
    xa_drm_dec_error_info.ppppb_error_msg_pointers[0][ 0] =
        ppb_xa_drm_dec_api_non_fatal;
    xa_drm_dec_error_info.ppppb_error_msg_pointers[1][ 0] =
        ppb_xa_drm_dec_api_fatal;
    xa_drm_dec_error_info.ppppb_error_msg_pointers[0][ 1] =
        ppb_xa_drm_dec_config_non_fatal;
    xa_drm_dec_error_info.ppppb_error_msg_pointers[1][ 1] =
        ppb_xa_drm_dec_config_fatal;
    xa_drm_dec_error_info.ppppb_error_msg_pointers[0][ 2] =
        ppb_xa_drm_dec_execute_non_fatal;
    xa_drm_dec_error_info.ppppb_error_msg_pointers[1][ 2] =
        ppb_xa_drm_dec_execute_fatal;
}

/*****************************************************************************/
/* xa_testbench ErrorCode Definitions                                        */
/*****************************************************************************/
/*****************************************************************************/
/* Class 0: Memory & File Manager Errors                                     */
/*****************************************************************************/
/* Non Fatal Errors */
/* Fatal Errors */
const char *ppb_xa_testbench_mem_file_man_fatal[] = {
    "Memory Allocation Error",
    "DRM File: Open Failed",
    "DRM File: Read Failed",
    "DRM File: Write Failed",
    "DRM File: Close Failed",
    "Help Requested",
    "Invalid Argument",
    "-if Argument missing/incorrect",
    "-of Argument missing/incorrect",
    "Initialization Failed",

};
const char *ppb_xa_testbench_mem_file_man_nonfatal[] = {
    "partial Frame",
    "Step Play Not Supported for DRM Decoding",
};
/*****************************************************************************/
/* error info structure                                                      */
/*****************************************************************************/
/* The Module's Error Info Structure */
xa_error_info_struct xa_testbench_error_info =
{
    /* The Module Name	*/
    "Tensilica DRM+ decode test bench",
    {
        /* The Class Names	*/
        "Memory & File Manager",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        ""
    },
    {
        /* The Message Pointers	*/
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
    }
};

/*****************************************************************************/
/*                                                                           */
/*  Function name : xa_testbench_error_handler_init                          */
/*                                                                           */
/*  Description   : Initialize the error struct with string pointers         */
/*                                                                           */
/*  Inputs        : none                                                     */
/*                                                                           */
/*  Globals       : xa_error_info_struct xa_testbench_error_info             */
/*                  const char * ppb_xa_testbench_mem_file_man_fatal         */
/*                                                                           */
/*  Processing    : Init the struct with error string pointers               */
/*                                                                           */
/*  Outputs       : none                                                     */
/*                                                                           */
/*  Returns       : none                                                     */
/*                                                                           */
/*  Issues        : none                                                     */
/*                                                                           */
/*  Revision history :                                                       */
/*                                                                           */
/*        DD MM YYYY       Author                Changes                     */
/*                                                                           */
/*****************************************************************************/

VOID xa_testbench_error_handler_init()
{
    /* The Message Pointers	*/
    xa_testbench_error_info.ppppb_error_msg_pointers[1][ 0] =
        ppb_xa_testbench_mem_file_man_fatal;
    xa_testbench_error_info.ppppb_error_msg_pointers[0][ 0] =
        ppb_xa_testbench_mem_file_man_nonfatal;
}

/*****************************************************************************/
/*                                                                           */
/*  Function name : xa_error_handler                                         */
/*                                                                           */
/*  Description   : Called Prints the status error code from the err_info    */
/*                                                                           */
/*  Inputs        : xa_error_info_struct *p_mod_err_info (Error info struct) */
/*                  const char *pb_context (Context of error)                */
/*                  XA_ERRORCODE code (Error code)                           */
/*                                                                           */
/*  Globals       : none                                                     */
/*                                                                           */
/*  Processing    : whenever any module calls the errorhandler,  it  informs */
/*                  it about the module for which it is called and a context */
/*                  in which it was  called  in addition to  the  error_code */
/*                  the message is displayed  based  on the  module's  error */
/*                  message  array  that maps to  the error_code the context */
/*                  gives specific info in which the error occured  e.g. for */
/*                  testbench   module,  memory  allocator   can   call  the */
/*                  errorhandler   for  memory  inavailability  in   various */
/*                  contexts like input_buf or output_buf e.g.  for  given   */
/*                  module, there can be various instances running.  context */
/*                  can be used to  identify  the  particular  instance  the */
/*                  error handler is being called for                        */
/*                                                                           */
/*  Outputs       : None                                                     */
/*                                                                           */
/*  Returns       : XA_ERRORCODE error_value  (Error value)                  */
/*                                                                           */
/*  Issues        : none                                                     */
/*                                                                           */
/*  Revision history :                                                       */
/*                                                                           */
/*        DD MM YYYY       Author                Changes                     */
/*                                                                           */
/*****************************************************************************/

    XA_ERRORCODE
xa_error_handler(xa_error_info_struct *p_mod_err_info,
        const char *pb_context,
        XA_ERRORCODE code) 
{
    if (code == XA_NO_ERROR) 
    {
        return XA_NO_ERROR;
    }
    {
        int is_fatal     = XA_ERROR_SEVERITY(code) != 0;
        int err_class    = XA_ERROR_CLASS(code);
        /* int err_codec    = XA_ERROR_CODEC(code); */
        int err_sub_code = XA_ERROR_SUBCODE(code);

        fprintf(stderr, "\n");
        if (!is_fatal)
        {
            fprintf(stderr, "non ");
        }
        fprintf(stderr, "fatal error: ");

        if ((p_mod_err_info->pb_module_name != NULL) &&
                (p_mod_err_info->pb_module_name[0] != '\0'))
        {
            fprintf(stderr, "%s: ", p_mod_err_info->pb_module_name);
        }
        if (p_mod_err_info->ppb_class_names[err_class] != NULL)
        {
            fprintf(stderr, "%s: ", p_mod_err_info->ppb_class_names[err_class]);
        }
        if (pb_context != NULL)
        {
            fprintf(stderr, "%s: ", pb_context);
        }

        if ( p_mod_err_info->ppppb_error_msg_pointers[is_fatal][err_class] == NULL)
        {
            fprintf(stderr, "Unknown Error Code %d\n", err_sub_code);
        }
        else if  (p_mod_err_info->ppppb_error_msg_pointers[is_fatal][err_class][err_sub_code] == NULL)
        {
            fprintf(stderr, "Unknown Error Code %d\n", err_sub_code);
        }
        else
        {
            fprintf(stderr, "%s\n", p_mod_err_info->ppppb_error_msg_pointers[is_fatal][err_class][err_sub_code]);
        }
    }
    return XA_NO_ERROR;
}
