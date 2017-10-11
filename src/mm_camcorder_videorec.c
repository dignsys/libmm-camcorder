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
|  INCLUDE FILES																		|
=======================================================================================*/
#include <gst/video/cameracontrol.h>
#include <gst/app/gstappsrc.h>
#include "mm_camcorder_internal.h"
#include "mm_camcorder_videorec.h"

/*---------------------------------------------------------------------------------------
|    GLOBAL VARIABLE DEFINITIONS for internal											|
---------------------------------------------------------------------------------------*/
#define _MMCAMCORDER_LOCATION_INFO		/* for add gps information */
#define MAX_ERROR_MESSAGE_LEN			128

/*---------------------------------------------------------------------------------------
|    LOCAL VARIABLE DEFINITIONS for internal											|
---------------------------------------------------------------------------------------*/
#define _MMCAMCORDER_MINIMUM_FRAME		5
#define _MMCAMCORDER_RETRIAL_COUNT		10
#define _MMCAMCORDER_FRAME_WAIT_TIME	200000	/* ms */
#define _OFFSET_COMPOSITION_MATRIX		40L
#define _GOP_GEN_INTERVAL				1000000000	/*nano seconds*/
#define _MMCAMCORDER_VIDEO_MINIMUM_SPACE	(_MMCAMCORDER_MINIMUM_SPACE << 1)      /* byte */

/*---------------------------------------------------------------------------------------
|    LOCAL FUNCTION PROTOTYPES:															|
---------------------------------------------------------------------------------------*/
/* STATIC INTERNAL FUNCTION */
static gboolean __mmcamcorder_video_stream_cb(GstElement *element, GstSample *sample, gpointer u_data);
static GstPadProbeReturn __mmcamcorder_audio_dataprobe_check(GstPad *pad, GstPadProbeInfo *info, gpointer u_data);
static GstPadProbeReturn __mmcamcorder_video_dataprobe_record(GstPad *pad, GstPadProbeInfo *info, gpointer u_data);
static GstPadProbeReturn __mmcamcorder_audioque_dataprobe(GstPad *pad, GstPadProbeInfo *info, gpointer u_data);
static GstPadProbeReturn __mmcamcorder_video_dataprobe_audio_disable(GstPad *pad, GstPadProbeInfo *info, gpointer u_data);
static GstPadProbeReturn __mmcamcorder_audio_dataprobe_audio_mute(GstPad *pad, GstPadProbeInfo *info, gpointer u_data);
static gboolean __mmcamcorder_add_metadata(MMHandleType handle, int fileformat);
static gboolean __mmcamcorder_add_metadata_mp4(MMHandleType handle);

/*=======================================================================================
|  FUNCTION DEFINITIONS									|
=======================================================================================*/
/*---------------------------------------------------------------------------------------
|    GLOBAL FUNCTION DEFINITIONS:							|
---------------------------------------------------------------------------------------*/
static gboolean __mmcamcorder_video_stream_cb(GstElement *element, GstSample *sample, gpointer u_data)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(u_data);
	_MMCamcorderSubContext *sc = NULL;

	GstBuffer *buffer = gst_sample_get_buffer(sample);
	mmf_return_val_if_fail(buffer, FALSE);
	mmf_return_val_if_fail(gst_buffer_n_memory(buffer), FALSE);
	mmf_return_val_if_fail(hcamcorder, FALSE);

	sc = MMF_CAMCORDER_SUBCONTEXT(hcamcorder);
	mmf_return_val_if_fail(sc, FALSE);

	/*
	_mmcam_dbg_log("ENTER - push_encoding_buffer %d, buffer %p, MALLOCDATA %p, size %d",
		sc->info_video->push_encoding_buffer, buffer, GST_BUFFER_MALLOCDATA(buffer), GST_BUFFER_SIZE(buffer));
	*/

	/* push buffer in appsrc to encode */
	if (sc->info_video->push_encoding_buffer == PUSH_ENCODING_BUFFER_RUN &&
	    sc->info_video->record_dual_stream &&
	    sc->encode_element[_MMCAMCORDER_ENCSINK_SRC].gst) {
		GstFlowReturn ret = 0;
		GstClock *pipe_clock = NULL;

		if (sc->encode_element[_MMCAMCORDER_AUDIOSRC_SRC].gst) {
			if (sc->info_video->is_firstframe) {
				sc->info_video->is_firstframe = FALSE;
				pipe_clock = GST_ELEMENT_CLOCK(sc->encode_element[_MMCAMCORDER_AUDIOSRC_SRC].gst);
				if (pipe_clock) {
					gst_object_ref(pipe_clock);
					sc->info_video->base_video_ts = GST_BUFFER_PTS(buffer) - (gst_clock_get_time(pipe_clock) - GST_ELEMENT(sc->encode_element[_MMCAMCORDER_ENCSINK_SRC].gst)->base_time);
					gst_object_unref(pipe_clock);
				}
			}
		} else {
			if (sc->info_video->is_firstframe) {
				sc->info_video->is_firstframe = FALSE;
				sc->info_video->base_video_ts = GST_BUFFER_PTS(buffer);
			}
		}

		GST_BUFFER_PTS(buffer) = GST_BUFFER_PTS(buffer) - sc->info_video->base_video_ts;
		GST_BUFFER_DTS(buffer) = GST_BUFFER_PTS(buffer);

		ret = gst_app_src_push_buffer((GstAppSrc *)sc->encode_element[_MMCAMCORDER_ENCSINK_SRC].gst, buffer);
		if (ret != GST_FLOW_OK && ret != GST_FLOW_FLUSHING) {
			_mmcam_dbg_err("gst_app_src_push_buffer failed [0x%x]", ret);
			gst_buffer_unref(buffer);
			buffer = NULL;
		}

		/*_mmcam_dbg_log("push buffer result : 0x%x", ret);*/
	} else {
		_mmcam_dbg_warn("unref video buffer immediately - push encoding buffer %d",
			sc->info_video->push_encoding_buffer);

		gst_buffer_unref(buffer);
		buffer = NULL;
	}

	return TRUE;
}


int _mmcamcorder_create_recorder_pipeline(MMHandleType handle)
{
	int i = 0;
	int err = MM_ERROR_NONE;
	const char* gst_element_rsink_name = NULL;

	GstBus *bus = NULL;
	GstPad *srcpad = NULL;
	GstPad *sinkpad = NULL;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	type_element *RecordsinkElement = NULL;

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	mmf_return_val_if_fail(sc->element, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	_mmcam_dbg_warn("start");

	err = _mmcamcorder_check_videocodec_fileformat_compatibility(handle);
	if (err != MM_ERROR_NONE)
		return err;

	/* Main pipeline */
	if (sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst) {
		_mmcam_dbg_log("pipeline is exist so need to remove pipeline _MMCAMCORDER_ENCODE_MAIN_PIPE = %p",
			sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst);
		_mmcamcorder_remove_recorder_pipeline((MMHandleType)hcamcorder);
	}

	_MMCAMCORDER_PIPELINE_MAKE(sc, sc->encode_element, _MMCAMCORDER_ENCODE_MAIN_PIPE, "recorder_pipeline", err);

	/* get audio disable */
	mm_camcorder_get_attributes(handle, NULL,
		MMCAM_AUDIO_DISABLE, &sc->audio_disable,
		NULL);

	_mmcam_dbg_log("MMCAM_AUDIO_DISABLE %d, is_modified_rate %d",
		sc->audio_disable, sc->is_modified_rate);

	sc->audio_disable |= sc->is_modified_rate;

	if (sc->audio_disable == FALSE) {
		/* create audiosrc bin */
		err = _mmcamcorder_create_audiosrc_bin((MMHandleType)hcamcorder);
		if (err != MM_ERROR_NONE)
			return err;
	}

	err = _mmcamcorder_create_encodesink_bin((MMHandleType)hcamcorder, MM_CAMCORDER_ENCBIN_PROFILE_VIDEO);
	if (err != MM_ERROR_NONE)
		return err;

	if (sc->audio_disable == FALSE) {
		gst_bin_add(GST_BIN(sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst),
			sc->encode_element[_MMCAMCORDER_AUDIOSRC_BIN].gst);
	}

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
	sinkpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_BIN].gst, "video_sink0");
	_MM_GST_PAD_LINK_UNREF(srcpad, sinkpad, err, pipeline_creation_error);

	if (sc->audio_disable == FALSE) {
		srcpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_AUDIOSRC_BIN].gst, "src");
		sinkpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_BIN].gst, "audio_sink0");
		_MM_GST_PAD_LINK_UNREF(srcpad, sinkpad, err, pipeline_creation_error);
	}

	_mmcamcorder_conf_get_element(handle, hcamcorder->conf_main,
		CONFIGURE_CATEGORY_MAIN_RECORD,
		"RecordsinkElement",
		&RecordsinkElement);
	_mmcamcorder_conf_get_value_element_name(RecordsinkElement, &gst_element_rsink_name);

	if (!gst_element_rsink_name) {
		_mmcam_dbg_err("failed to get recordsink name");
		err = MM_ERROR_CAMCORDER_INTERNAL;
		goto pipeline_creation_error;
	}

	/* set data probe function */

	/* register message cb */

	/* set data probe functions */
	if (sc->audio_disable == FALSE) {
		sinkpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_AENC].gst, "sink");
		MMCAMCORDER_ADD_BUFFER_PROBE(sinkpad, _MMCAMCORDER_HANDLER_VIDEOREC,
			__mmcamcorder_audioque_dataprobe, hcamcorder);
		gst_object_unref(sinkpad);
		sinkpad = NULL;

		/* for voice mute */
		srcpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_AUDIOSRC_SRC].gst, "src");
		MMCAMCORDER_ADD_BUFFER_PROBE(srcpad, _MMCAMCORDER_HANDLER_VIDEOREC,
			__mmcamcorder_audio_dataprobe_audio_mute, hcamcorder);
		gst_object_unref(srcpad);
		srcpad = NULL;

		if (sc->encode_element[_MMCAMCORDER_ENCSINK_AENC_QUE].gst) {
			srcpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_AENC_QUE].gst, "src");
			MMCAMCORDER_ADD_EVENT_PROBE(srcpad, _MMCAMCORDER_HANDLER_VIDEOREC,
				__mmcamcorder_eventprobe_monitor, hcamcorder);
			gst_object_unref(srcpad);
			srcpad = NULL;
		}
	}

	if (sc->encode_element[_MMCAMCORDER_ENCSINK_VENC_QUE].gst) {
		srcpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_VENC_QUE].gst, "src");
		MMCAMCORDER_ADD_EVENT_PROBE(srcpad, _MMCAMCORDER_HANDLER_VIDEOREC,
			__mmcamcorder_eventprobe_monitor, hcamcorder);
		gst_object_unref(srcpad);
		srcpad = NULL;
	}

	if (sc->audio_disable) {
		sinkpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_VENC].gst, "sink");
		MMCAMCORDER_ADD_BUFFER_PROBE(sinkpad, _MMCAMCORDER_HANDLER_VIDEOREC,
			__mmcamcorder_video_dataprobe_audio_disable, hcamcorder);
		gst_object_unref(sinkpad);
		sinkpad = NULL;
	}

	if (!strcmp(gst_element_rsink_name, "filesink")) {
		srcpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_VENC].gst, "src");
		MMCAMCORDER_ADD_BUFFER_PROBE(srcpad, _MMCAMCORDER_HANDLER_VIDEOREC,
			__mmcamcorder_video_dataprobe_record, hcamcorder);
		gst_object_unref(srcpad);
		srcpad = NULL;

		srcpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_AENC].gst, "src");
		MMCAMCORDER_ADD_BUFFER_PROBE(srcpad, _MMCAMCORDER_HANDLER_VIDEOREC,
			__mmcamcorder_audio_dataprobe_check, hcamcorder);
		gst_object_unref(srcpad);
		srcpad = NULL;
	}

	sinkpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_SINK].gst, "sink");
	MMCAMCORDER_ADD_BUFFER_PROBE(sinkpad, _MMCAMCORDER_HANDLER_VIDEOREC,
		__mmcamcorder_muxed_dataprobe, hcamcorder);
	MMCAMCORDER_ADD_EVENT_PROBE(sinkpad, _MMCAMCORDER_HANDLER_VIDEOREC,
		__mmcamcorder_eventprobe_monitor, hcamcorder);
	gst_object_unref(sinkpad);
	sinkpad = NULL;

	bus = gst_pipeline_get_bus(GST_PIPELINE(sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst));

	/* register pipeline message callback */
	hcamcorder->encode_pipeline_cb_event_id = gst_bus_add_watch(bus, (GstBusFunc)_mmcamcorder_pipeline_cb_message, hcamcorder);

	/* set sync handler */
	gst_bus_set_sync_handler(bus, _mmcamcorder_encode_pipeline_bus_sync_callback, (gpointer)hcamcorder, NULL);

	gst_object_unref(bus);
	bus = NULL;

	return MM_ERROR_NONE;

