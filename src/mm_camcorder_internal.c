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
========================================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <gst/gst.h>
#include <gst/gstutils.h>
#include <gst/gstpad.h>
#include <sys/time.h>

#include <mm_error.h>
#include "mm_camcorder_internal.h"
#include <mm_types.h>

#include <gst/video/colorbalance.h>
#include <gst/video/cameracontrol.h>
#include <asm/types.h>

#include <system_info.h>

#ifdef _MMCAMCORDER_RM_SUPPORT
#include "mm_camcorder_rm.h"
#endif /* _MMCAMCORDER_RM_SUPPORT */

/*---------------------------------------------------------------------------------------
|    LOCAL VARIABLE DEFINITIONS for internal						|
---------------------------------------------------------------------------------------*/
#define __MMCAMCORDER_CMD_ITERATE_MAX           3
#define __MMCAMCORDER_SET_GST_STATE_TIMEOUT     5
#define __MMCAMCORDER_FORCE_STOP_TRY_COUNT      30
#define __MMCAMCORDER_FORCE_STOP_WAIT_TIME      100000  /* us */
#define __MMCAMCORDER_SOUND_WAIT_TIMEOUT        3
#define __MMCAMCORDER_FOCUS_CHANGE_REASON_LEN   64
#define __MMCAMCORDER_CONF_FILENAME_LENGTH      32

#define DPM_ALLOWED                             1
#define DPM_DISALLOWED                          0

/*---------------------------------------------------------------------------------------
|    LOCAL FUNCTION PROTOTYPES:								|
---------------------------------------------------------------------------------------*/
/* STATIC INTERNAL FUNCTION */
static gint     __mmcamcorder_init_handle(mmf_camcorder_t **hcamcorder, int device_type);
static void     __mmcamcorder_deinit_handle(mmf_camcorder_t *hcamcorder);
static gint     __mmcamcorder_init_configure_video_capture(mmf_camcorder_t *hcamcorder);
static gint     __mmcamcorder_init_configure_audio(mmf_camcorder_t *hcamcorder);
static void     __mmcamcorder_deinit_configure(mmf_camcorder_t *hcamcorder);
static gboolean __mmcamcorder_init_gstreamer(camera_conf *conf);
static void     __mmcamcorder_get_system_info(mmf_camcorder_t *hcamcorder);

static GstBusSyncReply __mmcamcorder_handle_gst_sync_error(mmf_camcorder_t *hcamcorder, GstMessage *message);
static GstBusSyncReply __mmcamcorder_gst_handle_sync_audio_error(mmf_camcorder_t *hcamcorder, gint err_code);
static GstBusSyncReply __mmcamcorder_gst_handle_sync_others_error(mmf_camcorder_t *hcamcorder, gint err_code);
static gboolean __mmcamcorder_handle_gst_error(MMHandleType handle, GstMessage *message, GError *error);
static gint     __mmcamcorder_gst_handle_stream_error(MMHandleType handle, int code, GstMessage *message);
static gint     __mmcamcorder_gst_handle_resource_error(MMHandleType handle, int code, GstMessage *message);
static gint     __mmcamcorder_gst_handle_library_error(MMHandleType handle, int code, GstMessage *message);
static gint     __mmcamcorder_gst_handle_core_error(MMHandleType handle, int code, GstMessage *message);
static gint     __mmcamcorder_gst_handle_resource_warning(MMHandleType handle, GstMessage *message , GError *error);
static gboolean __mmcamcorder_handle_gst_warning(MMHandleType handle, GstMessage *message, GError *error);
#ifdef _MMCAMCORDER_MM_RM_SUPPORT
static int      __mmcamcorder_resource_release_cb(mm_resource_manager_h rm,
	mm_resource_manager_res_h res, void *user_data);
#endif /* _MMCAMCORDER_MM_RM_SUPPORT */
#ifdef _MMCAMCORDER_USE_SET_ATTR_CB
static gboolean __mmcamcorder_set_attr_to_camsensor_cb(gpointer data);
#endif /* _MMCAMCORDER_USE_SET_ATTR_CB */

/*=======================================================================================
|  FUNCTION DEFINITIONS									|
=======================================================================================*/
static gint __mmcamcorder_init_handle(mmf_camcorder_t **hcamcorder, int device_type)
{
	int ret = MM_ERROR_NONE;
	mmf_camcorder_t *new_handle = NULL;

	if (!hcamcorder) {
		_mmcam_dbg_err("NULL handle");
		return MM_ERROR_CAMCORDER_INVALID_ARGUMENT;
	}

	/* Create mmf_camcorder_t handle and initialize every variable */
	new_handle = (mmf_camcorder_t *)malloc(sizeof(mmf_camcorder_t));
	if (!new_handle) {
		_mmcam_dbg_err("new handle allocation failed");
		return MM_ERROR_CAMCORDER_LOW_MEMORY;
	}

	memset(new_handle, 0x00, sizeof(mmf_camcorder_t));

	/* set device type */
	new_handle->device_type = device_type;

	_mmcam_dbg_warn("Device Type : %d", new_handle->device_type);

	new_handle->type = MM_CAMCORDER_MODE_VIDEO_CAPTURE;
	new_handle->state = MM_CAMCORDER_STATE_NONE;
	new_handle->old_state = MM_CAMCORDER_STATE_NONE;
	new_handle->capture_in_recording = FALSE;

	/* init mutex and cond */
	g_mutex_init(&(new_handle->mtsafe).lock);
	g_cond_init(&(new_handle->mtsafe).cond);
	g_mutex_init(&(new_handle->mtsafe).cmd_lock);
	g_cond_init(&(new_handle->mtsafe).cmd_cond);
	g_mutex_init(&(new_handle->mtsafe).interrupt_lock);
	g_mutex_init(&(new_handle->mtsafe).state_lock);
	g_mutex_init(&(new_handle->mtsafe).gst_state_lock);
	g_mutex_init(&(new_handle->mtsafe).gst_encode_state_lock);
	g_mutex_init(&(new_handle->mtsafe).message_cb_lock);
	g_mutex_init(&(new_handle->mtsafe).vcapture_cb_lock);
	g_mutex_init(&(new_handle->mtsafe).vstream_cb_lock);
	g_mutex_init(&(new_handle->mtsafe).astream_cb_lock);
	g_mutex_init(&(new_handle->mtsafe).mstream_cb_lock);
#ifdef _MMCAMCORDER_MM_RM_SUPPORT
	g_mutex_init(&(new_handle->mtsafe).resource_lock);
#endif /* _MMCAMCORDER_MM_RM_SUPPORT */

	g_mutex_init(&new_handle->restart_preview_lock);

	g_mutex_init(&new_handle->snd_info.open_mutex);
	g_cond_init(&new_handle->snd_info.open_cond);
	g_mutex_init(&new_handle->snd_info.play_mutex);
	g_cond_init(&new_handle->snd_info.play_cond);

	g_mutex_init(&new_handle->task_thread_lock);
	g_cond_init(&new_handle->task_thread_cond);
	new_handle->task_thread_state = _MMCAMCORDER_TASK_THREAD_STATE_NONE;

	if (device_type != MM_VIDEO_DEVICE_NONE) {
		new_handle->gdbus_info_sound.mm_handle = new_handle;
		g_mutex_init(&new_handle->gdbus_info_sound.sync_mutex);
		g_cond_init(&new_handle->gdbus_info_sound.sync_cond);

		new_handle->gdbus_info_solo_sound.mm_handle = new_handle;
		g_mutex_init(&new_handle->gdbus_info_solo_sound.sync_mutex);
		g_cond_init(&new_handle->gdbus_info_solo_sound.sync_cond);
	}

	/* create task thread */
	new_handle->task_thread = g_thread_try_new("MMCAM_TASK_THREAD",
		(GThreadFunc)_mmcamcorder_util_task_thread_func, (gpointer)new_handle, NULL);
	if (new_handle->task_thread == NULL) {
		_mmcam_dbg_err("_mmcamcorder_create::failed to create task thread");
		ret = MM_ERROR_CAMCORDER_RESOURCE_CREATION;
		goto _INIT_HANDLE_FAILED;
	}

	/* Get Camera Configure information from Camcorder INI file */
	ret = _mmcamcorder_conf_get_info((MMHandleType)new_handle, CONFIGURE_TYPE_MAIN, CONFIGURE_MAIN_FILE, &new_handle->conf_main);
	if (ret != MM_ERROR_NONE) {
		_mmcam_dbg_err("Failed to get configure(main) info.");
		goto _INIT_HANDLE_FAILED;
	}

	/* allocate attribute */
	new_handle->attributes = _mmcamcorder_alloc_attribute((MMHandleType)new_handle);
	if (!new_handle->attributes) {
		ret = MM_ERROR_CAMCORDER_RESOURCE_CREATION;
		goto _INIT_HANDLE_FAILED;
	}

	*hcamcorder = new_handle;

	_mmcam_dbg_log("new handle %p", new_handle);

	return MM_ERROR_NONE;

_INIT_HANDLE_FAILED:
	__mmcamcorder_deinit_handle(new_handle);
	return ret;
}


static void __mmcamcorder_deinit_handle(mmf_camcorder_t *hcamcorder)
{
	if (!hcamcorder) {
		_mmcam_dbg_err("NULL handle");
		return;
	}

	/* Remove exif info */
	if (hcamcorder->exif_info) {
		mm_exif_destory_exif_info(hcamcorder->exif_info);
		hcamcorder->exif_info = NULL;
	}

	/* remove attributes */
	if (hcamcorder->attributes) {
		_mmcamcorder_dealloc_attribute((MMHandleType)hcamcorder, hcamcorder->attributes);
		hcamcorder->attributes = 0;
	}

	/* Release configure info */
	__mmcamcorder_deinit_configure(hcamcorder);

	/* remove task thread */
	if (hcamcorder->task_thread) {
		g_mutex_lock(&hcamcorder->task_thread_lock);
		_mmcam_dbg_log("send signal for task thread exit");
		hcamcorder->task_thread_state = _MMCAMCORDER_TASK_THREAD_STATE_EXIT;
		g_cond_signal(&hcamcorder->task_thread_cond);
		g_mutex_unlock(&hcamcorder->task_thread_lock);
		g_thread_join(hcamcorder->task_thread);
		hcamcorder->task_thread = NULL;
	}

	/* Release lock, cond */
	g_mutex_clear(&(hcamcorder->mtsafe).lock);
	g_cond_clear(&(hcamcorder->mtsafe).cond);
	g_mutex_clear(&(hcamcorder->mtsafe).cmd_lock);
	g_cond_clear(&(hcamcorder->mtsafe).cmd_cond);
	g_mutex_clear(&(hcamcorder->mtsafe).interrupt_lock);
	g_mutex_clear(&(hcamcorder->mtsafe).state_lock);
	g_mutex_clear(&(hcamcorder->mtsafe).gst_state_lock);
	g_mutex_clear(&(hcamcorder->mtsafe).gst_encode_state_lock);
	g_mutex_clear(&(hcamcorder->mtsafe).message_cb_lock);
	g_mutex_clear(&(hcamcorder->mtsafe).vcapture_cb_lock);
	g_mutex_clear(&(hcamcorder->mtsafe).vstream_cb_lock);
	g_mutex_clear(&(hcamcorder->mtsafe).astream_cb_lock);
	g_mutex_clear(&(hcamcorder->mtsafe).mstream_cb_lock);
#ifdef _MMCAMCORDER_MM_RM_SUPPORT
	g_mutex_clear(&(hcamcorder->mtsafe).resource_lock);
#endif /* _MMCAMCORDER_MM_RM_SUPPORT */

	g_mutex_clear(&hcamcorder->snd_info.open_mutex);
	g_cond_clear(&hcamcorder->snd_info.open_cond);
	g_mutex_clear(&hcamcorder->restart_preview_lock);

	if (hcamcorder->device_type != MM_VIDEO_DEVICE_NONE) {
		g_mutex_clear(&hcamcorder->gdbus_info_sound.sync_mutex);
		g_cond_clear(&hcamcorder->gdbus_info_sound.sync_cond);
		g_mutex_clear(&hcamcorder->gdbus_info_solo_sound.sync_mutex);
		g_cond_clear(&hcamcorder->gdbus_info_solo_sound.sync_cond);
	}

	g_mutex_clear(&hcamcorder->task_thread_lock);
	g_cond_clear(&hcamcorder->task_thread_cond);

	if (hcamcorder->model_name)
		free(hcamcorder->model_name);

	if (hcamcorder->software_version)
		free(hcamcorder->software_version);

	/* Release handle */
	memset(hcamcorder, 0x00, sizeof(mmf_camcorder_t));

	free(hcamcorder);

	return;
}


static void __mmcamcorder_get_system_info(mmf_camcorder_t *hcamcorder)
{
	int ret = 0;

	if (!hcamcorder) {
		_mmcam_dbg_err("NULL handle");
		return;
	}

	/* get shutter sound policy */
	vconf_get_int(VCONFKEY_CAMERA_SHUTTER_SOUND_POLICY, &hcamcorder->shutter_sound_policy);
	_mmcam_dbg_log("current shutter sound policy : %d", hcamcorder->shutter_sound_policy);

	/* get model name */
	ret = system_info_get_platform_string("http://tizen.org/system/model_name", &hcamcorder->model_name);

	_mmcam_dbg_warn("model name [%s], ret 0x%x",
		hcamcorder->model_name ? hcamcorder->model_name : "NULL", ret);

	/* get software version */
	ret = system_info_get_platform_string("http://tizen.org/system/build.string", &hcamcorder->software_version);

	_mmcam_dbg_warn("software version [%s], ret 0x%x",
		hcamcorder->software_version ? hcamcorder->software_version : "NULL", ret);

	return;
}


static gint __mmcamcorder_init_configure_video_capture(mmf_camcorder_t *hcamcorder)
{
	int ret = MM_ERROR_NONE;
	int resolution_width = 0;
	int resolution_height = 0;
	int rcmd_fmt_capture = MM_PIXEL_FORMAT_YUYV;
	int rcmd_fmt_recording = MM_PIXEL_FORMAT_NV12;
	int rcmd_dpy_rotation = MM_DISPLAY_ROTATION_270;
	int play_capture_sound = TRUE;
	int camera_device_count = MM_VIDEO_DEVICE_NUM;
	int camera_default_flip = MM_FLIP_NONE;
	int camera_facing_direction = MM_CAMCORDER_CAMERA_FACING_DIRECTION_REAR;
	char *err_attr_name = NULL;
	char conf_file_name[__MMCAMCORDER_CONF_FILENAME_LENGTH] = {'\0',};
	MMCamAttrsInfo fps_info;

	if (!hcamcorder) {
		_mmcam_dbg_err("NULL handle");
		return MM_ERROR_CAMCORDER_NOT_INITIALIZED;
	}

	snprintf(conf_file_name, sizeof(conf_file_name), "%s%d.ini",
		CONFIGURE_CTRL_FILE_PREFIX, hcamcorder->device_type);

	_mmcam_dbg_log("Load control configure file [%d][%s]", hcamcorder->device_type, conf_file_name);

	ret = _mmcamcorder_conf_get_info((MMHandleType)hcamcorder,
		CONFIGURE_TYPE_CTRL, (const char *)conf_file_name, &hcamcorder->conf_ctrl);
	if (ret != MM_ERROR_NONE) {
		_mmcam_dbg_err("Failed to get configure(control) info.");
		return ret;
	}
/*
	_mmcamcorder_conf_print_info(&hcamcorder->conf_main);
	_mmcamcorder_conf_print_info(&hcamcorder->conf_ctrl);
*/
	ret = _mmcamcorder_init_convert_table((MMHandleType)hcamcorder);
	if (ret != MM_ERROR_NONE) {
		_mmcam_dbg_warn("converting table initialize error!!");
		return MM_ERROR_CAMCORDER_INTERNAL;
	}

	ret = _mmcamcorder_init_attr_from_configure((MMHandleType)hcamcorder, MM_CAMCONVERT_CATEGORY_ALL);
	if (ret != MM_ERROR_NONE) {
		_mmcam_dbg_warn("attribute initialize from configure error!!");
		return MM_ERROR_CAMCORDER_INTERNAL;
	}

	/* Get device info, recommend preview fmt and display rotation from INI */
	_mmcamcorder_conf_get_value_int((MMHandleType)hcamcorder, hcamcorder->conf_ctrl,
		CONFIGURE_CATEGORY_CTRL_CAMERA,
		"RecommendPreviewFormatCapture",
		&rcmd_fmt_capture);

	_mmcamcorder_conf_get_value_int((MMHandleType)hcamcorder, hcamcorder->conf_ctrl,
		CONFIGURE_CATEGORY_CTRL_CAMERA,
		"RecommendPreviewFormatRecord",
		&rcmd_fmt_recording);

	_mmcamcorder_conf_get_value_int((MMHandleType)hcamcorder, hcamcorder->conf_ctrl,
		CONFIGURE_CATEGORY_CTRL_CAMERA,
		"RecommendDisplayRotation",
		&rcmd_dpy_rotation);

	_mmcamcorder_conf_get_value_int((MMHandleType)hcamcorder, hcamcorder->conf_main,
		CONFIGURE_CATEGORY_MAIN_CAPTURE,
		"PlayCaptureSound",
		&play_capture_sound);

	_mmcamcorder_conf_get_value_int((MMHandleType)hcamcorder, hcamcorder->conf_main,
		CONFIGURE_CATEGORY_MAIN_VIDEO_INPUT,
		"DeviceCount",
		&camera_device_count);

	_mmcamcorder_conf_get_value_int((MMHandleType)hcamcorder, hcamcorder->conf_ctrl,
		CONFIGURE_CATEGORY_CTRL_CAMERA,
		"FacingDirection",
		&camera_facing_direction);

	_mmcamcorder_conf_get_value_int((MMHandleType)hcamcorder, hcamcorder->conf_ctrl,
		CONFIGURE_CATEGORY_CTRL_EFFECT,
		"BrightnessStepDenominator",
		&hcamcorder->brightness_step_denominator);

	_mmcamcorder_conf_get_value_int((MMHandleType)hcamcorder, hcamcorder->conf_ctrl,
		CONFIGURE_CATEGORY_CTRL_CAPTURE,
		"SupportZSL",
		&hcamcorder->support_zsl_capture);

	_mmcam_dbg_log("Recommend fmt[cap:%d,rec:%d], dpy rot %d, cap snd %d, dev cnt %d, cam facing dir %d, step denom %d, support zsl %d",
		rcmd_fmt_capture, rcmd_fmt_recording, rcmd_dpy_rotation,
		play_capture_sound, camera_device_count, camera_facing_direction,
		hcamcorder->brightness_step_denominator, hcamcorder->support_zsl_capture);

	/* Get UseZeroCopyFormat value from INI */
	_mmcamcorder_conf_get_value_int((MMHandleType)hcamcorder, hcamcorder->conf_main,
		CONFIGURE_CATEGORY_MAIN_VIDEO_INPUT,
		"UseZeroCopyFormat",
		&hcamcorder->use_zero_copy_format);

	/* Get SupportUserBuffer value from INI */
	_mmcamcorder_conf_get_value_int((MMHandleType)hcamcorder, hcamcorder->conf_main,
		CONFIGURE_CATEGORY_MAIN_VIDEO_INPUT,
		"SupportUserBuffer",
		&hcamcorder->support_user_buffer);

	/* Get SupportMediaPacketPreviewCb value from INI */
	_mmcamcorder_conf_get_value_int((MMHandleType)hcamcorder, hcamcorder->conf_main,
		CONFIGURE_CATEGORY_MAIN_VIDEO_INPUT,
		"SupportMediaPacketPreviewCb",
		&hcamcorder->support_media_packet_preview_cb);

	/* Get UseVideoconvert value from INI */
	_mmcamcorder_conf_get_value_int((MMHandleType)hcamcorder, hcamcorder->conf_main,
		CONFIGURE_CATEGORY_MAIN_VIDEO_OUTPUT,
		"UseVideoconvert",
		&hcamcorder->use_videoconvert);

	ret = mm_camcorder_get_attributes((MMHandleType)hcamcorder, NULL,
		MMCAM_CAMERA_WIDTH, &resolution_width,
		MMCAM_CAMERA_HEIGHT, &resolution_height,
		NULL);
	if (ret != MM_ERROR_NONE) {
		_mmcam_dbg_err("get attribute[resolution] error");
		return MM_ERROR_CAMCORDER_INTERNAL;
	}

	ret = mm_camcorder_get_fps_list_by_resolution((MMHandleType)hcamcorder, resolution_width, resolution_height, &fps_info);
	if (ret != MM_ERROR_NONE) {
		_mmcam_dbg_err("get fps list failed with [%dx%d]", resolution_width, resolution_height);
		return MM_ERROR_CAMCORDER_INTERNAL;
	}

	_mmcam_dbg_log("ZeroCopy %d, UserBuffer %d, Videoconvert %d, MPPreviewCb %d",
		hcamcorder->use_zero_copy_format, hcamcorder->support_user_buffer,
		hcamcorder->use_videoconvert, hcamcorder->support_media_packet_preview_cb);

	_mmcam_dbg_log("res : %d X %d, Default FPS by resolution  : %d",
		resolution_width, resolution_height, fps_info.int_array.def);

	if (camera_facing_direction == 1) {
		if (rcmd_dpy_rotation == MM_DISPLAY_ROTATION_270 || rcmd_dpy_rotation == MM_DISPLAY_ROTATION_90)
			camera_default_flip = MM_FLIP_VERTICAL;
		else
			camera_default_flip = MM_FLIP_HORIZONTAL;

		_mmcam_dbg_log("camera_default_flip : [%d]", camera_default_flip);
	}

	ret = mm_camcorder_set_attributes((MMHandleType)hcamcorder, &err_attr_name,
		MMCAM_CAMERA_DEVICE_COUNT, camera_device_count,
		MMCAM_CAMERA_FACING_DIRECTION, camera_facing_direction,
		MMCAM_RECOMMEND_PREVIEW_FORMAT_FOR_CAPTURE, rcmd_fmt_capture,
		MMCAM_RECOMMEND_PREVIEW_FORMAT_FOR_RECORDING, rcmd_fmt_recording,
		MMCAM_RECOMMEND_DISPLAY_ROTATION, rcmd_dpy_rotation,
		MMCAM_SUPPORT_ZSL_CAPTURE, hcamcorder->support_zsl_capture,
		MMCAM_SUPPORT_ZERO_COPY_FORMAT, hcamcorder->use_zero_copy_format,
		MMCAM_SUPPORT_MEDIA_PACKET_PREVIEW_CB, hcamcorder->support_media_packet_preview_cb,
		MMCAM_SUPPORT_USER_BUFFER, hcamcorder->support_user_buffer,
		MMCAM_CAMERA_FPS, fps_info.int_array.def,
		MMCAM_DISPLAY_FLIP, camera_default_flip,
		MMCAM_CAPTURE_SOUND_ENABLE, play_capture_sound,
		NULL);
	if (ret != MM_ERROR_NONE) {
		_mmcam_dbg_err("[0x%x] Set %s FAILED.", ret, err_attr_name ? err_attr_name : "[UNKNOWN]");
		SAFE_FREE(err_attr_name);
		return MM_ERROR_CAMCORDER_INTERNAL;
	}

	/* Get default value of brightness */
	ret = mm_camcorder_get_attributes((MMHandleType)hcamcorder, NULL,
		MMCAM_FILTER_BRIGHTNESS, &hcamcorder->brightness_default,
		NULL);
	if (ret != MM_ERROR_NONE) {
		_mmcam_dbg_err("Get brightness FAILED.");
		return MM_ERROR_CAMCORDER_INTERNAL;
	}

	_mmcam_dbg_log("Default brightness : %d", hcamcorder->brightness_default);

	return ret;
}


