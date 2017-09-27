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
#include <stdlib.h>
#include <sys/time.h>
#include <sys/times.h>
#include <gst/video/cameracontrol.h>
#include <mm_sound.h>
#include "mm_camcorder_internal.h"
#include "mm_camcorder_stillshot.h"
#include "mm_camcorder_exifinfo.h"
#include "mm_camcorder_exifdef.h"
#include <libexif/exif-loader.h>
#include <libexif/exif-utils.h>
#include <libexif/exif-data.h>

/*---------------------------------------------------------------------------------------
|    GLOBAL VARIABLE DEFINITIONS for internal						|
---------------------------------------------------------------------------------------*/
#define EXIF_SET_ERR(return_type, tag_id) \
do { \
	_mmcam_dbg_err("error=%x,tag=%x", return_type, tag_id); \
	if (return_type == (int)MM_ERROR_CAMCORDER_LOW_MEMORY) { \
		goto exit; \
	} \
} while (0)

/*---------------------------------------------------------------------------------------
|    LOCAL VARIABLE DEFINITIONS for internal						|
---------------------------------------------------------------------------------------*/
#define THUMBNAIL_WIDTH         320
#define THUMBNAIL_HEIGHT        240
#define THUMBNAIL_JPEG_QUALITY  90
#define TRY_LOCK_MAX_COUNT      100
#define TRY_LOCK_TIME           20000   /* ms */
#define H264_PREVIEW_BITRATE    1024*10 /* kbps */
#define H264_PREVIEW_NEWGOP_INTERVAL    1000 /* ms */
#define _MMCAMCORDER_MAKE_THUMBNAIL_INTERNAL_ENCODE


/*---------------------------------------------------------------------------------------
|    LOCAL FUNCTION PROTOTYPES:								|
---------------------------------------------------------------------------------------*/
/** STATIC INTERNAL FUNCTION **/
/* Functions for JPEG capture without Encode bin */
int _mmcamcorder_image_cmd_capture(MMHandleType handle);
int _mmcamcorder_image_cmd_preview_start(MMHandleType handle);
int _mmcamcorder_image_cmd_preview_stop(MMHandleType handle);
static void __mmcamcorder_image_capture_cb(GstElement *element, GstSample *sample1, GstSample *sample2, GstSample *sample3, gpointer u_data);

/* sound status changed callback */
static void __sound_status_changed_cb(keynode_t* node, void *data);

/*=======================================================================================
|  FUNCTION DEFINITIONS									|
=======================================================================================*/

/*---------------------------------------------------------------------------------------
|    GLOBAL FUNCTION DEFINITIONS:							|
---------------------------------------------------------------------------------------*/


int _mmcamcorder_create_stillshot_pipeline(MMHandleType handle)
{
	int err = MM_ERROR_UNKNOWN;
	GstPad *srcpad = NULL;
	GstPad *sinkpad = NULL;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc && sc->info_image && sc->element, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	_mmcam_dbg_log("");

	/* Create gstreamer element */
	_mmcam_dbg_log("Using Encodebin for capturing");

	/* Create capture pipeline */
	_MMCAMCORDER_PIPELINE_MAKE(sc, sc->encode_element, _MMCAMCORDER_ENCODE_MAIN_PIPE, "capture_pipeline", err);

	err = _mmcamcorder_create_encodesink_bin((MMHandleType)hcamcorder, MM_CAMCORDER_ENCBIN_PROFILE_IMAGE);
	if (err != MM_ERROR_NONE)
		return err;

	/* add element and encodesink bin to encode main pipeline */
	gst_bin_add_many(GST_BIN(sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst),
		sc->encode_element[_MMCAMCORDER_ENCSINK_SRC].gst,
		sc->encode_element[_MMCAMCORDER_ENCSINK_FILT].gst,
		sc->encode_element[_MMCAMCORDER_ENCSINK_BIN].gst,
		NULL);

	/* Link each element : appsrc - capsfilter - encodesink bin */
	srcpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_SRC].gst, "src");
	sinkpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_FILT].gst, "sink");
	_MM_GST_PAD_LINK_UNREF(srcpad, sinkpad, err, pipeline_creation_error);

	srcpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_FILT].gst, "src");
	sinkpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_BIN].gst, "image_sink0");
	_MM_GST_PAD_LINK_UNREF(srcpad, sinkpad, err, pipeline_creation_error);

	/* connect handoff signal to get capture data */
	MMCAMCORDER_SIGNAL_CONNECT(sc->encode_element[_MMCAMCORDER_ENCSINK_SINK].gst,
		_MMCAMCORDER_HANDLER_STILLSHOT, "handoff",
		G_CALLBACK(__mmcamcorder_handoff_callback),
		hcamcorder);

	return MM_ERROR_NONE;

pipeline_creation_error:
	_mmcamcorder_remove_stillshot_pipeline(handle);
	return err;
}


int _mmcamcorder_connect_capture_signal(MMHandleType handle)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc && sc->element, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	/* check video source element */
	if (sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst) {
		_mmcam_dbg_warn("connect capture signal to _MMCAMCORDER_VIDEOSRC_SRC");
		MMCAMCORDER_SIGNAL_CONNECT(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst,
			_MMCAMCORDER_HANDLER_STILLSHOT, "still-capture",
			G_CALLBACK(__mmcamcorder_image_capture_cb),
			hcamcorder);

		return MM_ERROR_NONE;
	} else {
		_mmcam_dbg_err("videosrc element is not created yet");
		return MM_ERROR_CAMCORDER_NOT_INITIALIZED;
	}
}


int _mmcamcorder_remove_stillshot_pipeline(MMHandleType handle)
{
	int ret = MM_ERROR_NONE;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc && sc->info_image && sc->element, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	_mmcam_dbg_log("");

	/* Check pipeline */
	if (sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst) {
		ret = _mmcamcorder_gst_set_state(handle, sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst, GST_STATE_NULL);
		if (ret != MM_ERROR_NONE) {
			_mmcam_dbg_err("Faile to change encode main pipeline state [0x%x]", ret);
			return ret;
		}

		_mmcamcorder_remove_all_handlers(handle, _MMCAMCORDER_HANDLER_STILLSHOT | _MMCAMCORDER_HANDLER_VIDEOREC);

		GstPad *reqpad = NULL;

		/* release request pad */
		reqpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, "image");
		if (reqpad) {
			gst_element_release_request_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, reqpad);
			gst_object_unref(reqpad);
			reqpad = NULL;
		}

		/* release encode main pipeline */
		gst_object_unref(sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst);

		_mmcam_dbg_log("Encoder pipeline removed");
	} else {
		_mmcam_dbg_log("encode main pipeline is already removed");
	}

	return MM_ERROR_NONE;
}


void _mmcamcorder_destroy_video_capture_pipeline(MMHandleType handle)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;
	int display_reuse_hint = FALSE;
	GstElement *sink_element = NULL;
	mmf_attrs_t *attrs = NULL;
	int attr_index = 0;
	mmf_attribute_t *attr_item = NULL;

	mmf_return_if_fail(hcamcorder);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_if_fail(sc && sc->element);

	_mmcam_dbg_log("");

	if (sc->element[_MMCAMCORDER_MAIN_PIPE].gst) {
		MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_QUE].gst, "empty-buffers", TRUE);
		MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_QUE].gst, "empty-buffers", TRUE);

		/* check reuse flag for display element */
		mm_camcorder_get_attributes(handle, NULL,
			MMCAM_DISPLAY_REUSE_HINT, &display_reuse_hint,
			NULL);

		_mmcam_dbg_log("display reuse hint %d", display_reuse_hint);

		if (!display_reuse_hint)
			goto _REUSE_CHECK_DONE;

		sink_element = sc->element[_MMCAMCORDER_VIDEOSINK_SINK].gst;
		if (!sink_element) {
			_mmcam_dbg_warn("sink element is NULL");
			goto _REUSE_CHECK_DONE;
		}

		attrs = (mmf_attrs_t *)MMF_CAMCORDER_ATTRS(handle);
		if (!attrs) {
			_mmcam_dbg_warn("attribute is NULL");
			goto _REUSE_CHECK_DONE;
		}

		_mmcam_dbg_log("REF sink element %p and set it to attribute", sink_element);

		/* ref element before remove it from pipeline */
		gst_object_ref(sink_element);

		if (!gst_bin_remove(GST_BIN(sc->element[_MMCAMCORDER_MAIN_PIPE].gst), sink_element))
			_mmcam_dbg_warn("failed to remove sink element from pipeline");

		/* reset floating reference flag to avoid leak after next use */
		g_object_force_floating(G_OBJECT(sink_element));

		/* set sink element pointer to attribute */
		mm_attrs_get_index((MMHandleType)attrs, MMCAM_DISPLAY_REUSE_ELEMENT, &attr_index);
		attr_item = &attrs->items[attr_index];
		mmf_attribute_set_data(attr_item, (void *)sink_element, sizeof(*sink_element));
		mmf_attribute_commit(attr_item);

		/* remove notify callback */
		g_object_weak_unref(G_OBJECT(sink_element), (GWeakNotify)_mmcamcorder_element_release_noti, sc);

		sc->element[_MMCAMCORDER_VIDEOSINK_SINK].gst = NULL;
		sc->element[_MMCAMCORDER_VIDEOSINK_SINK].id = _MMCAMCORDER_NONE;

_REUSE_CHECK_DONE:
		traceBegin(TTRACE_TAG_CAMERA, "MMCAMCORDER:UNREALIZE:SET_NULL_TO_PIPELINE");

		_mmcamcorder_gst_set_state(handle, sc->element[_MMCAMCORDER_MAIN_PIPE].gst, GST_STATE_NULL);

		traceEnd(TTRACE_TAG_CAMERA);

		_mmcamcorder_remove_all_handlers(handle, _MMCAMCORDER_HANDLER_CATEGORY_ALL);

		traceBegin(TTRACE_TAG_CAMERA, "MMCAMCORDER:UNREALIZE:UNREF_PIPELINE");

		gst_object_unref(sc->element[_MMCAMCORDER_MAIN_PIPE].gst);

		traceEnd(TTRACE_TAG_CAMERA);

		/* NULL initialization will be done in _mmcamcorder_element_release_noti */
	}

	return;
}


