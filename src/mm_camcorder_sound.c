/*
 * libmm-camcorder
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jeongmo Yang <jm80.yang@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

/*=======================================================================================
|  INCLUDE FILES									|
=======================================================================================*/
#include <mm_sound.h>
#include <mm_sound_private.h>
#include "mm_camcorder_internal.h"
#include "mm_camcorder_sound.h"

/*---------------------------------------------------------------------------------------
|    GLOBAL VARIABLE DEFINITIONS for internal						|
---------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------
|    LOCAL VARIABLE DEFINITIONS for internal						|
---------------------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------------------
|    LOCAL FUNCTION PROTOTYPES:								|
---------------------------------------------------------------------------------------*/



gboolean _mmcamcorder_sound_init(MMHandleType handle)
{
	int sound_enable = TRUE;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	SOUND_INFO *info = NULL;

	mmf_return_val_if_fail(hcamcorder, FALSE);

	info = &(hcamcorder->snd_info);

	g_mutex_lock(&info->open_mutex);

	/* check sound state */
	if (info->state > _MMCAMCORDER_SOUND_STATE_NONE) {
		_mmcam_dbg_warn("already initialized [%d]", info->state);
		g_mutex_unlock(&info->open_mutex);
		return TRUE;
	}

	/* check sound play enable */
	mm_camcorder_get_attributes((MMHandleType)hcamcorder, NULL,
		"capture-sound-enable", &sound_enable,
		NULL);
	_mmcam_dbg_log("Capture sound enable %d", sound_enable);
	if (!sound_enable) {
		_mmcam_dbg_warn("capture sound disabled");
		g_mutex_unlock(&info->open_mutex);
		return FALSE;
	}

	/* check shutter sound policy and status of system */
	if (hcamcorder->shutter_sound_policy == VCONFKEY_CAMERA_SHUTTER_SOUND_POLICY_OFF &&
	    hcamcorder->sub_context->info_image->sound_status == FALSE) {
		_mmcam_dbg_warn("skip sound init : Sound Policy OFF and Silent status");
		g_mutex_unlock(&info->open_mutex);
		return FALSE;
	}

	info->state = _MMCAMCORDER_SOUND_STATE_INIT;

	_mmcam_dbg_log("init DONE");

	g_mutex_unlock(&info->open_mutex);

	return TRUE;
}


gboolean _mmcamcorder_sound_play(MMHandleType handle, const char *sample_name, gboolean sync_play)
{
	int sound_enable = TRUE;
	const char *volume_gain = NULL;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	SOUND_INFO *info = NULL;

	mmf_return_val_if_fail(hcamcorder && sample_name, FALSE);

	/* check sound play enable */
	mm_camcorder_get_attributes((MMHandleType)hcamcorder, NULL,
		"capture-sound-enable", &sound_enable,
		NULL);
	_mmcam_dbg_log("Capture sound enable %d", sound_enable);
	if (!sound_enable) {
		_mmcam_dbg_warn("capture sound disabled");
		return FALSE;
	}

	info = &(hcamcorder->snd_info);

	g_mutex_lock(&info->open_mutex);

	if (info->state < _MMCAMCORDER_SOUND_STATE_INIT) {
		_mmcam_dbg_log("not initialized state:[%d]", info->state);
		g_mutex_unlock(&info->open_mutex);
		return FALSE;
	}

	if (!strcmp(sample_name, _MMCAMCORDER_SAMPLE_SOUND_NAME_CAPTURE02))
		volume_gain = "shutter2";
	else if (!strcmp(sample_name, _MMCAMCORDER_SAMPLE_SOUND_NAME_REC_STOP))
		volume_gain = "camcording";

	_mmcam_dbg_log("Play start - sample name [%s]", sample_name);

	_mmcamcorder_send_sound_play_message(hcamcorder->gdbus_conn,
		&hcamcorder->gdbus_info_sound, sample_name, "system", volume_gain, sync_play);

	g_mutex_unlock(&info->open_mutex);

	_mmcam_dbg_log("Done");

	return TRUE;
}


gboolean _mmcamcorder_sound_finalize(MMHandleType handle)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	SOUND_INFO *info = NULL;

	mmf_return_val_if_fail(hcamcorder, FALSE);

	info = &(hcamcorder->snd_info);

	_mmcam_dbg_err("START");

	g_mutex_lock(&info->open_mutex);

	if (info->state < _MMCAMCORDER_SOUND_STATE_INIT) {
		_mmcam_dbg_warn("not initialized");
		g_mutex_unlock(&info->open_mutex);
		return TRUE;
	}

	info->state = _MMCAMCORDER_SOUND_STATE_NONE;

	_mmcam_dbg_err("DONE");

	g_mutex_unlock(&info->open_mutex);

	return TRUE;
}


int _mmcamcorder_sound_solo_play(MMHandleType handle, const char *sample_name, gboolean sync_play)
{
	int sound_enable = TRUE;
	int current_state = MM_CAMCORDER_STATE_NONE;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	mmf_return_val_if_fail(hcamcorder && sample_name, FALSE);

	_mmcam_dbg_log("START : [%s]", sample_name);

	_mmcamcorder_sound_solo_play_wait(handle);

	current_state = _mmcamcorder_get_state(handle);
	if (current_state < MM_CAMCORDER_STATE_PREPARE) {
		_mmcam_dbg_warn("invalid state %d", current_state);
		return FALSE;
	}

	/* check sound play enable */
	mm_camcorder_get_attributes((MMHandleType)hcamcorder, NULL,
		"capture-sound-enable", &sound_enable,
		NULL);
	_mmcam_dbg_log("Capture sound enable %d", sound_enable);
	if (!sound_enable) {
		_mmcam_dbg_warn("capture sound disabled");
		return FALSE;
	}

	_mmcam_dbg_log("Play start - sample name [%s]", sample_name);

	if (hcamcorder->shutter_sound_policy == VCONFKEY_CAMERA_SHUTTER_SOUND_POLICY_ON ||
		hcamcorder->sub_context->info_image->sound_status) {
		_mmcamcorder_send_sound_play_message(hcamcorder->gdbus_conn,
			&hcamcorder->gdbus_info_solo_sound, sample_name, "system", "shutter1", sync_play);
	} else {
		_mmcam_dbg_warn("skip shutter sound : sound policy %d, sound status %d",
			hcamcorder->shutter_sound_policy, hcamcorder->sub_context->info_image->sound_status);
	}

	_mmcam_dbg_log("Done");

	return TRUE;
}


void _mmcamcorder_sound_solo_play_wait(MMHandleType handle)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	mmf_return_if_fail(hcamcorder);

	_mmcam_dbg_log("START");

	/* check playing sound count */
	g_mutex_lock(&hcamcorder->gdbus_info_solo_sound.sync_mutex);

	if (hcamcorder->gdbus_info_solo_sound.is_playing) {
		gint64 end_time = 0;

		_mmcam_dbg_log("Wait for signal");

		end_time = g_get_monotonic_time() + G_TIME_SPAN_SECOND;

		if (g_cond_wait_until(&hcamcorder->gdbus_info_solo_sound.sync_cond,
			&hcamcorder->gdbus_info_solo_sound.sync_mutex, end_time)) {
			_mmcam_dbg_log("signal received.");
		} else {
			_mmcam_dbg_warn("capture sound play timeout.");
		}
	} else {
		_mmcam_dbg_warn("no playing sound");
	}

	g_mutex_unlock(&hcamcorder->gdbus_info_solo_sound.sync_mutex);

	_mmcam_dbg_log("DONE");

	return;
}