static gint __mmcamcorder_init_configure_audio(mmf_camcorder_t *hcamcorder)
{
	int ret = MM_ERROR_NONE;
	int camera_device_count = 0;

	if (!hcamcorder) {
		_mmcam_dbg_err("NULL handle");
		return MM_ERROR_CAMCORDER_NOT_INITIALIZED;
	}

	_mmcamcorder_conf_get_value_int((MMHandleType)hcamcorder, hcamcorder->conf_main,
		CONFIGURE_CATEGORY_MAIN_VIDEO_INPUT,
		"DeviceCount",
		&camera_device_count);

	ret = mm_camcorder_set_attributes((MMHandleType)hcamcorder, NULL,
		MMCAM_CAMERA_DEVICE_COUNT, camera_device_count,
		NULL);
	if (ret != MM_ERROR_NONE) {
		_mmcam_dbg_err("Set device count FAILED");
		return MM_ERROR_CAMCORDER_INTERNAL;
	}

	ret = _mmcamcorder_init_attr_from_configure((MMHandleType)hcamcorder, MM_CAMCONVERT_CATEGORY_AUDIO);
	if (ret != MM_ERROR_NONE) {
		_mmcam_dbg_err("there is no audio device");
		return MM_ERROR_CAMCORDER_NOT_SUPPORTED;
	}

	return ret;
}


static void __mmcamcorder_deinit_configure(mmf_camcorder_t *hcamcorder)
{
	if (!hcamcorder) {
		_mmcam_dbg_err("NULL handle");
		return;
	}

	if (hcamcorder->conf_ctrl)
		_mmcamcorder_conf_release_info((MMHandleType)hcamcorder, &hcamcorder->conf_ctrl);

	if (hcamcorder->conf_main)
		_mmcamcorder_conf_release_info((MMHandleType)hcamcorder, &hcamcorder->conf_main);

	return;
}


/*---------------------------------------------------------------------------------------
|    GLOBAL FUNCTION DEFINITIONS:							|
---------------------------------------------------------------------------------------*/
/* Internal command functions {*/
int _mmcamcorder_create(MMHandleType *handle, MMCamPreset *info)
{
	int ret = MM_ERROR_NONE;
	mmf_camcorder_t *hcamcorder = NULL;

	_mmcam_dbg_log("Entered");

	mmf_return_val_if_fail(handle && info, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	/* check device type */
	if (info->videodev_type < MM_VIDEO_DEVICE_NONE ||
	    info->videodev_type >= MM_VIDEO_DEVICE_NUM) {
		_mmcam_dbg_err("video device type[%d] is out of range.", info->videodev_type);
		return MM_ERROR_CAMCORDER_INVALID_ARGUMENT;
	}

	/* init handle */
	ret = __mmcamcorder_init_handle(&hcamcorder, info->videodev_type);
	if (ret != MM_ERROR_NONE)
		return ret;

	/* get DPM handle for camera/microphone restriction */
	hcamcorder->dpm_handle = dpm_manager_create();

	_mmcam_dbg_warn("DPM handle %p", hcamcorder->dpm_handle);

	if (hcamcorder->device_type != MM_VIDEO_DEVICE_NONE) {
		ret = __mmcamcorder_init_configure_video_capture(hcamcorder);
		if (ret != MM_ERROR_NONE) {
			goto _ERR_DEFAULT_VALUE_INIT;
		}

		/* add DPM camera policy changed callback */
		if (hcamcorder->dpm_handle) {
			ret = dpm_add_policy_changed_cb(hcamcorder->dpm_handle, "camera",
				_mmcamcorder_dpm_camera_policy_changed_cb, (void *)hcamcorder, &hcamcorder->dpm_camera_cb_id);
			if (ret != DPM_ERROR_NONE) {
				_mmcam_dbg_err("add DPM changed cb failed, keep going...");
				hcamcorder->dpm_camera_cb_id = 0;
			}

			_mmcam_dbg_log("DPM camera changed cb id %d", hcamcorder->dpm_camera_cb_id);
		}

#ifdef _MMCAMCORDER_MM_RM_SUPPORT
		/* initialize resource manager */
		ret = mm_resource_manager_create(MM_RESOURCE_MANAGER_APP_CLASS_MEDIA,
				__mmcamcorder_resource_release_cb, hcamcorder,
				&hcamcorder->resource_manager);
		if (ret != MM_RESOURCE_MANAGER_ERROR_NONE) {
			_mmcam_dbg_err("failed to initialize resource manager");
			ret = MM_ERROR_CAMCORDER_INTERNAL;
			goto _ERR_DEFAULT_VALUE_INIT;
		}
#endif /* _MMCAMCORDER_MM_RM_SUPPORT */
	} else {
		ret = __mmcamcorder_init_configure_audio(hcamcorder);
		if (ret != MM_ERROR_NONE) {
			goto _ERR_DEFAULT_VALUE_INIT;
		}
	}

	traceBegin(TTRACE_TAG_CAMERA, "MMCAMCORDER:CREATE:INIT_GSTREAMER");

	ret = __mmcamcorder_init_gstreamer(hcamcorder->conf_main);

	traceEnd(TTRACE_TAG_CAMERA);

	if (!ret) {
		_mmcam_dbg_err("Failed to initialize gstreamer!!");
		ret = MM_ERROR_CAMCORDER_NOT_INITIALIZED;
		goto _ERR_DEFAULT_VALUE_INIT;
	}

	/* Make some attributes as read-only type */
	_mmcamcorder_lock_readonly_attributes((MMHandleType)hcamcorder);

	/* Disable attributes in each model */
	_mmcamcorder_set_disabled_attributes((MMHandleType)hcamcorder);

	/* get system information */
	__mmcamcorder_get_system_info(hcamcorder);

	/* Set initial state */
	_mmcamcorder_set_state((MMHandleType)hcamcorder, MM_CAMCORDER_STATE_NULL);

	_mmcam_dbg_log("created handle %p", hcamcorder);

	*handle = (MMHandleType)hcamcorder;

	return MM_ERROR_NONE;

_ERR_DEFAULT_VALUE_INIT:
#ifdef _MMCAMCORDER_MM_RM_SUPPORT
	/* de-initialize resource manager */
	if (hcamcorder->resource_manager != NULL)
		mm_resource_manager_destroy(hcamcorder->resource_manager);
#endif /* _MMCAMCORDER_MM_RM_SUPPORT */

	/* release DPM related handle */
	if (hcamcorder->dpm_handle) {
		_mmcam_dbg_log("release DPM handle %p, camera changed cb id %d",
			hcamcorder->dpm_handle, hcamcorder->dpm_camera_cb_id);

		/* remove camera policy changed callback */
		if (hcamcorder->dpm_camera_cb_id > 0) {
			dpm_remove_policy_changed_cb(hcamcorder->dpm_handle, hcamcorder->dpm_camera_cb_id);
			hcamcorder->dpm_camera_cb_id = 0;
		} else {
			_mmcam_dbg_warn("invalid dpm camera cb id %d", hcamcorder->dpm_camera_cb_id);
		}

		dpm_manager_destroy(hcamcorder->dpm_handle);
		hcamcorder->dpm_handle = NULL;
	}

	/* release configure info */
	__mmcamcorder_deinit_configure(hcamcorder);

	/* deinitialize handle */
	__mmcamcorder_deinit_handle(hcamcorder);

	return ret;
}


int _mmcamcorder_destroy(MMHandleType handle)
{
	int ret = MM_ERROR_NONE;
	int state = MM_CAMCORDER_STATE_NONE;
	GstElement *sink_element = NULL;
	int sink_element_size = 0;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	_mmcam_dbg_log("");

	if (!hcamcorder) {
		_mmcam_dbg_err("Not initialized");
		ret = MM_ERROR_CAMCORDER_NOT_INITIALIZED;
		goto _ERR_CAMCORDER_CMD_PRECON;
	}

	if (!_MMCAMCORDER_TRYLOCK_CMD(hcamcorder)) {
		_mmcam_dbg_err("Another command is running.");
		ret = MM_ERROR_CAMCORDER_CMD_IS_RUNNING;
		goto _ERR_CAMCORDER_CMD_PRECON;
	}

	state = _mmcamcorder_get_state(handle);
	if (state != MM_CAMCORDER_STATE_NULL) {
		_mmcam_dbg_err("Wrong state(%d)", state);
		ret = MM_ERROR_CAMCORDER_INVALID_STATE;
		goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
	}

	/* remove kept, but not used sink element in attribute */
	mm_camcorder_get_attributes(handle, NULL,
		MMCAM_DISPLAY_REUSE_ELEMENT, &sink_element, &sink_element_size,
		NULL);
	if (sink_element) {
		GstStateChangeReturn result = gst_element_set_state(sink_element, GST_STATE_NULL);

		_mmcam_dbg_warn("remove sink element %p, set state NULL result %d", sink_element, result);

		gst_object_unref(sink_element);
		sink_element = NULL;
	}

	/* wait for completion of sound play */
	_mmcamcorder_sound_solo_play_wait(handle);

	/* Release SubContext and pipeline */
	if (hcamcorder->sub_context) {
		if (hcamcorder->sub_context->element)
			_mmcamcorder_destroy_pipeline(handle, hcamcorder->type);

		_mmcamcorder_dealloc_subcontext(hcamcorder->sub_context);
		hcamcorder->sub_context = NULL;
	}

#ifdef _MMCAMCORDER_MM_RM_SUPPORT
	/* de-initialize resource manager */
	_MMCAMCORDER_LOCK_RESOURCE(hcamcorder);

	if (hcamcorder->resource_manager != NULL) {
		ret = mm_resource_manager_destroy(hcamcorder->resource_manager);
		if (ret != MM_RESOURCE_MANAGER_ERROR_NONE)
			_mmcam_dbg_err("failed to de-initialize resource manager 0x%x", ret);
	}

	_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);
#endif /* _MMCAMCORDER_MM_RM_SUPPORT */

	/* Remove idle function which is not called yet */
	if (hcamcorder->setting_event_id) {
		_mmcam_dbg_log("Remove remaining idle function");
		g_source_remove(hcamcorder->setting_event_id);
		hcamcorder->setting_event_id = 0;
	}

	/* Remove messages which are not called yet */
	_mmcamcorder_remove_message_all(handle);

#ifdef _MMCAMCORDER_RM_SUPPORT
	_mmcamcorder_rm_release(handle);
#endif /* _MMCAMCORDER_RM_SUPPORT */

	/* release DPM handle */
	if (hcamcorder->dpm_handle) {
		_mmcam_dbg_log("release DPM handle %p, camera changed cb id %d",
			hcamcorder->dpm_handle, hcamcorder->dpm_camera_cb_id);

		/* remove camera policy changed callback */
		if (hcamcorder->dpm_camera_cb_id > 0) {
			dpm_remove_policy_changed_cb(hcamcorder->dpm_handle, hcamcorder->dpm_camera_cb_id);
			hcamcorder->dpm_camera_cb_id = 0;
		} else {
			_mmcam_dbg_warn("invalid dpm camera cb id %d", hcamcorder->dpm_camera_cb_id);
		}

		dpm_manager_destroy(hcamcorder->dpm_handle);
		hcamcorder->dpm_handle = NULL;
	}

	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

	__mmcamcorder_deinit_handle(hcamcorder);

	return MM_ERROR_NONE;

_ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK:
	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

_ERR_CAMCORDER_CMD_PRECON:
	if (hcamcorder)
		_mmcam_dbg_err("Destroy fail (type %d, state %d)", hcamcorder->type, state);

	_mmcam_dbg_err("Destroy fail (ret %x)", ret);

	return ret;
}