int _mmcamcorder_image_cmd_capture(MMHandleType handle)
{
	int ret = MM_ERROR_NONE;
	int UseCaptureMode = 0;
	int width = 0;
	int height = 0;
	int fps = 0;
	int cap_format = MM_PIXEL_FORMAT_NV12;
	int image_encoder = MM_IMAGE_CODEC_JPEG;
	int strobe_mode = MM_CAMCORDER_STROBE_MODE_OFF;
	int is_modified_size = FALSE;
	int tag_orientation = 0;
	unsigned int cap_fourcc = 0;

	char *err_name = NULL;
	const char *videosrc_name = NULL;

	GstCameraControl *control = NULL;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderImageInfo *info = NULL;
	_MMCamcorderSubContext *sc = NULL;
	type_element *VideosrcElement = NULL;
	MMCamcorderStateType current_state = MM_CAMCORDER_STATE_NONE;

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc && sc->info_image && sc->element, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	_mmcam_dbg_log("");

	info = sc->info_image;

	if (info->capturing) {
		_mmcam_dbg_err("already capturing");
		return MM_ERROR_CAMCORDER_DEVICE_BUSY;
	}

	_mmcamcorder_conf_get_value_int(handle, hcamcorder->conf_main,
		CONFIGURE_CATEGORY_MAIN_CAPTURE,
		"UseCaptureMode",
		&UseCaptureMode);

	_mmcamcorder_conf_get_element(handle, hcamcorder->conf_main,
		CONFIGURE_CATEGORY_MAIN_VIDEO_INPUT,
		"VideosrcElement",
		&VideosrcElement);

	_mmcamcorder_conf_get_value_element_name(VideosrcElement, &videosrc_name);

	/* get current state */
	mm_camcorder_get_state(handle, &current_state);

	/* set capture flag */
	info->capturing = TRUE;

	ret = mm_camcorder_get_attributes(handle, &err_name,
		MMCAM_IMAGE_ENCODER, &image_encoder,
		MMCAM_CAMERA_WIDTH, &width,
		MMCAM_CAMERA_HEIGHT, &height,
		MMCAM_CAMERA_FPS, &fps,
		MMCAM_CAPTURE_FORMAT, &cap_format,
		MMCAM_CAPTURE_WIDTH, &info->width,
		MMCAM_CAPTURE_HEIGHT, &info->height,
		MMCAM_CAPTURE_COUNT, &info->count,
		MMCAM_CAPTURE_INTERVAL, &info->interval,
		MMCAM_STROBE_MODE, &strobe_mode,
		MMCAM_TAG_ORIENTATION, &tag_orientation,
		NULL);
	if (err_name) {
		_mmcam_dbg_warn("get_attributes err %s, ret 0x%x", err_name, ret);
		SAFE_FREE(err_name);
	}

	ret = MM_ERROR_NONE;

	/* check capture count */
	if (info->count < 1) {
		_mmcam_dbg_err("capture count[%d] is invalid", info->count);
		ret = MM_ERROR_CAMCORDER_INVALID_ARGUMENT;
		goto cmd_done;
	} else if (info->count == 1) {
		info->type = _MMCamcorder_SINGLE_SHOT;
	} else {
		info->type = _MMCamcorder_MULTI_SHOT;
		info->next_shot_time = 0;
		info->multi_shot_stop = FALSE;

		/* sound init to pause other session */
		_mmcamcorder_sound_init(handle);
	}

	_mmcam_dbg_log("preview(%dx%d,fmt:%d), capture(%dx%d,fmt:%d), count(%d), hdr mode(%d), interval (%d)",
		width, height, info->preview_format, info->width, info->height, cap_format,
		info->count, info->hdr_capture_mode, info->interval);

	/* check state */
	if (current_state >= MM_CAMCORDER_STATE_RECORDING) {
		if (info->type == _MMCamcorder_MULTI_SHOT ||
		    info->hdr_capture_mode != MM_CAMCORDER_HDR_OFF) {
			_mmcam_dbg_err("does not support multi/HDR capture while recording");
			ret = MM_ERROR_CAMCORDER_INVALID_STATE;
			goto cmd_done;
		}

		/* check capture size if ZSL is not supported*/
		if (hcamcorder->support_zsl_capture == FALSE) {
			_mmcam_dbg_warn("Capture size should be same with preview size while recording");
			_mmcam_dbg_warn("Capture size %dx%d -> %dx%d", info->width, info->height, width, height);

			info->width = width;
			info->height = height;

			_mmcam_dbg_log("set capture width and height [%dx%d] to camera plugin", width, height);
			MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst, "capture-width", width);
			MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst, "capture-height", height);

			is_modified_size = TRUE;
		}
	}

	info->capture_cur_count = 0;
	info->capture_send_count = 0;

	sc->internal_encode = FALSE;

	if (!sc->bencbin_capture) {
		/* Check encoding method */
		if (cap_format == MM_PIXEL_FORMAT_ENCODED) {
			if ((sc->SensorEncodedCapture && info->type == _MMCamcorder_SINGLE_SHOT) ||
			    hcamcorder->support_zsl_capture || is_modified_size) {
				cap_fourcc = _mmcamcorder_get_fourcc(cap_format, image_encoder, hcamcorder->use_zero_copy_format);
				_mmcam_dbg_log("Sensor JPEG Capture [is_modified_size:%d]", is_modified_size);
			} else {
				/* no need to encode internally if ITLV format */
				if (info->preview_format != MM_PIXEL_FORMAT_ITLV_JPEG_UYVY)
					sc->internal_encode = TRUE;

				cap_fourcc = _mmcamcorder_get_fourcc(info->preview_format, image_encoder, hcamcorder->use_zero_copy_format);

				_mmcam_dbg_log("MSL JPEG Capture : capture fourcc %c%c%c%c",
					cap_fourcc, cap_fourcc>>8, cap_fourcc>>16, cap_fourcc>>24);
			}
		} else {
			cap_fourcc = _mmcamcorder_get_fourcc(cap_format, MM_IMAGE_CODEC_INVALID, hcamcorder->use_zero_copy_format);
		}

		_mmcam_dbg_log("capture format (%d)", cap_format);

		/* Note: width/height of capture is set in commit function of attribute or in create function of pipeline */
		MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst, "capture-fourcc", cap_fourcc);
		MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst, "capture-interval", info->interval);
		MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst, "capture-count", info->count);

		if (!GST_IS_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst)) {
			_mmcam_dbg_err("Can't cast Video source into camera control.");
			ret = MM_ERROR_CAMCORDER_NOT_SUPPORTED;
			goto cmd_done;
		}

		control = GST_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst);
		gst_camera_control_set_capture_command(control, GST_CAMERA_CONTROL_CAPTURE_COMMAND_START);
	} else {
		int need_change = 0;
		int set_width = 0;
		int set_height = 0;
		int cap_jpeg_quality = 0;

		if (current_state >= MM_CAMCORDER_STATE_RECORDING) {
			_mmcam_dbg_err("could not capture in this target while recording");
			ret = MM_ERROR_CAMCORDER_INVALID_STATE;
			goto cmd_done;
		}

		if (UseCaptureMode) {
			if (width != MMFCAMCORDER_HIGHQUALITY_WIDTH || height != MMFCAMCORDER_HIGHQUALITY_HEIGHT)
				need_change = 1;
		} else {
			if (width != info->width || height != info->height)
				need_change = 1;
		}

		if (need_change) {
			int rotation = 0;

			_mmcam_dbg_log("Need to change resolution");

			MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_QUE].gst, "empty-buffers", TRUE);
			MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_QUE].gst, "empty-buffers", TRUE);

			/* make pipeline state as READY */
			ret = _mmcamcorder_gst_set_state(handle, sc->element[_MMCAMCORDER_MAIN_PIPE].gst, GST_STATE_READY);

			/* check decoder recreation */
			if (!_mmcamcorder_recreate_decoder_for_encoded_preview(handle)) {
				_mmcam_dbg_err("_mmcamcorder_recreate_decoder_for_encoded_preview failed");
				return MM_ERROR_CAMCORDER_INTERNAL;
			}

			MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_QUE].gst, "empty-buffers", FALSE);
			MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_QUE].gst, "empty-buffers", FALSE);

			if (ret != MM_ERROR_NONE) {
				_mmcam_dbg_err("failed to set state PAUSED %x", ret);
				goto cmd_done;
			}

			if (UseCaptureMode) {
				set_width = MMFCAMCORDER_HIGHQUALITY_WIDTH;
				set_height = MMFCAMCORDER_HIGHQUALITY_HEIGHT;
			} else {
				set_width = info->width;
				set_height = info->height;
			}

			mm_camcorder_get_attributes(handle, &err_name,
				MMCAM_CAMERA_ROTATION, &rotation,
				NULL);
			if (err_name) {
				_mmcam_dbg_warn("get_attributes err %s", err_name);
				SAFE_FREE(err_name);
			}

			/* set new caps */
			ret = _mmcamcorder_set_videosrc_caps(handle, sc->fourcc, set_width, set_height, fps, rotation);
			if (!ret) {
				_mmcam_dbg_err("_mmcamcorder_set_videosrc_caps failed");
				ret = MM_ERROR_CAMCORDER_INTERNAL;
				goto cmd_done;
			}

			info->resolution_change = TRUE;

			/* make pipeline state as PLAYING */
			ret = _mmcamcorder_gst_set_state(handle, sc->element[_MMCAMCORDER_MAIN_PIPE].gst, GST_STATE_PLAYING);
			if (ret != MM_ERROR_NONE) {
				_mmcam_dbg_err("failed to set state PLAYING %x", ret);
				goto cmd_done;
			}

			_mmcam_dbg_log("Change to target resolution(%d, %d)", set_width, set_height);
		} else {
			_mmcam_dbg_log("No need to change resolution. Open toggle now.");
			info->resolution_change = FALSE;
		}

		/* add encodesinkbin */
		ret = _mmcamcorder_create_stillshot_pipeline((MMHandleType)hcamcorder);
		if (ret != MM_ERROR_NONE) {
			_mmcam_dbg_err("failed to create encodesinkbin %x", ret);
			goto cmd_done;
		}

		ret = mm_camcorder_get_attributes(handle, &err_name,
			MMCAM_IMAGE_ENCODER_QUALITY, &cap_jpeg_quality,
			NULL);
		if (err_name) {
			_mmcam_dbg_warn("get_attributes err %s, ret 0x%x", err_name, ret);
			SAFE_FREE(err_name);
		}

		/* set JPEG quality */
		MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_IENC].gst, "quality", cap_jpeg_quality);

		/* set handoff signal as TRUE */
		MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_SINK].gst, "signal-handoffs", TRUE);
		MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, "block", FALSE);

		/* Prepare for the shutter sound when it's the bencbin mode capture */
		sc->info_video->is_firstframe = TRUE;

		/* set push encoding buffer as TRUE */
		sc->info_video->push_encoding_buffer = PUSH_ENCODING_BUFFER_INIT;

		/* make encode pipeline state as PLAYING */
		ret = _mmcamcorder_gst_set_state(handle, sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst, GST_STATE_PLAYING);
		if (ret != MM_ERROR_NONE) {
			_mmcam_dbg_err("failed to set state PLAYING %x", ret);
			goto cmd_done;
		}
	}

	/* Play capture sound here if single capture */
	if ((info->type == _MMCamcorder_SINGLE_SHOT &&
	     (hcamcorder->support_zsl_capture == FALSE ||
	      strobe_mode == MM_CAMCORDER_STROBE_MODE_OFF)) ||
	    info->hdr_capture_mode) {
		if (current_state < MM_CAMCORDER_STATE_RECORDING &&
		    hcamcorder->support_zsl_capture == TRUE &&
		    !info->hdr_capture_mode) {
			_mmcamcorder_sound_solo_play((MMHandleType)hcamcorder, _MMCAMCORDER_SAMPLE_SOUND_NAME_CAPTURE01, FALSE);
		}

		/* set flag */
		info->played_capture_sound = TRUE;
	} else {
		/* set flag */
		info->played_capture_sound = FALSE;
	}

cmd_done:
	if (ret != MM_ERROR_NONE)
		info->capturing = FALSE;

	return ret;
}


int _mmcamcorder_image_cmd_preview_start(MMHandleType handle)
{
	int ret = MM_ERROR_NONE;
	int width = 0;
	int height = 0;
	int fps = 0;
	int rotation = 0;
	unsigned int current_framecount = 0;
	int current_state = MM_CAMCORDER_STATE_NONE;
	gboolean fps_auto = FALSE;
	char *err_name = NULL;
	const char *videosrc_name = NULL;

	GstState state;
	GstElement *pipeline = NULL;
	GstCameraControl *control = NULL;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderImageInfo *info = NULL;
	_MMCamcorderSubContext *sc = NULL;
	type_element *VideosrcElement = NULL;

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc && sc->info_image && sc->element, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	/*_mmcam_dbg_log("");*/

	pipeline = sc->element[_MMCAMCORDER_MAIN_PIPE].gst;
	info = sc->info_image;

	_mmcamcorder_conf_get_element(handle, hcamcorder->conf_main,
		CONFIGURE_CATEGORY_MAIN_VIDEO_INPUT,
		"VideosrcElement",
		&VideosrcElement);

	_mmcamcorder_conf_get_value_element_name(VideosrcElement, &videosrc_name);

	sc->display_interval = 0;
	sc->previous_slot_time = 0;

	/* Image info */
	info->next_shot_time = 0;
	info->multi_shot_stop = TRUE;
	info->capturing = FALSE;

	_mmcamcorder_vframe_stablize(handle);

	current_state = _mmcamcorder_get_state(handle);
	_mmcam_dbg_log("current state [%d]", current_state);

	if (!sc->bencbin_capture) {
		_mmcam_dbg_log("Preview start");

		/* just set capture stop command if current state is CAPTURING */
		if (current_state == MM_CAMCORDER_STATE_CAPTURING) {
			if (!GST_IS_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst)) {
				_mmcam_dbg_err("Can't cast Video source into camera control.");
				return MM_ERROR_CAMCORDER_NOT_SUPPORTED;
			}

			control = GST_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst);
			gst_camera_control_set_capture_command(control, GST_CAMERA_CONTROL_CAPTURE_COMMAND_STOP);

			current_framecount = sc->kpi.video_framecount;
			MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_SINK].gst, "stop-video", FALSE);
			if (info->type == _MMCamcorder_SINGLE_SHOT) {
				MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_QUE].gst, "empty-buffers", FALSE);
				MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_QUE].gst, "empty-buffers", FALSE);
			}
		} else {
			int focus_mode = 0;
			mmf_attrs_t *attr = (mmf_attrs_t *)MMF_CAMCORDER_ATTRS(handle);

			/* This case is starting of preview */
			ret = mm_camcorder_get_attributes(handle, &err_name,
				MMCAM_CAMERA_FPS_AUTO, &fps_auto,
				MMCAM_CAMERA_FOCUS_MODE, &focus_mode,
				NULL);
			if (ret != MM_ERROR_NONE) {
				_mmcam_dbg_warn("Get attrs fail. (%s:%x)", err_name, ret);
				SAFE_FREE(err_name);
			}

			_mmcam_dbg_log("focus mode %d", focus_mode);

			MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst, "fps-auto", fps_auto);

			/* set focus mode */
			if (attr) {
				if (_mmcamcorder_check_supported_attribute(handle, MM_CAM_CAMERA_FOCUS_MODE)) {
					mmf_attribute_set_modified(&(attr->items[MM_CAM_CAMERA_FOCUS_MODE]));
					mmf_attrs_commit((MMHandleType)attr);
				} else {
					_mmcam_dbg_log("focus mode is not supported");
				}
			} else {
				_mmcam_dbg_err("failed to get attributes");
			}
		}
	}

	/* init prev_time */
	sc->info_video->prev_preview_ts = 0;

	gst_element_get_state(pipeline, &state, NULL, -1);

	if (state == GST_STATE_PLAYING) {
		if (!sc->bencbin_capture) {
			int try_count = 0;

			if (hcamcorder->support_zsl_capture == FALSE) {
				mmf_attrs_t *attr = (mmf_attrs_t *)MMF_CAMCORDER_ATTRS(handle);

				/* Set strobe mode - strobe mode can not be set to driver while captuing */
				if (attr) {
					mmf_attribute_set_modified(&(attr->items[MM_CAM_STROBE_MODE]));
					if (mmf_attrs_commit((MMHandleType) attr) == -1)
						_mmcam_dbg_warn("Failed to set strobe mode");
				}

				while (current_framecount >= sc->kpi.video_framecount &&
					try_count++ < _MMCAMCORDER_CAPTURE_STOP_CHECK_COUNT) {
					usleep(_MMCAMCORDER_CAPTURE_STOP_CHECK_INTERVAL);
				}
			}

			if (info->type == _MMCamcorder_MULTI_SHOT) {
				MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_QUE].gst, "empty-buffers", FALSE);
				MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_QUE].gst, "empty-buffers", FALSE);
			}

			_mmcam_dbg_log("Wait Frame Done. count before[%d],after[%d], try_count[%d]",
				current_framecount, sc->kpi.video_framecount, try_count);
		} else {
			ret = _mmcamcorder_remove_stillshot_pipeline(handle);
			if (ret != MM_ERROR_NONE)
				goto cmd_error;

			if (info->resolution_change) {
				MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_QUE].gst, "empty-buffers", TRUE);
				MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_QUE].gst, "empty-buffers", TRUE);

				ret = _mmcamcorder_gst_set_state(handle, pipeline, GST_STATE_READY);

				/* check decoder recreation */
				if (!_mmcamcorder_recreate_decoder_for_encoded_preview(handle)) {
					_mmcam_dbg_err("_mmcamcorder_recreate_decoder_for_encoded_preview failed");
					return MM_ERROR_CAMCORDER_INTERNAL;
				}

				MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_QUE].gst, "empty-buffers", FALSE);
				MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_QUE].gst, "empty-buffers", FALSE);

				if (ret != MM_ERROR_NONE)
					goto cmd_error;

				/* check if resolution need to rollback */
				mm_camcorder_get_attributes(handle, &err_name,
					MMCAM_CAMERA_WIDTH, &width,
					MMCAM_CAMERA_HEIGHT, &height,
					MMCAM_CAMERA_FPS, &fps,
					MMCAM_CAMERA_ROTATION, &rotation,
					NULL);
				if (err_name) {
					_mmcam_dbg_warn("get_attributes err %s, ret 0x%x", err_name, ret);
					SAFE_FREE(err_name);
				}

				/* set new caps */
				ret = _mmcamcorder_set_videosrc_caps(handle, sc->fourcc, width, height, fps, rotation);
				if (!ret) {
					_mmcam_dbg_err("_mmcamcorder_set_videosrc_caps failed");
					ret = MM_ERROR_CAMCORDER_INTERNAL;
					goto cmd_error;
				}

				ret = _mmcamcorder_gst_set_state(handle, pipeline, GST_STATE_PLAYING);
				if (ret != MM_ERROR_NONE)
					goto cmd_error;
			}
		}

		/* sound finalize */
		if (info->type == _MMCamcorder_MULTI_SHOT)
			_mmcamcorder_sound_finalize(handle);
	} else {
		if (info->preview_format == MM_PIXEL_FORMAT_ENCODED_H264) {
			MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst, "bitrate", H264_PREVIEW_BITRATE);
			MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst, "newgop-interval", H264_PREVIEW_NEWGOP_INTERVAL);
		}

		MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_QUE].gst, "empty-buffers", FALSE);
		MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_QUE].gst, "empty-buffers", FALSE);

		traceBegin(TTRACE_TAG_CAMERA, "MMCAMCORDER:START:SET_PLAYING_TO_PIPELINE");

		ret = _mmcamcorder_gst_set_state(handle, pipeline, GST_STATE_PLAYING);

		traceEnd(TTRACE_TAG_CAMERA);

		if (ret != MM_ERROR_NONE)
			goto cmd_error;

		/* get sound status/volume level and register changed_cb */
		if (hcamcorder->shutter_sound_policy == VCONFKEY_CAMERA_SHUTTER_SOUND_POLICY_OFF &&
		    info->sound_status == _SOUND_STATUS_INIT) {
			_mmcam_dbg_log("register vconf changed_cb and get sound status");

			/* register changed_cb */
			vconf_notify_key_changed(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, __sound_status_changed_cb, hcamcorder);

			/* get sound status */
			vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &(info->sound_status));

			_mmcam_dbg_log("sound status %d", info->sound_status);
		}
	}

