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

#ifndef __MM_CAMCORDER_GSTCOMMON_H__
#define __MM_CAMCORDER_GSTCOMMON_H__

/*=======================================================================================
| INCLUDE FILES										|
========================================================================================*/
#include <mm_types.h>
#include "mm_camcorder_configure.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=======================================================================================
| GLOBAL DEFINITIONS AND DECLARATIONS FOR CAMCORDER					|
========================================================================================*/

/*=======================================================================================
| MACRO DEFINITIONS									|
========================================================================================*/

/*=======================================================================================
| ENUM DEFINITIONS									|
========================================================================================*/
/**
* Enumerations for AMR bitrate
*/
typedef enum _MMCamcorderAMRBitRate {
	MM_CAMCORDER_MR475,	/**< MR475 : 4.75 kbit/s */
	MM_CAMCORDER_MR515,	/**< MR515 : 5.15 kbit/s */
	MM_CAMCORDER_MR59,	/**< MR59 : 5.90 kbit/s */
	MM_CAMCORDER_MR67,	/**< MR67 : 6.70 kbit/s */
	MM_CAMCORDER_MR74,	/**< MR74 : 7.40 kbit/s */
	MM_CAMCORDER_MR795,	/**< MR795 : 7.95 kbit/s */
	MM_CAMCORDER_MR102,	/**< MR102 : 10.20 kbit/s */
	MM_CAMCORDER_MR122,	/**< MR122 : 12.20 kbit/s */
	MM_CAMCORDER_MRDTX	/**< MRDTX */
} MMCamcorderAMRBitRate;

/**
* Encodebin profile
*/
typedef enum _MMCamcorderEncodebinProfile {
	MM_CAMCORDER_ENCBIN_PROFILE_VIDEO = 0,  /**< Video recording profile */
	MM_CAMCORDER_ENCBIN_PROFILE_AUDIO,      /**< Audio recording profile */
	MM_CAMCORDER_ENCBIN_PROFILE_IMAGE,      /**< Image capture profile */
	MM_CAMCORDER_ENCBIN_PROFILE_NUM
} MMCamcorderEncodebinProfile;

/**
* Encoder Type
*/
typedef enum _MMCamcorderEncoderType {
	MM_CAMCORDER_ENCODER_TYPE_AUDIO,
	MM_CAMCORDER_ENCODER_TYPE_VIDEO
} MMCamcorderEncoderType;

/*=======================================================================================
| STRUCTURE DEFINITIONS									|
========================================================================================*/
/**
 * Element name table.
 * @note if name is NULL, not supported.
 */
typedef struct {
	unsigned int prof_id;		/**< id of mmcamcorder_profile_attrs_id */
	unsigned int id;		/**< id of value id */
	char *name;			/**< gstreamer element name*/
} _MMCamcorderElementName;

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
/* create pipeline */
/**
 * This function creates bin of video source.
 * Basically, main pipeline of camcorder is composed of several bin(a bundle
 *  of elements). Each bin operates its own function. This bin has a roll
 * inputting video data from camera.
 *
 * @param[in]	handle		Handle of camcorder context.
 * @return	This function returns MM_ERROR_NONE on success, or the other values on error.
 * @remarks
 * @see		__mmcamcorder_create_preview_pipeline()
 */
int _mmcamcorder_create_preview_elements(MMHandleType handle);

/**
 * This function creates bin of audio source.
 * Basically, main pipeline of camcorder is composed of several bin(a bundle
 *  of elements). Each bin operates its own function. This bin has a roll
 * inputting audio data from audio source(such as mike).
 *
 * @param[in]	handle		Handle of camcorder context.
 * @return	This function returns MM_ERROR_NONE on success, or the other values on error.
 * @remarks
 * @see		__mmcamcorder_create_preview_pipeline()
 */
int _mmcamcorder_create_audiosrc_bin(MMHandleType handle);

/**
 * This function creates outputsink bin.
 *
 * @param[in]	handle		Handle of camcorder context.
 * @param[in]	profile		profile of encodesinkbin.
 * @return	This function returns MM_ERROR_NONE on success, or the other values on error.
 * @remarks
 * @see		__mmcamcorder_create_preview_pipeline()
 */
int _mmcamcorder_create_encodesink_bin(MMHandleType handle, MMCamcorderEncodebinProfile profile);

/**
 * This function creates main pipeline of camcorder.
 * Basically, main pipeline of camcorder is composed of several bin(a bundle
 *  of elements). And if the appilcation wants to make pipeline working, the
 * application assemble bin that is proper to that task.
 * When this function is called, bins that is needed for preview will be composed.
 *
 * @param[in]	handle Handle of camcorder context.
 * @return	This function returns MM_ERROR_NONE on success, or the other values on error.
 * @remarks
 * @see		_mmcamcorder_create_pipeline()
 */
int _mmcamcorder_create_preview_pipeline(MMHandleType handle);

/* plug-in related */
void _mmcamcorder_ready_to_encode_callback(GstElement *element, guint size, gpointer handle);
bool _mmcamcorder_recreate_decoder_for_encoded_preview(MMHandleType handle);

/* etc */
int _mmcamcorder_videosink_window_set(MMHandleType handle, type_element *VideosinkElement);
int _mmcamcorder_vframe_stablize(MMHandleType handle);
gboolean _mmcamcorder_get_device_info(MMHandleType handle);
int _mmcamcorder_get_eos_message(MMHandleType handle);
void _mmcamcorder_remove_element_handle(MMHandleType handle, void *element, int first_elem, int last_elem);
int _mmcamcorder_check_codec_fileformat_compatibility(const char *codec_type, int codec, int file_format);
int _mmcamcorder_check_audiocodec_fileformat_compatibility(MMHandleType handle);
int _mmcamcorder_check_videocodec_fileformat_compatibility(MMHandleType handle);
bool _mmcamcorder_set_display_rotation(MMHandleType handle, int display_rotate, int videosink_index);
bool _mmcamcorder_set_display_flip(MMHandleType handle, int display_flip, int videosink_index);
bool _mmcamcorder_set_videosrc_rotation(MMHandleType handle, int videosrc_rotate);
bool _mmcamcorder_set_videosrc_caps(MMHandleType handle, unsigned int fourcc, int width, int height, int fps, int rotate);
bool _mmcamcorder_set_videosrc_flip(MMHandleType handle, int viderosrc_flip);
bool _mmcamcorder_set_videosrc_anti_shake(MMHandleType handle, int anti_shake);
bool _mmcamcorder_set_videosrc_stabilization(MMHandleType handle, int stabilization);
bool _mmcamcorder_set_camera_resolution(MMHandleType handle, int width, int height);
bool _mmcamcorder_set_encoded_preview_bitrate(MMHandleType handle, int bitrate);
bool _mmcamcorder_set_encoded_preview_gop_interval(MMHandleType handle, int gop);
bool _mmcamcorder_set_sound_stream_info(GstElement *element, char *stream_type, int stream_index);
void _mmcamcorder_set_encoder_bitrate(MMCamcorderEncoderType type, int codec, int bitrate, GstElement *element);
GstPadProbeReturn __mmcamcorder_muxed_dataprobe(GstPad *pad, GstPadProbeInfo *info, gpointer u_data);
GstPadProbeReturn __mmcamcorder_eventprobe_monitor(GstPad *pad, GstPadProbeInfo *info, gpointer u_data);
#ifdef __cplusplus
}
#endif
#endif /* __MM_CAMCORDER_GSTCOMMON_H__ */