pipeline_creation_error:
	for (i = _MMCAMCORDER_AUDIOSRC_BIN ; i <= _MMCAMCORDER_ENCSINK_SINK ; i++)
		_MMCAMCORDER_ELEMENT_REMOVE(sc->encode_element, i);

	_MMCAMCORDER_ELEMENT_REMOVE(sc->encode_element, _MMCAMCORDER_ENCODE_MAIN_PIPE);
	return err;
}


int _mmcamcorder_remove_audio_pipeline(MMHandleType handle)
{
	GstPad *srcpad = NULL;
	GstPad *sinkpad = NULL;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	mmf_return_val_if_fail(sc->element, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	_mmcam_dbg_log("");

	if (sc->encode_element[_MMCAMCORDER_AUDIOSRC_BIN].gst != NULL) {
		srcpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_AUDIOSRC_BIN].gst, "src");
		sinkpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_BIN].gst, "audio_sink0");
		_MM_GST_PAD_UNLINK_UNREF(srcpad, sinkpad);

		/* release audiosrc bin */
		gst_bin_remove(GST_BIN(sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst),
			sc->encode_element[_MMCAMCORDER_AUDIOSRC_BIN].gst);

		/*
			To avoid conflicting between old elements and newly created elements,
			I clean element handles here. Real elements object will be finalized as the 'unref' process goes on.
			This is a typical problem of unref. Even though I unref bin here, it takes much time to finalize each elements.
			So I clean handles first, make them unref later. Audio recording, however, isn't needed this process.
			It's because the pipeline of audio recording destroys at the same time,
			and '_mmcamcorder_element_release_noti' will perfom removing handle.
		*/
		_mmcamcorder_remove_element_handle(handle, (void *)sc->encode_element, _MMCAMCORDER_AUDIOSRC_BIN, _MMCAMCORDER_AUDIOSRC_VOL);

		_mmcam_dbg_log("Audio pipeline removed");
	}

	return MM_ERROR_NONE;
}


int _mmcamcorder_remove_encode_pipeline(MMHandleType handle)
{
	GstPad *reqpad = NULL;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;
#ifdef _MMCAMCORDER_MURPHY_SUPPORT
	int ret = MM_ERROR_NONE;
	MMCamcorderResourceManager *resource_manager = NULL;
#endif /* _MMCAMCORDER_MURPHY_SUPPORT */

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	mmf_return_val_if_fail(sc->element, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	_mmcam_dbg_log("");

	if (sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst != NULL) {
		/* release request pad */
		reqpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, "audio");
		if (reqpad) {
			gst_element_release_request_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, reqpad);
			gst_object_unref(reqpad);
			reqpad = NULL;
		}

		reqpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, "video");
		if (reqpad) {
			gst_element_release_request_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, reqpad);
			gst_object_unref(reqpad);
			reqpad = NULL;
		}

		/* release encode main pipeline */
		gst_object_unref(sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst);

		/*
			To avoid conflicting between old elements and newly created elements,
			I clean element handles here. Real elements object will be finalized as the 'unref' process goes on.
			This is a typical problem of unref. Even though I unref bin here, it takes much time to finalize each elements.
			So I clean handles first, make them unref later. Audio recording, however, isn't needed this process.
			It's because the pipeline of audio recording destroys at the same time,
			and '_mmcamcorder_element_release_noti' will perfom removing handle.
		*/
		/* _mmcamcorder_remove_element_handle(handle, (void *)sc->encode_element,
			_MMCAMCORDER_ENCODE_MAIN_PIPE, _MMCAMCORDER_ENCSINK_SINK); */

		_mmcam_dbg_warn("Encoder pipeline removed");

#ifdef _MMCAMCORDER_MURPHY_SUPPORT
		resource_manager = &hcamcorder->resource_manager_sub;

		_mmcam_dbg_warn("lock resource - cb calling %d", resource_manager->is_release_cb_calling);

		_MMCAMCORDER_LOCK_RESOURCE(hcamcorder);

		if (resource_manager->is_release_cb_calling == FALSE) {
			/* release resource */
			ret = _mmcamcorder_resource_manager_release(resource_manager);

			_mmcam_dbg_warn("release resource 0x%x", ret);

			if (resource_manager->acquire_remain < resource_manager->acquire_count) {
				/* wait for resource release */
				gint64 end_time = g_get_monotonic_time() + (__MMCAMCORDER_RESOURCE_WAIT_TIME * G_TIME_SPAN_SECOND);
				_mmcam_dbg_log("resource is not released all. wait for signal...");
				_MMCAMCORDER_RESOURCE_WAIT_UNTIL(hcamcorder, end_time);
			}
		}

		_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);

		_mmcam_dbg_warn("unlock resource");
#endif /* _MMCAMCORDER_MURPHY_SUPPORT */
	}

	return MM_ERROR_NONE;
}


int _mmcamcorder_remove_recorder_pipeline(MMHandleType handle)
{
	int ret = MM_ERROR_NONE;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	GstBus *bus = NULL;

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	_mmcam_dbg_log("start");

	if (!sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst) {
		_mmcam_dbg_warn("pipeline is not existed.");
		return MM_ERROR_NONE;
	}

	_mmcamcorder_remove_all_handlers((MMHandleType)hcamcorder, _MMCAMCORDER_HANDLER_VIDEOREC);

	ret = _mmcamcorder_gst_set_state(handle, sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst, GST_STATE_NULL);
	if (ret != MM_ERROR_NONE) {
		_mmcam_dbg_err("Faile to change encode main pipeline [0x%x]", ret);
		return ret;
	}

	bus = gst_pipeline_get_bus(GST_PIPELINE(sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst));

	/* remove audio pipeline first */
	ret = _mmcamcorder_remove_audio_pipeline(handle);
	if (ret != MM_ERROR_NONE) {
		_mmcam_dbg_err("Fail to remove audio pipeline");
		return ret;
	}

	ret = _mmcamcorder_remove_encode_pipeline(handle);
	if (ret != MM_ERROR_NONE) {
		_mmcam_dbg_err("Fail to remove encoder pipeline");
		return ret;
	}

	/* Remove pipeline message callback */
	if (hcamcorder->encode_pipeline_cb_event_id != 0) {
		g_source_remove(hcamcorder->encode_pipeline_cb_event_id);
		hcamcorder->encode_pipeline_cb_event_id = 0;
	}

	/* Remove remained message */
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

	_mmcam_dbg_log("done");

	return ret;
}