int _mmcamcorder_realize(MMHandleType handle)
{
	int ret = MM_ERROR_NONE;
	int state = MM_CAMCORDER_STATE_NONE;
	int display_surface_type = MM_DISPLAY_SURFACE_OVERLAY;
	double motion_rate = _MMCAMCORDER_DEFAULT_RECORDING_MOTION_RATE;
	char *videosink_element_type = NULL;
	const char *videosink_name = NULL;
	char *socket_path = NULL;
	int socket_path_len = 0;
	int conn_size = 0;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	/*_mmcam_dbg_log("");*/

	if (!hcamcorder) {
		_mmcam_dbg_err("Not initialized");
		ret = MM_ERROR_CAMCORDER_NOT_INITIALIZED;
		return ret;
	}

	if (!_MMCAMCORDER_TRYLOCK_CMD(hcamcorder)) {
		_mmcam_dbg_err("Another command is running.");
		ret = MM_ERROR_CAMCORDER_CMD_IS_RUNNING;
		goto _ERR_CAMCORDER_CMD_PRECON;
	}

	state = _mmcamcorder_get_state(handle);
	if (state != MM_CAMCORDER_STATE_NULL) {
		_mmcam_dbg_err("Wrong state(%d)", state);
		ret = MM_ERROR_CAMCORDER_INVALID_STATE;
		goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
	}

	/* Get profile mode and gdbus connection */
	mm_camcorder_get_attributes(handle, NULL,
		MMCAM_MODE, &hcamcorder->type,
		MMCAM_GDBUS_CONNECTION, &hcamcorder->gdbus_conn, &conn_size,
		NULL);

	if (!hcamcorder->gdbus_conn) {
		_mmcam_dbg_err("gdbus connection NULL");
		ret = MM_ERROR_CAMCORDER_RESOURCE_CREATION;
		goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
	}

	_mmcam_dbg_log("Profile mode [%d], gdbus connection [%p]",
		hcamcorder->type, hcamcorder->gdbus_conn);

	mm_camcorder_get_attributes(handle, NULL,
		MMCAM_DISPLAY_SURFACE, &display_surface_type,
		MMCAM_CAMERA_RECORDING_MOTION_RATE, &motion_rate,
		NULL);

	/* alloc sub context */
	hcamcorder->sub_context = _mmcamcorder_alloc_subcontext(hcamcorder->type);
	if (!hcamcorder->sub_context) {
		ret = MM_ERROR_CAMCORDER_RESOURCE_CREATION;
		goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
	}

	/* Set basic configure information */
	if (motion_rate != _MMCAMCORDER_DEFAULT_RECORDING_MOTION_RATE)
		hcamcorder->sub_context->is_modified_rate = TRUE;

	_mmcamcorder_conf_get_value_int(handle, hcamcorder->conf_main,
		CONFIGURE_CATEGORY_MAIN_CAPTURE,
		"UseEncodebin",
		&(hcamcorder->sub_context->bencbin_capture));
	_mmcam_dbg_warn("UseEncodebin [%d]", hcamcorder->sub_context->bencbin_capture);

	if (hcamcorder->type != MM_CAMCORDER_MODE_AUDIO) {
		/* get dual stream support info */
		_mmcamcorder_conf_get_value_int(handle, hcamcorder->conf_main,
			CONFIGURE_CATEGORY_MAIN_RECORD,
			"SupportDualStream",
			&(hcamcorder->sub_context->info_video->support_dual_stream));

		_mmcam_dbg_warn("SupportDualStream [%d]", hcamcorder->sub_context->info_video->support_dual_stream);
	}

	switch (display_surface_type) {
	case MM_DISPLAY_SURFACE_OVERLAY:
		videosink_element_type = strdup("VideosinkElementOverlay");
		break;
	case MM_DISPLAY_SURFACE_EVAS:
		videosink_element_type = strdup("VideosinkElementEvas");
		break;
	case MM_DISPLAY_SURFACE_GL:
		videosink_element_type = strdup("VideosinkElementGL");
		break;
	case MM_DISPLAY_SURFACE_NULL:
		videosink_element_type = strdup("VideosinkElementNull");
		break;
	case MM_DISPLAY_SURFACE_REMOTE:
		mm_camcorder_get_attributes(handle, NULL,
			MMCAM_DISPLAY_SOCKET_PATH, &socket_path, &socket_path_len,
			NULL);
		if (socket_path == NULL) {
			_mmcam_dbg_warn("REMOTE surface, but socket path is NULL -> to NullSink");
			videosink_element_type = strdup("VideosinkElementNull");
		} else {
			videosink_element_type = strdup("VideosinkElementRemote");
		}
		break;
	default:
		videosink_element_type = strdup("VideosinkElementOverlay");
		break;
	}

	/* check string of videosink element */
	if (videosink_element_type) {
		_mmcamcorder_conf_get_element(handle, hcamcorder->conf_main,
			CONFIGURE_CATEGORY_MAIN_VIDEO_OUTPUT,
			videosink_element_type,
			&hcamcorder->sub_context->VideosinkElement);
		free(videosink_element_type);
		videosink_element_type = NULL;
	} else {
		_mmcam_dbg_warn("strdup failed(display_surface_type %d). Use default X type",
			display_surface_type);

		_mmcamcorder_conf_get_element(handle, hcamcorder->conf_main,
			CONFIGURE_CATEGORY_MAIN_VIDEO_OUTPUT,
			_MMCAMCORDER_DEFAULT_VIDEOSINK_TYPE,
			&hcamcorder->sub_context->VideosinkElement);
	}

	_mmcamcorder_conf_get_value_element_name(hcamcorder->sub_context->VideosinkElement, &videosink_name);
	_mmcam_dbg_log("Videosink name : %s", videosink_name);

	/* get videoconvert element */
	_mmcamcorder_conf_get_element(handle, hcamcorder->conf_main,
		CONFIGURE_CATEGORY_MAIN_VIDEO_OUTPUT,
		"VideoconvertElement",
		&hcamcorder->sub_context->VideoconvertElement);

	_mmcamcorder_conf_get_value_int(handle, hcamcorder->conf_ctrl,
		CONFIGURE_CATEGORY_CTRL_CAPTURE,
		"SensorEncodedCapture",
		&(hcamcorder->sub_context->SensorEncodedCapture));
	_mmcam_dbg_log("Support sensor encoded capture : %d", hcamcorder->sub_context->SensorEncodedCapture);

	if (hcamcorder->type == MM_CAMCORDER_MODE_VIDEO_CAPTURE) {
		int dpm_camera_state = DPM_ALLOWED;

		/* check camera policy from DPM */
		if (hcamcorder->dpm_handle) {
			if (dpm_restriction_get_camera_state(hcamcorder->dpm_handle, &dpm_camera_state) == DPM_ERROR_NONE) {
				_mmcam_dbg_log("DPM camera state %d", dpm_camera_state);
				if (dpm_camera_state == DPM_DISALLOWED) {
					_mmcam_dbg_err("CAMERA DISALLOWED by DPM");
					ret = MM_ERROR_POLICY_RESTRICTED;

					_mmcamcorder_request_dpm_popup(hcamcorder->gdbus_conn, "camera");

					goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
				}
			} else {
				_mmcam_dbg_err("get DPM camera state failed, keep going...");
			}
		} else {
			_mmcam_dbg_warn("NULL dpm_handle");
		}

#ifdef _MMCAMCORDER_MM_RM_SUPPORT
		_MMCAMCORDER_LOCK_RESOURCE(hcamcorder);
		/* prepare resource manager for camera */
		if (hcamcorder->camera_resource == NULL) {
			ret = mm_resource_manager_mark_for_acquire(hcamcorder->resource_manager,
					MM_RESOURCE_MANAGER_RES_TYPE_CAMERA,
					1,
					&hcamcorder->camera_resource);
			if (ret != MM_RESOURCE_MANAGER_ERROR_NONE) {
				_mmcam_dbg_err("could not prepare for camera resource");
				ret = MM_ERROR_RESOURCE_INTERNAL;
				_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);
				goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
			}
		} else {
			_mmcam_dbg_log("camera already acquired");
		}

		/* prepare resource manager for "video_overlay only if display surface is X" */
		if (display_surface_type == MM_DISPLAY_SURFACE_OVERLAY) {
			if (hcamcorder->video_overlay_resource == NULL) {
				ret = mm_resource_manager_mark_for_acquire(hcamcorder->resource_manager,
						MM_RESOURCE_MANAGER_RES_TYPE_VIDEO_OVERLAY,
						MM_RESOURCE_MANAGER_RES_VOLUME_FULL,
						&hcamcorder->video_overlay_resource);
				if (ret != MM_RESOURCE_MANAGER_ERROR_NONE) {
					_mmcam_dbg_err("could not prepare for overlay resource");
					ret = MM_ERROR_RESOURCE_INTERNAL;
					_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);
					goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
				}
			} else {
				_mmcam_dbg_log("overlay already acquired");
			}
		}

		/* acquire resources */
		ret = mm_resource_manager_commit(hcamcorder->resource_manager);
		if (ret != MM_RESOURCE_MANAGER_ERROR_NONE) {
			_mmcam_dbg_err("could not acquire resources");
			ret = MM_ERROR_RESOURCE_INTERNAL;
			_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);
			goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
		}
		_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);
#endif /* _MMCAMCORDER_MM_RM_SUPPORT */

#ifdef _MMCAMCORDER_RM_SUPPORT
		ret = _mmcamcorder_rm_create(handle);
		if (ret != MM_ERROR_NONE) {
			_mmcam_dbg_err("Resource create failed");
			goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
		}
		ret = _mmcamcorder_rm_allocate(handle);
		if (ret != MM_ERROR_NONE) {
			_mmcam_dbg_err("Resource allocation request failed");
			_mmcamcorder_rm_release(handle);
			goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
		}
#endif /* _MMCAMCORDER_RM_SUPPORT */
	}

	/* create pipeline */
	traceBegin(TTRACE_TAG_CAMERA, "MMCAMCORDER:REALIZE:CREATE_PIPELINE");

	ret = _mmcamcorder_create_pipeline(handle, hcamcorder->type);

	traceEnd(TTRACE_TAG_CAMERA);

	if (ret != MM_ERROR_NONE) {
		/* check internal error of gstreamer */
		if (hcamcorder->error_code != MM_ERROR_NONE) {
			ret = hcamcorder->error_code;
			_mmcam_dbg_log("gstreamer error is occurred. return it %x", ret);
		}

		/* release sub context */
		_mmcamcorder_dealloc_subcontext(hcamcorder->sub_context);
		hcamcorder->sub_context = NULL;
		goto _ERR_CAMCORDER_CMD;
	}

	/* set command function */
	ret = _mmcamcorder_set_functions(handle, hcamcorder->type);
	if (ret != MM_ERROR_NONE) {
		_mmcamcorder_destroy_pipeline(handle, hcamcorder->type);
		_mmcamcorder_dealloc_subcontext(hcamcorder->sub_context);
		hcamcorder->sub_context = NULL;
		goto _ERR_CAMCORDER_CMD;
	}

	_mmcamcorder_set_state(handle, MM_CAMCORDER_STATE_READY);

	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

	return MM_ERROR_NONE;

_ERR_CAMCORDER_CMD:
#ifdef _MMCAMCORDER_MM_RM_SUPPORT
	/* release hw resources */
	_MMCAMCORDER_LOCK_RESOURCE(hcamcorder);
	if (hcamcorder->type == MM_CAMCORDER_MODE_VIDEO_CAPTURE) {
		if (hcamcorder->camera_resource != NULL) {
			mm_resource_manager_mark_for_release(hcamcorder->resource_manager,
					hcamcorder->camera_resource);
			hcamcorder->camera_resource = NULL;
		}
		if (hcamcorder->video_overlay_resource != NULL) {
			mm_resource_manager_mark_for_release(hcamcorder->resource_manager,
					hcamcorder->video_overlay_resource);
			hcamcorder->video_overlay_resource = NULL;
		}
		mm_resource_manager_commit(hcamcorder->resource_manager);
	}
	_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);
#endif /* _MMCAMCORDER_MM_RM_SUPPORT */
#ifdef _MMCAMCORDER_RM_SUPPORT
	_mmcamcorder_rm_deallocate(handle);
	_mmcamcorder_rm_release(handle);
#endif /* _MMCAMCORDER_RM_SUPPORT*/

_ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK:
	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

_ERR_CAMCORDER_CMD_PRECON:
	_mmcam_dbg_err("Realize fail (type %d, state %d, ret %x)", hcamcorder->type, state, ret);

	return ret;
}


int _mmcamcorder_unrealize(MMHandleType handle)
{
	int ret = MM_ERROR_NONE;
	int state = MM_CAMCORDER_STATE_NONE;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	_mmcam_dbg_log("");

	if (!hcamcorder) {
		_mmcam_dbg_err("Not initialized");
		ret = MM_ERROR_CAMCORDER_NOT_INITIALIZED;
		return ret;
	}

	if (!_MMCAMCORDER_TRYLOCK_CMD(hcamcorder)) {
		_mmcam_dbg_err("Another command is running.");
		ret = MM_ERROR_CAMCORDER_CMD_IS_RUNNING;
		goto _ERR_CAMCORDER_CMD_PRECON;
	}

	state = _mmcamcorder_get_state(handle);
	if (state != MM_CAMCORDER_STATE_READY) {
		_mmcam_dbg_err("Wrong state(%d)", state);
		ret = MM_ERROR_CAMCORDER_INVALID_STATE;
		goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
	}

	/* Release SubContext */
	if (hcamcorder->sub_context) {
		/* destroy pipeline */
		_mmcamcorder_destroy_pipeline(handle, hcamcorder->type);
		/* Deallocate SubContext */
		_mmcamcorder_dealloc_subcontext(hcamcorder->sub_context);
		hcamcorder->sub_context = NULL;
	}

#ifdef _MMCAMCORDER_MM_RM_SUPPORT
	_MMCAMCORDER_LOCK_RESOURCE(hcamcorder);
	_mmcam_dbg_warn("lock resource - cb calling %d", hcamcorder->is_release_cb_calling);

	if (hcamcorder->type == MM_CAMCORDER_MODE_VIDEO_CAPTURE &&
		hcamcorder->state_change_by_system != _MMCAMCORDER_STATE_CHANGE_BY_RM &&
		hcamcorder->is_release_cb_calling == FALSE) {

		/* release resource */
		if (hcamcorder->camera_resource != NULL) {
			ret = mm_resource_manager_mark_for_release(hcamcorder->resource_manager,
					hcamcorder->camera_resource);
			if (ret != MM_RESOURCE_MANAGER_ERROR_NONE) {
				_mmcam_dbg_err("could not mark camera resource for release");
				ret = MM_ERROR_CAMCORDER_INTERNAL;
				_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);
				_mmcam_dbg_log("unlock resource");
				goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
			}
		}

		if (hcamcorder->video_overlay_resource != NULL) {
			ret = mm_resource_manager_mark_for_release(hcamcorder->resource_manager,
					hcamcorder->video_overlay_resource);
			if (ret != MM_RESOURCE_MANAGER_ERROR_NONE) {
				_mmcam_dbg_err("could not mark overlay resource for release");
				ret = MM_ERROR_CAMCORDER_INTERNAL;
				_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);
				_mmcam_dbg_log("unlock resource");
				goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
			}
		}

		ret = mm_resource_manager_commit(hcamcorder->resource_manager);

		if (ret != MM_RESOURCE_MANAGER_ERROR_NONE) {
			_mmcam_dbg_err("failed to release resource, ret(0x%x)", ret);
			ret = MM_ERROR_CAMCORDER_INTERNAL;

			_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);
			_mmcam_dbg_log("unlock resource");

			goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
		} else {
			if (hcamcorder->camera_resource != NULL)
				hcamcorder->camera_resource = NULL;
			if (hcamcorder->video_overlay_resource != NULL)
				hcamcorder->video_overlay_resource = NULL;
		}
	}

	_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);

	_mmcam_dbg_warn("unlock resource");
#endif /* _MMCAMCORDER_MM_RM_SUPPORT */

#ifdef _MMCAMCORDER_RM_SUPPORT
	_mmcamcorder_rm_deallocate(handle);
#endif /* _MMCAMCORDER_RM_SUPPORT*/

	/* Deinitialize main context member */
	hcamcorder->command = NULL;

	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

	_mmcamcorder_set_state(handle, MM_CAMCORDER_STATE_NULL);

	return MM_ERROR_NONE;

_ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK:
	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

_ERR_CAMCORDER_CMD_PRECON:
	/* send message */
	_mmcam_dbg_err("Unrealize fail (type %d, state %d, ret %x)", hcamcorder->type, state, ret);

	return ret;
}

int _mmcamcorder_start(MMHandleType handle)
{
	int ret = MM_ERROR_NONE;
	int state = MM_CAMCORDER_STATE_NONE;

	_MMCamcorderSubContext *sc = NULL;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	_mmcam_dbg_log("");

	if (!hcamcorder) {
		_mmcam_dbg_err("Not initialized");
		ret = MM_ERROR_CAMCORDER_NOT_INITIALIZED;
		return ret;
	}

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	if (!_MMCAMCORDER_TRYLOCK_CMD(hcamcorder)) {
		_mmcam_dbg_err("Another command is running.");
		ret = MM_ERROR_CAMCORDER_CMD_IS_RUNNING;
		goto _ERR_CAMCORDER_CMD_PRECON;
	}

	state = _mmcamcorder_get_state(handle);
	if (state != MM_CAMCORDER_STATE_READY) {
		_mmcam_dbg_err("Wrong state(%d)", state);
		ret = MM_ERROR_CAMCORDER_INVALID_STATE;
		goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
	}

	/* initialize error code */
	hcamcorder->error_code = MM_ERROR_NONE;

	/* set attributes related sensor */
	if (hcamcorder->type != MM_CAMCORDER_MODE_AUDIO)
		_mmcamcorder_set_attribute_to_camsensor(handle);

	ret = hcamcorder->command((MMHandleType)hcamcorder, _MMCamcorder_CMD_PREVIEW_START);
	if (ret != MM_ERROR_NONE) {
		/* check internal error of gstreamer */
		if (hcamcorder->error_code != MM_ERROR_NONE) {
			ret = hcamcorder->error_code;
			_mmcam_dbg_log("gstreamer error is occurred. return it %x", ret);
		}
		goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
	}

	_mmcamcorder_set_state(handle, MM_CAMCORDER_STATE_PREPARE);

	/* set attributes related sensor - after start preview */
	if (hcamcorder->type != MM_CAMCORDER_MODE_AUDIO)
		_mmcamcorder_set_attribute_to_camsensor2(handle);

	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

	return MM_ERROR_NONE;

_ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK:
	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

_ERR_CAMCORDER_CMD_PRECON:
	/* check internal error of gstreamer */
	if (hcamcorder->error_code != MM_ERROR_NONE) {
		ret = hcamcorder->error_code;
		hcamcorder->error_code = MM_ERROR_NONE;

		_mmcam_dbg_log("gstreamer error is occurred. return it %x", ret);
	}

	_mmcam_dbg_err("Start fail (type %d, state %d, ret %x)",
				   hcamcorder->type, state, ret);

	return ret;
}

int _mmcamcorder_stop(MMHandleType handle)
{
	int ret = MM_ERROR_NONE;
	int state = MM_CAMCORDER_STATE_NONE;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	_mmcam_dbg_log("");

	if (!hcamcorder) {
		_mmcam_dbg_err("Not initialized");
		ret = MM_ERROR_CAMCORDER_NOT_INITIALIZED;
		return ret;
	}

	if (!_MMCAMCORDER_TRYLOCK_CMD(hcamcorder)) {
		_mmcam_dbg_err("Another command is running.");
		ret = MM_ERROR_CAMCORDER_CMD_IS_RUNNING;
		goto _ERR_CAMCORDER_CMD_PRECON;
	}

	state = _mmcamcorder_get_state(handle);
	if (state != MM_CAMCORDER_STATE_PREPARE) {
		_mmcam_dbg_err("Wrong state(%d)", state);
		ret = MM_ERROR_CAMCORDER_INVALID_STATE;
		goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
	}

	ret = hcamcorder->command((MMHandleType)hcamcorder, _MMCamcorder_CMD_PREVIEW_STOP);
	if (ret != MM_ERROR_NONE)
		goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;

	_mmcamcorder_set_state(handle, MM_CAMCORDER_STATE_READY);

	if (hcamcorder->type != MM_CAMCORDER_MODE_AUDIO) {
		/* unsubscribe remained unsubscribed signal */
		g_mutex_lock(&hcamcorder->gdbus_info_sound.sync_mutex);
		if (hcamcorder->gdbus_info_sound.subscribe_id > 0) {
			_mmcam_dbg_warn("subscribe_id[%u] is remained. remove it.", hcamcorder->gdbus_info_sound.subscribe_id);
			g_dbus_connection_signal_unsubscribe(hcamcorder->gdbus_conn, hcamcorder->gdbus_info_sound.subscribe_id);
		}
		g_mutex_unlock(&hcamcorder->gdbus_info_sound.sync_mutex);

		g_mutex_lock(&hcamcorder->gdbus_info_solo_sound.sync_mutex);
		if (hcamcorder->gdbus_info_solo_sound.subscribe_id > 0) {
			_mmcam_dbg_warn("subscribe_id[%u] is remained. remove it.", hcamcorder->gdbus_info_solo_sound.subscribe_id);
			g_dbus_connection_signal_unsubscribe(hcamcorder->gdbus_conn, hcamcorder->gdbus_info_solo_sound.subscribe_id);
		}
		g_mutex_unlock(&hcamcorder->gdbus_info_solo_sound.sync_mutex);
	}

	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

	return MM_ERROR_NONE;

_ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK:
	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

_ERR_CAMCORDER_CMD_PRECON:
	/* send message */
	_mmcam_dbg_err("Stop fail (type %d, state %d, ret %x)", hcamcorder->type, state, ret);

	return ret;
}


