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

#ifndef __MM_CAMCORDER_ATTRIBUTE_H__
#define __MM_CAMCORDER_ATTRIBUTE_H__

/*=======================================================================================
| INCLUDE FILES										|
========================================================================================*/
#include <gst/gst.h>
#include <mm_types.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/*=======================================================================================
| GLOBAL DEFINITIONS AND DECLARATIONS FOR CAMCORDER					|
========================================================================================*/
/* Disabled
#define GET_AND_STORE_ATTRS_AFTER_SCENE_MODE
*/

/*=======================================================================================
| MACRO DEFINITIONS									|
========================================================================================*/
/**
 * Caster of attributes handle
 */
#define MMF_CAMCORDER_ATTRS(h) (((mmf_camcorder_t *)(h))->attributes)

/*=======================================================================================
| ENUM DEFINITIONS									|
========================================================================================*/
/**
 * Enumerations for camcorder attribute ID.
 */
typedef enum {
	MM_CAM_MODE,					/* 0 */
	MM_CAM_AUDIO_DEVICE,
	MM_CAM_CAMERA_DEVICE_COUNT,
	MM_CAM_AUDIO_ENCODER,
	MM_CAM_VIDEO_ENCODER,
	MM_CAM_IMAGE_ENCODER,
	MM_CAM_FILE_FORMAT,
	MM_CAM_CAMERA_DEVICE_NAME,
	MM_CAM_AUDIO_SAMPLERATE,
	MM_CAM_AUDIO_FORMAT,
	MM_CAM_AUDIO_CHANNEL,				/* 10 */
	MM_CAM_AUDIO_VOLUME,
	MM_CAM_AUDIO_INPUT_ROUTE,
	MM_CAM_FILTER_SCENE_MODE,
	MM_CAM_FILTER_BRIGHTNESS,
	MM_CAM_FILTER_CONTRAST,
	MM_CAM_FILTER_WB,
	MM_CAM_FILTER_COLOR_TONE,
	MM_CAM_FILTER_SATURATION,
	MM_CAM_FILTER_HUE,
	MM_CAM_FILTER_SHARPNESS,			/* 20 */
	MM_CAM_CAMERA_FORMAT,
	MM_CAM_CAMERA_RECORDING_MOTION_RATE,
	MM_CAM_CAMERA_FPS,
	MM_CAM_CAMERA_WIDTH,
	MM_CAM_CAMERA_HEIGHT,
	MM_CAM_CAMERA_DIGITAL_ZOOM,
	MM_CAM_CAMERA_OPTICAL_ZOOM,
	MM_CAM_CAMERA_FOCUS_MODE,
	MM_CAM_CAMERA_AF_SCAN_RANGE,
	MM_CAM_CAMERA_EXPOSURE_MODE,			/* 30 */
	MM_CAM_CAMERA_EXPOSURE_VALUE,
	MM_CAM_CAMERA_F_NUMBER,
	MM_CAM_CAMERA_SHUTTER_SPEED,
	MM_CAM_CAMERA_ISO,
	MM_CAM_CAMERA_WDR,
	MM_CAM_CAMERA_ANTI_HANDSHAKE,
	MM_CAM_CAMERA_FPS_AUTO,
	MM_CAM_CAMERA_DELAY_ATTR_SETTING,
	MM_CAM_AUDIO_ENCODER_BITRATE,
	MM_CAM_VIDEO_ENCODER_BITRATE,			/* 40 */
	MM_CAM_IMAGE_ENCODER_QUALITY,
	MM_CAM_CAPTURE_FORMAT,
	MM_CAM_CAPTURE_WIDTH,
	MM_CAM_CAPTURE_HEIGHT,
	MM_CAM_CAPTURE_COUNT,
	MM_CAM_CAPTURE_INTERVAL,
	MM_CAM_CAPTURE_BREAK_CONTINUOUS_SHOT,
	MM_CAM_DISPLAY_HANDLE,
	MM_CAM_DISPLAY_DEVICE,
	MM_CAM_DISPLAY_SURFACE,				/* 50 */
	MM_CAM_DISPLAY_RECT_X,
	MM_CAM_DISPLAY_RECT_Y,
	MM_CAM_DISPLAY_RECT_WIDTH,
	MM_CAM_DISPLAY_RECT_HEIGHT,
	MM_CAM_DISPLAY_SOURCE_X,
	MM_CAM_DISPLAY_SOURCE_Y,
	MM_CAM_DISPLAY_SOURCE_WIDTH,
	MM_CAM_DISPLAY_SOURCE_HEIGHT,
	MM_CAM_DISPLAY_ROTATION,
	MM_CAM_DISPLAY_VISIBLE,				/* 60 */
	MM_CAM_DISPLAY_SCALE,
	MM_CAM_DISPLAY_GEOMETRY_METHOD,
	MM_CAM_TARGET_FILENAME,
	MM_CAM_TARGET_MAX_SIZE,
	MM_CAM_TARGET_TIME_LIMIT,
	MM_CAM_TAG_ENABLE,
	MM_CAM_TAG_IMAGE_DESCRIPTION,
	MM_CAM_TAG_ORIENTATION,
	MM_CAM_TAG_SOFTWARE,
	MM_CAM_TAG_LATITUDE,				/* 70 */
	MM_CAM_TAG_LONGITUDE,
	MM_CAM_TAG_ALTITUDE,
	MM_CAM_STROBE_CONTROL,
	MM_CAM_STROBE_CAPABILITIES,
	MM_CAM_STROBE_MODE,
	MM_CAM_DETECT_MODE,
	MM_CAM_DETECT_NUMBER,
	MM_CAM_DETECT_FOCUS_SELECT,
	MM_CAM_DETECT_SELECT_NUMBER,
	MM_CAM_DETECT_STATUS,				/* 80 */
	MM_CAM_CAPTURE_ZERO_SYSTEMLAG,
	MM_CAM_CAMERA_AF_TOUCH_X,
	MM_CAM_CAMERA_AF_TOUCH_Y,
	MM_CAM_CAMERA_AF_TOUCH_WIDTH,
	MM_CAM_CAMERA_AF_TOUCH_HEIGHT,
	MM_CAM_CAMERA_FOCAL_LENGTH,
	MM_CAM_RECOMMEND_PREVIEW_FORMAT_FOR_CAPTURE,
	MM_CAM_RECOMMEND_PREVIEW_FORMAT_FOR_RECORDING,
	MM_CAM_TAG_GPS_ENABLE,
	MM_CAM_TAG_GPS_TIME_STAMP,			/* 90 */
	MM_CAM_TAG_GPS_DATE_STAMP,
	MM_CAM_TAG_GPS_PROCESSING_METHOD,
	MM_CAM_CAMERA_ROTATION,
	MM_CAM_CAPTURED_SCREENNAIL,
	MM_CAM_CAPTURE_SOUND_ENABLE,
	MM_CAM_RECOMMEND_DISPLAY_ROTATION,
	MM_CAM_CAMERA_FLIP,
	MM_CAM_CAMERA_HDR_CAPTURE,
	MM_CAM_DISPLAY_MODE,
	MM_CAM_AUDIO_DISABLE,				/* 100 */
	MM_CAM_RECOMMEND_CAMERA_WIDTH,
	MM_CAM_RECOMMEND_CAMERA_HEIGHT,
	MM_CAM_CAPTURED_EXIF_RAW_DATA,
	MM_CAM_DISPLAY_EVAS_SURFACE_SINK,
	MM_CAM_DISPLAY_EVAS_DO_SCALING,
	MM_CAM_CAMERA_FACING_DIRECTION,
	MM_CAM_DISPLAY_FLIP,
	MM_CAM_CAMERA_VIDEO_STABILIZATION,
	MM_CAM_TAG_VIDEO_ORIENTATION,
	MM_CAM_CAMERA_PAN_MECHA,			/* 110 */
	MM_CAM_CAMERA_PAN_ELEC,
	MM_CAM_CAMERA_TILT_MECHA,
	MM_CAM_CAMERA_TILT_ELEC,
	MM_CAM_CAMERA_PTZ_TYPE,
	MM_CAM_VIDEO_WIDTH,
	MM_CAM_VIDEO_HEIGHT,
	MM_CAM_SUPPORT_ZSL_CAPTURE,
	MM_CAM_SUPPORT_ZERO_COPY_FORMAT,
	MM_CAM_SUPPORT_MEDIA_PACKET_PREVIEW_CB,
	MM_CAM_ENCODED_PREVIEW_BITRATE,		/* 120 */
	MM_CAM_ENCODED_PREVIEW_GOP_INTERVAL,
	MM_CAM_RECORDER_TAG_ENABLE,
	MM_CAM_DISPLAY_SOCKET_PATH,
	MM_CAM_CLIENT_PID,
	MM_CAM_ROOT_DIRECTORY,
	MM_CAM_SOUND_STREAM_INDEX,
	MM_CAM_SOUND_STREAM_TYPE,
	MM_CAM_DISPLAY_REUSE_HINT,
	MM_CAM_DISPLAY_REUSE_ELEMENT,
	MM_CAM_GDBUS_CONNECTION,			/* 130 */
	MM_CAM_AUDIO_REPLAY_GAIN_ENABLE,
	MM_CAM_AUDIO_REPLAY_GAIN_REFERENCE_LEVEL,
	MM_CAM_SUPPORT_USER_BUFFER,
	MM_CAM_USER_BUFFER_FD,
	MM_CAM_ATTRIBUTE_NUM
} MMCamcorderAttrsID;