int _mmcamcorder_video_command(MMHandleType handle, int command)
{
	int size = 0;
	int fileformat = 0;
	int count = 0;
	int ret = MM_ERROR_NONE;
	double motion_rate = _MMCAMCORDER_DEFAULT_RECORDING_MOTION_RATE;
	char *err_name = NULL;
	char *temp_filename = NULL;
	GstCameraControl *CameraControl = NULL;
	GstCameraControlChannel *CameraControlChannel = NULL;
	const GList *controls = NULL;
	const GList *item = NULL;

	gint fps = 0;
	GstElement *pipeline = NULL;

	_MMCamcorderVideoInfo *info = NULL;
	_MMCamcorderSubContext *sc = NULL;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	mmf_return_val_if_fail(sc->info_video, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	mmf_return_val_if_fail(sc->element, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	info = sc->info_video;

	_mmcam_dbg_log("Command(%d)", command);

	pipeline = sc->element[_MMCAMCORDER_MAIN_PIPE].gst;

	switch (command) {
	case _MMCamcorder_CMD_RECORD:
	{
		if (_mmcamcorder_get_state((MMHandleType)hcamcorder) != MM_CAMCORDER_STATE_PAUSED) {
			/**
			 * start recording
			 */
			guint imax_size = 0;
			guint imax_time = 0;
			int ret_free_space = 0;
			char *dir_name = NULL;
			guint64 free_space = 0;
			int file_system_type = 0;
			int root_directory_length = 0;

			/* Recording */
			_mmcam_dbg_log("Record Start - dual stream %d", info->support_dual_stream);

#ifdef _MMCAMCORDER_MURPHY_SUPPORT
			/* check connection */
			ret = _mmcamcorder_resource_check_connection(&hcamcorder->resource_manager_sub);
			if (ret != MM_ERROR_NONE)
				goto _ERR_CAMCORDER_VIDEO_COMMAND;

			/* create resource set */
			ret = _mmcamcorder_resource_create_resource_set(&hcamcorder->resource_manager_sub);
			if (ret != MM_ERROR_NONE)
				goto _ERR_CAMCORDER_VIDEO_COMMAND;

			hcamcorder->resource_manager_sub.acquire_count = 0;

			/* prepare resource manager for H/W encoder */
			ret = _mmcamcorder_resource_manager_prepare(&hcamcorder->resource_manager_sub, MM_CAMCORDER_RESOURCE_TYPE_VIDEO_ENCODER);
			if (ret != MM_ERROR_NONE) {
				_mmcam_dbg_err("could not prepare for video_encoder resource");
				ret = MM_ERROR_CAMCORDER_INTERNAL;
				goto _ERR_CAMCORDER_VIDEO_COMMAND;
			}

			/* acquire resources */
			_MMCAMCORDER_LOCK_RESOURCE(hcamcorder);

			ret = _mmcamcorder_resource_manager_acquire(&hcamcorder->resource_manager_sub);
			if (ret != MM_ERROR_NONE) {
				_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);
				_mmcam_dbg_err("could not acquire resource");
				goto _ERR_CAMCORDER_VIDEO_COMMAND;
			}

			if (hcamcorder->resource_manager_sub.acquire_remain > 0) {
				gint64 end_time = 0;

				_mmcam_dbg_warn("wait for resource state change");

				/* wait for resource state change */
				end_time = g_get_monotonic_time() + (__MMCAMCORDER_RESOURCE_WAIT_TIME * G_TIME_SPAN_SECOND);

				if (_MMCAMCORDER_RESOURCE_WAIT_UNTIL(hcamcorder, end_time)) {
					_mmcam_dbg_warn("signal received");
				} else {
					_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);
					_mmcam_dbg_err("timeout");
					ret = MM_ERROR_RESOURCE_INTERNAL;
					goto _ERR_CAMCORDER_VIDEO_COMMAND;
				}
			} else {
				_mmcam_dbg_log("already acquired");
			}

			_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);
#endif /* _MMCAMCORDER_MURPHY_SUPPORT */

			/* init record_dual_stream */
			info->record_dual_stream = FALSE;

			ret = mm_camcorder_get_attributes(handle, &err_name,
				MMCAM_CAMERA_FPS, &fps,
				MMCAM_CAMERA_WIDTH, &(info->preview_width),
				MMCAM_CAMERA_HEIGHT, &(info->preview_height),
				MMCAM_VIDEO_WIDTH, &(info->video_width),
				MMCAM_VIDEO_HEIGHT, &(info->video_height),
				MMCAM_FILE_FORMAT, &fileformat,
				MMCAM_TARGET_FILENAME, &temp_filename, &size,
				MMCAM_TARGET_MAX_SIZE, &imax_size,
				MMCAM_TARGET_TIME_LIMIT, &imax_time,
				MMCAM_FILE_FORMAT, &(info->fileformat),
				MMCAM_CAMERA_RECORDING_MOTION_RATE, &motion_rate,
				MMCAM_ROOT_DIRECTORY, &hcamcorder->root_directory, &root_directory_length,
				NULL);
			if (ret != MM_ERROR_NONE) {
				_mmcam_dbg_warn("Get attrs fail. (%s:%x)", err_name, ret);
				SAFE_FREE(err_name);
				goto _ERR_CAMCORDER_VIDEO_COMMAND;
			}

			if (!temp_filename && !hcamcorder->mstream_cb) {
				_mmcam_dbg_err("filename is not set and muxed stream cb is NULL");
				ret = MM_ERROR_CAMCORDER_INVALID_ARGUMENT;
				goto _ERR_CAMCORDER_VIDEO_COMMAND;
			}

			/* set max size */
			if (imax_size <= 0)
				info->max_size = 0; /* do not check */
			else
				info->max_size = ((guint64)imax_size) << 10; /* to byte */

			/* set max time */
			if (imax_time <= 0)
				info->max_time = 0; /* do not check */
			else
				info->max_time = (guint64)((double)imax_time * (double)1000 * motion_rate); /* to millisecond */

			dir_name = g_path_get_dirname(temp_filename);
			if (dir_name) {
				ret = _mmcamcorder_get_storage_info(dir_name, hcamcorder->root_directory, &hcamcorder->storage_info);
				if (ret != 0) {
					_mmcam_dbg_err("get storage info failed");
					g_free(dir_name);
					dir_name = NULL;
					return MM_ERROR_OUT_OF_STORAGE;
				}

				ret_free_space = _mmcamcorder_get_freespace(hcamcorder->storage_info.type, &free_space);

				_mmcam_dbg_warn("current space - %s [%" G_GUINT64_FORMAT "]", dir_name, free_space);

				if (_mmcamcorder_get_file_system_type(dir_name, &file_system_type) == 0) {
					/* MSDOS_SUPER_MAGIC : 0x4d44 */
					if (file_system_type == MSDOS_SUPER_MAGIC &&
					    (info->max_size == 0 || info->max_size > FAT32_FILE_SYSTEM_MAX_SIZE)) {
						_mmcam_dbg_warn("FAT32 and too large max[%"G_GUINT64_FORMAT"], set max as %"G_GUINT64_FORMAT,
							info->max_size, FAT32_FILE_SYSTEM_MAX_SIZE);
						info->max_size = FAT32_FILE_SYSTEM_MAX_SIZE;
					} else {
						_mmcam_dbg_warn("file system 0x%x, max size %"G_GUINT64_FORMAT,
							file_system_type, info->max_size);
					}
				} else {
					_mmcam_dbg_warn("_mmcamcorder_get_file_system_type failed");
				}

				g_free(dir_name);
				dir_name = NULL;
			} else {
				_mmcam_dbg_err("failed to get directory name");
				ret_free_space = -1;
			}

			if (temp_filename &&
				(ret_free_space == -1 || free_space <= _MMCAMCORDER_VIDEO_MINIMUM_SPACE)) {
				_mmcam_dbg_err("OUT of STORAGE [ret_free_space:%d or free space [%" G_GUINT64_FORMAT "] is smaller than [%d]",
					ret_free_space, free_space, _MMCAMCORDER_VIDEO_MINIMUM_SPACE);
				return MM_ERROR_OUT_OF_STORAGE;
			}

			g_mutex_lock(&hcamcorder->task_thread_lock);
			if (sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst == NULL &&
			    hcamcorder->task_thread_state == _MMCAMCORDER_TASK_THREAD_STATE_NONE) {
				/* Play record start sound */
				_mmcamcorder_sound_solo_play(handle, _MMCAMCORDER_SAMPLE_SOUND_NAME_REC_START, FALSE);
			}
			g_mutex_unlock(&hcamcorder->task_thread_lock);

			_mmcam_dbg_warn("video size [%dx%d]", info->video_width, info->video_height);

			if (info->video_width == 0 || info->video_height == 0) {
				_mmcam_dbg_warn("video size is invalid [%dx%d] use preview size [%dx%d]",
					info->video_width, info->video_height, info->preview_width, info->preview_height);
				info->video_width = info->preview_width;
				info->video_height = info->preview_height;
			}

			if (info->support_dual_stream) {
				_mmcam_dbg_warn("DUAL STREAM MODE");

				info->record_dual_stream = TRUE;

				/* No need to restart preview */
				info->restart_preview = FALSE;

				MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst, "video-width", info->video_width);
				MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst, "video-height", info->video_height);
			} else if (sc->info_image->preview_format == MM_PIXEL_FORMAT_ENCODED_H264 &&
				info->preview_width == info->video_width &&
				info->preview_height == info->video_height) {
				_mmcam_dbg_log("H264 preview mode and same resolution");

				/* No need to restart preview */
				info->restart_preview = FALSE;
			} else {
				/* always need to restart preview  */
				info->restart_preview = TRUE;
			}

			/* set recording hint */
			MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst, "recording-hint", TRUE);

			if (info->restart_preview) {
				/* stop preview and set new size */
				_mmcam_dbg_log("restart preview");

				MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_QUE].gst, "empty-buffers", TRUE);
				MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_QUE].gst, "empty-buffers", TRUE);
				MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_SINK].gst, "keep-camera-preview", TRUE);

				ret = _mmcamcorder_gst_set_state(handle, pipeline, GST_STATE_READY);

				/* check decoder recreation */
				if (!_mmcamcorder_recreate_decoder_for_encoded_preview(handle)) {
					_mmcam_dbg_err("_mmcamcorder_recreate_decoder_for_encoded_preview failed");
					ret = MM_ERROR_CAMCORDER_INTERNAL;
					goto _ERR_CAMCORDER_VIDEO_COMMAND;
				}

				MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_QUE].gst, "empty-buffers", FALSE);
				MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_QUE].gst, "empty-buffers", FALSE);
				MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_SINK].gst, "keep-camera-preview", FALSE);

				if (ret != MM_ERROR_NONE)
					goto _ERR_CAMCORDER_VIDEO_COMMAND;

				if (!_mmcamcorder_set_camera_resolution(handle, info->video_width, info->video_height)) {
					ret = MM_ERROR_CAMCORDER_INTERNAL;
					goto _ERR_CAMCORDER_VIDEO_COMMAND;
				}

				/* Start preview again with new setting */
				ret = _mmcamcorder_gst_set_state(handle, pipeline, GST_STATE_PLAYING);
				if (ret != MM_ERROR_NONE)
					goto _ERR_CAMCORDER_VIDEO_COMMAND;

				if (motion_rate < 1.0) {
					_mmcam_dbg_warn("wait for stabilization of frame");
					usleep(300000);
				}
			} else {
				_mmcam_dbg_log("no need to restart preview");
			}

			_mmcamcorder_conf_get_value_int(handle, hcamcorder->conf_main,
				CONFIGURE_CATEGORY_MAIN_RECORD,
				"DropVideoFrame",
				&(sc->drop_vframe));

			_mmcamcorder_conf_get_value_int(handle, hcamcorder->conf_main,
				CONFIGURE_CATEGORY_MAIN_RECORD,
				"PassFirstVideoFrame",
				&(sc->pass_first_vframe));

			_mmcam_dbg_log("Drop video frame count[%d], Pass fisrt video frame count[%d]",
				sc->drop_vframe, sc->pass_first_vframe);

			info->record_drop_count = (guint)motion_rate;
			info->record_motion_rate = motion_rate;
			if (sc->is_modified_rate)
				info->record_timestamp_ratio = (_MMCAMCORDER_DEFAULT_RECORDING_MOTION_RATE/motion_rate);
			else
				info->record_timestamp_ratio = _MMCAMCORDER_DEFAULT_RECORDING_MOTION_RATE;

			_mmcam_dbg_warn("recording fps %d, motion rate %f, timestamp_ratio %f",
				fps, info->record_motion_rate, info->record_timestamp_ratio);

			/* set push buffer flag */
			info->push_encoding_buffer = PUSH_ENCODING_BUFFER_INIT;
			info->base_video_ts = 0;

			/* connect video stream cb signal */
			/*130826 Connect video stream cb for handling fast record frame cb*/
			if (info->record_dual_stream) {
				if (_mmcamcorder_connect_video_stream_cb_signal((MMHandleType)hcamcorder) != MM_ERROR_NONE)
					goto _ERR_CAMCORDER_VIDEO_COMMAND;
			}

			/* start video stream */
			if (info->record_dual_stream) {
				CameraControl =  GST_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst);
				if (CameraControl) {
					MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_SINK].gst, "stop-video", TRUE);

					_mmcam_dbg_log("GST_CAMERA_CONTROL_RECORD_COMMAND_START");
					gst_camera_control_set_record_command(CameraControl, GST_CAMERA_CONTROL_RECORD_COMMAND_START);

					MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_SINK].gst, "stop-video", FALSE);
				} else {
					_mmcam_dbg_err("could not get camera control");
				}
			}

			/* check pre-created encode pipeline */
			g_mutex_lock(&hcamcorder->task_thread_lock);
			if (sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst == NULL &&
			    hcamcorder->task_thread_state == _MMCAMCORDER_TASK_THREAD_STATE_NONE) {
				/* create encoding pipeline */
				ret = _mmcamcorder_video_prepare_record((MMHandleType)hcamcorder);
				if (ret != MM_ERROR_NONE) {
					g_mutex_unlock(&hcamcorder->task_thread_lock);
					goto _ERR_CAMCORDER_VIDEO_COMMAND;
				}
			}
			g_mutex_unlock(&hcamcorder->task_thread_lock);

			/* check recording start sound */
			_mmcamcorder_sound_solo_play_wait(handle);

			/**< To fix video recording hanging
				1. use gst_element_set_start_time() instead of gst_pipeline_set_new_stream_time()
				2. Set (GstClockTime)1 instead of (GstClockTime)0. Because of strict check in gstreamer 0.25,
				 basetime wouldn't change if you set (GstClockTime)0.
				3. Move set start time position below PAUSED of pipeline.
			*/
			/*
			gst_element_set_start_time(GST_ELEMENT(sc->element[_MMCAMCORDER_MAIN_PIPE].gst), (GstClockTime)1);
			gst_element_set_start_time(GST_ELEMENT(sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst), (GstClockTime)1);
			*/

			info->video_frame_count = 0;
			info->is_firstframe = TRUE;
			info->audio_frame_count = 0;
			info->filesize = 0;
			sc->ferror_send = FALSE;
			sc->ferror_count = 0;
			hcamcorder->error_occurs = FALSE;
			sc->bget_eos = FALSE;
			sc->muxed_stream_offset = 0;

			ret = _mmcamcorder_gst_set_state(handle, sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst, GST_STATE_PLAYING);
			if (ret != MM_ERROR_NONE) {
				/* stop video stream */
				if (info->record_dual_stream) {
					CameraControl = GST_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst);
					if (CameraControl) {
						MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_SINK].gst, "stop-video", TRUE);

						_mmcam_dbg_log("GST_CAMERA_CONTROL_RECORD_COMMAND_STOP");
						gst_camera_control_set_record_command(CameraControl, GST_CAMERA_CONTROL_RECORD_COMMAND_STOP);

						MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_SINK].gst, "stop-video", FALSE);
					} else {
						_mmcam_dbg_err("failed to get camera control");
					}
				}

				/* Remove recorder pipeline and recording file which size maybe zero */
				_mmcamcorder_remove_recorder_pipeline((MMHandleType)hcamcorder);
				if (info->filename) {
					_mmcam_dbg_log("file delete(%s)", info->filename);
					unlink(info->filename);
				}
				goto _ERR_CAMCORDER_VIDEO_COMMAND;
			}

			/*set the camera control to create the GOP so that video record will get a new key frame*/
			if (sc->info_image->preview_format == MM_PIXEL_FORMAT_ENCODED_H264 &&
			    GST_IS_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst)) {
				CameraControl = GST_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst);
				controls = gst_camera_control_list_channels(CameraControl);
				if (controls != NULL) {
					for (item = controls ; item && item->data ; item = item->next) {
						CameraControlChannel = item->data;
						_mmcam_dbg_log("CameraControlChannel->label %s", CameraControlChannel->label);
						if (!strcmp(CameraControlChannel->label, "new-gop")) {
							/* gst_camera_control_set_value(CameraControl, CameraControlChannel, 1); */
							break;
						}
					}

					if (item == NULL)
						_mmcam_dbg_warn("failed to find new-gop control channel");
				}
			} else {
				_mmcam_dbg_warn("Can't cast Video source into camera control or not H264 prevew format[%d]",
					sc->info_image->preview_format);
			}
		} else {
			/* Resume case */

			if (sc->info_image->preview_format == MM_PIXEL_FORMAT_ENCODED_H264 &&
			    GST_IS_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst)) {
				/* generate and I-frame on resuming */
				CameraControl = GST_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst);
				controls = gst_camera_control_list_channels(CameraControl);
				if (controls != NULL) {
					for (item = controls ; item && item->data ; item = item->next) {
						CameraControlChannel = item->data;
						_mmcam_dbg_log("CameraControlChannel->label %s", CameraControlChannel->label);
						if (!strcmp(CameraControlChannel->label, "new-gop")) {
							/* gst_camera_control_set_value(CameraControl, CameraControlChannel, 1); */
							break;
						}
					}

					if (item == NULL)
						_mmcam_dbg_warn("failed to find new-gop control channel");
				}
			}

			MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, "runtime-pause", FALSE);

			_mmcam_dbg_log("Object property settings done");
		}
	}
		break;
	case _MMCamcorder_CMD_PAUSE:
	{
		if (info->b_commiting) {
			_mmcam_dbg_warn("now on commiting previous file!!(command : %d)", command);
			return MM_ERROR_CAMCORDER_CMD_IS_RUNNING;
		}

		for (count = 0 ; count <= _MMCAMCORDER_RETRIAL_COUNT ; count++) {
			if (sc->audio_disable) {
				/* check only video frame */
				if (info->video_frame_count >= _MMCAMCORDER_MINIMUM_FRAME) {
					break;
				} else if (count == _MMCAMCORDER_RETRIAL_COUNT) {
					_mmcam_dbg_err("Pause fail, frame count %llu", info->video_frame_count);
					return MM_ERROR_CAMCORDER_INVALID_CONDITION;
				} else {
					_mmcam_dbg_warn("Waiting for enough video frame, retrial[%d], frame %llu", count, info->video_frame_count);
				}

				usleep(_MMCAMCORDER_FRAME_WAIT_TIME);
			} else {
				/* check both of video and audio frame */
				if (info->video_frame_count >= _MMCAMCORDER_MINIMUM_FRAME && info->audio_frame_count) {
					break;
				} else if (count == _MMCAMCORDER_RETRIAL_COUNT) {
					_mmcam_dbg_err("Pause fail, frame count VIDEO[%llu], AUDIO [%llu]",
						info->video_frame_count, info->audio_frame_count);
					return MM_ERROR_CAMCORDER_INVALID_CONDITION;
				} else {
					_mmcam_dbg_warn("Waiting for enough frames, retrial [%d], VIDEO[%llu], AUDIO [%llu]",
						count, info->video_frame_count, info->audio_frame_count);
				}

				usleep(_MMCAMCORDER_FRAME_WAIT_TIME);
			}
		}

		/* block encodebin */
		MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, "runtime-pause", TRUE);
		break;
	}
	case _MMCamcorder_CMD_CANCEL:
	{
		if (info->b_commiting) {
			_mmcam_dbg_warn("now on commiting previous file!!(command : %d)", command);
			return MM_ERROR_CAMCORDER_CMD_IS_RUNNING;
		}

		for (count = 0 ; count <= _MMCAMCORDER_RETRIAL_COUNT ; count++) {
			/* capturing */
			if (hcamcorder->capture_in_recording == FALSE) {
				break;
			} else if (count == _MMCAMCORDER_RETRIAL_COUNT) {
				_mmcam_dbg_err("Failed to Wait capture data");
				hcamcorder->capture_in_recording = FALSE;
				break;
			} else {
				_mmcam_dbg_warn("Waiting for capture data - retrial [%d]", count);
			}

			usleep(_MMCAMCORDER_FRAME_WAIT_TIME);
		}

		/* block push buffer */
		info->push_encoding_buffer = PUSH_ENCODING_BUFFER_STOP;

		ret = _mmcamcorder_remove_recorder_pipeline((MMHandleType)hcamcorder);
		if (ret != MM_ERROR_NONE)
			goto _ERR_CAMCORDER_VIDEO_COMMAND;

		/* set recording hint */
		MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst, "recording-hint", FALSE);

		/* stop video stream */
		if (info->record_dual_stream) {
			CameraControl = GST_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst);
			if (CameraControl) {
				MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_SINK].gst, "stop-video", TRUE);

				_mmcam_dbg_log("GST_CAMERA_CONTROL_RECORD_COMMAND_STOP");
				gst_camera_control_set_record_command(CameraControl, GST_CAMERA_CONTROL_RECORD_COMMAND_STOP);

				MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_SINK].gst, "stop-video", FALSE);
			} else {
				_mmcam_dbg_err("failed to get camera control");
			}
		}

		if (info->restart_preview) {
			/* restart preview */
			MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_QUE].gst, "empty-buffers", TRUE);
			MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_QUE].gst, "empty-buffers", TRUE);
			MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_SINK].gst, "keep-camera-preview", TRUE);

			ret = _mmcamcorder_gst_set_state(handle, pipeline, GST_STATE_READY);

			/* check decoder recreation */
			if (!_mmcamcorder_recreate_decoder_for_encoded_preview(handle)) {
				_mmcam_dbg_err("_mmcamcorder_recreate_decoder_for_encoded_preview failed");
				ret = MM_ERROR_CAMCORDER_INTERNAL;
			}

			MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_QUE].gst, "empty-buffers", FALSE);
			MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_QUE].gst, "empty-buffers", FALSE);
			MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_SINK].gst, "keep-camera-preview", FALSE);

			if (ret != MM_ERROR_NONE)
				goto _ERR_CAMCORDER_VIDEO_COMMAND;

			/* reset restart_preview for inset window layout */
			info->restart_preview = FALSE;

			if (!_mmcamcorder_set_camera_resolution(handle, info->preview_width, info->preview_height)) {
				ret = MM_ERROR_CAMCORDER_INTERNAL;
				goto _ERR_CAMCORDER_VIDEO_COMMAND;
			}

			ret = _mmcamcorder_gst_set_state(handle, pipeline, GST_STATE_PLAYING);
			if (ret != MM_ERROR_NONE)
				goto _ERR_CAMCORDER_VIDEO_COMMAND;
		}

		/* remove target file */
		if (info->filename) {
			_mmcam_dbg_log("file delete(%s)", info->filename);
			unlink(info->filename);
		}

		sc->isMaxsizePausing = FALSE;
		sc->isMaxtimePausing = FALSE;

		sc->display_interval = 0;
		sc->previous_slot_time = 0;
		info->video_frame_count = 0;
		info->audio_frame_count = 0;
		info->filesize = 0;
		hcamcorder->capture_in_recording = FALSE;
		break;
	}
	case _MMCamcorder_CMD_COMMIT:
	{
		guint64 free_space;

		if (info->b_commiting) {
			_mmcam_dbg_err("now on commiting previous file!!(command : %d)", command);
			return MM_ERROR_CAMCORDER_CMD_IS_RUNNING;
		} else {
			_mmcam_dbg_log("_MMCamcorder_CMD_COMMIT : start");
			info->b_commiting = TRUE;
			sc->bget_eos = FALSE;
		}

		for (count = 0 ; count <= _MMCAMCORDER_RETRIAL_COUNT ; count++) {
			if (sc->audio_disable) {
				/* check only video frame */
				if (info->video_frame_count >= _MMCAMCORDER_MINIMUM_FRAME &&
				    hcamcorder->capture_in_recording == FALSE) {
					break;
				} else if (count == _MMCAMCORDER_RETRIAL_COUNT) {
					_mmcam_dbg_err("Commit fail, frame count is %llu, capturing %d",
						info->video_frame_count, hcamcorder->capture_in_recording);

					if (info->video_frame_count >= _MMCAMCORDER_MINIMUM_FRAME) {
						_mmcam_dbg_warn("video frames are enough. keep going...");
					} else {
						info->b_commiting = FALSE;
						return MM_ERROR_CAMCORDER_INVALID_CONDITION;
					}
				} else {
					_mmcam_dbg_warn("Waiting for enough video frame, retrial [%d], frame %llu, capturing %d",
						count, info->video_frame_count, hcamcorder->capture_in_recording);
				}
			} else {
				/* check both of video and audio frame */
				if (info->video_frame_count >= _MMCAMCORDER_MINIMUM_FRAME &&
				    info->audio_frame_count &&
				    hcamcorder->capture_in_recording == FALSE) {
					break;
				} else if (count == _MMCAMCORDER_RETRIAL_COUNT) {
					_mmcam_dbg_err("Commit fail, VIDEO[%llu], AUDIO [%llu], capturing %d",
						info->video_frame_count, info->audio_frame_count, hcamcorder->capture_in_recording);

					if (info->video_frame_count >= _MMCAMCORDER_MINIMUM_FRAME && info->audio_frame_count) {
						_mmcam_dbg_warn("video/audio frames are enough. keep going...");
					} else {
						info->b_commiting = FALSE;
						return MM_ERROR_CAMCORDER_INVALID_CONDITION;
					}

					return MM_ERROR_CAMCORDER_INVALID_CONDITION;
				} else {
					_mmcam_dbg_warn("Waiting for enough frames, retrial [%d], VIDEO[%llu], AUDIO [%llu], capturing %d",
						count, info->video_frame_count, info->audio_frame_count, hcamcorder->capture_in_recording);
				}
			}

			if (hcamcorder->capture_in_recording) {
				gint64 end_time = g_get_monotonic_time() + (200 * G_TIME_SPAN_MILLISECOND);
				if (!_MMCAMCORDER_CMD_WAIT_UNTIL(handle, end_time))
					_mmcam_dbg_warn("timeout");
			} else {
				usleep(_MMCAMCORDER_FRAME_WAIT_TIME);
			}
		}

		/* block push buffer */
		info->push_encoding_buffer = PUSH_ENCODING_BUFFER_STOP;
		_mmcam_dbg_log("block push buffer to appsrc");

		_mmcamcorder_get_freespace(hcamcorder->storage_info.type, &free_space);
		if (free_space < _MMCAMCORDER_MINIMUM_SPACE) {
			_mmcam_dbg_warn("_MMCamcorder_CMD_COMMIT out of storage [%" G_GUINT64_FORMAT "]", free_space);
			ret = MM_ERROR_OUT_OF_STORAGE;
			goto _ERR_CAMCORDER_VIDEO_COMMAND;
		}

		if (sc->encode_element[_MMCAMCORDER_ENCSINK_SRC].gst != NULL) {
			if (gst_element_send_event(sc->encode_element[_MMCAMCORDER_ENCSINK_SRC].gst, gst_event_new_eos())) {
				_mmcam_dbg_warn("VIDEO: send eos to appsrc done");
			} else {
				_mmcam_dbg_err("VIDEO: send EOS failed");
				info->b_commiting = FALSE;
				ret = MM_ERROR_CAMCORDER_INTERNAL;
				goto _ERR_CAMCORDER_VIDEO_COMMAND;
			}
		} else {
			_mmcam_dbg_err("No video stream source");
			info->b_commiting = FALSE;
			ret = MM_ERROR_CAMCORDER_INTERNAL;
			goto _ERR_CAMCORDER_VIDEO_COMMAND;
		}

		if (sc->encode_element[_MMCAMCORDER_AUDIOSRC_SRC].gst != NULL) {
			if (gst_element_send_event(sc->encode_element[_MMCAMCORDER_AUDIOSRC_SRC].gst, gst_event_new_eos())) {
				_mmcam_dbg_warn("AUDIO: send eos to audiosrc done");
			} else {
				_mmcam_dbg_err("AUDIO: send EOS failed");
				info->b_commiting = FALSE;
				ret = MM_ERROR_CAMCORDER_INTERNAL;
				goto _ERR_CAMCORDER_VIDEO_COMMAND;
			}
		} else {
			_mmcam_dbg_log("No audio stream");
		}

		/* sc */
		sc->display_interval = 0;
		sc->previous_slot_time = 0;

		/* Wait EOS */
		_mmcam_dbg_log("Start to wait EOS");
		ret = _mmcamcorder_get_eos_message(handle);
		if (ret != MM_ERROR_NONE) {
			info->b_commiting = FALSE;
			goto _ERR_CAMCORDER_VIDEO_COMMAND;
		}

		/* reset flag */
		hcamcorder->capture_in_recording = FALSE;
	}
		break;
	default:
		ret =  MM_ERROR_CAMCORDER_INVALID_ARGUMENT;
		goto _ERR_CAMCORDER_VIDEO_COMMAND;
	}

	return MM_ERROR_NONE;

