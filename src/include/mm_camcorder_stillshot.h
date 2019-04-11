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

#ifndef __MM_CAMCORDER_STILLSHOT_H__
#define __MM_CAMCORDER_STILLSHOT_H__

/*=======================================================================================
| INCLUDE FILES										|
========================================================================================*/
#include <gst/gst.h>
#include <mm_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*=======================================================================================
| GLOBAL DEFINITIONS AND DECLARATIONS FOR CAMCORDER					|
========================================================================================*/

/*=======================================================================================
| MACRO DEFINITIONS									|
========================================================================================*/
#define _MMCAMCORDER_CAPTURE_STOP_CHECK_INTERVAL	(50*1000)
#define _MMCAMCORDER_CAPTURE_STOP_CHECK_COUNT		60

#define _MNOTE_VALUE_NONE				0
#define _SOUND_STATUS_INIT				-1

/*=======================================================================================
| ENUM DEFINITIONS									|
========================================================================================*/


/*=======================================================================================
| STRUCTURE DEFINITIONS									|
========================================================================================*/
/**
 * MMCamcorder information for image(preview/capture) mode
 */
typedef struct {
	int type;					/**< Still-shot/Multi-shot */
	int count;					/**< Multi-shot capture count */
	int capture_cur_count;				/**< Multi-shot capture current count */
	int capture_send_count;				/**< Multi-shot capture send count */
	unsigned long long next_shot_time;		/**< next still capture time */
	gboolean multi_shot_stop;			/**< Multi-shot stop flag */
	gboolean capturing;				/**< whether MSL is on capturing */
	gboolean resolution_change;			/**< whether on resolution changing for capturing. After changing to capture resolution, it sets to FALSE again. */
	int width;					/**< Width of capture image */
	int height;					/**< Height of capture image */
	int interval;					/**< Capture interval */
	int preview_format;				/**< Preview format */
	int capture_format;				/**< Capture format */
	int hdr_capture_mode;				/**< HDR Capture mode */
	gboolean sound_status;				/**< sound status of system */
	gboolean played_capture_sound;			/**< whether play capture sound when capture starts */
} _MMCamcorderImageInfo;

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
 * This function add still shot bin to main pipeline.
 * When application creates initial pipeline, there are only bins for preview.
 * If application wants to add stil shot function, bins for stillshot should be added.
 *
 * @param[in]	handle		Handle of camcorder context.
 * @return	This function returns MM_ERROR_NONE on success, or the other values on error.
 * @remarks
 * @see		__mmcamcorder_create_preview_pipeline()
 */
int _mmcamcorder_create_stillshot_pipeline(MMHandleType handle);

/**
 * This function remove still shot bin from main pipeline.
 *
 * @param[in]	handle		Handle of camcorder context.
 * @return	This function returns MM_ERROR_NONE on success, or the other values on error.
 * @remarks
 * @see		__mmcamcorder_create_preview_pipeline(), __mmcamcorder_add_stillshot_pipeline()
 */
int _mmcamcorder_remove_stillshot_pipeline(MMHandleType handle);

/**
 * This function connects capture signal.
 *
 * @param[in]	handle		Handle of camcorder context.
 * @return	This function returns MM_ERROR_NONE on success, or the other values on error.
 * @remarks
 */
int _mmcamcorder_connect_capture_signal(MMHandleType handle);

/**
 * This function destroy image pipeline.
 *
 * @param[in]	handle		Handle of camcorder context.
 * @return	void
 * @remarks
 * @see		_mmcamcorder_destroy_pipeline()
 *
 */
void _mmcamcorder_destroy_video_capture_pipeline(MMHandleType handle);
int _mmcamcorder_video_capture_command(MMHandleType handle, int command);

/* Function for capture */
int __mmcamcorder_set_exif_basic_info(MMHandleType handle, int image_width, int image_height);
int __mmcamcorder_update_exif_info(MMHandleType handle, void *imagedata, int imgln);
void __mmcamcorder_init_stillshot_info(MMHandleType handle);
void __mmcamcorder_get_capture_data_from_buffer(MMCamcorderCaptureDataType *capture_data, int pixtype, GstSample *sample);
void __mmcamcorder_release_jpeg_data(MMHandleType handle, MMCamcorderCaptureDataType *dest, int tag_enable, int provide_exif);
int __mmcamcorder_capture_save_exifinfo(MMHandleType handle, MMCamcorderCaptureDataType *original, MMCamcorderCaptureDataType *thumbnail, int provide_exif);
int __mmcamcorder_set_jpeg_data(MMHandleType handle, MMCamcorderCaptureDataType *dest, MMCamcorderCaptureDataType *thumbnail, int provide_exif);
gboolean __mmcamcorder_handoff_callback(GstElement *fakesink, GstBuffer *buffer, GstPad *pad, gpointer u_data);

#ifdef __cplusplus
}
#endif

#endif /* __MM_CAMCORDER_STILLSHOT_H__ */