cmd_error:
	return ret;
}


int _mmcamcorder_image_cmd_preview_stop(MMHandleType handle)
{
	int ret = MM_ERROR_NONE;
	int strobe_mode = MM_CAMCORDER_STROBE_MODE_OFF;
	int set_strobe = 0;
	int display_reuse_hint = FALSE;
	GstCameraControl *control = NULL;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	GstElement *pipeline = NULL;

	_MMCamcorderSubContext *sc = NULL;

	_mmcam_dbg_log("");

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc && sc->element, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	/* check strobe and set OFF if PERMANENT mode */
	mm_camcorder_get_attributes(handle, NULL,
		MMCAM_STROBE_MODE, &strobe_mode,
		NULL);
	if (strobe_mode == MM_CAMCORDER_STROBE_MODE_PERMANENT &&
	    GST_IS_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst)) {
		_mmcam_dbg_log("current strobe mode is PERMANENT, set OFF");

		/* get camera control */
		control = GST_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst);
		if (control) {
			/* convert MSL to sensor value */
			set_strobe = _mmcamcorder_convert_msl_to_sensor(handle, MM_CAM_STROBE_MODE, MM_CAMCORDER_STROBE_MODE_OFF);

			/* set strobe OFF */
			gst_camera_control_set_strobe(control, GST_CAMERA_CONTROL_STROBE_MODE, set_strobe);

			_mmcam_dbg_log("set strobe OFF done - value: %d", set_strobe);
		} else {
			_mmcam_dbg_warn("cast CAMERA_CONTROL failed");
		}
	}

	pipeline = sc->element[_MMCAMCORDER_MAIN_PIPE].gst;

	if (sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst) {
		_mmcam_dbg_log("pipeline is exist so need to remove pipeline and sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst=%p",
			sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst);
		_mmcamcorder_remove_recorder_pipeline(handle);
	}

	mm_camcorder_get_attributes(handle, NULL,
		MMCAM_DISPLAY_REUSE_HINT, &display_reuse_hint,
		NULL);

	_mmcam_dbg_log("display reuse hint %d", display_reuse_hint);

	MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_SINK].gst, "keep-camera-preview", display_reuse_hint);

	MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_QUE].gst, "empty-buffers", TRUE);
	MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_QUE].gst, "empty-buffers", TRUE);

	traceBegin(TTRACE_TAG_CAMERA, "MMCAMCORDER:STOP:SET_READY_TO_PIPELINE");

	ret = _mmcamcorder_gst_set_state(handle, pipeline, GST_STATE_READY);

	traceEnd(TTRACE_TAG_CAMERA);

	MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_QUE].gst, "empty-buffers", FALSE);
	MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_QUE].gst, "empty-buffers", FALSE);

	if (display_reuse_hint)
		MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_SINK].gst, "keep-camera-preview", FALSE);

	/* deregister sound status callback */
	if (sc->info_image->sound_status != _SOUND_STATUS_INIT) {
		_mmcam_dbg_log("deregister sound status callback");

		vconf_ignore_key_changed(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, __sound_status_changed_cb);

		sc->info_image->sound_status = _SOUND_STATUS_INIT;
	}

	return ret;
}


int _mmcamcorder_video_capture_command(MMHandleType handle, int command)
{
	int ret = MM_ERROR_NONE;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc && sc->element, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	_mmcam_dbg_log("command %d", command);

	switch (command) {
	case _MMCamcorder_CMD_CAPTURE:
		ret = _mmcamcorder_image_cmd_capture(handle);
		break;
	case _MMCamcorder_CMD_PREVIEW_START:
		ret = _mmcamcorder_image_cmd_preview_start(handle);

		/* I place this function last because it miscalculates a buffer that sents in GST_STATE_PAUSED */
		_mmcamcorder_video_current_framerate_init(handle);
		break;
	case _MMCamcorder_CMD_PREVIEW_STOP:
		ret = _mmcamcorder_image_cmd_preview_stop(handle);
		break;
	case _MMCamcorder_CMD_RECORD:
	case _MMCamcorder_CMD_PAUSE:
	case _MMCamcorder_CMD_CANCEL:
	case _MMCamcorder_CMD_COMMIT:
		/* video recording command */
		ret = _mmcamcorder_video_command(handle, command);
		break;
	default:
		ret =  MM_ERROR_CAMCORDER_INVALID_ARGUMENT;
		break;
	}

	return ret;
}


void __mmcamcorder_init_stillshot_info(MMHandleType handle)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;
	_MMCamcorderImageInfo *info = NULL;

	mmf_return_if_fail(hcamcorder);

	sc = MMF_CAMCORDER_SUBCONTEXT(hcamcorder);
	mmf_return_if_fail(sc && sc->info_image);

	info = sc->info_image;

	_mmcam_dbg_log("capture type[%d], capture send count[%d]", info->type, info->capture_send_count);

	if (info->type == _MMCamcorder_SINGLE_SHOT || info->capture_send_count == info->count) {
		info->capture_cur_count = 0;
		info->capture_send_count = 0;
		info->multi_shot_stop = TRUE;
		info->next_shot_time = 0;

		/* capturing flag set to FALSE here */
		info->capturing = FALSE;
	}

	return;
}


int __mmcamcorder_capture_save_exifinfo(MMHandleType handle, MMCamcorderCaptureDataType *original, MMCamcorderCaptureDataType *thumbnail, int provide_exif)
{
	int ret = MM_ERROR_NONE;
	unsigned char *data = NULL;
	unsigned int datalen = 0;
	_MMCamcorderSubContext *sc = NULL;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	mmf_return_val_if_fail(hcamcorder, FALSE);
	sc = MMF_CAMCORDER_SUBCONTEXT(hcamcorder);
	mmf_return_val_if_fail(sc, FALSE);

	_mmcam_dbg_log("");

	if (!original || original->data == NULL || original->length == 0) {
		if (original) {
			_mmcam_dbg_err("data=%p, length=%d",
				original->data, original->length);
		} else {
			_mmcam_dbg_err("original is null");
		}

		return MM_ERROR_CAMCORDER_INVALID_ARGUMENT;
	} else {
		/* original is input/output param. save original values to local var. */
		data = original->data;
		datalen = original->length;
	}

	if (provide_exif == FALSE) {
		if (thumbnail) {
			if (thumbnail->data && thumbnail->length > 0) {
				_mmcam_dbg_log("thumbnail is added!thumbnail->data=%p thumbnail->width=%d ,thumbnail->height=%d",
					thumbnail->data, thumbnail->width, thumbnail->height);

				/* add thumbnail exif info */
				ret = mm_exif_add_thumbnail_info(hcamcorder->exif_info,
					thumbnail->data,
					thumbnail->width,
					thumbnail->height,
					thumbnail->length);
			} else {
				_mmcam_dbg_err("Skip adding thumbnail (data=%p, length=%d)",
					thumbnail->data, thumbnail->length);
			}
		}
	} else {
		ret = MM_ERROR_NONE;
	}

	if (ret == MM_ERROR_NONE) {
		/* write jpeg with exif */
		ret = mm_exif_write_exif_jpeg_to_memory(&original->data, &original->length, hcamcorder->exif_info, data, datalen);
		if (ret != MM_ERROR_NONE)
			_mmcam_dbg_err("mm_exif_write_exif_jpeg_to_memory error! [0x%x]", ret);
	}

	_mmcam_dbg_log("END ret 0x%x", ret);

	return ret;
}


void __mmcamcorder_get_capture_data_from_buffer(MMCamcorderCaptureDataType *capture_data, int pixtype, GstSample *sample)
{
	GstCaps *caps = NULL;
	GstMapInfo mapinfo;
	const GstStructure *structure;

	mmf_return_if_fail(capture_data && sample);

	caps = gst_sample_get_caps(sample);
	if (caps == NULL) {
		_mmcam_dbg_err("failed to get caps");
		goto GET_FAILED;
	}

	structure = gst_caps_get_structure(caps, 0);
	if (structure == NULL) {
		_mmcam_dbg_err("failed to get structure");
		goto GET_FAILED;
	}

	memset(&mapinfo, 0x0, sizeof(GstMapInfo));

	gst_buffer_map(gst_sample_get_buffer(sample), &mapinfo, GST_MAP_READ);
	capture_data->data = mapinfo.data;
	capture_data->format = pixtype;
	gst_structure_get_int(structure, "width", &capture_data->width);
	gst_structure_get_int(structure, "height", &capture_data->height);
	capture_data->length = mapinfo.size;
	gst_buffer_unmap(gst_sample_get_buffer(sample), &mapinfo);

	_mmcam_dbg_warn("buffer data[%p],size[%dx%d],length[%d],format[%d]",
		capture_data->data, capture_data->width, capture_data->height,
		capture_data->length, capture_data->format);
	return;

GET_FAILED:
	capture_data->data = NULL;
	capture_data->format = MM_PIXEL_FORMAT_INVALID;
	capture_data->length = 0;

	return;
}


int __mmcamcorder_set_jpeg_data(MMHandleType handle, MMCamcorderCaptureDataType *dest, MMCamcorderCaptureDataType *thumbnail, int provide_exif)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder && dest, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	sc = MMF_CAMCORDER_SUBCONTEXT(hcamcorder);
	mmf_return_val_if_fail(sc, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	_mmcam_dbg_log("");

	/* if tag enable and doesn't provide exif, we make it */
	_mmcam_dbg_log("Add exif information if existed(thumbnail[%p])", thumbnail);
	if (thumbnail && thumbnail->data)
		return __mmcamcorder_capture_save_exifinfo(handle, dest, thumbnail, provide_exif);
	else
		return __mmcamcorder_capture_save_exifinfo(handle, dest, NULL, provide_exif);
}


void __mmcamcorder_release_jpeg_data(MMHandleType handle, MMCamcorderCaptureDataType *dest, int tag_enable, int provide_exif)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_if_fail(hcamcorder);
	mmf_return_if_fail(dest);

	sc = MMF_CAMCORDER_SUBCONTEXT(hcamcorder);
	mmf_return_if_fail(sc);

	_mmcam_dbg_log("tag : %d, provide exif : %d", tag_enable, provide_exif);

	/* if dest->data is allocated in MSL, release it */
	if (tag_enable && !provide_exif) {
		if (dest->data) {
			free(dest->data);
			dest->length = 0;
			dest->data = NULL;
			_mmcam_dbg_log("Jpeg is released!");
		}
	} else {
		_mmcam_dbg_log("No need to release");
	}

	return;
}