_ERR_CAMCORDER_VIDEO_COMMAND:
	if (command == _MMCamcorder_CMD_RECORD)
		_mmcamcorder_remove_recorder_pipeline((MMHandleType)hcamcorder);

	return ret;
}


int _mmcamcorder_video_handle_eos(MMHandleType handle)
{
	int ret = MM_ERROR_NONE;
	int enabletag = 0;
	guint64 file_size = 0;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;
	_MMCamcorderVideoInfo *info = NULL;
	_MMCamcorderMsgItem msg;
	MMCamRecordingReport *report = NULL;

	mmf_return_val_if_fail(hcamcorder, FALSE);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc, FALSE);
	mmf_return_val_if_fail(sc->info_video, FALSE);

	info = sc->info_video;

	_mmcam_dbg_err("");

	if (hcamcorder->state_change_by_system != _MMCAMCORDER_STATE_CHANGE_BY_FOCUS) {
		/* Play record stop sound */
		_mmcamcorder_sound_solo_play(handle, _MMCAMCORDER_SAMPLE_SOUND_NAME_REC_STOP, FALSE);
	} else {
		_mmcam_dbg_warn("Play stop sound through pulseaudio");

		_mmcamcorder_sound_init(handle);

		_mmcamcorder_sound_play((MMHandleType)hcamcorder, _MMCAMCORDER_SAMPLE_SOUND_NAME_REC_STOP, TRUE);

		_mmcamcorder_sound_finalize(handle);
	}

	/* remove blocking part */
	MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, "block", FALSE);

	mm_camcorder_get_attributes(handle, NULL,
		MMCAM_RECORDER_TAG_ENABLE, &enabletag,
		NULL);

	ret = _mmcamcorder_remove_recorder_pipeline((MMHandleType)hcamcorder);
	if (ret != MM_ERROR_NONE)
		_mmcam_dbg_warn("_MMCamcorder_CMD_COMMIT:__mmcamcorder_remove_recorder_pipeline failed. error[%x]", ret);

	/* set recording hint */
	MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst, "recording-hint", FALSE);

	/* stop video stream */
	if (info->record_dual_stream) {
		GstCameraControl *control = GST_CAMERA_CONTROL(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst);
		if (control) {
			MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_SINK].gst, "stop-video", TRUE);

			_mmcam_dbg_log("GST_CAMERA_CONTROL_RECORD_COMMAND_STOP");
			gst_camera_control_set_record_command(control, GST_CAMERA_CONTROL_RECORD_COMMAND_STOP);

			MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_SINK].gst, "stop-video", FALSE);
		} else {
			_mmcam_dbg_err("failed to get camera control");
		}
	}

	if (enabletag && !(sc->ferror_send)) {
		ret = __mmcamcorder_add_metadata((MMHandleType)hcamcorder, info->fileformat);
		_mmcam_dbg_log("Writing location information [%s] !!", ret ? "SUCCEEDED" : "FAILED");
	}

	/* Check file size */
	if (info->max_size > 0) {
		_mmcamcorder_get_file_size(info->filename, &file_size);
		_mmcam_dbg_log("MAX size %lld byte - created filesize %lld byte",
					   info->max_size, file_size);

		if (file_size > info->max_size) {
			_MMCamcorderMsgItem message;
			_mmcam_dbg_err("File size is greater than max size !!");
			message.id = MM_MESSAGE_CAMCORDER_ERROR;
			message.param.code = MM_ERROR_CAMCORDER_FILE_SIZE_OVER;
			_mmcamcorder_send_message((MMHandleType)hcamcorder, &message);
		}
	}

	if (info->restart_preview) {
		/* block queue */
		MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_QUE].gst, "empty-buffers", TRUE);
		MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_QUE].gst, "empty-buffers", TRUE);
		MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_SINK].gst, "keep-camera-preview", TRUE);

		_mmcam_dbg_log("Set state of pipeline as READY");
		ret = _mmcamcorder_gst_set_state(handle, sc->element[_MMCAMCORDER_MAIN_PIPE].gst, GST_STATE_READY);

		/* check decoder recreation */
		if (!_mmcamcorder_recreate_decoder_for_encoded_preview(handle)) {
			_mmcam_dbg_err("_mmcamcorder_recreate_decoder_for_encoded_preview failed");
			ret = MM_ERROR_CAMCORDER_INTERNAL;
		}

		/* unblock queue */
		MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_QUE].gst, "empty-buffers", FALSE);
		MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSRC_QUE].gst, "empty-buffers", FALSE);
		MMCAMCORDER_G_OBJECT_SET(sc->element[_MMCAMCORDER_VIDEOSINK_SINK].gst, "keep-camera-preview", FALSE);

		if (ret != MM_ERROR_NONE) {
			msg.id = MM_MESSAGE_CAMCORDER_ERROR;
			msg.param.code = ret;
			_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);
			_mmcam_dbg_err("Failed to set state READY[%x]", ret);
		}

		/* reset restart_preview for inset window layout */
		info->restart_preview = FALSE;

		/* recover preview size */
		if (!_mmcamcorder_set_camera_resolution(handle, info->preview_width, info->preview_height)) {
			msg.id = MM_MESSAGE_CAMCORDER_ERROR;
			msg.param.code = MM_ERROR_CAMCORDER_INTERNAL;
			_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);
			_mmcam_dbg_err("Failed to set camera resolution %dx%d",
				info->preview_width, info->preview_height);
		}

		ret = _mmcamcorder_gst_set_state(handle, sc->element[_MMCAMCORDER_MAIN_PIPE].gst, GST_STATE_PLAYING);
		/* Do not return when error is occurred.
		   Recording file was created successfully, but starting pipeline failed */
		if (ret != MM_ERROR_NONE) {
			msg.id = MM_MESSAGE_CAMCORDER_ERROR;
			msg.param.code = ret;
			_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);
			_mmcam_dbg_err("Failed to set state PLAYING[%x]", ret);
		}
	} else {
		_mmcam_dbg_log("No need to restart preview");
	}

	/* Send recording report to application */
	msg.id = MM_MESSAGE_CAMCORDER_VIDEO_CAPTURED;
	report = (MMCamRecordingReport *)g_malloc(sizeof(MMCamRecordingReport));
	if (!report) {
		_mmcam_dbg_err("Recording report fail(%s). Out of memory.", info->filename);
	} else {
		report->recording_filename = g_strdup(info->filename);
		msg.param.data = report;
		msg.param.code = 1;
		_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);
	}

	/* Finishing */
	sc->pipeline_time = 0;
	sc->pause_time = 0;
	sc->isMaxsizePausing = FALSE; /*In async function, this variable should set in callback function. */
	sc->isMaxtimePausing = FALSE;
	hcamcorder->error_occurs = FALSE;

	info->video_frame_count = 0;
	info->audio_frame_count = 0;
	info->filesize = 0;
	info->b_commiting = FALSE;

	if (hcamcorder->state_change_by_system != _MMCAMCORDER_STATE_CHANGE_BY_FOCUS) {
		/* check recording stop sound */
		_mmcamcorder_sound_solo_play_wait(handle);
	}

	_mmcam_dbg_err("_MMCamcorder_CMD_COMMIT : end");

	return TRUE;
}


