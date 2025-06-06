
/**
 *  \file sco_audio_pl.c
 *
 *
 */

/*
 *  Copyright (C) 2016. Mindtree Limited.
 *  All rights reserved.
 */


/* --------------------------------------------- Header File Inclusion */
#include "sco_audio_pl.h"

#ifndef CONFIG_BR_SCO_PCM_DIRECTION
/* PCM direction, default 0
 * 0 = port A receive, port B transmit
 * 1 = port A transmit, port B receive
 */
#define CONFIG_BR_SCO_PCM_DIRECTION 0
#endif /* CONFIG_BR_SCO_PCM_DIRECTION */

/* --------------------------------------------- External Global Variables */

/* --------------------------------------------- Exported Global Variables */

/* --------------------------------------------- Static Global Variables */

/* --------------------------------------------- Functions */

__attribute__((weak)) void sco_audio_init_pl_ext (void)
{
}

__attribute__((weak)) void sco_audio_shutdown_pl_ext (void)
{
}

__attribute__((weak)) API_RESULT sco_audio_setup_pl_ext (SCO_AUDIO_EP_INFO *ep_info)
{
    return API_SUCCESS;
}

__attribute__((weak)) API_RESULT sco_audio_start_pl_ext (void)
{
    return API_SUCCESS;
}

__attribute__((weak)) API_RESULT sco_audio_stop_pl_ext (void)
{
    return API_SUCCESS;
}

__attribute__((weak)) void platform_audio_play_ringtone (void)
{
}

__attribute__((weak)) API_RESULT platform_audio_outgoing_call_ringtone(void)
{
    return API_SUCCESS;
}

__attribute__((weak)) void platform_audio_set_speaker_volume (UCHAR value)
{
}

__attribute__((weak)) void platform_audio_set_microphone_gain (UCHAR value)
{
}

__attribute__((weak)) void sco_audio_play_ringtone_pl_ext (void)
{
}

__attribute__((weak)) void sco_audio_play_inband_ringtone_pl_ext (void)
{
}

__attribute__((weak)) void sco_audio_play_ringtone_exit_pl_ext (void)
{
}

__attribute__((weak))  API_RESULT sco_audio_set_speaker_volume(UCHAR volume)
{
    return API_SUCCESS;
}
__attribute__((weak))  API_RESULT sco_audio_set_microphone_gain(UCHAR volume)
{
    return API_SUCCESS;
}
#ifdef HCI_SCO
__attribute__((weak)) void sco_audio_spkr_play_pl_ext (UCHAR * m_data, UINT16 m_datalen)
{
}

#endif /* HCI_SCO */

static UCHAR WBS_mode = 0U;
void sco_audio_init_pl (void)
{
#ifndef HFP_BRIDGING
    sco_audio_init_pl_ext();
#endif //HFP_BRIDGING
}

void sco_audio_shutdown_pl (void)
{
    sco_audio_shutdown_pl_ext();
}

API_RESULT sco_audio_setup_pl (SCO_AUDIO_EP_INFO *ep_info)
{
    API_RESULT  retval;

    /* param check */
    if (NULL == ep_info)
    {
        printf ("SCO EndPoint Info. is NULL\n");
        return API_FAILURE;
    }
#ifndef HFP_BRIDGING
    (BT_IGNORE_RETURN_VALUE) sco_audio_setup_pl_ext (ep_info);
#endif //HFP_BRIDGING
    /**
     * TODO: Validate Config. parameters
     */

    retval = API_SUCCESS;

    return retval;
}

#ifdef HFP_BRIDGING
static int num_sco_conn = 0U;
#endif //HFP_BRIDGING

API_RESULT sco_audio_start_pl (void)
{
	API_RESULT  retval = API_SUCCESS;
#ifdef NVRAM_WORKAROUND
    /* Disable storage update */
    BT_storage_disable_store();
#endif /* NVRAM_WORKAROUND */
    printf("Sending Vendor command 006f now\n");
    UCHAR new[6U] = {0x00U, 0x00U, 0x08U, 0x00U, 0x00U, 0x00U};
    (BT_IGNORE_RETURN_VALUE) BT_hci_vendor_specific_command(0x006fU, new, sizeof(new));
#ifndef HFP_BRIDGING
    retval = sco_audio_start_pl_ext();
#endif //HFP_BRIDGING
    return retval;
}

API_RESULT sco_audio_stop_pl (void)
{
#ifdef HFP_BRIDGING
	if(num_sco_conn > 0)
	{
		num_sco_conn--;
	}
    printf("num_sco_conn = %d\n",num_sco_conn);
#endif //HFP_BRIDGING
   /* Send VSC 0x73 command to enable WBS for second next call */
    if (0U != WBS_mode)
    {
        printf(" sco_audio_stop_pl: Sending Vendor command 0073 with WBS enabled\n");
        UCHAR new4[1U] = {0x01U};
        (BT_IGNORE_RETURN_VALUE) BT_hci_vendor_specific_command(0x0073U, new4, sizeof(new4));
    }
    else
    {
        printf(" sco_audio_stop_pl: Sending Vendor command 0073 with WBS disabled\n");
        UCHAR new4[1U] = {0x00U};
        (BT_IGNORE_RETURN_VALUE) BT_hci_vendor_specific_command(0x0073U, new4, sizeof(new4));
    }

#ifndef HFP_BRIDGING
    (BT_IGNORE_RETURN_VALUE) sco_audio_stop_pl_ext();
#endif //HFP_BRIDGING

#ifdef NVRAM_WORKAROUND
    /* Disable storage update */
    BT_storage_sync_db(STORAGE_TYPE_PERSISTENT);
#endif /* NVRAM_WORKAROUND */

    return API_SUCCESS;
}