static void __mmcamcorder_image_capture_cb(GstElement *element, GstSample *sample1, GstSample *sample2, GstSample *sample3, gpointer u_data)
{
	int ret = MM_ERROR_NONE;
	int pixtype_main = MM_PIXEL_FORMAT_INVALID;
	int pixtype_thumb = MM_PIXEL_FORMAT_INVALID;
	int pixtype_scrnl = MM_PIXEL_FORMAT_INVALID;
	int codectype = MM_IMAGE_CODEC_JPEG;
	int attr_index = 0;
	int count = 0;
	int stop_cont_shot = FALSE;
	int tag_enable = FALSE;
	int provide_exif = FALSE;
	int capture_quality = 0;
	int try_lock_count = 0;
	gboolean send_captured_message = FALSE;
	unsigned char *exif_raw_data = NULL;
	unsigned char *internal_main_data = NULL;
	unsigned int internal_main_length = 0;
	unsigned char *internal_thumb_data = NULL;
	unsigned int internal_thumb_length = 0;
	unsigned char *compare_data = NULL;

	MMCamcorderStateType current_state = MM_CAMCORDER_STATE_NONE;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(u_data);
	_MMCamcorderImageInfo *info = NULL;
	_MMCamcorderSubContext *sc = NULL;
	MMCamcorderCaptureDataType dest = {0,};
	MMCamcorderCaptureDataType thumb = {0,};
	MMCamcorderCaptureDataType scrnail = {0,};
	MMCamcorderCaptureDataType encode_src = {0,};
	GstMapInfo mapinfo1;
	GstMapInfo mapinfo2;
	GstMapInfo mapinfo3;

	mmf_attrs_t *attrs = NULL;
	mmf_attribute_t *item_screennail = NULL;
	mmf_attribute_t *item_exif_raw_data = NULL;

	mmf_return_if_fail(hcamcorder);

	sc = MMF_CAMCORDER_SUBCONTEXT(hcamcorder);
	mmf_return_if_fail(sc && sc->info_image);

	info = sc->info_image;

	memset(&mapinfo1, 0x0, sizeof(GstMapInfo));
	memset(&mapinfo2, 0x0, sizeof(GstMapInfo));
	memset(&mapinfo3, 0x0, sizeof(GstMapInfo));

	/* get current state */
	current_state = _mmcamcorder_get_state((MMHandleType)hcamcorder);

	_mmcam_dbg_err("START - current state %d", current_state);

	/* check capture state */
	if (info->type == _MMCamcorder_MULTI_SHOT && info->capture_send_count > 0)
		mm_camcorder_get_attributes((MMHandleType)hcamcorder, NULL, "capture-break-cont-shot", &stop_cont_shot, NULL);

	if (!info->capturing || stop_cont_shot) {
		_mmcam_dbg_warn("stop command[%d] or not capturing state[%d]. skip this...", stop_cont_shot, info->capturing);

		/* set FALSE here for the case that info->capturing is still FALSE
			(== capture_send_count is 0 at the time _mmcamcorder_commit_capture_break_cont_shot is called) */
		info->capturing = FALSE;

		/*free GstBuffer*/
		if (sample1)
			gst_sample_unref(sample1);

		if (sample2)
			gst_sample_unref(sample2);

		if (sample3)
			gst_sample_unref(sample3);

		return;
	}

	/* check command lock to block capture callback if capture start API is not returned
	   wait for 2 seconds at worst case */
	try_lock_count = 0;
	do {
		_mmcam_dbg_log("Try command LOCK");
		if (_MMCAMCORDER_TRYLOCK_CMD(hcamcorder)) {
			_mmcam_dbg_log("command LOCK OK");
			_MMCAMCORDER_UNLOCK_CMD(hcamcorder);
			break;
		}

		if (try_lock_count++ < TRY_LOCK_MAX_COUNT) {
			_mmcam_dbg_warn("command LOCK Failed, retry...[count %d]", try_lock_count);
			usleep(TRY_LOCK_TIME);
		} else {
			_mmcam_dbg_err("failed to lock command LOCK");
			break;
		}
	} while (TRUE);

	if (current_state < MM_CAMCORDER_STATE_RECORDING) {
		/* play capture sound here if multi capture
		   or preview format is ITLV(because of AF and flash control in plugin) */
		if (info->type == _MMCamcorder_MULTI_SHOT) {
			g_mutex_lock(&hcamcorder->task_thread_lock);
			_mmcam_dbg_log("send signal for sound play");
			hcamcorder->task_thread_state = _MMCAMCORDER_TASK_THREAD_STATE_SOUND_PLAY_START;
			g_cond_signal(&hcamcorder->task_thread_cond);
			g_mutex_unlock(&hcamcorder->task_thread_lock);
		} else if (!info->played_capture_sound) {
			_mmcamcorder_sound_solo_play((MMHandleType)hcamcorder, _MMCAMCORDER_SAMPLE_SOUND_NAME_CAPTURE01, FALSE);
		}
	}

	/* init capture data */
	memset((void *)&dest, 0x0, sizeof(MMCamcorderCaptureDataType));
	memset((void *)&thumb, 0x0, sizeof(MMCamcorderCaptureDataType));
	memset((void *)&scrnail, 0x0, sizeof(MMCamcorderCaptureDataType));

	/* Prepare main, thumbnail buffer */
	pixtype_main = _mmcamcorder_get_pixel_format(gst_sample_get_caps(sample1));
	if (pixtype_main == MM_PIXEL_FORMAT_INVALID) {
		_mmcam_dbg_err("Unsupported pixel type");
		MMCAM_SEND_MESSAGE(hcamcorder, MM_MESSAGE_CAMCORDER_ERROR, MM_ERROR_CAMCORDER_INTERNAL);
		goto error;
	}

	/* Main image buffer */
	if (!sample1 || !gst_buffer_map(gst_sample_get_buffer(sample1), &mapinfo1, GST_MAP_READ)) {
		_mmcam_dbg_err("sample1[%p] is NULL or gst_buffer_map failed", sample1);
		MMCAM_SEND_MESSAGE(hcamcorder, MM_MESSAGE_CAMCORDER_ERROR, MM_ERROR_CAMCORDER_INTERNAL);
		goto error;
	} else {
		if ((mapinfo1.data == NULL) && (mapinfo1.size == 0)) {
			_mmcam_dbg_err("mapinfo1 is wrong (%p, size %d)", mapinfo1.data, mapinfo1.size);
			MMCAM_SEND_MESSAGE(hcamcorder, MM_MESSAGE_CAMCORDER_ERROR, MM_ERROR_CAMCORDER_INTERNAL);
			gst_buffer_unmap(gst_sample_get_buffer(sample1), &mapinfo1);
			goto error;
		} else {
			__mmcamcorder_get_capture_data_from_buffer(&dest, pixtype_main, sample1);
		}
	}

	if (!sample2 || !gst_buffer_map(gst_sample_get_buffer(sample2), &mapinfo2, GST_MAP_READ))
		_mmcam_dbg_log("sample2[%p] is NULL or gst_buffer_map failed. Not Error.", sample2);

	if (!sample3 || !gst_buffer_map(gst_sample_get_buffer(sample3), &mapinfo3, GST_MAP_READ))
		_mmcam_dbg_log("sample3[%p] is NULL or gst_buffer_map failed. Not Error.", sample3);

	/* Screennail image buffer */
	attrs = (mmf_attrs_t *)MMF_CAMCORDER_ATTRS(hcamcorder);
	mm_attrs_get_index((MMHandleType)attrs, MMCAM_CAPTURED_SCREENNAIL, &attr_index);
	item_screennail = &attrs->items[attr_index];

	if (sample3 && mapinfo3.data && mapinfo3.size != 0) {
		_mmcam_dbg_log("Screennail (sample3=%p,size=%d)", sample3, mapinfo3.size);

		pixtype_scrnl = _mmcamcorder_get_pixel_format(gst_sample_get_caps(sample3));
		__mmcamcorder_get_capture_data_from_buffer(&scrnail, pixtype_scrnl, sample3);

		/* Set screennail attribute for application */
		ret = mmf_attribute_set_data(item_screennail, &scrnail, sizeof(scrnail));
		_mmcam_dbg_log("Screennail set attribute data %p, size %d, ret %x", &scrnail, sizeof(scrnail), ret);
	} else {
		_mmcam_dbg_log("Sample3 has wrong pointer. Not Error. (sample3=%p)", sample3);
		mmf_attribute_set_data(item_screennail, NULL, 0);
	}

	/* commit screennail data */
	mmf_attribute_commit(item_screennail);

	/* init thumb data */
	memset(&encode_src, 0x0, sizeof(MMCamcorderCaptureDataType));

	/* get provide-exif */
	MMCAMCORDER_G_OBJECT_GET(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst, "provide-exif", &provide_exif);

	/* Thumbnail image buffer */
	if (sample2 && mapinfo2.data && (mapinfo2.size != 0)) {
		_mmcam_dbg_log("Thumbnail (buffer2=%p)", gst_sample_get_buffer(sample2));
		pixtype_thumb = _mmcamcorder_get_pixel_format(gst_sample_get_caps(sample2));
		__mmcamcorder_get_capture_data_from_buffer(&thumb, pixtype_thumb, sample2);
	} else {
		_mmcam_dbg_log("Sample2 has wrong pointer. Not Error. (sample2 %p)", sample2);

		if (pixtype_main == MM_PIXEL_FORMAT_ENCODED && provide_exif) {
			ExifLoader *l;
			/* get thumbnail from EXIF */
			l = exif_loader_new();
			if (l) {
				ExifData *ed;
				char  width[10];
				char  height[10];
				ExifEntry *entry = NULL;

				exif_loader_write(l, dest.data, dest.length);

				/* Get a pointer to the EXIF data */
				ed = exif_loader_get_data(l);

				/* The loader is no longer needed--free it */
				exif_loader_unref(l);
				l = NULL;
				if (ed) {
					entry = exif_content_get_entry(ed->ifd[EXIF_IFD_1], EXIF_TAG_IMAGE_WIDTH);
					if (entry != NULL)
						exif_entry_get_value(entry, width, 10);

					entry = NULL;
					entry = exif_content_get_entry(ed->ifd[EXIF_IFD_1], EXIF_TAG_IMAGE_LENGTH);
					if (entry != NULL)
						exif_entry_get_value(entry, height, 10);

					entry = NULL;
					/* Make sure the image had a thumbnail before trying to write it */
					if (ed->data && ed->size) {
						thumb.data = malloc(ed->size);
						if (thumb.data) {
							memcpy(thumb.data, ed->data, ed->size);
							thumb.length = ed->size;
							thumb.format = MM_PIXEL_FORMAT_ENCODED;
							thumb.width = atoi(width);
							thumb.height = atoi(height);
							internal_thumb_data = thumb.data;
						} else {
							_mmcam_dbg_err("failed to alloc thumbnail data");
						}
					}
					exif_data_unref(ed);
					ed = NULL;
				} else {
					_mmcam_dbg_warn("failed to get exif data");
				}
			} else {
				_mmcam_dbg_warn("failed to create exif loader");
			}
		}

		if (thumb.data == NULL) {
			if (pixtype_main == MM_PIXEL_FORMAT_ENCODED &&
			    scrnail.data && scrnail.length != 0) {
				/* make thumbnail image with screennail data */
				memcpy(&encode_src, &scrnail, sizeof(MMCamcorderCaptureDataType));
			} else if (sc->internal_encode) {
				/* make thumbnail image with main data, this is raw data */
				memcpy(&encode_src, &dest, sizeof(MMCamcorderCaptureDataType));
			}
		}

		/* encode thumbnail */
		if (encode_src.data) {
			unsigned int thumb_width = 0;
			unsigned int thumb_height = 0;
			unsigned int thumb_length = 0;
			unsigned char *thumb_raw_data = NULL;

			/* encode image */
			_mmcam_dbg_log("Encode Thumbnail");

			if (encode_src.width > THUMBNAIL_WIDTH) {
				/* calculate thumbnail size */
				thumb_width = THUMBNAIL_WIDTH;
				thumb_height = (thumb_width * encode_src.height) / encode_src.width;
				if (thumb_height % 2 != 0)
					thumb_height += 1;

				_mmcam_dbg_log("need to resize : thumbnail size %dx%d, format %d",
					thumb_width, thumb_height, encode_src.format);

				if ((encode_src.format == MM_PIXEL_FORMAT_UYVY ||
				     encode_src.format == MM_PIXEL_FORMAT_YUYV) &&
				     encode_src.width % thumb_width == 0 &&
				     encode_src.height % thumb_height == 0) {
					if (!_mmcamcorder_downscale_UYVYorYUYV(encode_src.data, encode_src.width, encode_src.height,
						&thumb_raw_data, thumb_width, thumb_height)) {
						thumb_raw_data = NULL;
						_mmcam_dbg_warn("_mmcamcorder_downscale_UYVYorYUYV failed. skip thumbnail making...");
					}
				} else {
					if (!_mmcamcorder_resize_frame(encode_src.data, encode_src.width, encode_src.height,
						encode_src.length, encode_src.format,
						&thumb_raw_data, &thumb_width, &thumb_height, &thumb_length)) {
						thumb_raw_data = NULL;
						_mmcam_dbg_warn("_mmcamcorder_resize_frame failed. skip thumbnail making...");
					}
				}
			} else {
				thumb_width = encode_src.width;
				thumb_height = encode_src.height;

				_mmcam_dbg_log("NO need to resize : thumbnail size %dx%d", thumb_width, thumb_height);

				thumb_raw_data = encode_src.data;
				thumb_length = encode_src.length;
			}

			if (thumb_raw_data) {
				ret = _mmcamcorder_encode_jpeg(thumb_raw_data, thumb_width, thumb_height,
					encode_src.format, thumb_length, THUMBNAIL_JPEG_QUALITY,
					(void **)&internal_thumb_data, &internal_thumb_length);
				if (ret) {
					_mmcam_dbg_log("encode THUMBNAIL done - data %p, length %d", internal_thumb_data, internal_thumb_length);

					thumb.data = internal_thumb_data;
					thumb.length = internal_thumb_length;
					thumb.width = thumb_width;
					thumb.height = thumb_height;
					thumb.format = MM_PIXEL_FORMAT_ENCODED;
				} else {
					_mmcam_dbg_warn("failed to encode THUMBNAIL");
				}

				/* release allocated raw data memory */
				if (thumb_raw_data != encode_src.data) {
					free(thumb_raw_data);
					thumb_raw_data = NULL;
					_mmcam_dbg_log("release thumb_raw_data");
				}
			} else {
				_mmcam_dbg_warn("thumb_raw_data is NULL");
			}
		} else {
			/* no raw data src for thumbnail */
			_mmcam_dbg_log("no need to encode thumbnail");
		}
	}

	/* Encode JPEG */
	if (sc->internal_encode && pixtype_main != MM_PIXEL_FORMAT_ENCODED) {
		mm_camcorder_get_attributes((MMHandleType)hcamcorder, NULL,
			MMCAM_IMAGE_ENCODER_QUALITY, &capture_quality,
			NULL);
		_mmcam_dbg_log("Start Internal Encode - capture_quality %d", capture_quality);

		ret = _mmcamcorder_encode_jpeg(mapinfo1.data, dest.width, dest.height,
			pixtype_main, dest.length, capture_quality,
			(void **)&internal_main_data, &internal_main_length);
		if (!ret) {
			_mmcam_dbg_err("_mmcamcorder_encode_jpeg failed");

			MMCAM_SEND_MESSAGE(hcamcorder, MM_MESSAGE_CAMCORDER_ERROR, MM_ERROR_CAMCORDER_INTERNAL);

			goto error;
		}

		/* set format */
		dest.data = internal_main_data;
		dest.length = internal_main_length;
		dest.format = MM_PIXEL_FORMAT_ENCODED;

		_mmcam_dbg_log("Done Internal Encode - data %p, length %d", dest.data, dest.length);
	}

	/* create EXIF info */
	if (!provide_exif) {	/* make new exif */
		ret = mm_exif_create_exif_info(&(hcamcorder->exif_info));
	} else {	/* load from jpeg buffer dest.data */
		ret = mm_exif_load_exif_info(&(hcamcorder->exif_info), dest.data, dest.length);
		if (ret != MM_ERROR_NONE) {
			_mmcam_dbg_err("Failed to load exif_info [%x], try to create EXIF", ret);
			provide_exif = FALSE;
			ret = mm_exif_create_exif_info(&(hcamcorder->exif_info));
		}
	}
	if (ret != MM_ERROR_NONE) {
		_mmcam_dbg_err("Failed to create exif_info [%x], but keep going...", ret);
	} else {
		/* add basic exif info */
		if (!provide_exif) {
			_mmcam_dbg_log("add basic exif info");
			ret = __mmcamcorder_set_exif_basic_info((MMHandleType)hcamcorder, dest.width, dest.height);
		} else {
			_mmcam_dbg_log("update exif info");
			ret = __mmcamcorder_update_exif_info((MMHandleType)hcamcorder, dest.data, dest.length);
		}

		if (ret != MM_ERROR_NONE) {
			_mmcam_dbg_warn("Failed set_exif_basic_info [%x], but keep going...", ret);
			ret = MM_ERROR_NONE;
		}
	}

	/* get attribute item for EXIF data */
	mm_attrs_get_index((MMHandleType)attrs, MMCAM_CAPTURED_EXIF_RAW_DATA, &attr_index);
	item_exif_raw_data = &attrs->items[attr_index];

	/* set EXIF data to attribute */
	if (hcamcorder->exif_info && hcamcorder->exif_info->data) {
		exif_raw_data = (unsigned char *)malloc(hcamcorder->exif_info->size);
		if (exif_raw_data) {
			memcpy(exif_raw_data, hcamcorder->exif_info->data, hcamcorder->exif_info->size);
			mmf_attribute_set_data(item_exif_raw_data, exif_raw_data, hcamcorder->exif_info->size);
			_mmcam_dbg_log("set EXIF raw data %p, size %d", exif_raw_data, hcamcorder->exif_info->size);
		} else {
			_mmcam_dbg_warn("failed to alloc for EXIF, size %d", hcamcorder->exif_info->size);
		}
	} else {
		_mmcam_dbg_warn("failed to create EXIF. set EXIF as NULL");
		mmf_attribute_set_data(item_exif_raw_data, NULL, 0);
	}

	/* commit EXIF data */
	mmf_attribute_commit(item_exif_raw_data);

	/* get tag-enable */
	mm_camcorder_get_attributes((MMHandleType)hcamcorder, NULL, MMCAM_TAG_ENABLE, &tag_enable, NULL);

	/* Set extra data for JPEG if tag enabled and doesn't provide EXIF */
	if (dest.format == MM_PIXEL_FORMAT_ENCODED) {
		if (tag_enable) {
			mm_camcorder_get_attributes((MMHandleType)hcamcorder, NULL,
				MMCAM_IMAGE_ENCODER, &codectype,
				NULL);
			_mmcam_dbg_log("codectype %d", codectype);

			switch (codectype) {
			case MM_IMAGE_CODEC_JPEG:
			case MM_IMAGE_CODEC_SRW:
			case MM_IMAGE_CODEC_JPEG_SRW:
				ret = __mmcamcorder_set_jpeg_data((MMHandleType)hcamcorder, &dest, &thumb, provide_exif);
				if (ret != MM_ERROR_NONE) {
					_mmcam_dbg_err("Error on setting extra data to jpeg");
					MMCAM_SEND_MESSAGE(hcamcorder, MM_MESSAGE_CAMCORDER_ERROR, ret);
					goto error;
				}
				break;
			default:
				_mmcam_dbg_err("The codectype is not supported. (%d)", codectype);
				MMCAM_SEND_MESSAGE(hcamcorder, MM_MESSAGE_CAMCORDER_ERROR, MM_ERROR_CAMCORDER_INTERNAL);
				goto error;
			}
		}
	}

	/* Handle Capture Callback */
	_MMCAMCORDER_LOCK_VCAPTURE_CALLBACK(hcamcorder);

	if (hcamcorder->vcapture_cb) {
		_mmcam_dbg_log("APPLICATION CALLBACK START");
		if (thumb.data)
			ret = hcamcorder->vcapture_cb(&dest, &thumb, hcamcorder->vcapture_cb_param);
		else
			ret = hcamcorder->vcapture_cb(&dest, NULL, hcamcorder->vcapture_cb_param);

		_mmcam_dbg_log("APPLICATION CALLBACK END");
	} else {
		_mmcam_dbg_err("Capture callback is NULL.");

		MMCAM_SEND_MESSAGE(hcamcorder, MM_MESSAGE_CAMCORDER_ERROR, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

		goto err_release_exif;
	}

	/* Set capture count */
	count = ++(info->capture_send_count);
	send_captured_message = TRUE;

err_release_exif:
	_MMCAMCORDER_UNLOCK_VCAPTURE_CALLBACK(hcamcorder);

	/* init screennail and EXIF raw data */
	mmf_attribute_set_data(item_screennail, NULL, 0);
	mmf_attribute_commit(item_screennail);
	if (exif_raw_data) {
		free(exif_raw_data);
		exif_raw_data = NULL;

		mmf_attribute_set_data(item_exif_raw_data, NULL, 0);
		mmf_attribute_commit(item_exif_raw_data);
	}

	/* Release jpeg data */
	if (pixtype_main == MM_PIXEL_FORMAT_ENCODED)
		__mmcamcorder_release_jpeg_data((MMHandleType)hcamcorder, &dest, tag_enable, provide_exif);

error:
	/* Check end condition and set proper value */
	if (info->hdr_capture_mode != MM_CAMCORDER_HDR_ON_AND_ORIGINAL ||
	    (info->hdr_capture_mode == MM_CAMCORDER_HDR_ON_AND_ORIGINAL && count == 2)) {
		__mmcamcorder_init_stillshot_info((MMHandleType)hcamcorder);
	}

	/* release internal allocated data */
	if (sc->internal_encode)
		compare_data = internal_main_data;
	else
		compare_data = mapinfo1.data;

	if (dest.data && compare_data &&
	    dest.data != compare_data) {
		_mmcam_dbg_log("release internal allocated data %p", dest.data);
		free(dest.data);
		dest.data = NULL;
		dest.length = 0;
	}
	if (internal_main_data) {
		_mmcam_dbg_log("release internal main data %p", internal_main_data);
		free(internal_main_data);
		internal_main_data = NULL;
	}
	if (internal_thumb_data) {
		_mmcam_dbg_log("release internal thumb data %p", internal_thumb_data);
		free(internal_thumb_data);
		internal_thumb_data = NULL;
	}

	/* reset compare_data */
	compare_data = NULL;

	/*free GstBuffer*/
	if (sample1) {
		gst_buffer_unmap(gst_sample_get_buffer(sample1), &mapinfo1);
		gst_sample_unref(sample1);
	}
	if (sample2) {
		gst_buffer_unmap(gst_sample_get_buffer(sample2), &mapinfo2);
		gst_sample_unref(sample2);
	}
	if (sample3) {
		gst_buffer_unmap(gst_sample_get_buffer(sample3), &mapinfo3);
		gst_sample_unref(sample3);
	}

	/* destroy exif info */
	mm_exif_destory_exif_info(hcamcorder->exif_info);
	hcamcorder->exif_info = NULL;

	/* send captured message */
	if (send_captured_message) {
		if (info->hdr_capture_mode != MM_CAMCORDER_HDR_ON_AND_ORIGINAL) {
			/* Send CAPTURED message and count - capture success */
			if (current_state >= MM_CAMCORDER_STATE_RECORDING)
				MMCAM_SEND_MESSAGE(hcamcorder, MM_MESSAGE_CAMCORDER_VIDEO_SNAPSHOT_CAPTURED, count);
			else
				MMCAM_SEND_MESSAGE(hcamcorder, MM_MESSAGE_CAMCORDER_CAPTURED, count);
		} else if (info->hdr_capture_mode == MM_CAMCORDER_HDR_ON_AND_ORIGINAL && count == 2) {
			/* send captured message only once in HDR and Original Capture mode */
			MMCAM_SEND_MESSAGE(hcamcorder, MM_MESSAGE_CAMCORDER_CAPTURED, 1);
		}
	}

	if (current_state >= MM_CAMCORDER_STATE_RECORDING) {
		/* Handle capture in recording case */
		hcamcorder->capture_in_recording = FALSE;

		g_mutex_lock(&hcamcorder->task_thread_lock);

		if (hcamcorder->task_thread_state == _MMCAMCORDER_TASK_THREAD_STATE_CHECK_CAPTURE_IN_RECORDING) {
			_mmcam_dbg_log("send signal for capture in recording");
			g_cond_signal(&hcamcorder->task_thread_cond);
		} else {
			_mmcam_dbg_warn("unexpected task thread state : %d", hcamcorder->task_thread_state);
		}

		g_mutex_unlock(&hcamcorder->task_thread_lock);

		_MMCAMCORDER_CMD_SIGNAL(hcamcorder);
	}

	_mmcam_dbg_err("END");

	return;
}


gboolean __mmcamcorder_handoff_callback(GstElement *fakesink, GstBuffer *buffer, GstPad *pad, gpointer u_data)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(u_data);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder, FALSE);

	sc = MMF_CAMCORDER_SUBCONTEXT(hcamcorder);
	mmf_return_val_if_fail(sc && sc->element, FALSE);

	_mmcam_dbg_log("");

	/* FIXME. How could you get a thumbnail? */
	__mmcamcorder_image_capture_cb(fakesink, gst_sample_new(buffer, gst_pad_get_current_caps(pad), NULL, NULL), NULL, NULL, u_data);

	if (sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst) {
		MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_SINK].gst, "signal-handoffs", FALSE);
		MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, "block", TRUE);
		sc->info_video->push_encoding_buffer = PUSH_ENCODING_BUFFER_STOP;
	}

	return TRUE;
}