static GstPadProbeReturn __mmcamcorder_audio_dataprobe_check(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(u_data);
	GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
	GstMapInfo mapinfo;
	_MMCamcorderSubContext *sc = NULL;
	_MMCamcorderVideoInfo *videoinfo = NULL;
	_MMCamcorderMsgItem msg;
	guint64 buffer_size = 0;
	guint64 trailer_size = 0;
	guint64 max_size = 0;

	mmf_return_val_if_fail(hcamcorder, GST_PAD_PROBE_OK);
	mmf_return_val_if_fail(buffer, GST_PAD_PROBE_DROP);
	sc = MMF_CAMCORDER_SUBCONTEXT(hcamcorder);

	mmf_return_val_if_fail(sc && sc->info_video, GST_PAD_PROBE_OK);
	videoinfo = sc->info_video;

	/* get buffer size */
	if (!gst_buffer_map(buffer, &mapinfo, GST_MAP_READ)) {
		_mmcam_dbg_warn("map failed : buffer %p", buffer);
		return GST_PAD_PROBE_OK;
	}

	buffer_size = mapinfo.size;
	gst_buffer_unmap(buffer, &mapinfo);

	/*_mmcam_dbg_err("[%" GST_TIME_FORMAT "]", GST_TIME_ARGS(GST_BUFFER_PTS(buffer)));*/

	g_mutex_lock(&videoinfo->size_check_lock);

	if (videoinfo->audio_frame_count == 0) {
		videoinfo->filesize += buffer_size;
		videoinfo->audio_frame_count++;
		g_mutex_unlock(&videoinfo->size_check_lock);
		return GST_PAD_PROBE_OK;
	}

	if (sc->ferror_send || sc->isMaxsizePausing) {
		_mmcam_dbg_warn("Recording is paused, drop frames");
		g_mutex_unlock(&videoinfo->size_check_lock);
		return GST_PAD_PROBE_DROP;
	}

	/* get trailer size */
	if (videoinfo->fileformat == MM_FILE_FORMAT_3GP || videoinfo->fileformat == MM_FILE_FORMAT_MP4)
		MMCAMCORDER_G_OBJECT_GET(sc->encode_element[_MMCAMCORDER_ENCSINK_MUX].gst, "expected-trailer-size", &trailer_size);
	else
		trailer_size = 0;

	/* check max size of recorded file */
	max_size = videoinfo->filesize + buffer_size + trailer_size + _MMCAMCORDER_MMS_MARGIN_SPACE;
	if (videoinfo->max_size > 0 && videoinfo->max_size < max_size) {
		GstState pipeline_state = GST_STATE_VOID_PENDING;
		GstElement *pipeline = sc->element[_MMCAMCORDER_MAIN_PIPE].gst;
		_mmcam_dbg_warn("Max size!!! Recording is paused.");
		_mmcam_dbg_warn("Max size : [%" G_GUINT64_FORMAT "], current file size : [%" G_GUINT64_FORMAT "]," \
			" buffer size : [%" G_GUINT64_FORMAT "], trailer size : [%" G_GUINT64_FORMAT "]",
			videoinfo->max_size, videoinfo->filesize, buffer_size, trailer_size);

		if (!sc->isMaxsizePausing) {
			sc->isMaxsizePausing = TRUE;
			gst_element_get_state(pipeline, &pipeline_state, NULL, -1) ;
			if (pipeline_state == GST_STATE_PLAYING)
				MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, "block", TRUE);

			msg.id = MM_MESSAGE_CAMCORDER_MAX_SIZE;
			_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);
		}

		g_mutex_unlock(&videoinfo->size_check_lock);

		return FALSE;
	}

	videoinfo->filesize += buffer_size;
	videoinfo->audio_frame_count++;

	g_mutex_unlock(&videoinfo->size_check_lock);

	return GST_PAD_PROBE_OK;
}