/*=======================================================================================
| TYPE DEFINITIONS									|
========================================================================================*/
typedef bool (*mmf_cam_commit_func_t)(MMHandleType handle, int attr_idx, const MMAttrsValue *value);

/*=======================================================================================
| STRUCTURE DEFINITIONS									|
========================================================================================*/
typedef struct {
	MMCamcorderAttrsID attrid;
	char *name;
	int value_type;
	int flags;
	union {
		void *value_void;
		char *value_string;
		int value_int;
		double value_double;
	} default_value;              /* default value */
	MMCamAttrsValidType validity_type;
	union {
		int *int_array;
		int int_min;
		double *double_array;
		double double_min;
	} validity_value_1;
	union {
		int int_max;
		double double_max;
		int count;
	} validity_value_2;
	mmf_cam_commit_func_t attr_commit;
} mm_cam_attr_construct_info;

/*=======================================================================================
| CONSTANT DEFINITIONS									|
========================================================================================*/

/*=======================================================================================
| STATIC VARIABLES									|
========================================================================================*/

/*=======================================================================================
| EXTERN GLOBAL VARIABLE								|
========================================================================================*/

/*=======================================================================================
| GLOBAL FUNCTION PROTOTYPES								|
========================================================================================*/
/**
 * This function allocates structure of attributes and sets initial values.
 *
 * @param[in]	handle		Handle of camcorder.
 * @param[in]	info		Preset information of camcorder.
 * @return	This function returns allocated structure of attributes.
 * @remarks
 * @see		_mmcamcorder_dealloc_attribute()
 *
 */