static ExifData *__mmcamcorder_update_exif_orientation(MMHandleType handle, ExifData *ed)
{
	int value = 0;
	ExifShort eshort = 0;
	int ret = MM_ERROR_NONE;

	mm_camcorder_get_attributes(handle, NULL, MMCAM_TAG_ORIENTATION, &value, NULL);
	_mmcam_dbg_log("get orientation [%d]", value);
	if (value == 0)
		value = MM_EXIF_ORIENTATION;

	exif_set_short((unsigned char *)&eshort, exif_data_get_byte_order(ed), value);
	ret = mm_exif_set_add_entry(ed, EXIF_IFD_0, EXIF_TAG_ORIENTATION,
		EXIF_FORMAT_SHORT, 1, (const char *)&eshort);
	if (ret != MM_ERROR_NONE)
		EXIF_SET_ERR(ret, EXIF_TAG_MAKER_NOTE);

exit:
	return ed;
}


static ExifData *__mmcamcorder_update_exif_make(MMHandleType handle, ExifData *ed)
{
	int ret = MM_ERROR_NONE;
	char *make = strdup(MM_MAKER_NAME);

	if (make) {
		_mmcam_dbg_log("maker [%s]", make);
		ret = mm_exif_set_add_entry(ed, EXIF_IFD_0, EXIF_TAG_MAKE,
			EXIF_FORMAT_ASCII, strlen(make)+1, (const char *)make);
		free(make);
		if (ret != MM_ERROR_NONE)
			EXIF_SET_ERR(ret, EXIF_TAG_MAKE);
	} else {
		_mmcam_dbg_err("strdup failed [%s]", MM_MAKER_NAME);
	}

exit:
	return ed;
}


static ExifData *__mmcamcorder_update_exif_software(MMHandleType handle, ExifData *ed)
{
	int ret = MM_ERROR_NONE;
	mmf_camcorder_t *hcamcorder = (mmf_camcorder_t *)handle;

	if (hcamcorder == NULL || ed == NULL) {
		_mmcam_dbg_err("NULL parameter %p,%p", hcamcorder, ed);
		return NULL;
	}

	if (hcamcorder->software_version) {
		ret = mm_exif_set_add_entry(ed, EXIF_IFD_0, EXIF_TAG_SOFTWARE, EXIF_FORMAT_ASCII,
			strlen(hcamcorder->software_version)+1, (const char *)hcamcorder->software_version);

		_mmcam_dbg_log("set software_version[%s] ret[0x%x]",
			hcamcorder->software_version, ret);
	} else {
		_mmcam_dbg_err("model_name is NULL");
	}

	return ed;
}


static ExifData *__mmcamcorder_update_exif_model(MMHandleType handle, ExifData *ed)
{
	int ret = MM_ERROR_NONE;
	mmf_camcorder_t *hcamcorder = (mmf_camcorder_t *)handle;

	if (hcamcorder == NULL || ed == NULL) {
		_mmcam_dbg_err("NULL parameter %p,%p", hcamcorder, ed);
		return NULL;
	}

	if (hcamcorder->model_name) {
		ret = mm_exif_set_add_entry(ed, EXIF_IFD_0, EXIF_TAG_MODEL, EXIF_FORMAT_ASCII,
			strlen(hcamcorder->model_name)+1, (const char *)hcamcorder->model_name);

		_mmcam_dbg_err("set model name[%s] ret[0x%x]",
			hcamcorder->model_name, ret);
	} else {
		_mmcam_dbg_err("model_name is NULL");
	}

	return ed;
}