int _mmcamcorder_capture_start(MMHandleType handle)
{
	int ret = MM_ERROR_NONE;
	int state = MM_CAMCORDER_STATE_NONE;
	int state_FROM_0 = MM_CAMCORDER_STATE_PREPARE;
	int state_FROM_1 = MM_CAMCORDER_STATE_RECORDING;
	int state_FROM_2 = MM_CAMCORDER_STATE_PAUSED;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	_mmcam_dbg_log("");

	if (!hcamcorder) {
		_mmcam_dbg_err("Not initialized");
		ret = MM_ERROR_CAMCORDER_NOT_INITIALIZED;
		return ret;
	}

	if (!_MMCAMCORDER_TRYLOCK_CMD(hcamcorder)) {
		_mmcam_dbg_err("Another command is running.");
		ret = MM_ERROR_CAMCORDER_CMD_IS_RUNNING;
		goto _ERR_CAMCORDER_CMD_PRECON;
	}

	state = _mmcamcorder_get_state(handle);
	if (state != state_FROM_0 && state != state_FROM_1 && state != state_FROM_2) {
		_mmcam_dbg_err("Wrong state(%d)", state);
		ret = MM_ERROR_CAMCORDER_INVALID_STATE;
		goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
	}

	/* Handle capture in recording case */
	if (state == state_FROM_1 || state == state_FROM_2) {
		if (hcamcorder->capture_in_recording == TRUE) {
			_mmcam_dbg_err("Capturing in recording (%d)", state);
			ret = MM_ERROR_CAMCORDER_DEVICE_BUSY;
			goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
		} else {
			g_mutex_lock(&hcamcorder->task_thread_lock);
			if (hcamcorder->task_thread_state == _MMCAMCORDER_TASK_THREAD_STATE_NONE) {
				hcamcorder->capture_in_recording = TRUE;
				hcamcorder->task_thread_state = _MMCAMCORDER_TASK_THREAD_STATE_CHECK_CAPTURE_IN_RECORDING;
				_mmcam_dbg_log("send signal for capture in recording");
				g_cond_signal(&hcamcorder->task_thread_cond);
				g_mutex_unlock(&hcamcorder->task_thread_lock);
			} else {
				_mmcam_dbg_err("task thread busy : %d", hcamcorder->task_thread_state);
				g_mutex_unlock(&hcamcorder->task_thread_lock);
				ret = MM_ERROR_CAMCORDER_INVALID_STATE;
				goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
			}
		}
	}

	ret = hcamcorder->command((MMHandleType)hcamcorder, _MMCamcorder_CMD_CAPTURE);
	if (ret != MM_ERROR_NONE)
		goto _ERR_CAMCORDER_CMD;

	/* Do not change state when recording snapshot capture */
	if (state == state_FROM_0)
		_mmcamcorder_set_state(handle, MM_CAMCORDER_STATE_CAPTURING);

	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

	/* Init break continuous shot attr */
	if (mm_camcorder_set_attributes(handle, NULL, MMCAM_CAPTURE_BREAK_CONTINUOUS_SHOT, 0, NULL) != MM_ERROR_NONE)
		_mmcam_dbg_warn("capture-break-cont-shot set 0 failed");

	return MM_ERROR_NONE;

_ERR_CAMCORDER_CMD:
	if (hcamcorder->capture_in_recording)
		hcamcorder->capture_in_recording = FALSE;

_ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK:
	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

_ERR_CAMCORDER_CMD_PRECON:
	/* send message */
	_mmcam_dbg_err("Capture start fail (type %d, state %d, ret %x)",
		hcamcorder->type, state, ret);

	return ret;
}

int _mmcamcorder_capture_stop(MMHandleType handle)
{
	int ret = MM_ERROR_NONE;
	int state = MM_CAMCORDER_STATE_NONE;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	_mmcam_dbg_log("");

	if (!hcamcorder) {
		_mmcam_dbg_err("Not initialized");
		ret = MM_ERROR_CAMCORDER_NOT_INITIALIZED;
		return ret;
	}

	if (!_MMCAMCORDER_TRYLOCK_CMD(hcamcorder)) {
		_mmcam_dbg_err("Another command is running.");
		ret = MM_ERROR_CAMCORDER_CMD_IS_RUNNING;
		goto _ERR_CAMCORDER_CMD_PRECON;
	}

	state = _mmcamcorder_get_state(handle);
	if (state != MM_CAMCORDER_STATE_CAPTURING) {
		_mmcam_dbg_err("Wrong state(%d)", state);
		ret = MM_ERROR_CAMCORDER_INVALID_STATE;
		goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
	}

	ret = hcamcorder->command((MMHandleType)hcamcorder, _MMCamcorder_CMD_PREVIEW_START);
	if (ret != MM_ERROR_NONE)
		goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;

	_mmcamcorder_set_state(handle, MM_CAMCORDER_STATE_PREPARE);

	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

	return MM_ERROR_NONE;

_ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK:
	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

_ERR_CAMCORDER_CMD_PRECON:
	/* send message */
	_mmcam_dbg_err("Capture stop fail (type %d, state %d, ret %x)",
		hcamcorder->type, state, ret);

	return ret;
}

int _mmcamcorder_record(MMHandleType handle)
{
	int ret = MM_ERROR_NONE;
	int state = MM_CAMCORDER_STATE_NONE;
	int dpm_mic_state = DPM_ALLOWED;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	_mmcam_dbg_log("");

	if (!hcamcorder || !MMF_CAMCORDER_SUBCONTEXT(hcamcorder)) {
		_mmcam_dbg_err("Not initialized");
		ret = MM_ERROR_CAMCORDER_NOT_INITIALIZED;
		return ret;
	}

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);

	if (!_MMCAMCORDER_TRYLOCK_CMD(hcamcorder)) {
		_mmcam_dbg_err("Another command is running.");
		ret = MM_ERROR_CAMCORDER_CMD_IS_RUNNING;
		goto _ERR_CAMCORDER_CMD_PRECON;
	}

	state = _mmcamcorder_get_state(handle);
	if (state != MM_CAMCORDER_STATE_PREPARE && state != MM_CAMCORDER_STATE_PAUSED) {
		_mmcam_dbg_err("Wrong state(%d)", state);
		ret = MM_ERROR_CAMCORDER_INVALID_STATE;
		goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
	}

	/* initialize error code */
	hcamcorder->error_code = MM_ERROR_NONE;

	/* get audio disable */
	mm_camcorder_get_attributes(handle, NULL,
		MMCAM_AUDIO_DISABLE, &sc->audio_disable,
		NULL);

	sc->audio_disable |= sc->is_modified_rate;

	/* check mic policy from DPM */
	if (hcamcorder->dpm_handle && sc->audio_disable == FALSE) {
		if (dpm_restriction_get_microphone_state(hcamcorder->dpm_handle, &dpm_mic_state) == DPM_ERROR_NONE) {
			_mmcam_dbg_log("DPM mic state %d", dpm_mic_state);
			if (dpm_mic_state == DPM_DISALLOWED) {
				_mmcam_dbg_err("MIC DISALLOWED by DPM");
				ret = MM_ERROR_COMMON_INVALID_PERMISSION;

				_mmcamcorder_request_dpm_popup(hcamcorder->gdbus_conn, "microphone");

				goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
			}
		} else {
			_mmcam_dbg_err("get DPM mic state failed, keep going...");
		}
	} else {
		_mmcam_dbg_warn("skip dpm check - handle %p, audio disable %d",
			hcamcorder->dpm_handle, sc->audio_disable);
	}

	ret = hcamcorder->command((MMHandleType)hcamcorder, _MMCamcorder_CMD_RECORD);
	if (ret != MM_ERROR_NONE) {
		/* check internal error of gstreamer */
		if (hcamcorder->error_code != MM_ERROR_NONE) {
			ret = hcamcorder->error_code;
			_mmcam_dbg_log("gstreamer error is occurred. return it %x", ret);
		}
		goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
	}

	_mmcamcorder_set_state(handle, MM_CAMCORDER_STATE_RECORDING);

	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

	return MM_ERROR_NONE;

_ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK:
	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

_ERR_CAMCORDER_CMD_PRECON:
	/* check internal error of gstreamer */
	if (hcamcorder->error_code != MM_ERROR_NONE) {
		ret = hcamcorder->error_code;
		hcamcorder->error_code = MM_ERROR_NONE;

		_mmcam_dbg_log("gstreamer error is occurred. return it %x", ret);
	}

	_mmcam_dbg_err("Record fail (type %d, state %d, ret %x)",
				   hcamcorder->type, state, ret);

	return ret;
}


int _mmcamcorder_pause(MMHandleType handle)
{
	int ret = MM_ERROR_NONE;
	int state = MM_CAMCORDER_STATE_NONE;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	_mmcam_dbg_log("");

	if (!hcamcorder) {
		_mmcam_dbg_err("Not initialized");
		ret = MM_ERROR_CAMCORDER_NOT_INITIALIZED;
		return ret;
	}

	if (!_MMCAMCORDER_TRYLOCK_CMD(hcamcorder)) {
		_mmcam_dbg_err("Another command is running.");
		ret = MM_ERROR_CAMCORDER_CMD_IS_RUNNING;
		goto _ERR_CAMCORDER_CMD_PRECON;
	}

	state = _mmcamcorder_get_state(handle);
	if (state != MM_CAMCORDER_STATE_RECORDING) {
		_mmcam_dbg_err("Wrong state(%d)", state);
		ret = MM_ERROR_CAMCORDER_INVALID_STATE;
		goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
	}

	ret = hcamcorder->command((MMHandleType)hcamcorder, _MMCamcorder_CMD_PAUSE);
	if (ret != MM_ERROR_NONE)
		goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;

	_mmcamcorder_set_state(handle, MM_CAMCORDER_STATE_PAUSED);

	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

	return MM_ERROR_NONE;

_ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK:
	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

_ERR_CAMCORDER_CMD_PRECON:
	/* send message */
	_mmcam_dbg_err("Pause fail (type %d, state %d, ret %x)", hcamcorder->type, state, ret);

	return ret;
}


int _mmcamcorder_commit(MMHandleType handle)
{
	int ret = MM_ERROR_NONE;
	int state = MM_CAMCORDER_STATE_NONE;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	_mmcam_dbg_log("");

	if (!hcamcorder) {
		_mmcam_dbg_err("Not initialized");
		ret = MM_ERROR_CAMCORDER_NOT_INITIALIZED;
		return ret;
	}

	if (!_MMCAMCORDER_TRYLOCK_CMD(hcamcorder)) {
		_mmcam_dbg_err("Another command is running.");
		ret = MM_ERROR_CAMCORDER_CMD_IS_RUNNING;
		goto _ERR_CAMCORDER_CMD_PRECON;
	}

	state = _mmcamcorder_get_state(handle);
	if (state != MM_CAMCORDER_STATE_RECORDING && state != MM_CAMCORDER_STATE_PAUSED) {
		_mmcam_dbg_err("Wrong state(%d)", state);
		ret = MM_ERROR_CAMCORDER_INVALID_STATE;
		goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
	}

	/* initialize error code */
	hcamcorder->error_code = MM_ERROR_NONE;

	ret = hcamcorder->command((MMHandleType)hcamcorder, _MMCamcorder_CMD_COMMIT);
	if (ret != MM_ERROR_NONE)
		goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;

	_mmcamcorder_set_state(handle, MM_CAMCORDER_STATE_PREPARE);

	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

	return MM_ERROR_NONE;

_ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK:
	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

_ERR_CAMCORDER_CMD_PRECON:
	/* send message */
	_mmcam_dbg_err("Commit fail (type %d, state %d, ret %x)", hcamcorder->type, state, ret);

	return ret;
}


int _mmcamcorder_cancel(MMHandleType handle)
{
	int ret = MM_ERROR_NONE;
	int state = MM_CAMCORDER_STATE_NONE;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	_mmcam_dbg_log("");

	if (!hcamcorder) {
		_mmcam_dbg_err("Not initialized");
		ret = MM_ERROR_CAMCORDER_NOT_INITIALIZED;
		return ret;
	}

	if (!_MMCAMCORDER_TRYLOCK_CMD(hcamcorder)) {
		_mmcam_dbg_err("Another command is running.");
		ret = MM_ERROR_CAMCORDER_CMD_IS_RUNNING;
		goto _ERR_CAMCORDER_CMD_PRECON;
	}

	state = _mmcamcorder_get_state(handle);
	if (state != MM_CAMCORDER_STATE_RECORDING && state != MM_CAMCORDER_STATE_PAUSED) {
		_mmcam_dbg_err("Wrong state(%d)", state);
		ret = MM_ERROR_CAMCORDER_INVALID_STATE;
		goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;
	}

	ret = hcamcorder->command((MMHandleType)hcamcorder, _MMCamcorder_CMD_CANCEL);
	if (ret != MM_ERROR_NONE)
		goto _ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK;

	_mmcamcorder_set_state(handle, MM_CAMCORDER_STATE_PREPARE);

	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

	return MM_ERROR_NONE;

_ERR_CAMCORDER_CMD_PRECON_AFTER_LOCK:
	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

_ERR_CAMCORDER_CMD_PRECON:
	/* send message */
	_mmcam_dbg_err("Cancel fail (type %d, state %d, ret %x)",
				   hcamcorder->type, state, ret);

	return ret;
}
/* } Internal command functions */


int _mmcamcorder_commit_async_end(MMHandleType handle)
{
	_mmcam_dbg_log("");

	_mmcam_dbg_warn("_mmcamcorder_commit_async_end : MM_CAMCORDER_STATE_PREPARE");
	_mmcamcorder_set_state(handle, MM_CAMCORDER_STATE_PREPARE);

	return MM_ERROR_NONE;
}


int _mmcamcorder_set_message_callback(MMHandleType handle, MMMessageCallback callback, void *user_data)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	_mmcam_dbg_log("%p", hcamcorder);

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	if (callback == NULL)
		_mmcam_dbg_warn("Message Callback is disabled, because application sets it to NULL");

	if (!_MMCAMCORDER_TRYLOCK_MESSAGE_CALLBACK(hcamcorder)) {
		_mmcam_dbg_warn("Application's message callback is running now");
		return MM_ERROR_CAMCORDER_INVALID_CONDITION;
	}

	/* set message callback to message handle */
	hcamcorder->msg_cb = callback;
	hcamcorder->msg_cb_param = user_data;

	_MMCAMCORDER_UNLOCK_MESSAGE_CALLBACK(hcamcorder);

	return MM_ERROR_NONE;
}


int _mmcamcorder_set_video_stream_callback(MMHandleType handle, mm_camcorder_video_stream_callback callback, void *user_data)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	/*_mmcam_dbg_log("");*/

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	if (callback == NULL)
		_mmcam_dbg_warn("Video Stream Callback is disabled, because application sets it to NULL");

	_MMCAMCORDER_LOCK_VSTREAM_CALLBACK(hcamcorder);

	hcamcorder->vstream_cb = callback;
	hcamcorder->vstream_cb_param = user_data;

	_MMCAMCORDER_UNLOCK_VSTREAM_CALLBACK(hcamcorder);

	return MM_ERROR_NONE;
}


int _mmcamcorder_set_audio_stream_callback(MMHandleType handle, mm_camcorder_audio_stream_callback callback, void *user_data)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	_mmcam_dbg_log("");

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	if (callback == NULL)
		_mmcam_dbg_warn("Audio Stream Callback is disabled, because application sets it to NULL");

	if (!_MMCAMCORDER_TRYLOCK_ASTREAM_CALLBACK(hcamcorder)) {
		_mmcam_dbg_warn("Application's audio stream callback is running now");
		return MM_ERROR_CAMCORDER_INVALID_CONDITION;
	}

	hcamcorder->astream_cb = callback;
	hcamcorder->astream_cb_param = user_data;

	_MMCAMCORDER_UNLOCK_ASTREAM_CALLBACK(hcamcorder);

	return MM_ERROR_NONE;
}


int _mmcamcorder_set_muxed_stream_callback(MMHandleType handle, mm_camcorder_muxed_stream_callback callback, void *user_data)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	_mmcam_dbg_log("");

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	if (callback == NULL)
		_mmcam_dbg_warn("Muxed Stream Callback is disabled, because application sets it to NULL");

	if (!_MMCAMCORDER_TRYLOCK_MSTREAM_CALLBACK(hcamcorder)) {
		_mmcam_dbg_warn("Application's muxed stream callback is running now");
		return MM_ERROR_CAMCORDER_INVALID_CONDITION;
	}

	hcamcorder->mstream_cb = callback;
	hcamcorder->mstream_cb_param = user_data;

	_MMCAMCORDER_UNLOCK_MSTREAM_CALLBACK(hcamcorder);

	return MM_ERROR_NONE;
}


int _mmcamcorder_set_video_capture_callback(MMHandleType handle, mm_camcorder_video_capture_callback callback, void *user_data)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	_mmcam_dbg_log("");

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	if (callback == NULL)
		_mmcam_dbg_warn("Video Capture Callback is disabled, because application sets it to NULLL");

	if (!_MMCAMCORDER_TRYLOCK_VCAPTURE_CALLBACK(hcamcorder)) {
		_mmcam_dbg_warn("Application's video capture callback is running now");
		return MM_ERROR_CAMCORDER_INVALID_CONDITION;
	}

	hcamcorder->vcapture_cb = callback;
	hcamcorder->vcapture_cb_param = user_data;

	_MMCAMCORDER_UNLOCK_VCAPTURE_CALLBACK(hcamcorder);

	return MM_ERROR_NONE;
}

int _mmcamcorder_get_current_state(MMHandleType handle)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	_mmcam_dbg_log("");

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	return _mmcamcorder_get_state(handle);
}