static GstPadProbeReturn __mmcamcorder_video_dataprobe_record(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
	gint ret = 0;
	guint vq_size = 0;
	guint aq_size = 0;
	guint64 free_space = 0;
	guint64 buffer_size = 0;
	guint64 trailer_size = 0;
	guint64 queued_buffer = 0;
	guint64 max_size = 0;
	GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
	GstMapInfo mapinfo;
	storage_state_e storage_state = STORAGE_STATE_UNMOUNTABLE;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(u_data);
	_MMCamcorderMsgItem msg;
	_MMCamcorderSubContext *sc = NULL;
	_MMCamcorderVideoInfo *videoinfo = NULL;

	mmf_return_val_if_fail(hcamcorder, GST_PAD_PROBE_OK);
	mmf_return_val_if_fail(buffer, GST_PAD_PROBE_DROP);

	sc = MMF_CAMCORDER_SUBCONTEXT(hcamcorder);
	mmf_return_val_if_fail(sc && sc->info_video, GST_PAD_PROBE_OK);
	videoinfo = sc->info_video;

	/*_mmcam_dbg_log("[%" GST_TIME_FORMAT "]", GST_TIME_ARGS(GST_BUFFER_PTS(buffer)));*/
	if (sc->ferror_send) {
		_mmcam_dbg_warn("file write error, drop frames");
		return GST_PAD_PROBE_DROP;
	}

	gst_buffer_map(buffer, &mapinfo, GST_MAP_READ);
	buffer_size = mapinfo.size;
	gst_buffer_unmap(buffer, &mapinfo);

	videoinfo->video_frame_count++;
	if (videoinfo->video_frame_count <= (guint64)_MMCAMCORDER_MINIMUM_FRAME) {
		/* _mmcam_dbg_log("Pass minimum frame: info->video_frame_count: %" G_GUINT64_FORMAT " ",
			info->video_frame_count); */
		g_mutex_lock(&videoinfo->size_check_lock);
		videoinfo->filesize += buffer_size;
		g_mutex_unlock(&videoinfo->size_check_lock);
		return GST_PAD_PROBE_OK;
	}

	/* get trailer size */
	if (videoinfo->fileformat == MM_FILE_FORMAT_3GP || videoinfo->fileformat == MM_FILE_FORMAT_MP4)
		MMCAMCORDER_G_OBJECT_GET(sc->encode_element[_MMCAMCORDER_ENCSINK_MUX].gst, "expected-trailer-size", &trailer_size);
	else
		trailer_size = 0;

	/* check free space */
	ret = _mmcamcorder_get_freespace(hcamcorder->storage_info.type, &free_space);
	if (ret != 0) {
		_mmcam_dbg_err("Error occured. [%d]", ret);
		if (sc->ferror_count == 2 && sc->ferror_send == FALSE) {
			sc->ferror_send = TRUE;

			msg.id = MM_MESSAGE_CAMCORDER_ERROR;
			msg.param.code = MM_ERROR_FILE_READ;

			_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);
		} else {
			sc->ferror_count++;
		}

		return GST_PAD_PROBE_DROP; /* skip this buffer */
	}

	if (free_space == 0) {
		/* check storage state */
		storage_get_state(hcamcorder->storage_info.id, &storage_state);

		_mmcam_dbg_warn("storage state %d", storage_state);

		if (storage_state == STORAGE_STATE_REMOVED ||
			storage_state == STORAGE_STATE_UNMOUNTABLE) {
			_mmcam_dbg_err("storage was removed!");

			_MMCAMCORDER_LOCK(hcamcorder);

			if (sc->ferror_send == FALSE) {
				_mmcam_dbg_err("OUT_OF_STORAGE error");

				sc->ferror_send = TRUE;

				_MMCAMCORDER_UNLOCK(hcamcorder);

				msg.id = MM_MESSAGE_CAMCORDER_ERROR;
				msg.param.code = MM_ERROR_OUT_OF_STORAGE;

				_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);
			} else {
				_MMCAMCORDER_UNLOCK(hcamcorder);
				_mmcam_dbg_warn("error was already sent");
			}

			return GST_PAD_PROBE_DROP;
		}
	}

	/* get queued buffer size */
	if (sc->encode_element[_MMCAMCORDER_ENCSINK_AENC_QUE].gst)
		MMCAMCORDER_G_OBJECT_GET(sc->encode_element[_MMCAMCORDER_ENCSINK_AENC_QUE].gst, "current-level-bytes", &aq_size);

	if (sc->encode_element[_MMCAMCORDER_ENCSINK_VENC_QUE].gst)
		MMCAMCORDER_G_OBJECT_GET(sc->encode_element[_MMCAMCORDER_ENCSINK_VENC_QUE].gst, "current-level-bytes", &vq_size);

	queued_buffer = aq_size + vq_size;

	if (free_space < (_MMCAMCORDER_MINIMUM_SPACE + buffer_size + trailer_size + queued_buffer)) {
		_mmcam_dbg_warn("No more space for recording!!! Recording is paused.");
		_mmcam_dbg_warn("Free Space : [%" G_GUINT64_FORMAT "], trailer size : [%" G_GUINT64_FORMAT "]," \
			" buffer size : [%" G_GUINT64_FORMAT "], queued buffer size : [%" G_GUINT64_FORMAT "]", \
			free_space, trailer_size, buffer_size, queued_buffer);

		if (!sc->isMaxsizePausing) {
			MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, "block", TRUE);
			sc->isMaxsizePausing = TRUE;

			msg.id = MM_MESSAGE_CAMCORDER_NO_FREE_SPACE;
			_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);
		}

		return GST_PAD_PROBE_DROP;
	}

	g_mutex_lock(&videoinfo->size_check_lock);

	/* check max size of recorded file */
	max_size = videoinfo->filesize + buffer_size + trailer_size + _MMCAMCORDER_MMS_MARGIN_SPACE;
	if (videoinfo->max_size > 0 && videoinfo->max_size < max_size) {
		GstState pipeline_state = GST_STATE_VOID_PENDING;
		GstElement *pipeline = sc->element[_MMCAMCORDER_MAIN_PIPE].gst;
		_mmcam_dbg_warn("Max size!!! Recording is paused.");
		_mmcam_dbg_warn("Max size : [%" G_GUINT64_FORMAT "], current file size : [%" G_GUINT64_FORMAT "]," \
			" buffer size : [%" G_GUINT64_FORMAT "], trailer size : [%" G_GUINT64_FORMAT "]",
			videoinfo->max_size, videoinfo->filesize, buffer_size, trailer_size);

		if (!sc->isMaxsizePausing) {
			sc->isMaxsizePausing = TRUE;
			gst_element_get_state(pipeline, &pipeline_state, NULL, -1) ;
			if (pipeline_state == GST_STATE_PLAYING)
				MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, "block", TRUE);

			msg.id = MM_MESSAGE_CAMCORDER_MAX_SIZE;
			_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);
		}

		g_mutex_unlock(&videoinfo->size_check_lock);

		return GST_PAD_PROBE_DROP;
	}

	videoinfo->filesize += (guint64)buffer_size;

	/*
	_mmcam_dbg_log("filesize %lld Byte, ", videoinfo->filesize);
	*/

	g_mutex_unlock(&videoinfo->size_check_lock);

	return GST_PAD_PROBE_OK;
}