static ExifData *__mmcamcorder_update_exif_gps(MMHandleType handle, ExifData *ed)
{
	int ret = MM_ERROR_NONE;
	int gps_enable = TRUE;
	char *err_name = NULL;
	double latitude = INVALID_GPS_VALUE;
	double longitude = INVALID_GPS_VALUE;
	double altitude = INVALID_GPS_VALUE;

	ret = mm_camcorder_get_attributes(handle, NULL, MMCAM_TAG_GPS_ENABLE, &gps_enable, NULL);
	if (ret == MM_ERROR_NONE && gps_enable) {
		ExifByte GpsVersion[4] = {2, 2, 0, 0};

		_mmcam_dbg_log("Tag for GPS is ENABLED.");

		ret = mm_exif_set_add_entry(ed, EXIF_IFD_GPS, EXIF_TAG_GPS_VERSION_ID,
			EXIF_FORMAT_BYTE, 4, (const char *)&GpsVersion);
		if (ret != MM_ERROR_NONE)
			EXIF_SET_ERR(ret, EXIF_TAG_GPS_VERSION_ID);

		ret = mm_camcorder_get_attributes(handle, &err_name,
			MMCAM_TAG_LATITUDE, &latitude,
			MMCAM_TAG_LONGITUDE, &longitude,
			MMCAM_TAG_ALTITUDE, &altitude, NULL);
		if (ret != MM_ERROR_NONE) {
			_mmcam_dbg_err("failed to get gps info [%x][%s]", ret, err_name);
			SAFE_FREE(err_name);
			goto exit;
		}

		_mmcam_dbg_log("latitude [%f]", latitude);
		if (latitude != INVALID_GPS_VALUE) {
			unsigned char *b = NULL;
			unsigned int deg;
			unsigned int min;
			unsigned int sec;
			ExifRational rData;

			if (latitude < 0) {
				ret = mm_exif_set_add_entry(ed, EXIF_IFD_GPS, EXIF_TAG_GPS_LATITUDE_REF,
					EXIF_FORMAT_ASCII, 2, "S");
				if (ret != MM_ERROR_NONE)
					EXIF_SET_ERR(ret, EXIF_TAG_GPS_LATITUDE_REF);

				latitude = -latitude;
			} else if (latitude > 0) {
				ret = mm_exif_set_add_entry(ed, EXIF_IFD_GPS, EXIF_TAG_GPS_LATITUDE_REF,
					EXIF_FORMAT_ASCII, 2, "N");
				if (ret != MM_ERROR_NONE)
					EXIF_SET_ERR(ret, EXIF_TAG_GPS_LATITUDE_REF);
			}

			deg = (unsigned int)(latitude);
			min = (unsigned int)((latitude-deg) * 60);
			sec = (unsigned int)(((latitude-deg) * 3600) - min * 60);

			_mmcam_dbg_log("f_latitude deg[%d], min[%d], sec[%d]", deg, min, sec);
			b = malloc(3 * sizeof(ExifRational));
			if (b) {
				rData.numerator = deg;
				rData.denominator = 1;
				exif_set_rational(b, exif_data_get_byte_order(ed), rData);
				rData.numerator = min;
				exif_set_rational(b + 8, exif_data_get_byte_order(ed), rData);
				rData.numerator = sec;
				exif_set_rational(b + 16, exif_data_get_byte_order(ed), rData);

				ret = mm_exif_set_add_entry(ed, EXIF_IFD_GPS, EXIF_TAG_GPS_LATITUDE,
					EXIF_FORMAT_RATIONAL, 3, (const char *)b);
				free(b);
				if (ret != MM_ERROR_NONE)
					EXIF_SET_ERR(ret, EXIF_TAG_GPS_LATITUDE);
			} else {
				_mmcam_dbg_warn("malloc failed");
			}
		}

		_mmcam_dbg_log("longitude [%f]", longitude);
		if (longitude != INVALID_GPS_VALUE) {
			unsigned char *b = NULL;
			unsigned int deg;
			unsigned int min;
			unsigned int sec;
			ExifRational rData;

			if (longitude < 0) {
				ret = mm_exif_set_add_entry(ed, EXIF_IFD_GPS, EXIF_TAG_GPS_LONGITUDE_REF,
					EXIF_FORMAT_ASCII, 2, "W");
				if (ret != MM_ERROR_NONE)
					EXIF_SET_ERR(ret, EXIF_TAG_GPS_LONGITUDE_REF);

				longitude = -longitude;
			} else if (longitude > 0) {
				ret = mm_exif_set_add_entry(ed, EXIF_IFD_GPS, EXIF_TAG_GPS_LONGITUDE_REF,
					EXIF_FORMAT_ASCII, 2, "E");
				if (ret != MM_ERROR_NONE)
					EXIF_SET_ERR(ret, EXIF_TAG_GPS_LONGITUDE_REF);
			}

			deg = (unsigned int)(longitude);
			min = (unsigned int)((longitude-deg) * 60);
			sec = (unsigned int)(((longitude-deg) * 3600) - min * 60);

			_mmcam_dbg_log("longitude deg[%d], min[%d], sec[%d]", deg, min, sec);
			b = malloc(3 * sizeof(ExifRational));
			if (b) {
				rData.numerator = deg;
				rData.denominator = 1;
				exif_set_rational(b, exif_data_get_byte_order(ed), rData);
				rData.numerator = min;
				exif_set_rational(b+8, exif_data_get_byte_order(ed), rData);
				rData.numerator = sec;
				exif_set_rational(b+16, exif_data_get_byte_order(ed), rData);
				ret = mm_exif_set_add_entry(ed, EXIF_IFD_GPS, EXIF_TAG_GPS_LONGITUDE,
					EXIF_FORMAT_RATIONAL, 3, (const char *)b);
				free(b);
				if (ret != MM_ERROR_NONE)
					EXIF_SET_ERR(ret, EXIF_TAG_GPS_LONGITUDE);
			} else {
				_mmcam_dbg_warn("malloc failed");
			}
		}

		_mmcam_dbg_log("f_altitude [%f]", altitude);
		if (altitude != INVALID_GPS_VALUE) {
			ExifByte alt_ref = 0;
			unsigned char *b = NULL;
			ExifRational rData;
			b = malloc(sizeof(ExifRational));
			if (b) {
				if (altitude < 0) {
					alt_ref = 1;
					altitude = -altitude;
				}

				ret = mm_exif_set_add_entry(ed, EXIF_IFD_GPS, EXIF_TAG_GPS_ALTITUDE_REF,
					EXIF_FORMAT_BYTE, 1, (const char *)&alt_ref);
				if (ret != MM_ERROR_NONE) {
					_mmcam_dbg_err("error [%x], tag [%x]", ret, EXIF_TAG_GPS_ALTITUDE_REF);
					if (ret == (int)MM_ERROR_CAMCORDER_LOW_MEMORY) {
						free(b);
						b = NULL;
						goto exit;
					}
				}

				rData.numerator = (unsigned int)(altitude + 0.5)*100;
				rData.denominator = 100;
				exif_set_rational(b, exif_data_get_byte_order(ed), rData);
				ret = mm_exif_set_add_entry(ed, EXIF_IFD_GPS, EXIF_TAG_GPS_ALTITUDE,
					EXIF_FORMAT_RATIONAL, 1, (const char *)b);
				free(b);
				if (ret != MM_ERROR_NONE)
					EXIF_SET_ERR(ret, EXIF_TAG_GPS_ALTITUDE);
			} else {
				_mmcam_dbg_warn("malloc failed");
			}
		}

		{
			double gps_timestamp = INVALID_GPS_VALUE;
			mm_camcorder_get_attributes(handle, NULL, "tag-gps-time-stamp", &gps_timestamp, NULL);
			_mmcam_dbg_log("Gps timestamp [%f]", gps_timestamp);
			if (gps_timestamp > 0.0) {
				unsigned char *b = NULL;
				unsigned int hour;
				unsigned int min;
				unsigned int microsec;
				ExifRational rData;

				hour = (unsigned int)(gps_timestamp / 3600);
				min = (unsigned int)((gps_timestamp - 3600 * hour) / 60);
				microsec = (unsigned int)(((double)((double)gps_timestamp -(double)(3600 * hour)) -(double)(60 * min)) * 1000000);

				_mmcam_dbg_log("Gps timestamp hour[%d], min[%d], microsec[%d]", hour, min, microsec);
				b = malloc(3 * sizeof(ExifRational));
				if (b) {
					rData.numerator = hour;
					rData.denominator = 1;
					exif_set_rational(b, exif_data_get_byte_order(ed), rData);

					rData.numerator = min;
					rData.denominator = 1;
					exif_set_rational(b + 8, exif_data_get_byte_order(ed), rData);

					rData.numerator = microsec;
					rData.denominator = 1000000;
					exif_set_rational(b + 16, exif_data_get_byte_order(ed), rData);

					ret = mm_exif_set_add_entry(ed, EXIF_IFD_GPS, EXIF_TAG_GPS_TIME_STAMP,
						EXIF_FORMAT_RATIONAL, 3, (const char *)b);
					free(b);
					if (ret != MM_ERROR_NONE)
						EXIF_SET_ERR(ret, EXIF_TAG_GPS_TIME_STAMP);
				} else {
					_mmcam_dbg_warn("malloc failed.");
				}
			}
		}

		{
			unsigned char *date_stamp = NULL;
			int date_stamp_len = 0;

			mm_camcorder_get_attributes(handle, NULL, "tag-gps-date-stamp", &date_stamp, &date_stamp_len, NULL);

			if (date_stamp) {
				_mmcam_dbg_log("Date stamp [%s]", date_stamp);

				/* cause it should include NULL char */
				ret = mm_exif_set_add_entry(ed, EXIF_IFD_GPS, EXIF_TAG_GPS_DATE_STAMP,
					EXIF_FORMAT_ASCII, date_stamp_len + 1, (const char *)date_stamp);
				if (ret != MM_ERROR_NONE)
					EXIF_SET_ERR(ret, EXIF_TAG_GPS_DATE_STAMP);
			}
		}

		{
			unsigned char *processing_method = NULL;
			int processing_method_len = 0;

			mm_camcorder_get_attributes(handle, NULL, "tag-gps-processing-method", &processing_method, &processing_method_len, NULL);

			if (processing_method) {
				_mmcam_dbg_log("Processing method [%s]", processing_method);

				ret = mm_exif_set_add_entry(ed, EXIF_IFD_GPS, EXIF_TAG_GPS_PROCESSING_METHOD,
					EXIF_FORMAT_UNDEFINED, processing_method_len, (const char *)processing_method);
				if (ret != MM_ERROR_NONE)
					EXIF_SET_ERR(ret, EXIF_TAG_GPS_PROCESSING_METHOD);
			}
		}
	} else {
		_mmcam_dbg_log("Tag for GPS is DISABLED.");
	}

exit:
	return ed;
}


int __mmcamcorder_update_exif_info(MMHandleType handle, void* imagedata, int imgln)
{
	int ret = MM_ERROR_NONE;
	mmf_camcorder_t *hcamcorder = NULL;
	ExifData *ed = NULL;

	hcamcorder = MMF_CAMCORDER(handle);

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	ed = exif_data_new_from_data(imagedata, imgln);
	/*ed = mm_exif_get_exif_from_info(hcamcorder->exif_info);*/

	if (ed == NULL) {
		_mmcam_dbg_err("get exif data error!!");
		return MM_ERROR_INVALID_HANDLE;
	}

	/* update necessary */

	__mmcamcorder_update_exif_make(handle, ed);
	__mmcamcorder_update_exif_software(handle, ed);
	__mmcamcorder_update_exif_model(handle, ed);
	__mmcamcorder_update_exif_orientation(handle, ed);
	__mmcamcorder_update_exif_gps(handle, ed);
	ret = mm_exif_set_exif_to_info(hcamcorder->exif_info, ed);
	if (ret != MM_ERROR_NONE)
		_mmcam_dbg_err("mm_exif_set_exif_to_info err!! [%x]", ret);

	exif_data_unref(ed);
	ed = NULL;
	return ret;
}

int __mmcamcorder_set_exif_basic_info(MMHandleType handle, int image_width, int image_height)
{
	int ret = MM_ERROR_NONE;
	int value;
	int str_val_len = 0;
	int cntl = 0;
	int cnts = 0;
	char *str_value = NULL;
	char *user_comment = NULL;
	ExifData *ed = NULL;
	ExifLong config;
	ExifLong ExifVersion;
	static ExifShort eshort[20];
	static ExifLong elong[10];

	GstCameraControl *control = NULL;
	GstCameraControlExifInfo avsys_exif_info;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	_mmcam_dbg_log("");

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc && sc->element, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	CLEAR(avsys_exif_info);

	if (!GST_IS_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst)) {
		_mmcam_dbg_err("Can't cast Video source into camera control. Skip camera control values...");
	} else {
		control = GST_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst);
		/* get device information */
		gst_camera_control_get_exif_info(control, &avsys_exif_info);
	}

	/* get ExifData from exif info */
	ed = mm_exif_get_exif_from_info(hcamcorder->exif_info);
	if (ed == NULL) {
		_mmcam_dbg_err("get exif data error!!");
		return MM_ERROR_INVALID_HANDLE;
	}

	/* Receive attribute info */

	/* START INSERT IFD_0 */

	/*0. EXIF_TAG_EXIF_VERSION */
	ExifVersion = MM_EXIF_VERSION;
	ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_EXIF_VERSION,
		EXIF_FORMAT_UNDEFINED, 4, (const char *)&ExifVersion);
	if (ret != MM_ERROR_NONE)
		EXIF_SET_ERR(ret, EXIF_TAG_EXIF_VERSION);

	/*1. EXIF_TAG_IMAGE_WIDTH */ /*EXIF_TAG_PIXEL_X_DIMENSION*/
	value = image_width;

	exif_set_long((unsigned char *)&elong[cntl], exif_data_get_byte_order(ed), value);
	ret = mm_exif_set_add_entry(ed, EXIF_IFD_0, EXIF_TAG_IMAGE_WIDTH,
		EXIF_FORMAT_LONG, 1, (const char *)&elong[cntl]);
	if (ret != MM_ERROR_NONE)
		EXIF_SET_ERR(ret, EXIF_TAG_IMAGE_WIDTH);

	ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_PIXEL_X_DIMENSION,
		EXIF_FORMAT_LONG, 1, (const char *)&elong[cntl++]);
	if (ret != MM_ERROR_NONE)
		EXIF_SET_ERR(ret, EXIF_TAG_PIXEL_X_DIMENSION);

	_mmcam_dbg_log("width[%d]", value);

	/*2. EXIF_TAG_IMAGE_LENGTH*/ /*EXIF_TAG_PIXEL_Y_DIMENSION*/
	value = image_height;

	exif_set_long((unsigned char *)&elong[cntl], exif_data_get_byte_order(ed), value);
	ret = mm_exif_set_add_entry(ed, EXIF_IFD_0, EXIF_TAG_IMAGE_LENGTH,
		EXIF_FORMAT_LONG, 1, (const char *)&elong[cntl]);
	if (ret != MM_ERROR_NONE)
		EXIF_SET_ERR(ret, EXIF_TAG_IMAGE_LENGTH);

	ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_PIXEL_Y_DIMENSION,
		EXIF_FORMAT_LONG, 1, (const char *)&elong[cntl++]);
	if (ret != MM_ERROR_NONE)
		EXIF_SET_ERR(ret, EXIF_TAG_PIXEL_Y_DIMENSION);

	_mmcam_dbg_log("height[%d]", value);

	/*4. EXIF_TAG_DATE_TIME */

	/*12. EXIF_TAG_DATE_TIME_ORIGINAL */

	/*13. EXIF_TAG_DATE_TIME_DIGITIZED*/
	{
		unsigned char *b;
		time_t t;
		struct tm tm;

		b = malloc(20 * sizeof(unsigned char));
		if (b == NULL) {
			_mmcam_dbg_err("failed to alloc b");
			ret = MM_ERROR_CAMCORDER_LOW_MEMORY;
			EXIF_SET_ERR(ret, EXIF_TAG_DATE_TIME);
		}

		memset(b, '\0', 20);

		t = time(NULL);
		tzset();
		localtime_r(&t, &tm);

		snprintf((char *)b, 20, "%04i:%02i:%02i %02i:%02i:%02i",
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);

		ret = mm_exif_set_add_entry(ed, EXIF_IFD_0, EXIF_TAG_DATE_TIME, EXIF_FORMAT_ASCII, 20, (const char *)b);
		if (ret != MM_ERROR_NONE) {
			if (ret == (int)MM_ERROR_CAMCORDER_LOW_MEMORY)
				free(b);

			EXIF_SET_ERR(ret, EXIF_TAG_DATE_TIME);
		}

		ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_DATE_TIME_ORIGINAL, EXIF_FORMAT_ASCII, 20, (const char *)b);
		if (ret != MM_ERROR_NONE) {
			if (ret == (int)MM_ERROR_CAMCORDER_LOW_MEMORY)
				free(b);

			EXIF_SET_ERR(ret, EXIF_TAG_DATE_TIME_ORIGINAL);
		}

		ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_DATE_TIME_DIGITIZED, EXIF_FORMAT_ASCII, 20, (const char *)b);
		if (ret != MM_ERROR_NONE) {
			if (ret == (int)MM_ERROR_CAMCORDER_LOW_MEMORY)
				free(b);

			EXIF_SET_ERR(ret, EXIF_TAG_DATE_TIME_DIGITIZED);
		}

		free(b);
	}

	/*5. EXIF_TAG_MAKE */
	__mmcamcorder_update_exif_make(handle, ed);