int _mmcamcorder_init_focusing(MMHandleType handle)
{
	int ret = TRUE;
	int state = MM_CAMCORDER_STATE_NONE;
	int focus_mode = MM_CAMCORDER_FOCUS_MODE_NONE;
	int af_range = MM_CAMCORDER_AUTO_FOCUS_NORMAL;
	int sensor_focus_mode = 0;
	int sensor_af_range = 0;
	int current_focus_mode = 0;
	int current_af_range = 0;
	mmf_camcorder_t *hcamcorder = NULL;
	_MMCamcorderSubContext *sc = NULL;
	GstCameraControl *control = NULL;

	_mmcam_dbg_log("");

	hcamcorder = MMF_CAMCORDER(handle);
	mmf_return_val_if_fail(handle, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	sc = MMF_CAMCORDER_SUBCONTEXT(hcamcorder);
	mmf_return_val_if_fail(sc, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	if (!_MMCAMCORDER_TRYLOCK_CMD(hcamcorder)) {
		_mmcam_dbg_err("Another command is running.");
		return MM_ERROR_CAMCORDER_CMD_IS_RUNNING;
	}

	state = _mmcamcorder_get_state(handle);

	if (state == MM_CAMCORDER_STATE_CAPTURING ||
	    state < MM_CAMCORDER_STATE_PREPARE) {
		_mmcam_dbg_err("Not proper state. state[%d]", state);
		_MMCAMCORDER_UNLOCK_CMD(hcamcorder);
		return MM_ERROR_CAMCORDER_INVALID_STATE;
	}

	if (!GST_IS_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst)) {
		_mmcam_dbg_log("Can't cast Video source into camera control.");
		_MMCAMCORDER_UNLOCK_CMD(hcamcorder);
		return MM_ERROR_NONE;
	}

	control = GST_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst);
	if (control == NULL) {
		_mmcam_dbg_err("cast CAMERA_CONTROL failed");
		_MMCAMCORDER_UNLOCK_CMD(hcamcorder);
		return MM_ERROR_CAMCORDER_INTERNAL;
	}

	ret = gst_camera_control_stop_auto_focus(control);
	if (!ret) {
		_mmcam_dbg_err("Auto focusing stop fail.");
		_MMCAMCORDER_UNLOCK_CMD(hcamcorder);
		return MM_ERROR_CAMCORDER_DEVICE_IO;
	}

	/* Initialize lens position */
	mm_camcorder_get_attributes(handle, NULL,
		MMCAM_CAMERA_FOCUS_MODE, &focus_mode,
		MMCAM_CAMERA_AF_SCAN_RANGE, &af_range,
		NULL);
	sensor_af_range = _mmcamcorder_convert_msl_to_sensor(handle, MM_CAM_CAMERA_AF_SCAN_RANGE, af_range);
	sensor_focus_mode = _mmcamcorder_convert_msl_to_sensor(handle, MM_CAM_CAMERA_FOCUS_MODE, focus_mode);

	gst_camera_control_get_focus(control, &current_focus_mode, &current_af_range);

	if (current_focus_mode != sensor_focus_mode || current_af_range != sensor_af_range) {
		ret = gst_camera_control_set_focus(control, sensor_focus_mode, sensor_af_range);
	} else {
		_mmcam_dbg_log("No need to init FOCUS [mode:%d, range:%d]", focus_mode, af_range);
		ret = TRUE;
	}

	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

	if (ret) {
		_mmcam_dbg_log("Lens init success.");
		return MM_ERROR_NONE;
	} else {
		_mmcam_dbg_err("Lens init fail.");
		return MM_ERROR_CAMCORDER_DEVICE_IO;
	}
}

int _mmcamcorder_adjust_focus(MMHandleType handle, int direction)
{
	int state;
	int focus_mode = MM_CAMCORDER_FOCUS_MODE_NONE;
	int ret = MM_ERROR_UNKNOWN;
	char *err_attr_name = NULL;

	mmf_return_val_if_fail(handle, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	mmf_return_val_if_fail(direction, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	/*_mmcam_dbg_log("");*/

	if (!_MMCAMCORDER_TRYLOCK_CMD(handle)) {
		_mmcam_dbg_err("Another command is running.");
		return MM_ERROR_CAMCORDER_CMD_IS_RUNNING;
	}

	state = _mmcamcorder_get_state(handle);
	if (state == MM_CAMCORDER_STATE_CAPTURING || state < MM_CAMCORDER_STATE_PREPARE) {
		_mmcam_dbg_err("Not proper state. state[%d]", state);
		_MMCAMCORDER_UNLOCK_CMD(handle);
		return MM_ERROR_CAMCORDER_INVALID_STATE;
	}

	/* TODO : call a auto or manual focus function */
	ret = mm_camcorder_get_attributes(handle, &err_attr_name,
		MMCAM_CAMERA_FOCUS_MODE, &focus_mode,
		NULL);
	if (ret != MM_ERROR_NONE) {
		_mmcam_dbg_warn("Get focus-mode fail. (%s:%x)", err_attr_name, ret);
		SAFE_FREE(err_attr_name);
		_MMCAMCORDER_UNLOCK_CMD(handle);
		return ret;
	}

	switch (focus_mode) {
	case MM_CAMCORDER_FOCUS_MODE_MANUAL:
		ret = _mmcamcorder_adjust_manual_focus(handle, direction);
		break;
	case MM_CAMCORDER_FOCUS_MODE_AUTO:
	case MM_CAMCORDER_FOCUS_MODE_TOUCH_AUTO:
	case MM_CAMCORDER_FOCUS_MODE_CONTINUOUS:
		ret = _mmcamcorder_adjust_auto_focus(handle);
		break;
	default:
		_mmcam_dbg_err("It doesn't adjust focus. Focusing mode(%d)", focus_mode);
		ret = MM_ERROR_CAMCORDER_NOT_SUPPORTED;
	}

	_MMCAMCORDER_UNLOCK_CMD(handle);

	return ret;
}

int _mmcamcorder_adjust_manual_focus(MMHandleType handle, int direction)
{
	int max_level = 0;
	int min_level = 0;
	int cur_level = 0;
	int focus_level = 0;
	float unit_level = 0;
	GstCameraControl *control = NULL;
	_MMCamcorderSubContext *sc = NULL;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	_mmcam_dbg_log("");

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	mmf_return_val_if_fail(_MMFCAMCORDER_FOCUS_TOTAL_LEVEL != 1, MM_ERROR_CAMCORDER_NOT_SUPPORTED);
	mmf_return_val_if_fail((direction >= MM_CAMCORDER_MF_LENS_DIR_FORWARD) &&
		(direction <= MM_CAMCORDER_MF_LENS_DIR_BACKWARD),
		MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	sc = MMF_CAMCORDER_SUBCONTEXT(hcamcorder);
	mmf_return_val_if_fail(sc, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	if (!GST_IS_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst)) {
		_mmcam_dbg_log("Can't cast Video source into camera control.");
		return MM_ERROR_NONE;
	}

	control = GST_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst);
	if (control == NULL) {
		_mmcam_dbg_err("cast CAMERA_CONTROL failed");
		return MM_ERROR_CAMCORDER_INTERNAL;
	}

	/* TODO : get max, min level */
	if (max_level - min_level + 1 < _MMFCAMCORDER_FOCUS_TOTAL_LEVEL)
		_mmcam_dbg_warn("Total level  of manual focus of MMF is greater than that of the camera driver.");

	unit_level = ((float)max_level - (float)min_level)/(float)(_MMFCAMCORDER_FOCUS_TOTAL_LEVEL - 1);

	if (!gst_camera_control_get_focus_level(control, &cur_level)) {
		_mmcam_dbg_err("Can't get current level of manual focus.");
		return MM_ERROR_CAMCORDER_DEVICE_IO;
	}

	/* TODO : adjust unit level value */
	if (direction == MM_CAMCORDER_MF_LENS_DIR_FORWARD)
		focus_level = cur_level + unit_level;
	else if (direction == MM_CAMCORDER_MF_LENS_DIR_BACKWARD)
		focus_level = cur_level - unit_level;

	if (focus_level > max_level)
		focus_level = max_level;
	else if (focus_level < min_level)
		focus_level = min_level;

	if (!gst_camera_control_set_focus_level(control, focus_level)) {
		_mmcam_dbg_err("Manual focusing fail.");
		return MM_ERROR_CAMCORDER_DEVICE_IO;
	}

	return MM_ERROR_NONE;
}


int _mmcamcorder_adjust_auto_focus(MMHandleType handle)
{
	gboolean ret;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	GstCameraControl *control = NULL;
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(handle, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	/*_mmcam_dbg_log("");*/

	sc = MMF_CAMCORDER_SUBCONTEXT(hcamcorder);

	if (!GST_IS_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst)) {
		_mmcam_dbg_log("Can't cast Video source into camera control.");
		return MM_ERROR_NONE;
	}

	control = GST_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst);
	if (control) {
		/* Start AF */
		ret = gst_camera_control_start_auto_focus(control);
	} else {
		_mmcam_dbg_err("cast CAMERA_CONTROL failed");
		ret = FALSE;
	}

	if (ret) {
		_mmcam_dbg_log("Auto focusing start success.");
		return MM_ERROR_NONE;
	} else {
		_mmcam_dbg_err("Auto focusing start fail.");
		return MM_ERROR_CAMCORDER_DEVICE_IO;
	}
}

int _mmcamcorder_stop_focusing(MMHandleType handle)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	int ret, state;
	GstCameraControl *control = NULL;

	mmf_return_val_if_fail(handle, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	/*_mmcam_dbg_log("");*/

	sc = MMF_CAMCORDER_SUBCONTEXT(hcamcorder);

	if (!_MMCAMCORDER_TRYLOCK_CMD(hcamcorder)) {
		_mmcam_dbg_err("Another command is running.");
		return MM_ERROR_CAMCORDER_CMD_IS_RUNNING;
	}

	state = _mmcamcorder_get_state(handle);
	if (state == MM_CAMCORDER_STATE_CAPTURING || state < MM_CAMCORDER_STATE_PREPARE) {
		_mmcam_dbg_err("Not proper state. state[%d]", state);
		_MMCAMCORDER_UNLOCK_CMD(hcamcorder);
		return MM_ERROR_CAMCORDER_INVALID_STATE;
	}

	if (!GST_IS_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst)) {
		_mmcam_dbg_log("Can't cast Video source into camera control.");
		_MMCAMCORDER_UNLOCK_CMD(hcamcorder);
		return MM_ERROR_NONE;
	}

	control = GST_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst);
	if (control) {
		ret = gst_camera_control_stop_auto_focus(control);
	} else {
		_mmcam_dbg_err("cast CAMERA_CONTROL failed");
		ret = FALSE;
	}

	_MMCAMCORDER_UNLOCK_CMD(hcamcorder);

	if (ret) {
		_mmcam_dbg_log("Auto focusing stop success.");
		return MM_ERROR_NONE;
	} else {
		_mmcam_dbg_err("Auto focusing stop fail.");
		return MM_ERROR_CAMCORDER_DEVICE_IO;
	}
}


/*-----------------------------------------------
|	  CAMCORDER INTERNAL LOCAL		|
-----------------------------------------------*/
static gboolean __mmcamcorder_init_gstreamer(camera_conf *conf)
{
	static const int max_argc = 10;
	int i = 0;
	int cnt_str = 0;
	gint *argc = NULL;
	gchar **argv = NULL;
	GError *err = NULL;
	gboolean ret = FALSE;
	type_string_array *GSTInitOption = NULL;

	mmf_return_val_if_fail(conf, FALSE);

	_mmcam_dbg_log("");

	/* alloc */
	argc = malloc(sizeof(int));
	argv = malloc(sizeof(gchar *) * max_argc);

	if (!argc || !argv)
		goto ERROR;

	memset(argv, 0, sizeof(gchar *) * max_argc);

	/* add initial */
	*argc = 1;
	argv[0] = g_strdup("mmcamcorder");

	/* add gst_param */
	_mmcamcorder_conf_get_value_string_array(conf,
		CONFIGURE_CATEGORY_MAIN_GENERAL,
		"GSTInitOption",
		&GSTInitOption);
	if (GSTInitOption != NULL && GSTInitOption->value) {
		cnt_str = GSTInitOption->count;
		for ( ; *argc < max_argc && *argc <= cnt_str ; (*argc)++)
			argv[*argc] = g_strdup(GSTInitOption->value[(*argc)-1]);
	}

	_mmcam_dbg_log("initializing gstreamer with following parameter[argc:%d]", *argc);

	for (i = 0; i < *argc; i++)
		_mmcam_dbg_log("argv[%d] : %s", i, argv[i]);

	/* initializing gstreamer */
	ret = gst_init_check(argc, &argv, &err);
	if (!ret) {
		_mmcam_dbg_err("Could not initialize GStreamer: %s ", err ? err->message : "unknown error occurred");
		if (err)
			g_error_free(err);
	}

	/* release */
	for (i = 0; i < *argc; i++) {
		if (argv[i]) {
			g_free(argv[i]);
			argv[i] = NULL;
		}
	}

	if (argv) {
		free(argv);
		argv = NULL;
	}

	if (argc) {
		free(argc);
		argc = NULL;
	}

	return ret;

ERROR:
	_mmcam_dbg_err("failed to initialize gstreamer");

	if (argv) {
		free(argv);
		argv = NULL;
	}

	if (argc) {
		free(argc);
		argc = NULL;
	}

	return FALSE;
}


int _mmcamcorder_get_state(MMHandleType handle)
{
	int state;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	mmf_return_val_if_fail(hcamcorder, -1);

	_MMCAMCORDER_LOCK_STATE(handle);

	state = hcamcorder->state;
	/*_mmcam_dbg_log("state=%d",state);*/

	_MMCAMCORDER_UNLOCK_STATE(handle);

	return state;
}


int _mmcamcorder_get_state2(MMHandleType handle, int *state, int *old_state)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	mmf_return_val_if_fail(hcamcorder && state && old_state, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	_MMCAMCORDER_LOCK_STATE(handle);

	*state = hcamcorder->state;
	*old_state = hcamcorder->old_state;

	_MMCAMCORDER_UNLOCK_STATE(handle);

	return MM_ERROR_NONE;
}


void _mmcamcorder_set_state(MMHandleType handle, int state)
{
	int old_state;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderMsgItem msg;

	mmf_return_if_fail(hcamcorder);

	/*_mmcam_dbg_log("");*/

	_MMCAMCORDER_LOCK_STATE(handle);

	old_state = hcamcorder->state;
	if (old_state != state) {
		hcamcorder->state = state;
		hcamcorder->old_state = old_state;

		_mmcam_dbg_log("set state[%d -> %d] and send state-changed message", old_state, state);

		/* To discern who changes the state */
		switch (hcamcorder->state_change_by_system) {
		case _MMCAMCORDER_STATE_CHANGE_BY_RM:
			msg.id = MM_MESSAGE_CAMCORDER_STATE_CHANGED_BY_RM;
			msg.param.state.code = MM_ERROR_NONE;
			break;
		case _MMCAMCORDER_STATE_CHANGE_BY_DPM:
			msg.id = MM_MESSAGE_CAMCORDER_STATE_CHANGED_BY_SECURITY;
			msg.param.state.code = MM_ERROR_NONE;
			break;
		case _MMCAMCORDER_STATE_CHANGE_NORMAL:
		default:
			msg.id = MM_MESSAGE_CAMCORDER_STATE_CHANGED;
			msg.param.state.code = MM_ERROR_NONE;
			break;
		}

		msg.param.state.previous = old_state;
		msg.param.state.current = state;

		/*_mmcam_dbg_log("_mmcamcorder_send_message : msg : %p, id:%x", &msg, msg.id);*/
		_mmcamcorder_send_message(handle, &msg);
	}

	_MMCAMCORDER_UNLOCK_STATE(handle);

	return;
}


_MMCamcorderSubContext *_mmcamcorder_alloc_subcontext(int type)
{
	int i;
	_MMCamcorderSubContext *sc = NULL;

	/*_mmcam_dbg_log("");*/

	/* alloc container */
	sc = (_MMCamcorderSubContext *)malloc(sizeof(_MMCamcorderSubContext));
	mmf_return_val_if_fail(sc != NULL, NULL);

	/* init members */
	memset(sc, 0x00, sizeof(_MMCamcorderSubContext));

	sc->element_num = _MMCAMCORDER_PIPELINE_ELEMENT_NUM;
	sc->encode_element_num = _MMCAMCORDER_ENCODE_PIPELINE_ELEMENT_NUM;

	/* alloc info for each mode */
	switch (type) {
	case MM_CAMCORDER_MODE_AUDIO:
		sc->info_audio = g_malloc0(sizeof(_MMCamcorderAudioInfo));
		if (!sc->info_audio) {
			_mmcam_dbg_err("Failed to alloc info structure");
			goto ALLOC_SUBCONTEXT_FAILED;
		}
		break;
	case MM_CAMCORDER_MODE_VIDEO_CAPTURE:
	default:
		sc->info_image = g_malloc0(sizeof(_MMCamcorderImageInfo));
		if (!sc->info_image) {
			_mmcam_dbg_err("Failed to alloc info structure");
			goto ALLOC_SUBCONTEXT_FAILED;
		}

		/* init sound status */
		sc->info_image->sound_status = _SOUND_STATUS_INIT;

		sc->info_video = g_malloc0(sizeof(_MMCamcorderVideoInfo));
		if (!sc->info_video) {
			_mmcam_dbg_err("Failed to alloc info structure");
			goto ALLOC_SUBCONTEXT_FAILED;
		}
		g_mutex_init(&sc->info_video->size_check_lock);
		break;
	}

	/* alloc element array */
	sc->element = (_MMCamcorderGstElement *)malloc(sizeof(_MMCamcorderGstElement) * sc->element_num);
	if (!sc->element) {
		_mmcam_dbg_err("Failed to alloc element structure");
		goto ALLOC_SUBCONTEXT_FAILED;
	}

	sc->encode_element = (_MMCamcorderGstElement *)malloc(sizeof(_MMCamcorderGstElement) * sc->encode_element_num);
	if (!sc->encode_element) {
		_mmcam_dbg_err("Failed to alloc encode element structure");
		goto ALLOC_SUBCONTEXT_FAILED;
	}

	for (i = 0 ; i < sc->element_num ; i++) {
		sc->element[i].id = _MMCAMCORDER_NONE;
		sc->element[i].gst = NULL;
	}

	for (i = 0 ; i < sc->encode_element_num ; i++) {
		sc->encode_element[i].id = _MMCAMCORDER_NONE;
		sc->encode_element[i].gst = NULL;
	}

	sc->fourcc = 0x80000000;
	sc->frame_stability_count = 0;
	sc->drop_vframe = 0;
	sc->pass_first_vframe = 0;
	sc->is_modified_rate = FALSE;

	return sc;

ALLOC_SUBCONTEXT_FAILED:
	if (sc) {
		SAFE_G_FREE(sc->info_audio);
		SAFE_G_FREE(sc->info_image);
		if (sc->info_video)
			g_mutex_clear(&sc->info_video->size_check_lock);

		SAFE_G_FREE(sc->info_video);
		if (sc->element) {
			free(sc->element);
			sc->element = NULL;
		}
		if (sc->encode_element) {
			free(sc->encode_element);
			sc->encode_element = NULL;
		}
		free(sc);
		sc = NULL;
	}

	return NULL;
}