void sco_audio_set_wideband_pl (UCHAR enable)
{
    WBS_mode = enable;
    UCHAR config[3U] = {0x03U, 0x00U, 0xFFU};
    printf ("Wideband Config at Controller: %s\n",(enable)? "Enabled": "Disabled");

    printf("Sending Vendor command 0028\n");
    /* Inform the controller about 8K/16K configuration */
    config[2U] = (BT_TRUE == enable)? 0x07U: 0x03U;
    (BT_IGNORE_RETURN_VALUE) BT_hci_vendor_specific_command(0x0028U, config, sizeof(config));

    printf("Sending Vendor command 0007 now\n");
    UCHAR new1[1U] = {0x02U};
    new1[0] |= (CONFIG_BR_SCO_PCM_DIRECTION > 0) ? 1 : 0;
    (BT_IGNORE_RETURN_VALUE) BT_hci_vendor_specific_command(0x0007U, new1, sizeof(new1));

    UCHAR new2[2U] = {0x04U, 0x00U};
    printf(" Sending Vendor command 0029 now\n");
    (BT_IGNORE_RETURN_VALUE) BT_hci_vendor_specific_command(0x0029U, new2, sizeof(new2));

    printf(" Sending Vendor command 001d now\n");
    UCHAR new3[1U] = {0x01U};
    (BT_IGNORE_RETURN_VALUE) BT_hci_vendor_specific_command(0x001dU, new3, sizeof(new3));

    printf(" Sending Vendor command 0070 now\n");
    UCHAR new5[1U] = {0x01U};
    (BT_IGNORE_RETURN_VALUE) BT_hci_vendor_specific_command(0x0070U, new5, sizeof(new5));

    if (0U != enable)
    {
        printf(" Sending Vendor command 0073 with WBS enabled\n");
        UCHAR new4[1U] = {0x01U};
        (BT_IGNORE_RETURN_VALUE) BT_hci_vendor_specific_command(0x0073U, new4, sizeof(new4));
    }
    else
    {
           printf(" Sending Vendor command 0073 with WBS disabled\n");
           UCHAR new4[1U] = {0x00U};
           (BT_IGNORE_RETURN_VALUE) BT_hci_vendor_specific_command(0x0073U, new4, sizeof(new4));
    }
    (BT_IGNORE_RETURN_VALUE) BT_hci_vendor_specific_command(0x0028U, config, sizeof(config));
}

void sco_audio_play_ringtone_pl (void)
{
#ifndef HFP_BRIDGING
    sco_audio_play_ringtone_pl_ext();
#endif //HFP_BRIDGING
}

void sco_audio_play_inband_ringtone_pl (void)
{
#ifndef HFP_BRIDGING
    sco_audio_play_inband_ringtone_pl_ext();
#endif //HFP_BRIDGING
}

void sco_audio_set_speaker_volume_pl(UCHAR value)
{
#ifndef HFP_BRIDGING
    (void)sco_audio_set_speaker_volume(value);
#endif //HFP_BRIDGING
}

void sco_audio_set_microphone_gain_pl(UCHAR value)
{
#ifndef HFP_BRIDGING
    (void)sco_audio_set_microphone_gain(value);
#endif //HFP_BRIDGING
}
void sco_audio_play_ringtone_exit_pl (void)
{
#ifndef HFP_BRIDGING
    sco_audio_play_ringtone_exit_pl_ext();
#endif //HFP_BRIDGING
}

#ifdef HCI_SCO
void sco_audio_spkr_play_pl (UCHAR * m_data, UINT16 m_datalen)
{
    /* Write to Codec */
    sco_audio_spkr_play_pl_ext( m_data,  m_datalen);
}
#endif /* HCI_SCO */

#ifdef HFP_BRIDGING
API_RESULT sco_bridge_audio_start_pl (UINT16 sco_handle_1, UINT16 sco_handle_2)
{
	printf("Sending Vendor command for sco-bridging now for handles 0x%x, 0x%x\n", sco_handle_1, sco_handle_2);
	UCHAR new[6U];
	new[0] = 0x00U;
	new[1] = 0x01U;
	new[2] = sco_handle_1 & 0x00FFU;
	new[3] = (sco_handle_1 >> 8) & 0x00FFU;
	new[4] = sco_handle_2 & 0x00FFU;
	new[5] = (sco_handle_2 >> 8) & 0x00FFU;

	for(int i = 0; i < 6; i++)
	{
		printf("%x\n", new[i]);
	}
	return BT_hci_vendor_specific_command(0x006fU, new, sizeof(new));
}
#endif //HFP_BRIDGING