#ifdef WRITE_EXIF_MAKER_INFO /* FIXME */

	/*6. EXIF_TAG_MODEL */
	__mmcamcorder_update_exif_model(handle, ed);

#endif
	/*6. EXIF_TAG_IMAGE_DESCRIPTION */
	mm_camcorder_get_attributes(handle, NULL, MMCAM_TAG_IMAGE_DESCRIPTION, &str_value, &str_val_len, NULL);
	if (str_value && str_val_len > 0) {
		char *description = strdup(str_value);

		_mmcam_dbg_log("desctiption [%s]", str_value);

		if (description) {
			ret = mm_exif_set_add_entry(ed, EXIF_IFD_0, EXIF_TAG_IMAGE_DESCRIPTION,
				EXIF_FORMAT_ASCII, strlen(description), (const char *)description);
			free(description);
			str_value = NULL;
			str_val_len = 0;
			if (ret != MM_ERROR_NONE)
				EXIF_SET_ERR(ret, EXIF_TAG_IMAGE_DESCRIPTION);
		} else {
			_mmcam_dbg_err("strdup failed for [%s]", str_value);
		}
	} else {
		_mmcam_dbg_warn("failed to get description");
	}

	/*7. EXIF_TAG_SOFTWARE*/
	__mmcamcorder_update_exif_software(handle, ed);

/*
	if (control != NULL) {
		char software[50] = {0,};
		unsigned int len = 0;

		len = snprintf(software, sizeof(software), "%x.%x ", avsys_exif_info.software_used>>8, (avsys_exif_info.software_used & 0xff));
		_mmcam_dbg_log("software [%s], len [%d]", software, len);
		ret = mm_exif_set_add_entry(ed, EXIF_IFD_0, EXIF_TAG_SOFTWARE,
			EXIF_FORMAT_ASCII, len, (const char *)software);
		if (ret != MM_ERROR_NONE) {
			EXIF_SET_ERR(ret, EXIF_TAG_SOFTWARE);
		}
	}
*/

	__mmcamcorder_update_exif_orientation(handle, ed);

	/* START INSERT EXIF_IFD */

	/*3. User Comment*/
	/*FIXME : get user comment from real user */
	user_comment = strdup(MM_USER_COMMENT);
	if (user_comment) {
		_mmcam_dbg_log("user_comment=%s", user_comment);
		ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_USER_COMMENT,
			EXIF_FORMAT_ASCII, strlen(user_comment), (const char *)user_comment);
		free(user_comment);
		if (ret != MM_ERROR_NONE)
			EXIF_SET_ERR(ret, EXIF_TAG_USER_COMMENT);
	} else {
		ret = MM_ERROR_CAMCORDER_LOW_MEMORY;
		EXIF_SET_ERR(ret, EXIF_TAG_USER_COMMENT);
	}

	/*9. EXIF_TAG_COLOR_SPACE */
	if (control != NULL) {
		exif_set_short((unsigned char *)&eshort[cnts], exif_data_get_byte_order(ed), avsys_exif_info.colorspace);
		ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_COLOR_SPACE,
			EXIF_FORMAT_SHORT, 1, (const char *)&eshort[cnts++]);
		if (ret != MM_ERROR_NONE)
			EXIF_SET_ERR(ret, EXIF_TAG_COLOR_SPACE);
	}

	/*10. EXIF_TAG_COMPONENTS_CONFIGURATION */
	if (control != NULL) {
		config = avsys_exif_info.component_configuration;
		_mmcam_dbg_log("EXIF_TAG_COMPONENTS_CONFIGURATION [%4x] ", config);
		ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_COMPONENTS_CONFIGURATION,
			EXIF_FORMAT_UNDEFINED, 4, (const char *)&config);
		if (ret != MM_ERROR_NONE)
			EXIF_SET_ERR(ret, EXIF_TAG_COMPONENTS_CONFIGURATION);
	}

	/*11. EXIF_TAG_COMPRESSED_BITS_PER_PIXEL */
	/* written  the device_info */

	/*12. EXIF_TAG_DATE_TIME_ORIGINAL */
	/*13. EXIF_TAG_DATE_TIME_DIGITIZED*/

	/*14. EXIF_TAG_EXPOSURE_TIME*/
	if (avsys_exif_info.exposure_time_numerator && avsys_exif_info.exposure_time_denominator) {
		unsigned char *b = NULL;
		ExifRational rData;

		_mmcam_dbg_log("EXIF_TAG_EXPOSURE_TIME numerator [%d], denominator [%d]",
			avsys_exif_info.exposure_time_numerator, avsys_exif_info.exposure_time_denominator);

		b = malloc(sizeof(ExifRational));
		if (b) {
			rData.numerator = avsys_exif_info.exposure_time_numerator;
			rData.denominator = avsys_exif_info.exposure_time_denominator;

			exif_set_rational(b, exif_data_get_byte_order(ed), rData);
			ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_TIME,
				EXIF_FORMAT_RATIONAL, 1, (const char *)b);
			free(b);
			if (ret != MM_ERROR_NONE)
				EXIF_SET_ERR(ret, EXIF_TAG_EXPOSURE_TIME);
		} else {
			_mmcam_dbg_warn("malloc failed.");
		}
	} else {
		_mmcam_dbg_log("Skip set EXIF_TAG_EXPOSURE_TIME numerator [%d], denominator [%d]",
			avsys_exif_info.exposure_time_numerator, avsys_exif_info.exposure_time_denominator);
	}

	/*15. EXIF_TAG_FNUMBER */
	if (avsys_exif_info.aperture_f_num_numerator && avsys_exif_info.aperture_f_num_denominator) {
		unsigned char *b = NULL;
		ExifRational rData;

		_mmcam_dbg_log("EXIF_TAG_FNUMBER numerator [%d], denominator [%d]",
			avsys_exif_info.aperture_f_num_numerator, avsys_exif_info.aperture_f_num_denominator);

		b = malloc(sizeof(ExifRational));
		if (b) {
			rData.numerator = avsys_exif_info.aperture_f_num_numerator;
			rData.denominator = avsys_exif_info.aperture_f_num_denominator;
			exif_set_rational(b, exif_data_get_byte_order(ed), rData);
			ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_FNUMBER,
				EXIF_FORMAT_RATIONAL, 1, (const char *)b);
			free(b);
			if (ret != MM_ERROR_NONE)
				EXIF_SET_ERR(ret, EXIF_TAG_FNUMBER);
		} else {
			_mmcam_dbg_warn("malloc failed.");
		}
	} else {
		_mmcam_dbg_log("Skip set EXIF_TAG_FNUMBER numerator [%d], denominator [%d]",
			avsys_exif_info.aperture_f_num_numerator, avsys_exif_info.aperture_f_num_denominator);
	}

	/*16. EXIF_TAG_EXPOSURE_PROGRAM*/
	/*FIXME*/
	value = MM_EXPOSURE_PROGRAM;
	exif_set_short((unsigned char *)&eshort[cnts], exif_data_get_byte_order(ed), value);
	ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_PROGRAM,
		EXIF_FORMAT_SHORT, 1, (const char *)&eshort[cnts++]);
	if (ret != MM_ERROR_NONE)
		EXIF_SET_ERR(ret, EXIF_TAG_EXPOSURE_PROGRAM);

	/*17. EXIF_TAG_ISO_SPEED_RATINGS*/
	if (avsys_exif_info.iso) {
		_mmcam_dbg_log("EXIF_TAG_ISO_SPEED_RATINGS [%d]", avsys_exif_info.iso);
		exif_set_short((unsigned char *)&eshort[cnts], exif_data_get_byte_order(ed), avsys_exif_info.iso);
		ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_ISO_SPEED_RATINGS,
			EXIF_FORMAT_SHORT, 1, (const char *)&eshort[cnts++]);
		if (ret != MM_ERROR_NONE)
			EXIF_SET_ERR(ret, EXIF_TAG_ISO_SPEED_RATINGS);
	}

	/*18. EXIF_TAG_SHUTTER_SPEED_VALUE*/
	if (avsys_exif_info.shutter_speed_numerator && avsys_exif_info.shutter_speed_denominator) {
		unsigned char *b = NULL;
		ExifSRational rsData;

		_mmcam_dbg_log("EXIF_TAG_SHUTTER_SPEED_VALUE numerator [%d], denominator [%d]",
			avsys_exif_info.shutter_speed_numerator, avsys_exif_info.shutter_speed_denominator);

		b = malloc(sizeof(ExifSRational));
		if (b) {
			rsData.numerator = avsys_exif_info.shutter_speed_numerator;
			rsData.denominator = avsys_exif_info.shutter_speed_denominator;
			exif_set_srational(b, exif_data_get_byte_order(ed), rsData);
			ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_SHUTTER_SPEED_VALUE,
				EXIF_FORMAT_SRATIONAL, 1, (const char *)b);
			free(b);
			if (ret != MM_ERROR_NONE)
				EXIF_SET_ERR(ret, EXIF_TAG_SHUTTER_SPEED_VALUE);
		} else {
			_mmcam_dbg_warn("malloc failed.");
		}
	} else {
		_mmcam_dbg_log("Skip set EXIF_TAG_SHUTTER_SPEED_VALUE numerator [%d], denominator [%d]",
			avsys_exif_info.shutter_speed_numerator, avsys_exif_info.shutter_speed_denominator);
	}

	/*19. EXIF_TAG_APERTURE_VALUE*/
	if (avsys_exif_info.aperture_in_APEX) {
		unsigned char *b = NULL;
		ExifRational rData;

		_mmcam_dbg_log("EXIF_TAG_APERTURE_VALUE [%d]", avsys_exif_info.aperture_in_APEX);

		b = malloc(sizeof(ExifRational));
		if (b) {
			rData.numerator = avsys_exif_info.aperture_in_APEX;
			rData.denominator = 1;
			exif_set_rational(b, exif_data_get_byte_order(ed), rData);
			ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_APERTURE_VALUE,
				EXIF_FORMAT_RATIONAL, 1, (const char *)b);
			free(b);
			if (ret != MM_ERROR_NONE)
				EXIF_SET_ERR(ret, EXIF_TAG_APERTURE_VALUE);
		} else {
			_mmcam_dbg_warn("malloc failed.");
		}
	} else {
		_mmcam_dbg_log("Skip set EXIF_TAG_APERTURE_VALUE [%d]", avsys_exif_info.aperture_in_APEX);
	}

	/*20. EXIF_TAG_BRIGHTNESS_VALUE*/
	if (avsys_exif_info.brigtness_numerator && avsys_exif_info.brightness_denominator) {
		unsigned char *b = NULL;
		ExifSRational rsData;

		_mmcam_dbg_log("EXIF_TAG_BRIGHTNESS_VALUE numerator [%d], denominator [%d]",
			avsys_exif_info.brigtness_numerator, avsys_exif_info.brightness_denominator);

		b = malloc(sizeof(ExifSRational));
		if (b) {
			rsData.numerator = avsys_exif_info.brigtness_numerator;
			rsData.denominator = avsys_exif_info.brightness_denominator;
			exif_set_srational(b, exif_data_get_byte_order(ed), rsData);
			ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_BRIGHTNESS_VALUE,
				EXIF_FORMAT_SRATIONAL, 1, (const char *)b);
			free(b);
			if (ret != MM_ERROR_NONE)
				EXIF_SET_ERR(ret, EXIF_TAG_BRIGHTNESS_VALUE);
		} else {
			_mmcam_dbg_warn("malloc failed.");
		}
	} else {
		_mmcam_dbg_log("Skip set EXIF_TAG_BRIGHTNESS_VALUE numerator [%d], denominatorr [%d]",
			avsys_exif_info.brigtness_numerator, avsys_exif_info.brightness_denominator);
	}

	/*21. EXIF_TAG_EXPOSURE_BIAS_VALUE*/
	value = 0;
	ret = mm_camcorder_get_attributes(handle, NULL, MMCAM_FILTER_BRIGHTNESS, &value, NULL);
	if (ret == MM_ERROR_NONE) {
		unsigned char *b = NULL;
		ExifSRational rsData;

		_mmcam_dbg_log("EXIF_TAG_BRIGHTNESS_VALUE %d, default %d, step denominator %d",
			value, hcamcorder->brightness_default, hcamcorder->brightness_step_denominator);

		b = malloc(sizeof(ExifSRational));
		if (b) {
			rsData.numerator = value - hcamcorder->brightness_default;
			if (hcamcorder->brightness_step_denominator != 0) {
				rsData.denominator = hcamcorder->brightness_step_denominator;
			} else {
				_mmcam_dbg_warn("brightness_step_denominator is ZERO, so set 1");
				rsData.denominator = 1;
			}
			exif_set_srational(b, exif_data_get_byte_order(ed), rsData);
			ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_BIAS_VALUE,
				EXIF_FORMAT_SRATIONAL, 1, (const char *)b);
			free(b);
			if (ret != MM_ERROR_NONE)
				EXIF_SET_ERR(ret, EXIF_TAG_EXPOSURE_BIAS_VALUE);
		} else {
			_mmcam_dbg_warn("malloc failed.");
		}
	} else {
		_mmcam_dbg_log("failed to get MMCAM_FILTER_BRIGHTNESS [%x]", ret);
	}

	/*22  EXIF_TAG_MAX_APERTURE_VALUE*/