static GstPadProbeReturn __mmcamcorder_video_dataprobe_audio_disable(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
	guint64 trailer_size = 0;
	guint64 rec_pipe_time = 0;
	unsigned int remained_time = 0;

	GstClockTime b_time;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(u_data);
	_MMCamcorderMsgItem msg;
	_MMCamcorderSubContext *sc = NULL;
	_MMCamcorderVideoInfo *videoinfo = NULL;

	GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);

	mmf_return_val_if_fail(buffer, GST_PAD_PROBE_DROP);
	mmf_return_val_if_fail(hcamcorder, GST_PAD_PROBE_OK);

	sc = MMF_CAMCORDER_SUBCONTEXT(u_data);
	mmf_return_val_if_fail(sc, GST_PAD_PROBE_OK);
	mmf_return_val_if_fail(sc->info_video, GST_PAD_PROBE_OK);

	videoinfo = sc->info_video;

	b_time = GST_BUFFER_PTS(buffer);

	rec_pipe_time = GST_TIME_AS_MSECONDS(b_time);

	if (videoinfo->fileformat == MM_FILE_FORMAT_3GP || videoinfo->fileformat == MM_FILE_FORMAT_MP4)
		MMCAMCORDER_G_OBJECT_GET(sc->encode_element[_MMCAMCORDER_ENCSINK_MUX].gst, "expected-trailer-size", &trailer_size);
	else
		trailer_size = 0;

	/* check max time */
	if (videoinfo->max_time > 0 && rec_pipe_time > videoinfo->max_time) {
		_mmcam_dbg_warn("Time current [%" G_GUINT64_FORMAT "], Max [%" G_GUINT64_FORMAT "], motion rate [%lf]", \
			rec_pipe_time, videoinfo->max_time, videoinfo->record_motion_rate);

		if (!sc->isMaxtimePausing) {
			MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, "block", TRUE);

			sc->isMaxtimePausing = TRUE;

			msg.id = MM_MESSAGE_CAMCORDER_RECORDING_STATUS;
			msg.param.recording_status.elapsed = (unsigned long long)rec_pipe_time;
			msg.param.recording_status.filesize = (unsigned long long)((videoinfo->filesize + trailer_size) >> 10);
			msg.param.recording_status.remained_time = 0;
			_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);

			msg.id = MM_MESSAGE_CAMCORDER_TIME_LIMIT;
			_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);
		}

		return GST_PAD_PROBE_DROP;
	}

	/* calculate remained time can be recorded */
	if (videoinfo->max_time > 0 && videoinfo->max_time < (remained_time + rec_pipe_time)) {
		remained_time = videoinfo->max_time - rec_pipe_time;
	} else if (videoinfo->max_size > 0) {
		long double max_size = (long double)videoinfo->max_size;
		long double current_size = (long double)(videoinfo->filesize + trailer_size);

		remained_time = (unsigned int)((long double)rec_pipe_time * (max_size/current_size)) - rec_pipe_time;
	}

	msg.id = MM_MESSAGE_CAMCORDER_RECORDING_STATUS;
	msg.param.recording_status.elapsed = (unsigned long long)rec_pipe_time;
	msg.param.recording_status.filesize = (unsigned long long)((videoinfo->filesize + trailer_size) >> 10);
	msg.param.recording_status.remained_time = remained_time;
	_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);

	/*
	_mmcam_dbg_log("time [%" GST_TIME_FORMAT "], size [%d]",
		GST_TIME_ARGS(rec_pipe_time), msg.param.recording_status.filesize);
	*/

	if (videoinfo->record_timestamp_ratio != _MMCAMCORDER_DEFAULT_RECORDING_MOTION_RATE) {
		guint record_motion_rate = (guint)videoinfo->record_motion_rate;

		/*
		_mmcam_dbg_log("record_motion_rate %d, videoinfo->record_drop_count %d",
			record_motion_rate, videoinfo->record_drop_count);
		*/

		/* drop some frame if fast motion */
		if (videoinfo->record_motion_rate > _MMCAMCORDER_DEFAULT_RECORDING_MOTION_RATE) {
			if (record_motion_rate != (videoinfo->record_drop_count++)) {
				/*
				_mmcam_dbg_warn("drop frame");
				*/
				return GST_PAD_PROBE_DROP;
			}

			videoinfo->record_drop_count = 1;
			/*
			_mmcam_dbg_warn("pass frame");
			*/
		}

		GST_BUFFER_PTS(buffer) = b_time * (videoinfo->record_timestamp_ratio);
		GST_BUFFER_DTS(buffer) = GST_BUFFER_PTS(buffer);
	}

	return GST_PAD_PROBE_OK;
}


static GstPadProbeReturn __mmcamcorder_audioque_dataprobe(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
	_MMCamcorderMsgItem msg;
	guint64 trailer_size = 0;
	guint64 rec_pipe_time = 0;
	_MMCamcorderSubContext *sc = NULL;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(u_data);
	_MMCamcorderVideoInfo *videoinfo = NULL;
	unsigned int remained_time = 0;
	GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);

	mmf_return_val_if_fail(buffer, GST_PAD_PROBE_DROP);
	mmf_return_val_if_fail(hcamcorder, GST_PAD_PROBE_OK);
	sc = MMF_CAMCORDER_SUBCONTEXT(u_data);

	mmf_return_val_if_fail(sc, GST_PAD_PROBE_OK);
	mmf_return_val_if_fail(sc->info_video, GST_PAD_PROBE_OK);
	mmf_return_val_if_fail(sc->element, GST_PAD_PROBE_OK);

	videoinfo = sc->info_video;

	if (!GST_CLOCK_TIME_IS_VALID(GST_BUFFER_PTS(buffer))) {
		_mmcam_dbg_err("Buffer timestamp is invalid, check it");
		return GST_PAD_PROBE_OK;
	}

	rec_pipe_time = GST_TIME_AS_MSECONDS(GST_BUFFER_PTS(buffer));

	if (videoinfo->fileformat == MM_FILE_FORMAT_3GP || videoinfo->fileformat == MM_FILE_FORMAT_MP4)
		MMCAMCORDER_G_OBJECT_GET(sc->encode_element[_MMCAMCORDER_ENCSINK_MUX].gst, "expected-trailer-size", &trailer_size);
	else
		trailer_size = 0;

	/* calculate remained time can be recorded */
	if (videoinfo->max_time > 0 && videoinfo->max_time < (remained_time + rec_pipe_time)) {
		remained_time = videoinfo->max_time - rec_pipe_time;
	} else if (videoinfo->max_size > 0) {
		long double max_size = (long double)videoinfo->max_size;
		long double current_size = (long double)(videoinfo->filesize + trailer_size);

		remained_time = (unsigned long long)((long double)rec_pipe_time * (max_size/current_size)) - rec_pipe_time;
	}

	if (videoinfo->max_time > 0 && rec_pipe_time > videoinfo->max_time) {
		_mmcam_dbg_warn("Time current [%" G_GUINT64_FORMAT "], Max [%" G_GUINT64_FORMAT "], motion rate [%lf]", \
			rec_pipe_time, videoinfo->max_time, videoinfo->record_motion_rate);

		if (!sc->isMaxtimePausing) {
			MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, "block", TRUE);

			sc->isMaxtimePausing = TRUE;

			msg.id = MM_MESSAGE_CAMCORDER_RECORDING_STATUS;
			msg.param.recording_status.elapsed = (unsigned long long)rec_pipe_time;
			msg.param.recording_status.filesize = (unsigned long long)((videoinfo->filesize + trailer_size) >> 10);
			msg.param.recording_status.remained_time = 0;
			_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);

			msg.id = MM_MESSAGE_CAMCORDER_TIME_LIMIT;
			_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);
		}

		return GST_PAD_PROBE_DROP;
	}

	msg.id = MM_MESSAGE_CAMCORDER_RECORDING_STATUS;
	msg.param.recording_status.elapsed = (unsigned long long)rec_pipe_time;
	msg.param.recording_status.filesize = (unsigned long long)((videoinfo->filesize + trailer_size) >> 10);
	msg.param.recording_status.remained_time = remained_time;
	_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);

	/*
	_mmcam_dbg_log("audio data probe :: time [%" GST_TIME_FORMAT "], size [%lld KB]",
		GST_TIME_ARGS(rec_pipe_time), msg.param.recording_status.filesize);
	*/

	return GST_PAD_PROBE_OK;
}


