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

/* ==============================================================================
|  INCLUDE FILES								|
===============================================================================*/
#include <stdio.h>
#include <string.h>

#include <mm_error.h>
#include <mm_attrs_private.h>

#include "mm_camcorder.h"
#include "mm_camcorder_internal.h"


/*===============================================================================
|  FUNCTION DEFINITIONS								|
===============================================================================*/
/*-------------------------------------------------------------------------------
|    GLOBAL FUNCTION DEFINITIONS:						|
-------------------------------------------------------------------------------*/
int mm_camcorder_create(MMHandleType *camcorder, MMCamPreset *info)
{
	int error = MM_ERROR_NONE;

	mmf_return_val_if_fail((void *)camcorder, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);
	mmf_return_val_if_fail((void *)info, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	_mmcam_dbg_err("");

	traceBegin(TTRACE_TAG_CAMERA, "MMCAMCORDER:CREATE");

	error = _mmcamcorder_create(camcorder, info);

	traceEnd(TTRACE_TAG_CAMERA);

	_mmcam_dbg_err("END");

	return error;
}


int mm_camcorder_destroy(MMHandleType camcorder)
{
	int error = MM_ERROR_NONE;

	mmf_return_val_if_fail((void *)camcorder, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	_mmcam_dbg_err("");

	traceBegin(TTRACE_TAG_CAMERA, "MMCAMCORDER:DESTROY");

	error = _mmcamcorder_destroy(camcorder);

	traceEnd(TTRACE_TAG_CAMERA);

	_mmcam_dbg_err("END!!!");

	return error;
}


int mm_camcorder_realize(MMHandleType camcorder)
{
	int error = MM_ERROR_NONE;

	mmf_return_val_if_fail((void *)camcorder, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	_mmcam_dbg_err("");

	_MMCAMCORDER_LOCK_ASM(camcorder);

	traceBegin(TTRACE_TAG_CAMERA, "MMCAMCORDER:REALIZE");

	error = _mmcamcorder_realize(camcorder);

	traceEnd(TTRACE_TAG_CAMERA);

	_MMCAMCORDER_UNLOCK_ASM(camcorder);

	_mmcam_dbg_err("END");

	return error;
}


int mm_camcorder_unrealize(MMHandleType camcorder)
{
	int error = MM_ERROR_NONE;

	mmf_return_val_if_fail((void *)camcorder, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	_mmcam_dbg_err("");

	_MMCAMCORDER_LOCK_ASM(camcorder);

	traceBegin(TTRACE_TAG_CAMERA, "MMCAMCORDER:UNREALIZE");

	error = _mmcamcorder_unrealize(camcorder);

	traceEnd(TTRACE_TAG_CAMERA);

	_MMCAMCORDER_UNLOCK_ASM(camcorder);

	_mmcam_dbg_err("END");

	return error;
}


int mm_camcorder_start(MMHandleType camcorder)
{
	int error = MM_ERROR_NONE;

	mmf_return_val_if_fail((void *)camcorder, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	_mmcam_dbg_err("");

	_MMCAMCORDER_LOCK_ASM(camcorder);

	traceBegin(TTRACE_TAG_CAMERA, "MMCAMCORDER:START");

	error = _mmcamcorder_start(camcorder);

	traceEnd(TTRACE_TAG_CAMERA);

	_MMCAMCORDER_UNLOCK_ASM(camcorder);

	_mmcam_dbg_err("END");

	return error;
}


int mm_camcorder_stop(MMHandleType camcorder)
{
	int error = MM_ERROR_NONE;

	mmf_return_val_if_fail((void *)camcorder, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	_mmcam_dbg_err("");

	_MMCAMCORDER_LOCK_ASM(camcorder);

	traceBegin(TTRACE_TAG_CAMERA, "MMCAMCORDER:STOP");

	error = _mmcamcorder_stop(camcorder);

	traceEnd(TTRACE_TAG_CAMERA);

	_MMCAMCORDER_UNLOCK_ASM(camcorder);

	_mmcam_dbg_err("END");

	return error;
}


int mm_camcorder_capture_start(MMHandleType camcorder)
{
	int error = MM_ERROR_NONE;

	mmf_return_val_if_fail((void *)camcorder, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	_mmcam_dbg_err("");

	_MMCAMCORDER_LOCK_ASM(camcorder);

	error = _mmcamcorder_capture_start(camcorder);

	_MMCAMCORDER_UNLOCK_ASM(camcorder);

	_mmcam_dbg_err("END");

	return error;
}


int mm_camcorder_capture_stop(MMHandleType camcorder)
{
	int error = MM_ERROR_NONE;

	mmf_return_val_if_fail((void *)camcorder, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	_mmcam_dbg_err("");

	_MMCAMCORDER_LOCK_ASM(camcorder);

	error = _mmcamcorder_capture_stop(camcorder);

	_MMCAMCORDER_UNLOCK_ASM(camcorder);

	_mmcam_dbg_err("END");

	return error;
}


int mm_camcorder_record(MMHandleType camcorder)
{
	int error = MM_ERROR_NONE;

	mmf_return_val_if_fail((void*)camcorder, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	_mmcam_dbg_err("");

	_MMCAMCORDER_LOCK_ASM(camcorder);

	error = _mmcamcorder_record(camcorder);

	_MMCAMCORDER_UNLOCK_ASM(camcorder);

	_mmcam_dbg_err("END");

	return error;
}


int mm_camcorder_pause(MMHandleType camcorder)
{
	int error = MM_ERROR_NONE;

	mmf_return_val_if_fail((void *)camcorder, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	_mmcam_dbg_err("");

	_MMCAMCORDER_LOCK_ASM(camcorder);

	error = _mmcamcorder_pause(camcorder);

	_MMCAMCORDER_UNLOCK_ASM(camcorder);

	_mmcam_dbg_err("END");

	return error;
}


int mm_camcorder_commit(MMHandleType camcorder)
{
	int error = MM_ERROR_NONE;

	mmf_return_val_if_fail((void *)camcorder, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	_mmcam_dbg_err("");

	_MMCAMCORDER_LOCK_ASM(camcorder);

	error = _mmcamcorder_commit(camcorder);

	_MMCAMCORDER_UNLOCK_ASM(camcorder);

	_mmcam_dbg_err("END");

	return error;
}


int mm_camcorder_cancel(MMHandleType camcorder)
{
	int error = MM_ERROR_NONE;

	mmf_return_val_if_fail((void *)camcorder, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	_mmcam_dbg_err("");

	_MMCAMCORDER_LOCK_ASM(camcorder);

	error = _mmcamcorder_cancel(camcorder);

	_MMCAMCORDER_UNLOCK_ASM(camcorder);

	_mmcam_dbg_err("END");

	return error;
}


int mm_camcorder_set_message_callback(MMHandleType  camcorder, MMMessageCallback callback, void *user_data)
{
	mmf_return_val_if_fail((void *)camcorder, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	return _mmcamcorder_set_message_callback(camcorder, callback, user_data);
}


int mm_camcorder_set_video_stream_callback(MMHandleType camcorder, mm_camcorder_video_stream_callback callback, void* user_data)
{
	mmf_return_val_if_fail((void *)camcorder, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	return _mmcamcorder_set_video_stream_callback(camcorder, callback, user_data);
}


int mm_camcorder_set_audio_stream_callback(MMHandleType camcorder, mm_camcorder_audio_stream_callback callback, void* user_data)
{
	mmf_return_val_if_fail((void *)camcorder, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	return _mmcamcorder_set_audio_stream_callback(camcorder, callback, user_data);
}


int mm_camcorder_set_video_capture_callback(MMHandleType camcorder, mm_camcorder_video_capture_callback callback, void* user_data)
{
	mmf_return_val_if_fail((void *)camcorder, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	return _mmcamcorder_set_video_capture_callback(camcorder, callback, user_data);
}


int mm_camcorder_get_state(MMHandleType camcorder, MMCamcorderStateType *status)
{
	int ret = MM_ERROR_NONE;

	if (!camcorder) {
		_mmcam_dbg_warn("Empty handle.");
		return MM_ERROR_CAMCORDER_INVALID_ARGUMENT;
	}

	*status = _mmcamcorder_get_state(camcorder);

	return ret;

}


int mm_camcorder_get_attributes(MMHandleType camcorder, char **err_attr_name, const char *attribute_name, ...)
{
	va_list var_args;
	int ret = MM_ERROR_NONE;

	return_val_if_fail(attribute_name, MM_ERROR_COMMON_INVALID_ARGUMENT);

	va_start(var_args, attribute_name);
	ret = _mmcamcorder_get_attributes(camcorder, err_attr_name, attribute_name, var_args);
	va_end(var_args);

	return ret;
}


int mm_camcorder_set_attributes(MMHandleType camcorder,  char **err_attr_name, const char *attribute_name, ...)
{
	va_list var_args;
	int ret = MM_ERROR_NONE;

	return_val_if_fail(attribute_name, MM_ERROR_COMMON_INVALID_ARGUMENT);

	va_start(var_args, attribute_name);
	ret = _mmcamcorder_set_attributes(camcorder, err_attr_name, attribute_name, var_args);
	va_end(var_args);

	return ret;
}


int mm_camcorder_get_attribute_info(MMHandleType camcorder, const char *attribute_name, MMCamAttrsInfo *info)
{
	return _mmcamcorder_get_attribute_info(camcorder, attribute_name, info);
}

int mm_camcorder_get_fps_list_by_resolution(MMHandleType camcorder, int width, int height, MMCamAttrsInfo *fps_info)
{
	return _mmcamcorder_get_fps_array_by_resolution(camcorder, width, height, fps_info);
}


int mm_camcorder_init_focusing(MMHandleType camcorder)
{
	int error = MM_ERROR_NONE;

	mmf_return_val_if_fail((void *)camcorder, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	error = _mmcamcorder_init_focusing(camcorder);

	return error;
}


int mm_camcorder_start_focusing(MMHandleType camcorder)
{
	int error = MM_ERROR_NONE;

	mmf_return_val_if_fail((void *)camcorder, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	/* TODO : Add direction parameter */

	error = _mmcamcorder_adjust_focus(camcorder, MM_CAMCORDER_MF_LENS_DIR_FORWARD);

	return error;
}


int mm_camcorder_stop_focusing(MMHandleType camcorder)
{
	int error = MM_ERROR_NONE;

	mmf_return_val_if_fail((void *)camcorder, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	error = _mmcamcorder_stop_focusing(camcorder);

	return error;
}

void mm_camcorder_emit_signal(MMHandleType camcorder, const char *object_name,
	const char *interface_name, const char *signal_name, int value)
{
	mmf_return_if_fail((void *)camcorder);

	_mmcamcorder_emit_signal(camcorder,object_name, interface_name, signal_name, value);

	return;
}