/*
	if (avsys_exif_info.max_lens_aperture_in_APEX) {
		unsigned char *b = NULL;
		ExifRational rData;

		_mmcam_dbg_log("EXIF_TAG_MAX_APERTURE_VALUE [%d]", avsys_exif_info.max_lens_aperture_in_APEX);

		b = malloc(sizeof(ExifRational));
		if (b) {
			rData.numerator = avsys_exif_info.max_lens_aperture_in_APEX;
			rData.denominator = 1;
			exif_set_rational(b, exif_data_get_byte_order(ed), rData);
			ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_MAX_APERTURE_VALUE,
				EXIF_FORMAT_RATIONAL, 1, b);
			free(b);
			if (ret != MM_ERROR_NONE)
				EXIF_SET_ERR(ret, EXIF_TAG_MAX_APERTURE_VALUE);
		} else {
			_mmcam_dbg_warn("failed to alloc for MAX aperture value");
		}
	}
*/

	/*23. EXIF_TAG_SUBJECT_DISTANCE*/
	/* defualt : none */

	/*24. EXIF_TAG_METERING_MODE */
	if (control != NULL) {
		exif_set_short((unsigned char *)&eshort[cnts], exif_data_get_byte_order(ed), avsys_exif_info.metering_mode);
		_mmcam_dbg_log("EXIF_TAG_METERING_MODE [%d]", avsys_exif_info.metering_mode);
		ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_METERING_MODE,
			EXIF_FORMAT_SHORT, 1, (const char *)&eshort[cnts++]);
		if (ret != MM_ERROR_NONE)
			EXIF_SET_ERR(ret, EXIF_TAG_METERING_MODE);
	}

	/*25. EXIF_TAG_LIGHT_SOURCE*/

	/*26. EXIF_TAG_FLASH*/
	if (control != NULL) {
		exif_set_short((unsigned char *)&eshort[cnts], exif_data_get_byte_order(ed), avsys_exif_info.flash);
		_mmcam_dbg_log("EXIF_TAG_FLASH [%d]", avsys_exif_info.flash);
		ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_FLASH,
			EXIF_FORMAT_SHORT, 1, (const char *)&eshort[cnts++]);
		if (ret != MM_ERROR_NONE)
			EXIF_SET_ERR(ret, EXIF_TAG_FLASH);
	}

	/*27. EXIF_TAG_FOCAL_LENGTH*/
	if (avsys_exif_info.focal_len_numerator && avsys_exif_info.focal_len_denominator) {
		unsigned char *b = NULL;
		ExifRational rData;

		_mmcam_dbg_log("EXIF_TAG_FOCAL_LENGTH numerator [%d], denominator [%d]",
			avsys_exif_info.focal_len_numerator, avsys_exif_info.focal_len_denominator);

		b = malloc(sizeof(ExifRational));
		if (b) {
			rData.numerator = avsys_exif_info.focal_len_numerator;
			rData.denominator = avsys_exif_info.focal_len_denominator;
			exif_set_rational(b, exif_data_get_byte_order(ed), rData);
			ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_FOCAL_LENGTH,
				EXIF_FORMAT_RATIONAL, 1, (const char *)b);
			free(b);
			if (ret != MM_ERROR_NONE)
				EXIF_SET_ERR(ret, EXIF_TAG_FOCAL_LENGTH);
		} else {
			_mmcam_dbg_warn("malloc failed.");
		}
	} else {
		_mmcam_dbg_log("Skip set EXIF_TAG_FOCAL_LENGTH numerator [%d], denominator [%d]",
			avsys_exif_info.focal_len_numerator, avsys_exif_info.focal_len_denominator);
	}

	/*28. EXIF_TAG_SENSING_METHOD*/
	/*FIXME*/
	value = MM_SENSING_MODE;
	exif_set_short((unsigned char *)&eshort[cnts], exif_data_get_byte_order(ed), value);
	ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_SENSING_METHOD,
		EXIF_FORMAT_SHORT, 1, (const char *)&eshort[cnts++]);
	if (ret != MM_ERROR_NONE)
		EXIF_SET_ERR(ret, EXIF_TAG_SENSING_METHOD);

	/*29. EXIF_TAG_FILE_SOURCE*/
/*
	value = MM_FILE_SOURCE;
	exif_set_long(&elong[cntl], exif_data_get_byte_order(ed),value);
	ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_FILE_SOURCE,
		EXIF_FORMAT_UNDEFINED, 4, (const char *)&elong[cntl++]);
	if (ret != MM_ERROR_NONE)
		EXIF_SET_ERR(ret, EXIF_TAG_FILE_SOURCE);
*/

	/*30. EXIF_TAG_SCENE_TYPE*/
/*
	value = MM_SCENE_TYPE;
	exif_set_long(&elong[cntl], exif_data_get_byte_order(ed),value);
	ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_SCENE_TYPE,
		EXIF_FORMAT_UNDEFINED, 4, (const char *)&elong[cntl++]);
	if (ret != MM_ERROR_NONE)
		EXIF_SET_ERR(ret, EXIF_TAG_SCENE_TYPE);
*/

	/*31. EXIF_TAG_EXPOSURE_MODE*/
	/*FIXME*/
	value = MM_EXPOSURE_MODE;
	exif_set_short((unsigned char *)&eshort[cnts], exif_data_get_byte_order(ed), value);
	ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_EXPOSURE_MODE,
		EXIF_FORMAT_SHORT, 1, (const char *)&eshort[cnts++]);
	if (ret != MM_ERROR_NONE)
		EXIF_SET_ERR(ret, EXIF_TAG_EXPOSURE_MODE);

	/*32. EXIF_TAG_WHITE_BALANCE*/
	ret = mm_camcorder_get_attributes(handle, NULL, MMCAM_FILTER_WB, &value, NULL);
	if (ret == MM_ERROR_NONE) {
		int set_value = 0;
		_mmcam_dbg_log("WHITE BALANCE [%d]", value);

		if (value == MM_CAMCORDER_WHITE_BALANCE_AUTOMATIC)
			set_value = 0;
		else
			set_value = 1;

		exif_set_short((unsigned char *)&eshort[cnts], exif_data_get_byte_order(ed), set_value);
		ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_WHITE_BALANCE,
			EXIF_FORMAT_SHORT, 1, (const char *)&eshort[cnts++]);
		if (ret != MM_ERROR_NONE)
			EXIF_SET_ERR(ret, EXIF_TAG_WHITE_BALANCE);
	} else {
		_mmcam_dbg_warn("failed to get white balance [%x]", ret);
	}

	/*33. EXIF_TAG_DIGITAL_ZOOM_RATIO*/
/*
	ret = mm_camcorder_get_attributes(handle, NULL, MMCAM_CAMERA_DIGITAL_ZOOM, &value, NULL);
	if (ret == MM_ERROR_NONE) {
		_mmcam_dbg_log("DIGITAL ZOOM [%d]", value);

		exif_set_long(&elong[cntl], exif_data_get_byte_order(ed), value);
		ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_DIGITAL_ZOOM_RATIO,
			EXIF_FORMAT_LONG, 1, (const char *)&elong[cntl++]);
		if (ret != MM_ERROR_NONE)
			EXIF_SET_ERR(ret, EXIF_TAG_DIGITAL_ZOOM_RATIO);
	} else {
		_mmcam_dbg_warn("failed to get digital zoom [%x]", ret);
	}
*/

	/*34. EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM*/
	/*FIXME*/
/*
	value = MM_FOCAL_LENGTH_35MMFILM;
	exif_set_short(&eshort[cnts], exif_data_get_byte_order(ed),value);
	ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM,
		EXIF_FORMAT_SHORT, 1, (const char *)&eshort[cnts++]);
	if (ret != MM_ERROR_NONE)
		EXIF_SET_ERR(ret, EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM);
*/

	/*35. EXIF_TAG_SCENE_CAPTURE_TYPE*/
	{
		int scene_capture_type;

		ret = mm_camcorder_get_attributes(handle, NULL, MMCAM_FILTER_SCENE_MODE, &value, NULL);
		if (ret == MM_ERROR_NONE) {
			_mmcam_dbg_log("Scene mode(program mode) [%d]", value);

			switch (value) {
			case MM_CAMCORDER_SCENE_MODE_NORMAL:
				scene_capture_type = 0; /* standard */
				break;
			case MM_CAMCORDER_SCENE_MODE_PORTRAIT:
				scene_capture_type = 2; /* portrait */
				break;
			case MM_CAMCORDER_SCENE_MODE_LANDSCAPE:
				scene_capture_type = 1; /* landscape */
				break;
			case MM_CAMCORDER_SCENE_MODE_NIGHT_SCENE:
				scene_capture_type = 3; /* night scene */
				break;
			default:
				scene_capture_type = 4; /* Others */
				break;
			}

			exif_set_short((unsigned char *)&eshort[cnts], exif_data_get_byte_order(ed), scene_capture_type);
			ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_SCENE_CAPTURE_TYPE,
				EXIF_FORMAT_SHORT, 1, (const char *)&eshort[cnts++]);
			if (ret != MM_ERROR_NONE)
				EXIF_SET_ERR(ret, EXIF_TAG_SCENE_CAPTURE_TYPE);
		} else {
			_mmcam_dbg_warn("failed to get scene mode [%x]", ret);
		}
	}

	/*36. EXIF_TAG_GAIN_CONTROL*/
	/*FIXME*/
/*
	value = MM_GAIN_CONTROL;
	exif_set_long(&elong[cntl], exif_data_get_byte_order(ed), value);
	ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_GAIN_CONTROL,
		EXIF_FORMAT_LONG, 1, (const char *)&elong[cntl++]);
	if (ret != MM_ERROR_NONE)
		EXIF_SET_ERR(ret, EXIF_TAG_GAIN_CONTROL);
*/


	/*37. EXIF_TAG_CONTRAST */
	{
		type_int_range *irange = NULL;
		int level = 0;

		_mmcamcorder_conf_get_value_int_range(hcamcorder->conf_ctrl,
			CONFIGURE_CATEGORY_CTRL_EFFECT,
			"Contrast",
			&irange);
		if (irange != NULL) {
			mm_camcorder_get_attributes(handle, NULL, MMCAM_FILTER_CONTRAST, &value, NULL);

			_mmcam_dbg_log("CONTRAST currentt [%d], default [%d]", value, irange->default_value);

			if (value == irange->default_value)
				level = MM_VALUE_NORMAL;
			else if (value < irange->default_value)
				level = MM_VALUE_LOW;
			else
				level = MM_VALUE_HARD;

			exif_set_short((unsigned char *)&eshort[cnts], exif_data_get_byte_order(ed), level);
			ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_CONTRAST,
				EXIF_FORMAT_SHORT, 1, (const char *)&eshort[cnts++]);
			if (ret != MM_ERROR_NONE)
				EXIF_SET_ERR(ret, EXIF_TAG_CONTRAST);
		} else {
			_mmcam_dbg_warn("failed to get range of contrast");
		}
	}

	/*38. EXIF_TAG_SATURATION*/
	{
		type_int_range *irange = NULL;
		int level = 0;

		_mmcamcorder_conf_get_value_int_range(hcamcorder->conf_ctrl,
			CONFIGURE_CATEGORY_CTRL_EFFECT,
			"Saturation",
			&irange);
		if (irange != NULL) {
			mm_camcorder_get_attributes(handle, NULL, MMCAM_FILTER_SATURATION, &value, NULL);

			_mmcam_dbg_log("SATURATION current [%d], default [%d]", value, irange->default_value);

			if (value == irange->default_value)
				level = MM_VALUE_NORMAL;
			else if (value < irange->default_value)
				level = MM_VALUE_LOW;
			else
				level = MM_VALUE_HARD;

			exif_set_short((unsigned char *)&eshort[cnts], exif_data_get_byte_order(ed), level);
			ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_SATURATION,
				EXIF_FORMAT_SHORT, 1, (const char *)&eshort[cnts++]);
			if (ret != MM_ERROR_NONE)
				EXIF_SET_ERR(ret, EXIF_TAG_SATURATION);
		} else {
			_mmcam_dbg_warn("failed to get range of saturation");
		}
	}

	/*39. EXIF_TAG_SHARPNESS*/
	{
		type_int_range *irange = NULL;
		int level = 0;

		_mmcamcorder_conf_get_value_int_range(hcamcorder->conf_ctrl,
			CONFIGURE_CATEGORY_CTRL_EFFECT,
			"Sharpness",
			&irange);
		if (irange != NULL) {
			mm_camcorder_get_attributes(handle, NULL, MMCAM_FILTER_SHARPNESS, &value, NULL);

			_mmcam_dbg_log("SHARPNESS current [%d], default [%d]", value, irange->default_value);

			if (value == irange->default_value)
				level = MM_VALUE_NORMAL;
			else if (value < irange->default_value)
				level = MM_VALUE_LOW;
			else
				level = MM_VALUE_HARD;

			exif_set_short((unsigned char *)&eshort[cnts], exif_data_get_byte_order(ed), level);
			ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_SHARPNESS,
				EXIF_FORMAT_SHORT, 1, (const char *)&eshort[cnts++]);
			if (ret != MM_ERROR_NONE)
				EXIF_SET_ERR(ret, EXIF_TAG_SHARPNESS);
		} else {
			_mmcam_dbg_warn("failed to get range of sharpness");
		}
	}

	/*40. EXIF_TAG_SUBJECT_DISTANCE_RANGE*/
	/*FIXME*/
	value = MM_SUBJECT_DISTANCE_RANGE;
	_mmcam_dbg_log("DISTANCE_RANGE [%d]", value);
	exif_set_short((unsigned char *)&eshort[cnts], exif_data_get_byte_order(ed), value);
	ret = mm_exif_set_add_entry(ed, EXIF_IFD_EXIF, EXIF_TAG_SUBJECT_DISTANCE_RANGE,
		EXIF_FORMAT_SHORT, 1, (const char *)&eshort[cnts++]);
	if (ret != MM_ERROR_NONE)
		EXIF_SET_ERR(ret, EXIF_TAG_SUBJECT_DISTANCE_RANGE);

	/* GPS information */
	__mmcamcorder_update_exif_gps(handle, ed);

	_mmcam_dbg_log("");

	ret = mm_exif_set_exif_to_info(hcamcorder->exif_info, ed);
	if (ret != MM_ERROR_NONE)
		_mmcam_dbg_err("mm_exif_set_exif_to_info err!! [%x]", ret);

exit:
	_mmcam_dbg_log("finished!! [%x]", ret);

	if (ed)
		exif_data_unref(ed);

	return ret;
}


static void __sound_status_changed_cb(keynode_t* node, void *data)
{
	mmf_camcorder_t *hcamcorder = (mmf_camcorder_t *)data;
	_MMCamcorderImageInfo *info = NULL;

	mmf_return_if_fail(hcamcorder && hcamcorder->sub_context && hcamcorder->sub_context->info_image);

	_mmcam_dbg_log("START");

	info = hcamcorder->sub_context->info_image;

	vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &(info->sound_status));

	_mmcam_dbg_log("DONE : sound status %d", info->sound_status);

	return;
}