static GstPadProbeReturn __mmcamcorder_audio_dataprobe_audio_mute(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(u_data);
	double volume = 0.0;
	int format = 0;
	int channel = 0;
	int err = MM_ERROR_UNKNOWN;
	char *err_name = NULL;
	GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
	GstMapInfo mapinfo;

	mmf_return_val_if_fail(buffer, GST_PAD_PROBE_DROP);
	mmf_return_val_if_fail(hcamcorder, GST_PAD_PROBE_DROP);

	/*_mmcam_dbg_log("AUDIO SRC time stamp : [%" GST_TIME_FORMAT "] \n", GST_TIME_ARGS(GST_BUFFER_PTS(buffer)));*/
	err = mm_camcorder_get_attributes((MMHandleType)hcamcorder, &err_name,
		MMCAM_AUDIO_VOLUME, &volume,
		MMCAM_AUDIO_FORMAT, &format,
		MMCAM_AUDIO_CHANNEL, &channel,
		NULL);
	if (err != MM_ERROR_NONE) {
		_mmcam_dbg_warn("Get attrs fail. (%s:%x)", err_name, err);
		SAFE_FREE(err_name);
		return err;
	}

	memset(&mapinfo, 0x0, sizeof(GstMapInfo));

	gst_buffer_map(buffer, &mapinfo, GST_MAP_READWRITE);

	/* Set audio stream NULL */
	if (volume == 0.0)
		memset(mapinfo.data, 0, mapinfo.size);

	/* CALL audio stream callback */
	if (hcamcorder->astream_cb && buffer && mapinfo.data && mapinfo.size > 0) {
		MMCamcorderAudioStreamDataType stream;

		if (_mmcamcorder_get_state((MMHandleType)hcamcorder) < MM_CAMCORDER_STATE_PREPARE) {
			_mmcam_dbg_warn("Not ready for stream callback");
			gst_buffer_unmap(buffer, &mapinfo);
			return GST_PAD_PROBE_OK;
		}

		/*_mmcam_dbg_log("Call video steramCb, data[%p], Width[%d],Height[%d], Format[%d]",
			GST_BUFFER_DATA(buffer), width, height, format);*/

		stream.data = (void *)mapinfo.data;
		stream.format = format;
		stream.channel = channel;
		stream.length = mapinfo.size;
		stream.timestamp = (unsigned int)(GST_BUFFER_PTS(buffer)/1000000); /* nano -> milli second */

		_MMCAMCORDER_LOCK_ASTREAM_CALLBACK(hcamcorder);

		if (hcamcorder->astream_cb)
			hcamcorder->astream_cb(&stream, hcamcorder->astream_cb_param);

		_MMCAMCORDER_UNLOCK_ASTREAM_CALLBACK(hcamcorder);
	}

	gst_buffer_unmap(buffer, &mapinfo);
	return GST_PAD_PROBE_OK;
}


static gboolean __mmcamcorder_add_metadata(MMHandleType handle, int fileformat)
{
	gboolean bret = FALSE;

	switch (fileformat) {
	case MM_FILE_FORMAT_3GP:
	case MM_FILE_FORMAT_MP4:
		bret = __mmcamcorder_add_metadata_mp4(handle);
		break;
	default:
		_mmcam_dbg_warn("Unsupported fileformat to insert location info (%d)", fileformat);
		break;
	}

	return bret;
}


static gboolean __mmcamcorder_add_metadata_mp4(MMHandleType handle)
{
	FILE *f = NULL;
	guchar buf[4];
	guint64 udta_size = 0;
	gint64 current_pos = 0;
	gint64 moov_pos = 0;
	gint64 udta_pos = 0;
	gdouble longitude = 0;
	gdouble latitude = 0;
	gdouble altitude = 0;
	int err = 0;
	int orientation = 0;
	int gps_enable = 0;
	char *err_name = NULL;
	char err_msg[MAX_ERROR_MESSAGE_LEN] = {'\0',};
	_MMCamcorderLocationInfo location_info = {0, 0, 0};
	_MMCamcorderLocationInfo geo_info = {0, 0, 0};

	_MMCamcorderVideoInfo *info = NULL;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	sc = MMF_CAMCORDER_SUBCONTEXT(handle);

	mmf_return_val_if_fail(sc, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	mmf_return_val_if_fail(sc->info_video, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	_mmcam_dbg_log("");

	info = sc->info_video;

	f = fopen64(info->filename, "rb+");
	if (f == NULL) {
		strerror_r(errno, err_msg, MAX_ERROR_MESSAGE_LEN);
		_mmcam_dbg_err("file open failed [%s]", err_msg);
		return FALSE;
	}

	mm_camcorder_get_attributes(handle, &err_name,
		MMCAM_TAG_LATITUDE, &latitude,
		MMCAM_TAG_LONGITUDE, &longitude,
		MMCAM_TAG_ALTITUDE, &altitude,
		MMCAM_TAG_VIDEO_ORIENTATION, &orientation,
		MMCAM_TAG_GPS_ENABLE, &gps_enable,
		NULL);
	if (err_name) {
		_mmcam_dbg_warn("Get tag attrs fail. (%s:%x)", err_name, err);
		SAFE_FREE(err_name);
	}

	location_info.longitude = _mmcamcorder_double_to_fix(longitude);
	location_info.latitude = _mmcamcorder_double_to_fix(latitude);
	location_info.altitude = _mmcamcorder_double_to_fix(altitude);
	geo_info.longitude = longitude *10000;
	geo_info.latitude = latitude *10000;
	geo_info.altitude = altitude *10000;
	/* find udta container.
	   if, there are udta container, write loci box after that
	   else, make udta container and write loci box. */
	if (_mmcamcorder_find_fourcc(f, MMCAM_FOURCC('u', 'd', 't', 'a'), TRUE)) {
		size_t nread = 0;

		_mmcam_dbg_log("find udta container");

		/* read size */
		if (fseek(f, -8L, SEEK_CUR) != 0)
			goto fail;

		udta_pos = ftello(f);
		if (udta_pos < 0)
			goto ftell_fail;

		nread = fread(&buf, sizeof(char), sizeof(buf), f);

		_mmcam_dbg_log("recorded file fread %d", nread);

		udta_size = _mmcamcorder_get_container_size(buf);

		/* goto end of udta and write 'loci' box */
		if (fseek(f, (udta_size-4L), SEEK_CUR) != 0)
			goto fail;

		if (gps_enable) {
			if (!_mmcamcorder_write_loci(f, location_info)) {
				_mmcam_dbg_err("failed to write loci");
				goto fail;
			}

			if (!_mmcamcorder_write_geodata(f, geo_info)) {
				_mmcam_dbg_err("failed to write geodata");
				goto fail;
			}
		}

		current_pos = ftello(f);
		if (current_pos < 0)
			goto ftell_fail;

		if (!_mmcamcorder_update_size(f, udta_pos, current_pos))
			goto fail;
	} else {
		_mmcam_dbg_log("No udta container");
		if (fseek(f, 0, SEEK_END) != 0)
			goto fail;

		if (!_mmcamcorder_write_udta(f, gps_enable, location_info, geo_info)) {
			_mmcam_dbg_err("failed to write udta");
			goto fail;
		}
	}

	/* find moov container.
	   update moov container size. */
	if ((current_pos = ftello(f)) < 0)
		goto ftell_fail;

	if (_mmcamcorder_find_tag(f, MMCAM_FOURCC('m', 'o', 'o', 'v'), TRUE)) {
		gint64 internal_pos = ftello(f);

		_mmcam_dbg_log("found moov container");
		if (fseek(f, -8L, SEEK_CUR) != 0)
			goto fail;

		moov_pos = ftello(f);
		if (moov_pos < 0)
			goto ftell_fail;

		if (!_mmcamcorder_update_size(f, moov_pos, current_pos))
			goto fail;

		/* add orientation info */
		if (fseeko(f, internal_pos, SEEK_SET) < 0) {
			_mmcam_dbg_err("fseeko failed : errno %d", errno);
			goto fail;
		}

		if (!_mmcamcorder_find_tag(f, MMCAM_FOURCC('t', 'r', 'a', 'k'), FALSE)) {
			_mmcam_dbg_err("failed to find [trak] tag");
			goto fail;
		}

		if (!_mmcamcorder_find_tag(f, MMCAM_FOURCC('t', 'k', 'h', 'd'), FALSE)) {
			_mmcam_dbg_err("failed to find [tkhd] tag");
			goto fail;
		}

		_mmcam_dbg_log("found [tkhd] tag");

		/* seek to start position of composition matrix */
		if (fseek(f, _OFFSET_COMPOSITION_MATRIX, SEEK_CUR) == 0) {
			/* update composition matrix for orientation */
			_mmcamcorder_update_composition_matrix(f, orientation);
		} else {
			_mmcam_dbg_err("fseek failed : errno %d", errno);
			goto fail;
		}
	} else {
		_mmcam_dbg_err("No 'moov' container");
		goto fail;
	}

	fclose(f);
	return TRUE;

fail:
	fclose(f);
	return FALSE;

ftell_fail:
	_mmcam_dbg_err("ftell() returns negative value.");
	fclose(f);
	return FALSE;
}


int _mmcamcorder_connect_video_stream_cb_signal(MMHandleType handle)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc && sc->element, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	/* check video source element */
	if (sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst) {
		_mmcam_dbg_warn("connect video stream cb signal to _MMCAMCORDER_VIDEOSRC_SRC");
		MMCAMCORDER_SIGNAL_CONNECT(sc->element[_MMCAMCORDER_VIDEOSRC_SRC].gst,
			_MMCAMCORDER_HANDLER_VIDEOREC, "video-stream-cb",
			G_CALLBACK(__mmcamcorder_video_stream_cb),
			hcamcorder);
		return MM_ERROR_NONE;
	} else {
		_mmcam_dbg_err("videosrc element is not created yet");
		return MM_ERROR_CAMCORDER_NOT_INITIALIZED;
	}
}


int _mmcamcorder_video_prepare_record(MMHandleType handle)
{
	int ret = MM_ERROR_NONE;
	int size = 0;
	char *temp_filename = NULL;

	_MMCamcorderVideoInfo *info = NULL;
	_MMCamcorderSubContext *sc = NULL;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	sc = MMF_CAMCORDER_SUBCONTEXT(handle);
	mmf_return_val_if_fail(sc, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	mmf_return_val_if_fail(sc->info_video, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	info = sc->info_video;

	_mmcam_dbg_warn("start");

	/* create encoding pipeline */
	ret = _mmcamcorder_create_recorder_pipeline((MMHandleType)hcamcorder);
	if (ret != MM_ERROR_NONE)
		goto _ERR_PREPARE_RECORD;

	SAFE_G_FREE(info->filename);

	mm_camcorder_get_attributes(handle, NULL,
		MMCAM_TARGET_FILENAME, &temp_filename, &size,
		NULL);
	if (temp_filename) {
		info->filename = g_strdup(temp_filename);
		if (!info->filename) {
			_mmcam_dbg_err("strdup[src:%p] was failed", temp_filename);
			goto _ERR_PREPARE_RECORD;
		}

		_mmcam_dbg_log("Record file name [%s]", info->filename);
		MMCAMCORDER_G_OBJECT_SET_POINTER(sc->encode_element[_MMCAMCORDER_ENCSINK_SINK].gst, "location", info->filename);
	} else {
		_mmcam_dbg_log("Recorded data will be written in [%s]", _MMCamcorder_FILENAME_NULL);
		MMCAMCORDER_G_OBJECT_SET_POINTER(sc->encode_element[_MMCAMCORDER_ENCSINK_SINK].gst, "location", _MMCamcorder_FILENAME_NULL);
	}

	MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, "block", 0);

	/* Adjust display FPS */
	sc->display_interval = 0;
	sc->previous_slot_time = 0;

	ret = _mmcamcorder_gst_set_state(handle, sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst, GST_STATE_PAUSED);
	if (ret != MM_ERROR_NONE)
		goto _ERR_PREPARE_RECORD;

	_mmcam_dbg_warn("done");

	return ret;

_ERR_PREPARE_RECORD:
	/* Remove recorder pipeline and recording file which size maybe zero */
	_mmcamcorder_remove_recorder_pipeline((MMHandleType)hcamcorder);
	if (info && info->filename) {
		_mmcam_dbg_log("file delete(%s)", info->filename);
		unlink(info->filename);
	}

	return ret;
}