void _mmcamcorder_dealloc_subcontext(_MMCamcorderSubContext *sc)
{
	_mmcam_dbg_log("");

	if (sc) {
		if (sc->element) {
			_mmcam_dbg_log("release element");
			free(sc->element);
			sc->element = NULL;
		}

		if (sc->encode_element) {
			_mmcam_dbg_log("release encode_element");
			free(sc->encode_element);
			sc->encode_element = NULL;
		}

		if (sc->info_image) {
			_mmcam_dbg_log("release info_image");
			free(sc->info_image);
			sc->info_image = NULL;
		}

		if (sc->info_video) {
			_mmcam_dbg_log("release info_video");
			SAFE_G_FREE(sc->info_video->filename);
			g_mutex_clear(&sc->info_video->size_check_lock);
			free(sc->info_video);
			sc->info_video = NULL;
		}

		if (sc->info_audio) {
			_mmcam_dbg_log("release info_audio");
			SAFE_G_FREE(sc->info_audio->filename);
			free(sc->info_audio);
			sc->info_audio = NULL;
		}

		free(sc);
		sc = NULL;
	}

	return;
}


int _mmcamcorder_set_functions(MMHandleType handle, int type)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	/*_mmcam_dbg_log("");*/

	switch (type) {
	case MM_CAMCORDER_MODE_AUDIO:
		hcamcorder->command = _mmcamcorder_audio_command;
		break;
	case MM_CAMCORDER_MODE_VIDEO_CAPTURE:
	default:
		hcamcorder->command = _mmcamcorder_video_capture_command;
		break;
	}

	return MM_ERROR_NONE;
}


gboolean _mmcamcorder_pipeline_cb_message(GstBus *bus, GstMessage *message, gpointer data)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(data);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder, FALSE);
	mmf_return_val_if_fail(message, FALSE);
	/* _mmcam_dbg_log("message type=(%d)", GST_MESSAGE_TYPE(message)); */

	switch (GST_MESSAGE_TYPE(message)) {
	case GST_MESSAGE_UNKNOWN:
		_mmcam_dbg_log("GST_MESSAGE_UNKNOWN");
		break;
	case GST_MESSAGE_EOS:
	{
		_mmcam_dbg_err("Got EOS from element \"%s\"... but should not be reached here!",
			GST_STR_NULL(GST_ELEMENT_NAME(GST_MESSAGE_SRC(message))));
		break;
	}
	case GST_MESSAGE_ERROR:
	{
		GError *err = NULL;
		gchar *debug = NULL;

		gst_message_parse_error(message, &err, &debug);

		__mmcamcorder_handle_gst_error((MMHandleType)hcamcorder, message, err);

		if (err) {
			_mmcam_dbg_err("GSTERR: %s", err->message);
			g_error_free(err);
			err = NULL;
		}

		if (debug) {
			_mmcam_dbg_err("Error Debug: %s", debug);
			g_free(debug);
			debug = NULL;
		}
		break;
	}
	case GST_MESSAGE_WARNING:
	{
		GError *err;
		gchar *debug;
		gst_message_parse_warning(message, &err, &debug);

		_mmcam_dbg_warn("GSTWARN: %s", err->message);

		__mmcamcorder_handle_gst_warning((MMHandleType)hcamcorder, message, err);

		g_error_free(err);
		g_free(debug);
		break;
	}
	case GST_MESSAGE_INFO:
		_mmcam_dbg_log("GST_MESSAGE_INFO");
		break;
	case GST_MESSAGE_TAG:
		_mmcam_dbg_log("GST_MESSAGE_TAG");
		break;
	case GST_MESSAGE_BUFFERING:
		_mmcam_dbg_log("GST_MESSAGE_BUFFERING");
		break;
	case GST_MESSAGE_STATE_CHANGED:
	{
		const GValue *vnewstate;
		GstState newstate;
		GstElement *pipeline = NULL;

		sc = MMF_CAMCORDER_SUBCONTEXT(hcamcorder);
		if ((sc) && (sc->element)) {
			if (sc->element[_MMCAMCORDER_MAIN_PIPE].gst) {
				pipeline = sc->element[_MMCAMCORDER_MAIN_PIPE].gst;
				if (message->src == (GstObject*)pipeline) {
					vnewstate = gst_structure_get_value(gst_message_get_structure(message), "new-state");
					if (vnewstate) {
						newstate = (GstState)vnewstate->data[0].v_int;
						_mmcam_dbg_log("GST_MESSAGE_STATE_CHANGED[%s]", gst_element_state_get_name(newstate));
					} else {
						_mmcam_dbg_warn("get new state failed from msg");
					}
				}
			}
		}
		break;
	}
	case GST_MESSAGE_STATE_DIRTY:
		_mmcam_dbg_log("GST_MESSAGE_STATE_DIRTY");
		break;
	case GST_MESSAGE_STEP_DONE:
		_mmcam_dbg_log("GST_MESSAGE_STEP_DONE");
		break;
	case GST_MESSAGE_CLOCK_PROVIDE:
		_mmcam_dbg_log("GST_MESSAGE_CLOCK_PROVIDE");
		break;
	case GST_MESSAGE_CLOCK_LOST:
		_mmcam_dbg_log("GST_MESSAGE_CLOCK_LOST");
		break;
	case GST_MESSAGE_NEW_CLOCK:
	{
		GstClock *pipe_clock = NULL;
		gst_message_parse_new_clock(message, &pipe_clock);
		/*_mmcam_dbg_log("GST_MESSAGE_NEW_CLOCK : %s", (clock ? GST_OBJECT_NAME (clock) : "NULL"));*/
		break;
	}
	case GST_MESSAGE_STRUCTURE_CHANGE:
		_mmcam_dbg_log("GST_MESSAGE_STRUCTURE_CHANGE");
		break;
	case GST_MESSAGE_STREAM_STATUS:
		/*_mmcam_dbg_log("GST_MESSAGE_STREAM_STATUS");*/
		break;
	case GST_MESSAGE_APPLICATION:
		_mmcam_dbg_log("GST_MESSAGE_APPLICATION");
		break;
	case GST_MESSAGE_ELEMENT:
		/*_mmcam_dbg_log("GST_MESSAGE_ELEMENT");*/
		break;
	case GST_MESSAGE_SEGMENT_START:
		_mmcam_dbg_log("GST_MESSAGE_SEGMENT_START");
		break;
	case GST_MESSAGE_SEGMENT_DONE:
		_mmcam_dbg_log("GST_MESSAGE_SEGMENT_DONE");
		break;
	case GST_MESSAGE_DURATION_CHANGED:
		_mmcam_dbg_log("GST_MESSAGE_DURATION_CHANGED");
		break;
	case GST_MESSAGE_LATENCY:
		_mmcam_dbg_log("GST_MESSAGE_LATENCY");
		break;
	case GST_MESSAGE_ASYNC_START:
		_mmcam_dbg_log("GST_MESSAGE_ASYNC_START");
		break;
	case GST_MESSAGE_ASYNC_DONE:
		/*_mmcam_dbg_log("GST_MESSAGE_ASYNC_DONE");*/
		break;
	case GST_MESSAGE_ANY:
		_mmcam_dbg_log("GST_MESSAGE_ANY");
		break;
	case GST_MESSAGE_QOS:
		/* _mmcam_dbg_log("GST_MESSAGE_QOS"); */
		break;
	default:
		_mmcam_dbg_log("not handled message type=(%d)", GST_MESSAGE_TYPE(message));
		break;
	}

	return TRUE;
}


GstBusSyncReply _mmcamcorder_pipeline_bus_sync_callback(GstBus *bus, GstMessage *message, gpointer data)
{
	GstElement *element = NULL;
	GError *err = NULL;
	gchar *debug_info = NULL;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(data);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder, GST_BUS_PASS);
	mmf_return_val_if_fail(message, GST_BUS_PASS);

	sc = MMF_CAMCORDER_SUBCONTEXT(hcamcorder);
	mmf_return_val_if_fail(sc, GST_BUS_PASS);

	if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
		/* parse error message */
		gst_message_parse_error(message, &err, &debug_info);

		if (debug_info) {
			_mmcam_dbg_err("GST ERROR : %s", debug_info);
			g_free(debug_info);
			debug_info = NULL;
		}

		if (!err) {
			_mmcam_dbg_warn("failed to parse error message");
			return GST_BUS_PASS;
		}

		/* set videosrc element to compare */
		element = GST_ELEMENT_CAST(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst);

		/* check domain[RESOURCE] and element[VIDEOSRC] */
		if (err->domain == GST_RESOURCE_ERROR && GST_ELEMENT_CAST(message->src) == element) {
			switch (err->code) {
			case GST_RESOURCE_ERROR_BUSY:
				_mmcam_dbg_err("Camera device [busy]");
				hcamcorder->error_code = MM_ERROR_CAMCORDER_DEVICE_BUSY;
				break;
			case GST_RESOURCE_ERROR_OPEN_WRITE:
				_mmcam_dbg_err("Camera device [open failed]");
				hcamcorder->error_code = MM_ERROR_COMMON_INVALID_PERMISSION;
				/* sc->error_code = MM_ERROR_CAMCORDER_DEVICE_OPEN; // SECURITY PART REQUEST PRIVILEGE */
				break;
			case GST_RESOURCE_ERROR_OPEN_READ_WRITE:
				_mmcam_dbg_err("Camera device [open failed]");
				hcamcorder->error_code = MM_ERROR_CAMCORDER_DEVICE_OPEN;
				break;
			case GST_RESOURCE_ERROR_OPEN_READ:
				_mmcam_dbg_err("Camera device [register trouble]");
				hcamcorder->error_code = MM_ERROR_CAMCORDER_DEVICE_REG_TROUBLE;
				break;
			case GST_RESOURCE_ERROR_NOT_FOUND:
				_mmcam_dbg_err("Camera device [device not found]");
				hcamcorder->error_code = MM_ERROR_CAMCORDER_DEVICE_NOT_FOUND;
				break;
			case GST_RESOURCE_ERROR_TOO_LAZY:
				_mmcam_dbg_err("Camera device [timeout]");
				hcamcorder->error_code = MM_ERROR_CAMCORDER_DEVICE_TIMEOUT;
				break;
			case GST_RESOURCE_ERROR_SETTINGS:
				_mmcam_dbg_err("Camera device [not supported]");
				hcamcorder->error_code = MM_ERROR_CAMCORDER_NOT_SUPPORTED;
				break;
			case GST_RESOURCE_ERROR_FAILED:
				_mmcam_dbg_err("Camera device [working failed].");
				hcamcorder->error_code = MM_ERROR_CAMCORDER_DEVICE_IO;
				break;
			default:
				_mmcam_dbg_err("Camera device [General(%d)]", err->code);
				hcamcorder->error_code = MM_ERROR_CAMCORDER_DEVICE;
				break;
			}

			hcamcorder->error_occurs = TRUE;
		}

		g_error_free(err);

		/* store error code and drop this message if cmd is running */
		if (hcamcorder->error_code != MM_ERROR_NONE) {
			_MMCamcorderMsgItem msg;

			/* post error to application */
			hcamcorder->error_occurs = TRUE;
			msg.id = MM_MESSAGE_CAMCORDER_ERROR;
			msg.param.code = hcamcorder->error_code;
			_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);

			goto DROP_MESSAGE;
		}
	} else if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ELEMENT) {
		_MMCamcorderMsgItem msg;

		if (gst_structure_has_name(gst_message_get_structure(message), "avsysvideosrc-AF") ||
		    gst_structure_has_name(gst_message_get_structure(message), "camerasrc-AF")) {
			int focus_state = 0;

			gst_structure_get_int(gst_message_get_structure(message), "focus-state", &focus_state);
			_mmcam_dbg_log("Focus State:%d", focus_state);

			msg.id = MM_MESSAGE_CAMCORDER_FOCUS_CHANGED;
			msg.param.code = focus_state;
			_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);

			goto DROP_MESSAGE;
		} else if (gst_structure_has_name(gst_message_get_structure(message), "camerasrc-HDR")) {
			int progress = 0;
			int status = 0;

			if (gst_structure_get_int(gst_message_get_structure(message), "progress", &progress)) {
				gst_structure_get_int(gst_message_get_structure(message), "status", &status);
				_mmcam_dbg_log("HDR progress %d percent, status %d", progress, status);

				msg.id = MM_MESSAGE_CAMCORDER_HDR_PROGRESS;
				msg.param.code = progress;
				_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);
			}

			goto DROP_MESSAGE;
		} else if (gst_structure_has_name(gst_message_get_structure(message), "camerasrc-FD")) {
			int i = 0;
			const GValue *g_value = gst_structure_get_value(gst_message_get_structure(message), "face-info");;
			GstCameraControlFaceDetectInfo *fd_info = NULL;
			MMCamFaceDetectInfo *cam_fd_info = NULL;

			if (g_value)
				fd_info = (GstCameraControlFaceDetectInfo *)g_value_get_pointer(g_value);

			if (fd_info == NULL) {
				_mmcam_dbg_warn("fd_info is NULL");
				goto DROP_MESSAGE;
			}

			cam_fd_info = (MMCamFaceDetectInfo *)g_malloc(sizeof(MMCamFaceDetectInfo));
			if (cam_fd_info == NULL) {
				_mmcam_dbg_warn("cam_fd_info alloc failed");
				SAFE_FREE(fd_info);
				goto DROP_MESSAGE;
			}

			/* set total face count */
			cam_fd_info->num_of_faces = fd_info->num_of_faces;

			if (cam_fd_info->num_of_faces > 0) {
				cam_fd_info->face_info = (MMCamFaceInfo *)g_malloc(sizeof(MMCamFaceInfo) * cam_fd_info->num_of_faces);
				if (cam_fd_info->face_info) {
					/* set information of each face */
					for (i = 0 ; i < fd_info->num_of_faces ; i++) {
						cam_fd_info->face_info[i].id = fd_info->face_info[i].id;
						cam_fd_info->face_info[i].score = fd_info->face_info[i].score;
						cam_fd_info->face_info[i].rect.x = fd_info->face_info[i].rect.x;
						cam_fd_info->face_info[i].rect.y = fd_info->face_info[i].rect.y;
						cam_fd_info->face_info[i].rect.width = fd_info->face_info[i].rect.width;
						cam_fd_info->face_info[i].rect.height = fd_info->face_info[i].rect.height;
						/*
						_mmcam_dbg_log("id %d, score %d, [%d,%d,%dx%d]",
							fd_info->face_info[i].id,
							fd_info->face_info[i].score,
							fd_info->face_info[i].rect.x,
							fd_info->face_info[i].rect.y,
							fd_info->face_info[i].rect.width,
							fd_info->face_info[i].rect.height);
						*/
					}
				} else {
					_mmcam_dbg_warn("MMCamFaceInfo alloc failed");

					/* free allocated memory that is not sent */
					SAFE_G_FREE(cam_fd_info);
				}
			} else {
				cam_fd_info->face_info = NULL;
			}

			if (cam_fd_info) {
				/* send message  - cam_fd_info should be freed by application */
				msg.id = MM_MESSAGE_CAMCORDER_FACE_DETECT_INFO;
				msg.param.data = cam_fd_info;
				msg.param.size = sizeof(MMCamFaceDetectInfo);
				msg.param.code = 0;

				_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);
			}

			/* free fd_info allocated by plugin */
			free(fd_info);
			fd_info = NULL;

			goto DROP_MESSAGE;
		} else if (gst_structure_has_name(gst_message_get_structure(message), "camerasrc-Capture")) {
			int capture_done = FALSE;

			if (gst_structure_get_int(gst_message_get_structure(message), "capture-done", &capture_done)) {
				if (sc->info_image) {
					/* play capture sound */
					_mmcamcorder_sound_solo_play((MMHandleType)hcamcorder, _MMCAMCORDER_SAMPLE_SOUND_NAME_CAPTURE01, FALSE);
				}
			}

			goto DROP_MESSAGE;
		}
	}

	return GST_BUS_PASS;

DROP_MESSAGE:
	gst_message_unref(message);
	message = NULL;

	return GST_BUS_DROP;
}


static GstBusSyncReply __mmcamcorder_gst_handle_sync_audio_error(mmf_camcorder_t *hcamcorder, gint err_code)
{
	_MMCamcorderMsgItem msg;

	if (!hcamcorder) {
		_mmcam_dbg_err("NULL handle");
		return GST_BUS_PASS;
	}

	switch (err_code) {
	case GST_RESOURCE_ERROR_OPEN_READ_WRITE:
	case GST_RESOURCE_ERROR_OPEN_WRITE:
		_mmcam_dbg_err("audio device [open failed]");

		/* post error to application */
		hcamcorder->error_occurs = TRUE;
		hcamcorder->error_code = MM_ERROR_COMMON_INVALID_PERMISSION;

		msg.id = MM_MESSAGE_CAMCORDER_ERROR;
		msg.param.code = hcamcorder->error_code;

		_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);

		_mmcam_dbg_err("error : sc->error_occurs %d", hcamcorder->error_occurs);

		return GST_BUS_DROP;
	default:
		break;
	}

	return GST_BUS_PASS;
}


static GstBusSyncReply __mmcamcorder_gst_handle_sync_others_error(mmf_camcorder_t *hcamcorder, gint err_code)
{
	gboolean b_commiting = FALSE;
	storage_state_e storage_state = STORAGE_STATE_UNMOUNTABLE;
	_MMCamcorderMsgItem msg;
	_MMCamcorderSubContext *sc = NULL;

	if (!hcamcorder) {
		_mmcam_dbg_err("NULL handle");
		return GST_BUS_PASS;
	}

	sc = MMF_CAMCORDER_SUBCONTEXT(hcamcorder);
	mmf_return_val_if_fail(sc, GST_BUS_PASS);

	if (hcamcorder->type != MM_CAMCORDER_MODE_AUDIO) {
		mmf_return_val_if_fail(sc->info_video, GST_BUS_PASS);
		b_commiting = sc->info_video->b_commiting;
	} else {
		mmf_return_val_if_fail(sc->info_audio, GST_BUS_PASS);
		b_commiting = sc->info_audio->b_commiting;
	}

	_MMCAMCORDER_LOCK(hcamcorder);

	switch (err_code) {
	case GST_RESOURCE_ERROR_WRITE:
		storage_get_state(hcamcorder->storage_info.id, &storage_state);
		if (storage_state == STORAGE_STATE_REMOVED ||
			storage_state == STORAGE_STATE_UNMOUNTABLE) {
			_mmcam_dbg_err("storage was removed! [storage state %d]", storage_state);
			hcamcorder->error_code = MM_ERROR_OUT_OF_STORAGE;
		} else {
			_mmcam_dbg_err("File write error, storage state %d", storage_state);
			hcamcorder->error_code = MM_ERROR_FILE_WRITE;
		}

		if (sc->ferror_send == FALSE) {
			sc->ferror_send = TRUE;
		} else {
			_mmcam_dbg_err("error was already sent");
			_MMCAMCORDER_UNLOCK(hcamcorder);
			return GST_BUS_DROP;
		}
		break;
	case GST_RESOURCE_ERROR_NO_SPACE_LEFT:
		_mmcam_dbg_err("No left space");
		hcamcorder->error_code = MM_MESSAGE_CAMCORDER_NO_FREE_SPACE;
		break;
	case GST_RESOURCE_ERROR_OPEN_WRITE:
		_mmcam_dbg_err("Out of storage");
		hcamcorder->error_code = MM_ERROR_OUT_OF_STORAGE;
		break;
	case GST_RESOURCE_ERROR_SEEK:
		_mmcam_dbg_err("File read(seek)");
		hcamcorder->error_code = MM_ERROR_FILE_READ;
		break;
	default:
		_mmcam_dbg_err("Resource error(%d)", err_code);
		hcamcorder->error_code = MM_ERROR_CAMCORDER_GST_RESOURCE;
		break;
	}

	if (b_commiting) {
		_MMCAMCORDER_SIGNAL(hcamcorder);
		_MMCAMCORDER_UNLOCK(hcamcorder);
	} else {
		_MMCAMCORDER_UNLOCK(hcamcorder);

		msg.id = MM_MESSAGE_CAMCORDER_ERROR;
		msg.param.code = hcamcorder->error_code;

		_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);
	}

	return GST_BUS_DROP;
}