MMHandleType _mmcamcorder_alloc_attribute(MMHandleType handle);

/**
 * This function release structure of attributes.
 *
 * @param[in]	handle		Handle of camcorder.
 * @param[in]	attrs		Handle of camcorder attribute.
 * @return	void
 * @remarks
 * @see		_mmcamcorder_alloc_attribute()
 *
 */
void _mmcamcorder_dealloc_attribute(MMHandleType handle, MMHandleType attrs);

/**
 * This is a meta  function to get attributes of camcorder with given attribute names.
 *
 * @param[in]	handle		Handle of camcorder.
 * @param[out]	err_attr_name	Specifies the name of attributes that made an error. If the function doesn't make an error, this will be null.
 * @param[in]	attribute_name	attribute name that user want to get.
 * @param[in]	var_args	Specifies variable arguments.
 * @return	This function returns MM_ERROR_NONE on Success, minus on Failure.
 * @remarks	You can retrieve multiple attributes at the same time.  @n
 * @see		_mmcamcorder_set_attributes
 */
int _mmcamcorder_get_attributes(MMHandleType handle, char **err_attr_name, const char *attribute_name, va_list var_args);


/**
 * This is a meta  function to set attributes of camcorder with given attribute names.
 *
 * @param[in]	handle		Handle of camcorder.
 * @param[out]	err_attr_name	Specifies the name of attributes that made an error. If the function doesn't make an error, this will be null.
 * @param[in]	attribute_name	attribute name that user want to set.
 * @param[in]	var_args	Specifies variable arguments.
 * @return	This function returns MM_ERROR_NONE on Success, minus on Failure.
 * @remarks	You can put multiple attributes to camcorder at the same time.  @n
 * @see		_mmcamcorder_get_attributes
 */
int _mmcamcorder_set_attributes(MMHandleType handle, char **err_attr_name, const char *attribute_name, va_list var_args);