static GstBusSyncReply __mmcamcorder_handle_gst_sync_error(mmf_camcorder_t *hcamcorder, GstMessage *message)
{
	GError *err = NULL;
	gchar *debug_info = NULL;
	GstBusSyncReply ret = GST_BUS_PASS;
	_MMCamcorderSubContext *sc = NULL;

	if (!message || !hcamcorder) {
		_mmcam_dbg_err("NULL message(%p) or handle(%p)", message, hcamcorder);
		return GST_BUS_PASS;
	}

	sc = MMF_CAMCORDER_SUBCONTEXT(hcamcorder);
	mmf_return_val_if_fail(sc, GST_BUS_PASS);

	/* parse error message */
	gst_message_parse_error(message, &err, &debug_info);

	if (debug_info) {
		_mmcam_dbg_err("GST ERROR : %s", debug_info);
		g_free(debug_info);
		debug_info = NULL;
	}

	if (!err) {
		_mmcam_dbg_warn("failed to parse error message");
		return GST_BUS_PASS;
	}

	/* check domain[RESOURCE] and element[AUDIOSRC] */
	if (err->domain == GST_RESOURCE_ERROR &&
		GST_ELEMENT_CAST(message->src) == GST_ELEMENT_CAST(sc->encode_element[_MMCAMCORDER_AUDIOSRC_SRC].gst))
		ret = __mmcamcorder_gst_handle_sync_audio_error(hcamcorder, err->code);
	else
		ret = __mmcamcorder_gst_handle_sync_others_error(hcamcorder, err->code);

	g_error_free(err);

	return ret;
}


GstBusSyncReply _mmcamcorder_encode_pipeline_bus_sync_callback(GstBus *bus, GstMessage *message, gpointer data)
{
	GstBusSyncReply ret = GST_BUS_PASS;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(data);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder, GST_BUS_PASS);
	mmf_return_val_if_fail(message, GST_BUS_PASS);

	sc = MMF_CAMCORDER_SUBCONTEXT(hcamcorder);
	mmf_return_val_if_fail(sc, GST_BUS_PASS);

	if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS) {
		_mmcam_dbg_log("got EOS from pipeline");

		_MMCAMCORDER_LOCK(hcamcorder);

		sc->bget_eos = TRUE;
		_MMCAMCORDER_SIGNAL(hcamcorder);

		_MMCAMCORDER_UNLOCK(hcamcorder);

		ret = GST_BUS_DROP;
	} else if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
		ret = __mmcamcorder_handle_gst_sync_error(hcamcorder, message);
	}

	if (ret == GST_BUS_DROP) {
		gst_message_unref(message);
		message = NULL;
	}

	return ret;
}


void _mmcamcorder_dpm_camera_policy_changed_cb(const char *name, const char *value, void *user_data)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(user_data);
	int current_state = MM_CAMCORDER_STATE_NONE;

	mmf_return_if_fail(hcamcorder);
	mmf_return_if_fail(value);

	current_state = _mmcamcorder_get_state((MMHandleType)hcamcorder);
	if (current_state <= MM_CAMCORDER_STATE_NONE || current_state >= MM_CAMCORDER_STATE_NUM) {
		_mmcam_dbg_err("Abnormal state. Or null handle. (%p, %d)", hcamcorder, current_state);
		return;
	}

	_mmcam_dbg_warn("camera policy [%s], current state [%d]", value, current_state);

	if (!strcmp(value, "disallowed")) {
		_MMCAMCORDER_LOCK_INTERRUPT(hcamcorder);

		__mmcamcorder_force_stop(hcamcorder, _MMCAMCORDER_STATE_CHANGE_BY_DPM);

		_MMCAMCORDER_UNLOCK_INTERRUPT(hcamcorder);

		_mmcamcorder_request_dpm_popup(hcamcorder->gdbus_conn, "camera");
	}

	_mmcam_dbg_warn("done");

	return;
}


int _mmcamcorder_create_pipeline(MMHandleType handle, int type)
{
	int ret = MM_ERROR_NONE;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;
	GstElement *pipeline = NULL;

	_mmcam_dbg_log("handle : %p, type : %d", handle, type);

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	switch (type) {
	case MM_CAMCORDER_MODE_AUDIO:
		ret = _mmcamcorder_create_audio_pipeline(handle);
		if (ret != MM_ERROR_NONE)
			return ret;

		break;
	case MM_CAMCORDER_MODE_VIDEO_CAPTURE:
	default:
		ret = _mmcamcorder_create_preview_pipeline(handle);
		if (ret != MM_ERROR_NONE)
			return ret;

		/* connect capture signal */
		if (!sc->bencbin_capture) {
			ret = _mmcamcorder_connect_capture_signal(handle);
			if (ret != MM_ERROR_NONE)
				return ret;
		}
		break;
	}

	pipeline = sc->element[_MMCAMCORDER_MAIN_PIPE].gst;
	if (type != MM_CAMCORDER_MODE_AUDIO) {
		traceBegin(TTRACE_TAG_CAMERA, "MMCAMCORDER:REALIZE:SET_READY_TO_PIPELINE");

		ret = _mmcamcorder_gst_set_state(handle, pipeline, GST_STATE_READY);

		traceEnd(TTRACE_TAG_CAMERA);
	}
#ifdef _MMCAMCORDER_GET_DEVICE_INFO
	if (!_mmcamcorder_get_device_info(handle))
		_mmcam_dbg_err("Getting device information error!!");
#endif

	_mmcam_dbg_log("ret[%x]", ret);
	if (ret != MM_ERROR_NONE) {
		_mmcam_dbg_err("error : destroy pipeline");
		_mmcamcorder_destroy_pipeline(handle, hcamcorder->type);
	}

	return ret;
}


void _mmcamcorder_destroy_pipeline(MMHandleType handle, int type)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;
	gint i = 0;
	int element_num = 0;
	_MMCamcorderGstElement *element = NULL;
	GstBus *bus = NULL;

	mmf_return_if_fail(hcamcorder);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_if_fail(sc);

	_mmcam_dbg_log("");

	/* Inside each pipeline destroy function, Set GST_STATE_NULL to Main pipeline */
	switch (type) {
	case MM_CAMCORDER_MODE_VIDEO_CAPTURE:
		element = sc->element;
		element_num = sc->element_num;
		bus = gst_pipeline_get_bus(GST_PIPELINE(sc->element[_MMCAMCORDER_MAIN_PIPE].gst));
		_mmcamcorder_destroy_video_capture_pipeline(handle);
		break;
	case MM_CAMCORDER_MODE_AUDIO:
		element = sc->encode_element;
		element_num = sc->encode_element_num;
		bus = gst_pipeline_get_bus(GST_PIPELINE(sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst));
		_mmcamcorder_destroy_audio_pipeline(handle);
		break;
	default:
		_mmcam_dbg_err("unknown type %d", type);
		break;
	}

	_mmcam_dbg_log("Pipeline clear!!");

	/* Remove pipeline message callback */
	if (hcamcorder->pipeline_cb_event_id > 0) {
		g_source_remove(hcamcorder->pipeline_cb_event_id);
		hcamcorder->pipeline_cb_event_id = 0;
	}

	/* Remove remained message in bus */
	if (bus) {
		GstMessage *gst_msg = NULL;
		while ((gst_msg = gst_bus_pop(bus)) != NULL) {
			_mmcamcorder_pipeline_cb_message(bus, gst_msg, (gpointer)hcamcorder);
			gst_message_unref(gst_msg);
			gst_msg = NULL;
		}
		gst_object_unref(bus);
		bus = NULL;
	}

	/* checking unreleased element */
	for (i = 0 ; i < element_num ; i++) {
		if (element[i].gst) {
			if (GST_IS_ELEMENT(element[i].gst)) {
				_mmcam_dbg_warn("Still alive element - ID[%d], name [%s], ref count[%d], status[%s]",
					element[i].id,
					GST_OBJECT_NAME(element[i].gst),
					GST_OBJECT_REFCOUNT(element[i].gst),
					gst_element_state_get_name(GST_STATE(element[i].gst)));
				g_object_weak_unref(G_OBJECT(element[i].gst), (GWeakNotify)_mmcamcorder_element_release_noti, sc);
			} else {
				_mmcam_dbg_warn("The element[%d] is still aliving, check it", element[i].id);
			}

			element[i].id = _MMCAMCORDER_NONE;
			element[i].gst = NULL;
		}
	}

	return;
}


#ifdef _MMCAMCORDER_USE_SET_ATTR_CB
static gboolean __mmcamcorder_set_attr_to_camsensor_cb(gpointer data)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(data);

	mmf_return_val_if_fail(hcamcorder, FALSE);

	_mmcam_dbg_log("");

	_mmcamcorder_set_attribute_to_camsensor((MMHandleType)hcamcorder);

	/* initialize */
	hcamcorder->setting_event_id = 0;

	_mmcam_dbg_log("Done");

	/* once */
	return FALSE;
}
#endif /* _MMCAMCORDER_USE_SET_ATTR_CB */


int _mmcamcorder_gst_set_state(MMHandleType handle, GstElement *pipeline, GstState target_state)
{
	unsigned int k = 0;
	_MMCamcorderSubContext *sc = NULL;
	GstState pipeline_state = GST_STATE_VOID_PENDING;
	GstStateChangeReturn setChangeReturn = GST_STATE_CHANGE_FAILURE;
	GstStateChangeReturn getChangeReturn = GST_STATE_CHANGE_FAILURE;
	GstClockTime get_timeout = __MMCAMCORDER_SET_GST_STATE_TIMEOUT * GST_SECOND;
	GMutex *state_lock = NULL;

	mmf_return_val_if_fail(handle, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc && sc->element, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	if (sc->element[_MMCAMCORDER_MAIN_PIPE].gst == pipeline) {
		_mmcam_dbg_log("Set state to %d - PREVIEW PIPELINE", target_state);
		state_lock = &_MMCAMCORDER_GET_GST_STATE_LOCK(handle);
	} else {
		_mmcam_dbg_log("Set state to %d - ENDODE PIPELINE", target_state);
		state_lock = &_MMCAMCORDER_GET_GST_ENCODE_STATE_LOCK(handle);
	}

	g_mutex_lock(state_lock);

	for (k = 0; k < _MMCAMCORDER_STATE_SET_COUNT; k++) {
		setChangeReturn = gst_element_set_state(pipeline, target_state);
		_mmcam_dbg_log("gst_element_set_state[%d] return %d", target_state, setChangeReturn);

		if (setChangeReturn != GST_STATE_CHANGE_FAILURE) {
			getChangeReturn = gst_element_get_state(pipeline, &pipeline_state, NULL, get_timeout);
			switch (getChangeReturn) {
			case GST_STATE_CHANGE_NO_PREROLL:
				_mmcam_dbg_log("status=GST_STATE_CHANGE_NO_PREROLL.");
				/* fall through */
			case GST_STATE_CHANGE_SUCCESS:
				/* if we reached the final target state, exit */
				if (pipeline_state == target_state) {
					_mmcam_dbg_log("Set state to %d - DONE", target_state);
					g_mutex_unlock(state_lock);
					return MM_ERROR_NONE;
				}
				break;
			case GST_STATE_CHANGE_ASYNC:
				_mmcam_dbg_log("status=GST_STATE_CHANGE_ASYNC.");
				break;
			default:
				g_mutex_unlock(state_lock);
				_mmcam_dbg_log("status=GST_STATE_CHANGE_FAILURE.");
				return MM_ERROR_CAMCORDER_GST_STATECHANGE;
			}

			g_mutex_unlock(state_lock);

			_mmcam_dbg_err("timeout of gst_element_get_state()!!");

			return MM_ERROR_CAMCORDER_RESPONSE_TIMEOUT;
		}

		usleep(_MMCAMCORDER_STATE_CHECK_INTERVAL);
	}

	g_mutex_unlock(state_lock);

	_mmcam_dbg_err("Failure. gst_element_set_state timeout!!");

	return MM_ERROR_CAMCORDER_RESPONSE_TIMEOUT;
}


/* For performance check */
int _mmcamcorder_video_current_framerate(MMHandleType handle)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder, -1);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc, -1);

	return sc->kpi.current_fps;
}


int _mmcamcorder_video_average_framerate(MMHandleType handle)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder, -1);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc, -1);

	return sc->kpi.average_fps;
}


void _mmcamcorder_video_current_framerate_init(MMHandleType handle)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_if_fail(hcamcorder);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_if_fail(sc);

	memset(&(sc->kpi), 0x00, sizeof(_MMCamcorderKPIMeasure));

	return;
}


void __mmcamcorder_force_stop(mmf_camcorder_t *hcamcorder, int state_change_by_system)
{
	int i = 0;
	int loop = 0;
	int itr_cnt = 0;
	int result = MM_ERROR_NONE;
	int cmd_try_count = 0;
	int current_state = MM_CAMCORDER_STATE_NONE;
	_MMCamcorderMsgItem msg;

	mmf_return_if_fail(hcamcorder);

	/* check command running */
	do {
		if (!_MMCAMCORDER_TRYLOCK_CMD(hcamcorder)) {
			if (cmd_try_count++ < __MMCAMCORDER_FORCE_STOP_TRY_COUNT) {
				_mmcam_dbg_warn("Another command is running. try again after %d ms", __MMCAMCORDER_FORCE_STOP_WAIT_TIME/1000);
				usleep(__MMCAMCORDER_FORCE_STOP_WAIT_TIME);
			} else {
				_mmcam_dbg_err("wait timeout");
				break;
			}
		} else {
			_MMCAMCORDER_UNLOCK_CMD(hcamcorder);
			break;
		}
	} while (TRUE);

	current_state = _mmcamcorder_get_state((MMHandleType)hcamcorder);

	_mmcam_dbg_warn("Force STOP MMFW Camcorder by %d", state_change_by_system);

	/* set state_change_by_system for state change message */
	hcamcorder->state_change_by_system = state_change_by_system;

	if (current_state >= MM_CAMCORDER_STATE_READY) {
		_mmcam_dbg_warn("send state change started message to notify");

		memset(&msg, 0x0, sizeof(_MMCamcorderMsgItem));

		switch (state_change_by_system) {
		case _MMCAMCORDER_STATE_CHANGE_BY_RM:
			msg.id = MM_MESSAGE_CAMCORDER_STATE_CHANGE_STARTED_BY_RM;
			break;
		case _MMCAMCORDER_STATE_CHANGE_BY_DPM:
			msg.id = MM_MESSAGE_CAMCORDER_STATE_CHANGE_STARTED_BY_SECURITY;
			break;
		default:
			break;
		}

		if (msg.id != 0) {
			msg.param.state.code = hcamcorder->interrupt_code;
			_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);
		} else {
			_mmcam_dbg_err("should not be reached here %d", state_change_by_system);
		}
	}

	for (loop = 0 ; current_state > MM_CAMCORDER_STATE_NULL && loop < __MMCAMCORDER_CMD_ITERATE_MAX * 3 ; loop++) {
		itr_cnt = __MMCAMCORDER_CMD_ITERATE_MAX;
		switch (current_state) {
		case MM_CAMCORDER_STATE_CAPTURING:
		{
			_MMCamcorderSubContext *sc = MMF_CAMCORDER_SUBCONTEXT(hcamcorder);
			_MMCamcorderImageInfo *info = NULL;

			mmf_return_if_fail(sc);
			mmf_return_if_fail((info = sc->info_image));

			_mmcam_dbg_warn("Stop capturing.");

			/* if caturing isn't finished, waiting for 2 sec to finish real capture operation. check 'info->capturing'. */
			mm_camcorder_set_attributes((MMHandleType)hcamcorder, NULL,
				MMCAM_CAPTURE_BREAK_CONTINUOUS_SHOT, TRUE,
				NULL);

			for (i = 0; i < 20 && info->capturing; i++)
				usleep(100000);

			if (i == 20)
				_mmcam_dbg_err("Timeout. Can't check stop capturing.");

			while ((itr_cnt--) && ((result = _mmcamcorder_capture_stop((MMHandleType)hcamcorder)) != MM_ERROR_NONE))
				_mmcam_dbg_warn("Can't stop capturing.(%x)", result);

			break;
		}
		case MM_CAMCORDER_STATE_RECORDING:
		case MM_CAMCORDER_STATE_PAUSED:
		{
			_mmcam_dbg_warn("Stop recording.");

			while ((itr_cnt--) && ((result = _mmcamcorder_commit((MMHandleType)hcamcorder)) != MM_ERROR_NONE)) {
				_mmcam_dbg_warn("Can't commit.(%x), cancel it.", result);
				_mmcamcorder_cancel((MMHandleType)hcamcorder);
			}

			break;
		}
		case MM_CAMCORDER_STATE_PREPARE:
		{
			_mmcam_dbg_warn("Stop preview.");

			while ((itr_cnt--) && ((result = _mmcamcorder_stop((MMHandleType)hcamcorder)) != MM_ERROR_NONE))
				_mmcam_dbg_warn("Can't stop preview.(%x)", result);

			break;
		}
		case MM_CAMCORDER_STATE_READY:
		{
			_mmcam_dbg_warn("unrealize");

			if ((result = _mmcamcorder_unrealize((MMHandleType)hcamcorder)) != MM_ERROR_NONE)
				_mmcam_dbg_warn("Can't unrealize.(%x)", result);

			break;
		}
		default:
			_mmcam_dbg_warn("Already stopped.");
			break;
		}

		current_state = _mmcamcorder_get_state((MMHandleType)hcamcorder);
	}

	/* restore */
	hcamcorder->state_change_by_system = _MMCAMCORDER_STATE_CHANGE_NORMAL;

	_mmcam_dbg_warn("Done.");

	return;
}