/**
 * This is a meta  function to get detail information of the attribute.
 *
 * @param[in]	handle		Handle of camcorder.
 * @param[in]	attr_name	attribute name that user want to get information.
 * @param[out]	info		a structure that holds information related with the attribute.
 * @return	This function returns MM_ERROR_NONE on Success, minus on Failure.
 * @remarks	If the function succeeds, 'info' holds detail information about the attribute, such as type, flag, validity_type, validity_values  @n
 * @see		_mmcamcorder_get_attributes, _mmcamcorder_set_attributes
 */
int _mmcamcorder_get_attribute_info(MMHandleType handle, const char *attr_name, MMCamAttrsInfo *info);

/*=======================================================================================
| CAMCORDER INTERNAL LOCAL								|
========================================================================================*/
/**
 * A commit function to set camcorder attributes
 * If the attribute needs actual setting, this function handles that activity.
 * When application sets an attribute, setting function in MSL common calls this function.
 * If this function fails, original value will not change.
 *
 * @param[in]	attr_idx	Attribute index of subcategory.
 * @param[in]	attr_name	Attribute name.
 * @param[in]	value		Handle of camcorder.
 * @param[in]	commit_param	Allocation type of camcorder context.
 * @return	This function returns TRUE on success, or FALSE on failure
 * @remarks
 * @see
 *
 */
bool _mmcamcorder_commit_camcorder_attrs(int attr_idx, const char *attr_name, const MMAttrsValue *value, void *commit_param);

/**
 * A commit function to set videosource attribute
 * If the attribute needs actual setting, this function handles that activity.
 * When application sets an attribute, setting function in MSL common calls this function.
 * If this function fails, original value will not change.
 *
 * @param[in]	attr_idx	Attribute index of subcategory.
 * @param[in]	attr_name	Attribute name.
 * @param[in]	value		Handle of camcorder.
 * @param[in]	commit_param	Allocation type of camcorder context.
 * @return	This function returns TRUE on success, or FALSE on failure
 * @remarks
 * @see
 *
 */
bool _mmcamcorder_commit_capture_width(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_capture_height(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_capture_break_cont_shot(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_capture_count(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_capture_sound_enable(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_audio_volume(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_audio_input_route(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_audio_disable(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_camera_format(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_camera_fps(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_camera_recording_motion_rate(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_camera_width(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_camera_height(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_video_size(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_camera_zoom(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_camera_ptz_type(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_camera_pan(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_camera_tilt(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_camera_focus_mode(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_camera_af_scan_range(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_camera_af_touch_area(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_camera_capture_mode(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_camera_wdr(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_camera_anti_handshake(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_camera_video_stabilization(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_camera_rotate(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_image_encoder_quality(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_target_filename(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_encoder_bitrate(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_recording_max_limit(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_filter(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_filter_scene_mode(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_filter_flip(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_display_handle(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_display_mode(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_display_rotation(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_display_flip(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_display_visible(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_display_geometry_method(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_display_rect(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_display_scale(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_display_evas_do_scaling(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_strobe(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_detect(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_camera_flip(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_camera_hdr_capture(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_encoded_preview_bitrate(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_encoded_preview_gop_interval(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_sound_stream_info(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_tag(MMHandleType handle, int attr_idx, const MMAttrsValue *value);
bool _mmcamcorder_commit_audio_replay_gain(MMHandleType handle, int attr_idx, const MMAttrsValue *value);


/**
 * This function initialize effect setting.
 *
 * @param[in]	handle		Handle of camcorder.
 * @return	bool		Success on TRUE or return FALSE
 */
bool _mmcamcorder_set_attribute_to_camsensor(MMHandleType handle);
bool _mmcamcorder_set_attribute_to_camsensor2(MMHandleType handle);

/**
 * This function removes writable flag from pre-defined attributes.
 *
 * @param[in]	handle		Handle of camcorder.
 * @return	int		Success on MM_ERROR_NONE or return ERROR with error code
 */
int _mmcamcorder_lock_readonly_attributes(MMHandleType handle);

/**
 * This function disable pre-defined attributes.
 *
 * @param[in]	handle		Handle of camcorder.
 * @return	int		Success on MM_ERROR_NONE or return ERROR with error code
 */
int _mmcamcorder_set_disabled_attributes(MMHandleType handle);

/**
 * check whether supported or not
 *
 * @param[in]	handle		Handle of camcorder.
 * @param[in]	attr_index	index of attribute to check.
 * @return	bool		TRUE if supported or FALSE
 */
bool _mmcamcorder_check_supported_attribute(MMHandleType handle, int attr_index);

#ifdef __cplusplus
}
#endif

#endif /* __MM_CAMCORDER_ATTRIBUTE_H__ */