static gboolean __mmcamcorder_handle_gst_error(MMHandleType handle, GstMessage *message, GError *error)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderMsgItem msg;
	gchar *msg_src_element = NULL;
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder, FALSE);
	mmf_return_val_if_fail(error, FALSE);
	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc, FALSE);

	_mmcam_dbg_log("");

	/* filtering filesink related errors */
	if (hcamcorder->state == MM_CAMCORDER_STATE_RECORDING &&
	    (error->code ==  GST_RESOURCE_ERROR_WRITE || error->code ==  GST_RESOURCE_ERROR_SEEK)) {
		if (sc->ferror_count == 2 && sc->ferror_send == FALSE) {
			sc->ferror_send = TRUE;
			msg.param.code = __mmcamcorder_gst_handle_resource_error(handle, error->code, message);
		} else {
			sc->ferror_count++;
			_mmcam_dbg_warn("Skip error");
			return TRUE;
		}
	}

	if (error->domain == GST_CORE_ERROR) {
		msg.param.code = __mmcamcorder_gst_handle_core_error(handle, error->code, message);
	} else if (error->domain == GST_LIBRARY_ERROR) {
		msg.param.code = __mmcamcorder_gst_handle_library_error(handle, error->code, message);
	} else if (error->domain == GST_RESOURCE_ERROR) {
		msg.param.code = __mmcamcorder_gst_handle_resource_error(handle, error->code, message);
	} else if (error->domain == GST_STREAM_ERROR) {
		msg.param.code = __mmcamcorder_gst_handle_stream_error(handle, error->code, message);
	} else {
		_mmcam_dbg_warn("This error domain is not defined.");

		/* we treat system error as an internal error */
		msg.param.code = MM_ERROR_CAMCORDER_INTERNAL;
	}

	if (message->src) {
		msg_src_element = GST_ELEMENT_NAME(GST_ELEMENT_CAST(message->src));
		_mmcam_dbg_err("-Msg src : [%s] Domain : [%s]   Error : [%s]  Code : [%d] is tranlated to error code : [0x%x]",
			msg_src_element, g_quark_to_string(error->domain), error->message, error->code, msg.param.code);
	} else {
		_mmcam_dbg_err("Domain : [%s]   Error : [%s]  Code : [%d] is tranlated to error code : [0x%x]",
			g_quark_to_string(error->domain), error->message, error->code, msg.param.code);
	}

#ifdef _MMCAMCORDER_SKIP_GST_FLOW_ERROR
	/* Check whether send this error to application */
	if (msg.param.code == MM_ERROR_CAMCORDER_GST_FLOW_ERROR) {
		_mmcam_dbg_log("We got the error. But skip it.");
		return TRUE;
	}
#endif /* _MMCAMCORDER_SKIP_GST_FLOW_ERROR */

	/* post error to application except RESTRICTED case */
	if (msg.param.code != MM_ERROR_POLICY_RESTRICTED) {
		hcamcorder->error_occurs = TRUE;
		msg.id = MM_MESSAGE_CAMCORDER_ERROR;
		_mmcamcorder_send_message(handle, &msg);
	}

	return TRUE;
}


static gint __mmcamcorder_gst_handle_core_error(MMHandleType handle, int code, GstMessage *message)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;
	GstElement *element = NULL;

	_mmcam_dbg_log("");

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	/* Specific plugin - video encoder plugin */
	element = GST_ELEMENT_CAST(sc->encode_element[_MMCAMCORDER_ENCSINK_VENC].gst);

	if (GST_ELEMENT_CAST(message->src) == element) {
		if (code == GST_CORE_ERROR_NEGOTIATION)
			return MM_ERROR_CAMCORDER_GST_NEGOTIATION;
		else
			return MM_ERROR_CAMCORDER_ENCODER;
	}

	/* General */
	switch (code) {
	case GST_CORE_ERROR_STATE_CHANGE:
		return MM_ERROR_CAMCORDER_GST_STATECHANGE;
	case GST_CORE_ERROR_NEGOTIATION:
		return MM_ERROR_CAMCORDER_GST_NEGOTIATION;
	case GST_CORE_ERROR_MISSING_PLUGIN:
	case GST_CORE_ERROR_SEEK:
	case GST_CORE_ERROR_NOT_IMPLEMENTED:
	case GST_CORE_ERROR_FAILED:
	case GST_CORE_ERROR_TOO_LAZY:
	case GST_CORE_ERROR_PAD:
	case GST_CORE_ERROR_THREAD:
	case GST_CORE_ERROR_EVENT:
	case GST_CORE_ERROR_CAPS:
	case GST_CORE_ERROR_TAG:
	case GST_CORE_ERROR_CLOCK:
	case GST_CORE_ERROR_DISABLED:
	default:
		return MM_ERROR_CAMCORDER_GST_CORE;
	break;
	}
}

static gint __mmcamcorder_gst_handle_library_error(MMHandleType handle, int code, GstMessage *message)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	_mmcam_dbg_log("");

	/* Specific plugin - NONE */

	/* General */
	switch (code) {
	case GST_LIBRARY_ERROR_FAILED:
	case GST_LIBRARY_ERROR_TOO_LAZY:
	case GST_LIBRARY_ERROR_INIT:
	case GST_LIBRARY_ERROR_SHUTDOWN:
	case GST_LIBRARY_ERROR_SETTINGS:
	case GST_LIBRARY_ERROR_ENCODE:
	default:
		_mmcam_dbg_err("Library error(%d)", code);
		return MM_ERROR_CAMCORDER_GST_LIBRARY;
	}
}


static gint __mmcamcorder_gst_handle_resource_error(MMHandleType handle, int code, GstMessage *message)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;
	GstElement *element = NULL;

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	_mmcam_dbg_log("");

	/* Specific plugin */
	/* video sink */
	element = GST_ELEMENT_CAST(sc->element[_MMCAMCORDER_VIDEOSINK_SINK].gst);
	if (GST_ELEMENT_CAST(message->src) == element) {
		if (code == GST_RESOURCE_ERROR_WRITE) {
			_mmcam_dbg_err("Display device [Off]");
			return MM_ERROR_CAMCORDER_DISPLAY_DEVICE_OFF;
		} else {
			_mmcam_dbg_err("Display device [General(%d)]", code);
		}
	}

	/* audiosrc */
	element = GST_ELEMENT_CAST(sc->encode_element[_MMCAMCORDER_AUDIOSRC_SRC].gst);
	if (GST_ELEMENT_CAST(message->src) == element) {
		if (code == GST_RESOURCE_ERROR_FAILED) {
			int ret = MM_ERROR_NONE;
			int current_state = MM_CAMCORDER_STATE_NONE;

			_mmcam_dbg_err("DPM mic DISALLOWED - current state %d", current_state);

			_MMCAMCORDER_LOCK_INTERRUPT(hcamcorder);

			current_state = _mmcamcorder_get_state((MMHandleType)hcamcorder);
			if (current_state >= MM_CAMCORDER_STATE_RECORDING) {
				/* set value to inform a status is changed by DPM */
				hcamcorder->state_change_by_system = _MMCAMCORDER_STATE_CHANGE_BY_DPM;

				ret = _mmcamcorder_commit((MMHandleType)hcamcorder);
				_mmcam_dbg_log("commit result : 0x%x", ret);

				if (ret != MM_ERROR_NONE) {
					_mmcam_dbg_err("commit failed, cancel it");
					ret = _mmcamcorder_cancel((MMHandleType)hcamcorder);
				}
			}

			/* restore value */
			hcamcorder->state_change_by_system = _MMCAMCORDER_STATE_CHANGE_NORMAL;

			_MMCAMCORDER_UNLOCK_INTERRUPT(hcamcorder);

			_mmcamcorder_request_dpm_popup(hcamcorder->gdbus_conn, "microphone");

			return MM_ERROR_POLICY_RESTRICTED;
		}
	}

	/* encodebin */
	element = GST_ELEMENT_CAST(sc->encode_element[_MMCAMCORDER_ENCSINK_VENC].gst);
	if (GST_ELEMENT_CAST(message->src) == element) {
		if (code == GST_RESOURCE_ERROR_FAILED) {
			_mmcam_dbg_err("Encoder [Resource error]");
			return MM_ERROR_CAMCORDER_ENCODER_BUFFER;
		} else {
			_mmcam_dbg_err("Encoder [General(%d)]", code);
			return MM_ERROR_CAMCORDER_ENCODER;
		}
	}

	/* General */
	switch (code) {
	case GST_RESOURCE_ERROR_WRITE:
		_mmcam_dbg_err("File write error");
		return MM_ERROR_FILE_WRITE;
	case GST_RESOURCE_ERROR_NO_SPACE_LEFT:
		_mmcam_dbg_err("No left space");
		return MM_MESSAGE_CAMCORDER_NO_FREE_SPACE;
	case GST_RESOURCE_ERROR_OPEN_WRITE:
		_mmcam_dbg_err("Out of storage");
		return MM_ERROR_OUT_OF_STORAGE;
	case GST_RESOURCE_ERROR_SEEK:
		_mmcam_dbg_err("File read(seek)");
		return MM_ERROR_FILE_READ;
	case GST_RESOURCE_ERROR_NOT_FOUND:
	case GST_RESOURCE_ERROR_FAILED:
	case GST_RESOURCE_ERROR_TOO_LAZY:
	case GST_RESOURCE_ERROR_BUSY:
	case GST_RESOURCE_ERROR_OPEN_READ:
	case GST_RESOURCE_ERROR_OPEN_READ_WRITE:
	case GST_RESOURCE_ERROR_CLOSE:
	case GST_RESOURCE_ERROR_READ:
	case GST_RESOURCE_ERROR_SYNC:
	case GST_RESOURCE_ERROR_SETTINGS:
	default:
		_mmcam_dbg_err("Resource error(%d)", code);
		return MM_ERROR_CAMCORDER_GST_RESOURCE;
	}
}


static gint __mmcamcorder_gst_handle_stream_error(MMHandleType handle, int code, GstMessage *message)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;
	GstElement *element = NULL;

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	_mmcam_dbg_log("");

	/* Specific plugin */
	/* video encoder */
	element = GST_ELEMENT_CAST(sc->encode_element[_MMCAMCORDER_ENCSINK_VENC].gst);
	if (GST_ELEMENT_CAST(message->src) == element) {
		switch (code) {
		case GST_STREAM_ERROR_WRONG_TYPE:
			_mmcam_dbg_err("Video encoder [wrong stream type]");
			return MM_ERROR_CAMCORDER_ENCODER_WRONG_TYPE;
		case GST_STREAM_ERROR_ENCODE:
			_mmcam_dbg_err("Video encoder [encode error]");
			return MM_ERROR_CAMCORDER_ENCODER_WORKING;
		case GST_STREAM_ERROR_FAILED:
			_mmcam_dbg_err("Video encoder [stream failed]");
			return MM_ERROR_CAMCORDER_ENCODER_WORKING;
		default:
			_mmcam_dbg_err("Video encoder [General(%d)]", code);
			return MM_ERROR_CAMCORDER_ENCODER;
		}
	}

	/* General plugin */
	switch (code) {
	case GST_STREAM_ERROR_FORMAT:
		_mmcam_dbg_err("General [negotiation error(%d)]", code);
		return MM_ERROR_CAMCORDER_GST_NEGOTIATION;
	case GST_STREAM_ERROR_FAILED:
		_mmcam_dbg_err("General [flow error(%d)]", code);
		return MM_ERROR_CAMCORDER_GST_FLOW_ERROR;
	case GST_STREAM_ERROR_TYPE_NOT_FOUND:
	case GST_STREAM_ERROR_DECODE:
	case GST_STREAM_ERROR_CODEC_NOT_FOUND:
	case GST_STREAM_ERROR_NOT_IMPLEMENTED:
	case GST_STREAM_ERROR_TOO_LAZY:
	case GST_STREAM_ERROR_ENCODE:
	case GST_STREAM_ERROR_DEMUX:
	case GST_STREAM_ERROR_MUX:
	case GST_STREAM_ERROR_DECRYPT:
	case GST_STREAM_ERROR_DECRYPT_NOKEY:
	case GST_STREAM_ERROR_WRONG_TYPE:
	default:
		_mmcam_dbg_err("General [error(%d)]", code);
		return MM_ERROR_CAMCORDER_GST_STREAM;
	}
}


static gboolean __mmcamcorder_handle_gst_warning(MMHandleType handle, GstMessage *message, GError *error)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	gchar *debug = NULL;
	GError *err = NULL;

	mmf_return_val_if_fail(hcamcorder, FALSE);
	mmf_return_val_if_fail(error, FALSE);

	_mmcam_dbg_log("");

	gst_message_parse_warning(message, &err, &debug);

	if (error->domain == GST_CORE_ERROR) {
		_mmcam_dbg_warn("GST warning: GST_CORE domain");
	} else if (error->domain == GST_LIBRARY_ERROR) {
		_mmcam_dbg_warn("GST warning: GST_LIBRARY domain");
	} else if (error->domain == GST_RESOURCE_ERROR) {
		_mmcam_dbg_warn("GST warning: GST_RESOURCE domain");
		__mmcamcorder_gst_handle_resource_warning(handle, message, error);
	} else if (error->domain == GST_STREAM_ERROR) {
		_mmcam_dbg_warn("GST warning: GST_STREAM domain");
	} else {
		_mmcam_dbg_warn("This error domain(%d) is not defined.", error->domain);
	}

	if (err != NULL) {
		g_error_free(err);
		err = NULL;
	}
	if (debug != NULL) {
		_mmcam_dbg_err("Debug: %s", debug);
		g_free(debug);
		debug = NULL;
	}

	return TRUE;
}


static gint __mmcamcorder_gst_handle_resource_warning(MMHandleType handle, GstMessage *message , GError *error)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;
	GstElement *element = NULL;
	gchar *msg_src_element;

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	_mmcam_dbg_log("");

	/* Special message handling */
	/* video source */
	element = GST_ELEMENT_CAST(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst);
	if (GST_ELEMENT_CAST(message->src) == element) {
		if (error->code == GST_RESOURCE_ERROR_FAILED) {
			msg_src_element = GST_ELEMENT_NAME(GST_ELEMENT_CAST(message->src));
			_mmcam_dbg_warn("-Msg src:[%s] Domain:[%s] Error:[%s]",
				msg_src_element, g_quark_to_string(error->domain), error->message);
			return MM_ERROR_NONE;
		}
	}

	/* General plugin */
	switch (error->code) {
	case GST_RESOURCE_ERROR_WRITE:
	case GST_RESOURCE_ERROR_NO_SPACE_LEFT:
	case GST_RESOURCE_ERROR_SEEK:
	case GST_RESOURCE_ERROR_NOT_FOUND:
	case GST_RESOURCE_ERROR_FAILED:
	case GST_RESOURCE_ERROR_TOO_LAZY:
	case GST_RESOURCE_ERROR_BUSY:
	case GST_RESOURCE_ERROR_OPEN_READ:
	case GST_RESOURCE_ERROR_OPEN_WRITE:
	case GST_RESOURCE_ERROR_OPEN_READ_WRITE:
	case GST_RESOURCE_ERROR_CLOSE:
	case GST_RESOURCE_ERROR_READ:
	case GST_RESOURCE_ERROR_SYNC:
	case GST_RESOURCE_ERROR_SETTINGS:
	default:
		_mmcam_dbg_warn("General GST warning(%d)", error->code);
		break;
	}

	return MM_ERROR_NONE;
}


void _mmcamcorder_emit_signal(MMHandleType handle, const char *object_name,
	const char *interface_name, const char *signal_name, int value)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	mmf_return_if_fail(hcamcorder && object_name && interface_name && signal_name);

	_mmcam_dbg_log("object %s, interface %s, signal %s, value %d",
		object_name, interface_name, signal_name, value);

	_mmcamcorder_emit_dbus_signal(hcamcorder->gdbus_conn, object_name, interface_name, signal_name, value);

	return;
}


#ifdef _MMCAMCORDER_MM_RM_SUPPORT
static int __mmcamcorder_resource_release_cb(mm_resource_manager_h rm,
	mm_resource_manager_res_h res, void *user_data)
{
	mmf_camcorder_t *hcamcorder = (mmf_camcorder_t *) user_data;

	mmf_return_val_if_fail(hcamcorder, FALSE);

	_mmcam_dbg_warn("enter");

	_MMCAMCORDER_LOCK_RESOURCE(hcamcorder);

	/* set flag for resource release callback */
	hcamcorder->is_release_cb_calling = TRUE;

	_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);

	_MMCAMCORDER_LOCK_INTERRUPT(hcamcorder);

	if (res == hcamcorder->video_encoder_resource) {
		/* Stop video recording */
		if (_mmcamcorder_commit((MMHandleType)hcamcorder) != MM_ERROR_NONE) {
			_mmcam_dbg_err("commit failed, cancel it");
			_mmcamcorder_cancel((MMHandleType)hcamcorder);
		}
	} else {
		/* Stop camera */
		__mmcamcorder_force_stop(hcamcorder, _MMCAMCORDER_STATE_CHANGE_BY_RM);
	}

	_MMCAMCORDER_UNLOCK_INTERRUPT(hcamcorder);

	_MMCAMCORDER_LOCK_RESOURCE(hcamcorder);

	if (res == hcamcorder->camera_resource)
		hcamcorder->camera_resource = NULL;
	else if (res == hcamcorder->video_overlay_resource)
		hcamcorder->video_overlay_resource = NULL;
	else if (res == hcamcorder->video_encoder_resource)
		hcamcorder->video_encoder_resource = NULL;

	/* restore flag for resource release callback */
	hcamcorder->is_release_cb_calling = FALSE;

	_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);

	_mmcam_dbg_warn("leave");

	return FALSE;
}
#endif /* _MMCAMCORDER_MM_RM_SUPPORT */

int _mmcamcorder_manage_external_storage_state(MMHandleType handle, int storage_state)
{
	int ret = MM_ERROR_NONE;
	int current_state = MM_CAMCORDER_STATE_NONE;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderMsgItem msg;

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	_mmcam_dbg_warn("storage state %d", storage_state);

	/* check storage state */
	if (storage_state != STORAGE_STATE_UNMOUNTABLE &&
		storage_state != STORAGE_STATE_REMOVED) {
		_mmcam_dbg_warn("nothing to do");
		return MM_ERROR_NONE;
	}

	_MMCAMCORDER_LOCK_INTERRUPT(hcamcorder);

	/* check recording state */
	current_state = _mmcamcorder_get_state(handle);

	_mmcam_dbg_warn("current_state %d", current_state);

	if (current_state < MM_CAMCORDER_STATE_RECORDING) {
		_mmcam_dbg_warn("no recording now");
		goto _MANAGE_DONE;
	}

	/* check storage type */
	if (hcamcorder->storage_info.type != STORAGE_TYPE_EXTERNAL) {
		_mmcam_dbg_warn("external storage is not used");
		goto _MANAGE_DONE;
	}

	/* cancel recording */
	ret = _mmcamcorder_cancel(handle);

	/* send error message */
	msg.id = MM_MESSAGE_CAMCORDER_ERROR;
	msg.param.code = MM_ERROR_OUT_OF_STORAGE;

	_mmcamcorder_send_message(handle, &msg);

_MANAGE_DONE:
	_MMCAMCORDER_UNLOCK_INTERRUPT(hcamcorder);

	_mmcam_dbg_warn("done - ret 0x%x", ret);

	return ret;
}
