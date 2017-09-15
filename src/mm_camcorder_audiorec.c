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
#include "mm_camcorder_internal.h"
#include "mm_camcorder_audiorec.h"
#include "mm_camcorder_util.h"
#include <math.h>

/*---------------------------------------------------------------------------------------
|    GLOBAL VARIABLE DEFINITIONS for internal						|
---------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------
|    LOCAL VARIABLE DEFINITIONS for internal						|
---------------------------------------------------------------------------------------*/
#define RESET_PAUSE_TIME                        0
#define _MMCAMCORDER_AUDIO_MINIMUM_SPACE        ((100*1024) + (5*1024))
#define _MMCAMCORDER_RETRIAL_COUNT              10
#define _MMCAMCORDER_FRAME_WAIT_TIME            200000 /* micro second */
#define _MMCAMCORDER_FREE_SPACE_CHECK_INTERVAL  10

/*---------------------------------------------------------------------------------------
|    LOCAL FUNCTION PROTOTYPES:								|
---------------------------------------------------------------------------------------*/
/* STATIC INTERNAL FUNCTION */
static GstPadProbeReturn __mmcamcorder_audio_dataprobe_voicerecorder(GstPad *pad, GstPadProbeInfo *info, gpointer u_data);
static GstPadProbeReturn __mmcamcorder_audio_dataprobe_record(GstPad *pad, GstPadProbeInfo *info, gpointer u_data);
static int __mmcamcorder_create_audiop_with_encodebin(MMHandleType handle);
static gboolean __mmcamcorder_audio_add_metadata_info_m4a(MMHandleType handle);

/*=======================================================================================
|  FUNCTION DEFINITIONS									|
=======================================================================================*/

/*---------------------------------------------------------------------------------------
|    GLOBAL FUNCTION DEFINITIONS:							|
---------------------------------------------------------------------------------------*/

static int __mmcamcorder_create_audiop_with_encodebin(MMHandleType handle)
{
	int err = MM_ERROR_NONE;
	int file_name_len = 0;

	char *file_name = NULL;
	const char *aenc_name = NULL;
	const char *mux_name = NULL;
	const char *sink_name = NULL;

	GstBus *bus = NULL;
	GstPad *srcpad = NULL;
	GstPad *sinkpad = NULL;
	GList *element_list = NULL;

	_MMCamcorderAudioInfo *info = NULL;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;
	type_element *aenc_elem = NULL;
	type_element *mux_elem = NULL;
	type_element *sink_elem = NULL;

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	sc = MMF_CAMCORDER_SUBCONTEXT(handle);

	mmf_return_val_if_fail(sc, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	mmf_return_val_if_fail(sc->element, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	mmf_return_val_if_fail(sc->info_audio, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	info = (_MMCamcorderAudioInfo *)sc->info_audio;

	_mmcam_dbg_log("");

	mux_elem = _mmcamcorder_get_type_element(handle, MM_CAM_FILE_FORMAT);
	err = _mmcamcorder_conf_get_value_element_name(mux_elem, &mux_name);

	if (!mux_name || !strcmp(mux_name, "wavenc")) {
		/* IF MUX in not chosen then record in raw file */
		_mmcam_dbg_log("Record without muxing.");
		info->bMuxing = FALSE;
	} else {
		_mmcam_dbg_log("Record with mux.");
		info->bMuxing = TRUE;
	}

	/* Create GStreamer pipeline */
	_MMCAMCORDER_PIPELINE_MAKE(sc, sc->encode_element, _MMCAMCORDER_ENCODE_MAIN_PIPE, "camcorder_pipeline", err);

	err = _mmcamcorder_create_audiosrc_bin(handle);
	if (err != MM_ERROR_NONE)
		return err;

	if (info->bMuxing) {
		/* Muxing. can use encodebin. */
		err = _mmcamcorder_create_encodesink_bin((MMHandleType)hcamcorder, MM_CAMCORDER_ENCBIN_PROFILE_AUDIO);
		if (err != MM_ERROR_NONE)
			return err;

		/* Add and link elements */
		gst_bin_add_many(GST_BIN(sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst),
			sc->encode_element[_MMCAMCORDER_AUDIOSRC_BIN].gst,
			sc->encode_element[_MMCAMCORDER_ENCSINK_BIN].gst,
			NULL);

		srcpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_AUDIOSRC_BIN].gst, "src");
		sinkpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_BIN].gst, "audio_sink0");
		_MM_GST_PAD_LINK_UNREF(srcpad, sinkpad, err, pipeline_creation_error);
	} else {
		err = mm_camcorder_get_attributes(handle, NULL,
			MMCAM_TARGET_FILENAME, &file_name, &file_name_len,
			NULL);
		if (err != MM_ERROR_NONE) {
			_mmcam_dbg_err("failed to get filename [0x%x]", err);
			err = MM_ERROR_CAMCORDER_RESOURCE_CREATION;
			goto pipeline_creation_error;
		}

		/* without muxing. can't use encodebin. */
		aenc_elem = _mmcamcorder_get_type_element(handle, MM_CAM_AUDIO_ENCODER);
		if (!aenc_elem) {
			_mmcam_dbg_err("Fail to get type element");
			err = MM_ERROR_CAMCORDER_RESOURCE_CREATION;
			goto pipeline_creation_error;
		}

		err = _mmcamcorder_conf_get_value_element_name(aenc_elem, &aenc_name);
		if ((!err) || (!aenc_name)) {
			_mmcam_dbg_err("Fail to get element name");
			err = MM_ERROR_CAMCORDER_RESOURCE_CREATION;
			goto pipeline_creation_error;
		}

		element_list = g_list_append(element_list, &sc->encode_element[_MMCAMCORDER_AUDIOSRC_BIN]);

		_MMCAMCORDER_ELEMENT_MAKE(sc, sc->encode_element, _MMCAMCORDER_ENCSINK_AQUE, "queue",  NULL, element_list, err);

		if (strcmp(aenc_name, "wavenc") != 0)
			_MMCAMCORDER_ELEMENT_MAKE(sc, sc->encode_element, _MMCAMCORDER_ENCSINK_CONV, "audioconvert",  NULL, element_list, err);

		_MMCAMCORDER_ELEMENT_MAKE(sc, sc->encode_element, _MMCAMCORDER_ENCSINK_AENC, aenc_name, NULL, element_list, err);

		_mmcamcorder_conf_get_element(handle, hcamcorder->conf_main,
			CONFIGURE_CATEGORY_MAIN_RECORD,
			"RecordsinkElement",
			&sink_elem);
		_mmcamcorder_conf_get_value_element_name(sink_elem, &sink_name);

		_mmcam_dbg_log("encode sink : %s", sink_name);

		_MMCAMCORDER_ELEMENT_MAKE(sc, sc->encode_element, _MMCAMCORDER_ENCSINK_SINK, sink_name, NULL, element_list, err);

		_mmcamcorder_conf_set_value_element_property(sc->encode_element[_MMCAMCORDER_ENCSINK_SINK].gst, sink_elem);

		/* add elements to encode pipeline */
		if (!_mmcamcorder_add_elements_to_bin(GST_BIN(sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst), element_list)) {
			_mmcam_dbg_err("add encode elements error.");
			err = MM_ERROR_CAMCORDER_RESOURCE_CREATION;
			goto pipeline_creation_error;
		}

		/* link elements */
		if (!_mmcamcorder_link_elements(element_list)) {
			_mmcam_dbg_err("encode element link error.");
			err = MM_ERROR_CAMCORDER_GST_LINK;
			goto pipeline_creation_error;
		}
	}

	/* set data probe function */
	srcpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_AUDIOSRC_SRC].gst, "src");
	MMCAMCORDER_ADD_BUFFER_PROBE(srcpad, _MMCAMCORDER_HANDLER_AUDIOREC,
		__mmcamcorder_audio_dataprobe_voicerecorder, hcamcorder);
	gst_object_unref(srcpad);
	srcpad = NULL;

	srcpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_AENC].gst, "src");
	MMCAMCORDER_ADD_BUFFER_PROBE(srcpad, _MMCAMCORDER_HANDLER_AUDIOREC,
		__mmcamcorder_audio_dataprobe_record, hcamcorder);
	gst_object_unref(srcpad);
	srcpad = NULL;

	sinkpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_SINK].gst, "sink");
	MMCAMCORDER_ADD_BUFFER_PROBE(sinkpad, _MMCAMCORDER_HANDLER_AUDIOREC,
		__mmcamcorder_muxed_dataprobe, hcamcorder);
	MMCAMCORDER_ADD_EVENT_PROBE(sinkpad, _MMCAMCORDER_HANDLER_AUDIOREC,
		__mmcamcorder_eventprobe_monitor, hcamcorder);
	gst_object_unref(sinkpad);
	sinkpad = NULL;

	bus = gst_pipeline_get_bus(GST_PIPELINE(sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst));

	/* register message callback  */
	hcamcorder->pipeline_cb_event_id = gst_bus_add_watch(bus, (GstBusFunc)_mmcamcorder_pipeline_cb_message, hcamcorder);

	/* set sync callback */
	gst_bus_set_sync_handler(bus, _mmcamcorder_encode_pipeline_bus_sync_callback, hcamcorder, NULL);

	gst_object_unref(bus);
	bus = NULL;

	if (element_list) {
		g_list_free(element_list);
		element_list = NULL;
	}

	return MM_ERROR_NONE;

pipeline_creation_error:
	_MMCAMCORDER_ELEMENT_REMOVE(sc->encode_element, _MMCAMCORDER_ENCODE_MAIN_PIPE);
	_MMCAMCORDER_ELEMENT_REMOVE(sc->encode_element, _MMCAMCORDER_AUDIOSRC_BIN);
	_MMCAMCORDER_ELEMENT_REMOVE(sc->encode_element, _MMCAMCORDER_ENCSINK_AQUE);
	_MMCAMCORDER_ELEMENT_REMOVE(sc->encode_element, _MMCAMCORDER_ENCSINK_CONV);
	_MMCAMCORDER_ELEMENT_REMOVE(sc->encode_element, _MMCAMCORDER_ENCSINK_AENC);
	_MMCAMCORDER_ELEMENT_REMOVE(sc->encode_element, _MMCAMCORDER_ENCSINK_SINK);
	_MMCAMCORDER_ELEMENT_REMOVE(sc->encode_element, _MMCAMCORDER_ENCSINK_BIN);

	if (element_list) {
		g_list_free(element_list);
		element_list = NULL;
	}

	return err;
}


int
_mmcamcorder_create_audio_pipeline(MMHandleType handle)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	sc = MMF_CAMCORDER_SUBCONTEXT(handle);

	mmf_return_val_if_fail(sc, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	return __mmcamcorder_create_audiop_with_encodebin(handle);
}


/**
 * This function destroy audio pipeline.
 *
 * @param[in]	handle		Handle of camcorder.
 * @return	void
 * @remarks
 * @see		_mmcamcorder_destroy_pipeline()
 *
 */
void
_mmcamcorder_destroy_audio_pipeline(MMHandleType handle)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;
	_MMCamcorderAudioInfo *info = NULL;
	mmf_return_if_fail(hcamcorder);
	sc = MMF_CAMCORDER_SUBCONTEXT(handle);

	mmf_return_if_fail(sc && sc->info_audio);
	mmf_return_if_fail(sc->element);

	info = sc->info_audio;

	_mmcam_dbg_log("start");

	if (sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst) {
		_mmcam_dbg_warn("release audio pipeline");

		_mmcamcorder_gst_set_state(handle, sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst, GST_STATE_NULL);

		_mmcamcorder_remove_all_handlers((MMHandleType)hcamcorder, _MMCAMCORDER_HANDLER_CATEGORY_ALL);

		if (info->bMuxing) {
			GstPad *reqpad = NULL;
			/* FIXME:
			    Release request pad
			    The ref_count of mux is always # of streams in here, i don't know why it happens.
			    So, i unref the mux manually
			*/
			reqpad = gst_element_get_static_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, "audio");
			gst_element_release_request_pad(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, reqpad);
			gst_object_unref(reqpad);

			if (GST_IS_ELEMENT(sc->encode_element[_MMCAMCORDER_ENCSINK_MUX].gst) &&
			   GST_OBJECT_REFCOUNT(sc->encode_element[_MMCAMCORDER_ENCSINK_MUX].gst) > 1) {
				gst_object_unref(sc->encode_element[_MMCAMCORDER_ENCSINK_MUX].gst);
			}
		}
		gst_object_unref(sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst);
	}

	_mmcam_dbg_log("done");

	return;
}


int
_mmcamcorder_audio_command(MMHandleType handle, int command)
{
	int cmd = command;
	int ret = MM_ERROR_NONE;
	int err = 0;
	guint64 free_space = 0;
	char *err_attr_name = NULL;

	GstElement *pipeline = NULL;
	GstElement *audioSrc = NULL;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;
	_MMCamcorderAudioInfo *info = NULL;

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	sc = MMF_CAMCORDER_SUBCONTEXT(handle);

	mmf_return_val_if_fail(sc, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	mmf_return_val_if_fail(sc->element, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	mmf_return_val_if_fail(sc->info_audio, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	pipeline = sc->element[_MMCAMCORDER_MAIN_PIPE].gst;
	info = sc->info_audio;

	_mmcam_dbg_log("");

	pipeline = sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst;
	audioSrc = sc->encode_element[_MMCAMCORDER_AUDIOSRC_SRC].gst;
	switch (cmd) {
	case _MMCamcorder_CMD_RECORD:
		/* check status for resume case */
		if (_mmcamcorder_get_state((MMHandleType)hcamcorder) != MM_CAMCORDER_STATE_PAUSED) {
			guint imax_size = 0;
			guint imax_time = 0;
			char *temp_filename = NULL;
			char *dir_name = NULL;
			int file_system_type = 0;
			int filename_length = 0;
			int root_directory_length = 0;

			if (sc->pipeline_time)
				gst_element_set_start_time(pipeline, sc->pipeline_time);

			sc->pipeline_time = RESET_PAUSE_TIME;

			ret = mm_camcorder_get_attributes(handle, &err_attr_name,
				MMCAM_TARGET_MAX_SIZE, &imax_size,
				MMCAM_TARGET_TIME_LIMIT, &imax_time,
				MMCAM_FILE_FORMAT, &(info->fileformat),
				MMCAM_TARGET_FILENAME, &temp_filename, &filename_length,
				MMCAM_ROOT_DIRECTORY, &hcamcorder->root_directory, &root_directory_length,
				NULL);
			if (ret != MM_ERROR_NONE) {
				_mmcam_dbg_warn("failed to get attribute. (%s:%x)", err_attr_name, ret);
				SAFE_FREE(err_attr_name);
				goto _ERR_CAMCORDER_AUDIO_COMMAND;
			}

			if (!temp_filename && !hcamcorder->mstream_cb) {
				_mmcam_dbg_err("filename is not set and muxed stream cb is NULL");
				ret = MM_ERROR_CAMCORDER_INVALID_ARGUMENT;
				goto _ERR_CAMCORDER_AUDIO_COMMAND;
			}

			SAFE_G_FREE(info->filename);

			if (temp_filename) {
				info->filename = g_strdup(temp_filename);
				if (!info->filename) {
					_mmcam_dbg_err("STRDUP was failed for [%s]", temp_filename);
					goto _ERR_CAMCORDER_AUDIO_COMMAND;
				}

				_mmcam_dbg_log("Record start : file name [%s]", info->filename);

				MMCAMCORDER_G_OBJECT_SET_POINTER(sc->encode_element[_MMCAMCORDER_ENCSINK_SINK].gst, "location", info->filename);
			} else {
				_mmcam_dbg_log("Recorded data will be written in [%s]", _MMCamcorder_FILENAME_NULL);
				MMCAMCORDER_G_OBJECT_SET_POINTER(sc->encode_element[_MMCAMCORDER_ENCSINK_SINK].gst, "location", _MMCamcorder_FILENAME_NULL);
			}

			sc->ferror_send = FALSE;
			sc->ferror_count = 0;
			sc->bget_eos = FALSE;
			sc->muxed_stream_offset = 0;
			info->filesize = 0;

			/* set max size */
			if (imax_size <= 0)
				info->max_size = 0; /* do not check */
			else
				info->max_size = ((guint64)imax_size) << 10; /* to byte */

			/* set max time */
			if (imax_time <= 0)
				info->max_time = 0; /* do not check */
			else
				info->max_time = ((guint64)imax_time) * 1000; /* to millisecond */

			/* TODO : check free space before recording start */
			dir_name = g_path_get_dirname(info->filename);
			if (dir_name) {
				err = _mmcamcorder_get_storage_info(dir_name, hcamcorder->root_directory, &hcamcorder->storage_info);
				if (err != 0) {
					_mmcam_dbg_err("get storage info failed");
					g_free(dir_name);
					dir_name = NULL;
					return MM_ERROR_OUT_OF_STORAGE;
				}

				err = _mmcamcorder_get_freespace(hcamcorder->storage_info.type, &free_space);

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
				err = -1;
			}

			if (temp_filename &&
				(err == -1 || free_space <= _MMCAMCORDER_AUDIO_MINIMUM_SPACE)) {
				_mmcam_dbg_err("OUT of STORAGE [err:%d or free space [%" G_GUINT64_FORMAT "] is smaller than [%d]",
					err, free_space, _MMCAMCORDER_AUDIO_MINIMUM_SPACE);
				return MM_ERROR_OUT_OF_STORAGE;
			}
		}

		ret = _mmcamcorder_gst_set_state(handle, pipeline, GST_STATE_PLAYING);
		if (ret != MM_ERROR_NONE)
			goto _ERR_CAMCORDER_AUDIO_COMMAND;

		break;

	case _MMCamcorder_CMD_PAUSE:
	{
		GstClock *pipe_clock = NULL;
		int count = 0;

		if (info->b_commiting) {
			_mmcam_dbg_warn("now on commiting previous file!!(cmd : %d)", cmd);
			return MM_ERROR_CAMCORDER_CMD_IS_RUNNING;
		}

		for (count = 0 ; count <= _MMCAMCORDER_RETRIAL_COUNT ; count++) {
			if (info->filesize > 0) {
				break;
			} else if (count == _MMCAMCORDER_RETRIAL_COUNT) {
				_mmcam_dbg_err("Pause fail, wait 200 ms, but file size is %lld",
					info->filesize);
				return MM_ERROR_CAMCORDER_INVALID_CONDITION;
			} else {
				_mmcam_dbg_warn("Wait for enough audio frame, retry count[%d], file size is %lld",
					count, info->filesize);
			}
			usleep(_MMCAMCORDER_FRAME_WAIT_TIME);
		}

		ret = _mmcamcorder_gst_set_state(handle, pipeline, GST_STATE_PAUSED);
		if (ret != MM_ERROR_NONE)
			goto _ERR_CAMCORDER_AUDIO_COMMAND;

		/* FIXME: consider delay. */
		pipe_clock = gst_pipeline_get_clock(GST_PIPELINE(pipeline));
		sc->pipeline_time = gst_clock_get_time(pipe_clock) - gst_element_get_base_time(GST_ELEMENT(pipeline));
		break;
	}

	case _MMCamcorder_CMD_CANCEL:
		if (info->b_commiting) {
			_mmcam_dbg_warn("now on commiting previous file!!(cmd : %d)", cmd);
			return MM_ERROR_CAMCORDER_CMD_IS_RUNNING;
		}

		ret = _mmcamcorder_gst_set_state(handle, pipeline, GST_STATE_READY);
		if (ret != MM_ERROR_NONE)
			goto _ERR_CAMCORDER_AUDIO_COMMAND;

		if (info->bMuxing)
			MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, "block", FALSE);
		else
			MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_AQUE].gst, "empty-buffers", FALSE);

		_mmcamcorder_gst_set_state(handle, sc->encode_element[_MMCAMCORDER_ENCSINK_SINK].gst, GST_STATE_NULL);

		sc->pipeline_time = 0;
		sc->pause_time = 0;
		sc->isMaxsizePausing = FALSE;
		sc->isMaxtimePausing = FALSE;

		if (info->filename) {
			_mmcam_dbg_log("file delete(%s)", info->filename);
			unlink(info->filename);
			SAFE_G_FREE(info->filename);
		}
		break;

	case _MMCamcorder_CMD_COMMIT:
	{
		int count = 0;
		guint64 free_space = 0;
		_mmcam_dbg_log("_MMCamcorder_CMD_COMMIT");

		if (info->b_commiting) {
			_mmcam_dbg_warn("now on commiting previous file!!(cmd : %d)", cmd);
			return MM_ERROR_CAMCORDER_CMD_IS_RUNNING;
		} else {
			_mmcam_dbg_log("_MMCamcorder_CMD_COMMIT : start");
			info->b_commiting = TRUE;
		}

		for (count = 0 ; count <= _MMCAMCORDER_RETRIAL_COUNT ; count++) {
			if (info->filesize > 0) {
				break;
			} else if (count == _MMCAMCORDER_RETRIAL_COUNT) {
				_mmcam_dbg_err("Commit fail, waited 200 ms, but file size is %lld", info->filesize);
					info->b_commiting = FALSE;
				return MM_ERROR_CAMCORDER_INVALID_CONDITION;
			} else {
				_mmcam_dbg_warn("Waiting for enough audio frame, re-count[%d], file size is %lld",
					count, info->filesize);
			}
			usleep(_MMCAMCORDER_FRAME_WAIT_TIME);
		}

		_mmcamcorder_get_freespace(hcamcorder->storage_info.type, &free_space);
		if (free_space < _MMCAMCORDER_AUDIO_MINIMUM_SPACE) {
			_mmcam_dbg_warn("_MMCamcorder_CMD_COMMIT out of storage [%" G_GUINT64_FORMAT "]", free_space);
			ret = MM_ERROR_OUT_OF_STORAGE;
			goto _ERR_CAMCORDER_AUDIO_COMMAND;
		}

		if (audioSrc) {
			if (gst_element_send_event(audioSrc, gst_event_new_eos()) == FALSE) {
				_mmcam_dbg_err("send EOS failed");
				info->b_commiting = FALSE;
				ret = MM_ERROR_CAMCORDER_INTERNAL;
				goto _ERR_CAMCORDER_AUDIO_COMMAND;
			}

			_mmcam_dbg_log("send EOS done");

			/* for pause -> commit case */
			if (_mmcamcorder_get_state((MMHandleType)hcamcorder) == MM_CAMCORDER_STATE_PAUSED) {
				ret = _mmcamcorder_gst_set_state(handle, pipeline, GST_STATE_PLAYING);
				if (ret != MM_ERROR_NONE) {
					info->b_commiting = FALSE;
					goto _ERR_CAMCORDER_AUDIO_COMMAND;
				}
			}
		} else {
			_mmcam_dbg_err("No audio stream source");
			info->b_commiting = FALSE;
			ret = MM_ERROR_CAMCORDER_INTERNAL;
			goto _ERR_CAMCORDER_AUDIO_COMMAND;
		}

		/* wait until finishing EOS */
		_mmcam_dbg_log("Start to wait EOS");
		if ((ret = _mmcamcorder_get_eos_message(handle)) != MM_ERROR_NONE) {
			info->b_commiting = FALSE;
			goto _ERR_CAMCORDER_AUDIO_COMMAND;
		}
		break;
	}

	case _MMCamcorder_CMD_PREVIEW_START:
		/*MM_CAMCORDER_START_CHANGE_STATE;*/
		break;

	case _MMCamcorder_CMD_PREVIEW_STOP:
		/*MM_CAMCORDER_STOP_CHANGE_STATE;*/
		/*void*/
		break;

	default:
		ret = MM_ERROR_CAMCORDER_INVALID_ARGUMENT;
		break;
	}

_ERR_CAMCORDER_AUDIO_COMMAND:
	return ret;
}

int _mmcamcorder_audio_handle_eos(MMHandleType handle)
{
	int err = MM_ERROR_NONE;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;
	_MMCamcorderAudioInfo *info = NULL;
	GstElement *pipeline = NULL;
	_MMCamcorderMsgItem msg;
	MMCamRecordingReport * report;

	mmf_return_val_if_fail(hcamcorder, FALSE);
	sc = MMF_CAMCORDER_SUBCONTEXT(handle);

	mmf_return_val_if_fail(sc, FALSE);
	mmf_return_val_if_fail(sc->info_audio, FALSE);

	_mmcam_dbg_err("");

	info = sc->info_audio;

	pipeline = sc->encode_element[_MMCAMCORDER_ENCODE_MAIN_PIPE].gst;

	err = _mmcamcorder_gst_set_state(handle, pipeline, GST_STATE_READY);
	if (err != MM_ERROR_NONE)
		_mmcam_dbg_warn("Failed:_MMCamcorder_CMD_COMMIT:GST_STATE_READY. err[%x]", err);

	/* Send recording report message to application */
	msg.id = MM_MESSAGE_CAMCORDER_AUDIO_CAPTURED;
	report = (MMCamRecordingReport*) g_malloc(sizeof(MMCamRecordingReport));
	if (!report) {
		_mmcam_dbg_err("Recording report fail(%s). Out of memory.", info->filename);
		return FALSE;
	}

/* START TAG HERE */
	/* MM_AUDIO_CODEC_AAC + MM_FILE_FORMAT_MP4 */
	if (info->fileformat == MM_FILE_FORMAT_3GP || info->fileformat == MM_FILE_FORMAT_MP4)
		__mmcamcorder_audio_add_metadata_info_m4a(handle);
/* END TAG HERE */

	report->recording_filename = g_strdup(info->filename);
	msg.param.data = report;

	_mmcamcorder_send_message(handle, &msg);

	if (info->bMuxing)
		MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, "block", FALSE);
	else
		MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_AQUE].gst, "empty-buffers", FALSE);

	_mmcamcorder_gst_set_state(handle, pipeline, GST_STATE_NULL);

	sc->pipeline_time = 0;
	sc->pause_time = 0;
	sc->isMaxsizePausing = FALSE;
	sc->isMaxtimePausing = FALSE;

	SAFE_G_FREE(info->filename);

	_mmcam_dbg_err("_MMCamcorder_CMD_COMMIT : end");

	info->b_commiting = FALSE;

	return TRUE;
}


static float
__mmcamcorder_get_decibel(unsigned char* raw, int size, MMCamcorderAudioFormat format)
{
	#define MAX_AMPLITUDE_MEAN_16BIT (23170.115738161934)
	#define MAX_AMPLITUDE_MEAN_08BIT (89.803909382810)
	#define DEFAULT_DECIBEL          (-80.0)

	int i = 0;
	int depthByte = 0;
	int count = 0;

	short* pcm16 = 0;
	char* pcm8 = 0;

	float db = DEFAULT_DECIBEL;
	float rms = 0.0;
	unsigned long long square_sum = 0;

	if (format == MM_CAMCORDER_AUDIO_FORMAT_PCM_S16_LE)
		depthByte = 2;
	else		/*MM_CAMCORDER_AUDIO_FORMAT_PCM_U8*/
		depthByte = 1;

	for ( ; i < size ; i += (depthByte<<1)) {
		if (depthByte == 1) {
			pcm8 = (char *)(raw + i);
			square_sum += (*pcm8) * (*pcm8);
		} else { /*2byte*/
			pcm16 = (short*)(raw + i);
			square_sum += (*pcm16) * (*pcm16);
		}

		count++;
	}

	if (count > 0) {
		rms = sqrt((double)square_sum/(double)count);

		if (depthByte == 1)
			db = 20 * log10(rms/MAX_AMPLITUDE_MEAN_08BIT);
		else
			db = 20 * log10(rms/MAX_AMPLITUDE_MEAN_16BIT);
	}

	/*
	_mmcam_dbg_log("size[%d],depthByte[%d],count[%d],rms[%f],db[%f]",
					size, depthByte, count, rms, db);
	*/

	return db;
}


static GstPadProbeReturn __mmcamcorder_audio_dataprobe_voicerecorder(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(u_data);
	double volume = 0.0;
	int current_state = MM_CAMCORDER_STATE_NONE;
	int format = 0;
	int channel = 0;
	float curdcb = 0.0;
	_MMCamcorderMsgItem msg;
	int err = MM_ERROR_UNKNOWN;
	char *err_name = NULL;
	GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
	GstMapInfo mapinfo;

	mmf_return_val_if_fail(hcamcorder, GST_PAD_PROBE_OK);

	current_state = _mmcamcorder_get_state((MMHandleType)hcamcorder);
	if (current_state < MM_CAMCORDER_STATE_PREPARE) {
		_mmcam_dbg_warn("Not ready for stream callback");
		return GST_PAD_PROBE_OK;
	}

	memset(&mapinfo, 0x0, sizeof(GstMapInfo));

	/* Set volume to audio input */
	err = mm_camcorder_get_attributes((MMHandleType)hcamcorder, &err_name,
		MMCAM_AUDIO_VOLUME, &volume,
		MMCAM_AUDIO_FORMAT, &format,
		MMCAM_AUDIO_CHANNEL, &channel,
		NULL);

	if (err < 0) {
		_mmcam_dbg_warn("Get attrs fail. (%s:%x)", err_name, err);
		SAFE_FREE(err_name);
		return err;
	}

	gst_buffer_map(buffer, &mapinfo, GST_MAP_READWRITE);

	if (volume == 0)
		memset(mapinfo.data, 0, mapinfo.size);

	/* Get current volume level of real input stream */
	curdcb = __mmcamcorder_get_decibel(mapinfo.data, mapinfo.size, format);

	msg.id = MM_MESSAGE_CAMCORDER_CURRENT_VOLUME;
	msg.param.rec_volume_dB = curdcb;
	_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);

	_MMCAMCORDER_LOCK_ASTREAM_CALLBACK(hcamcorder);

	/* CALL audio stream callback */
	if (hcamcorder->astream_cb && buffer && mapinfo.data && mapinfo.size > 0) {
		MMCamcorderAudioStreamDataType stream;

		/*
		_mmcam_dbg_log("Call audio steramCb, data[%p], format[%d], channel[%d], length[%d], volume_dB[%f]",
			GST_BUFFER_DATA(buffer), format, channel, GST_BUFFER_SIZE(buffer), curdcb);
		*/

		stream.data = (void *)mapinfo.data;
		stream.format = format;
		stream.channel = channel;
		stream.length = mapinfo.size;
		stream.timestamp = (unsigned int)(GST_BUFFER_PTS(buffer)/1000000);	/* nano -> msecond */
		stream.volume_dB = curdcb;

		hcamcorder->astream_cb(&stream, hcamcorder->astream_cb_param);
	}

	_MMCAMCORDER_UNLOCK_ASTREAM_CALLBACK(hcamcorder);

	gst_buffer_unmap(buffer, &mapinfo);

	return GST_PAD_PROBE_OK;
}


static GstPadProbeReturn __mmcamcorder_audio_dataprobe_record(GstPad *pad, GstPadProbeInfo *info, gpointer u_data)
{
	static int count = 0;
	guint64 rec_pipe_time = 0;
	guint64 free_space = 0;
	guint64 buffer_size = 0;
	guint64 trailer_size = 0;
	unsigned long long remained_time = 0;
	int get_trailer_size = 0;

	_MMCamcorderSubContext *sc = NULL;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(u_data);
	_MMCamcorderAudioInfo *audioinfo = NULL;
	_MMCamcorderMsgItem msg;
	GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);

	mmf_return_val_if_fail(hcamcorder, GST_PAD_PROBE_DROP);
	mmf_return_val_if_fail(buffer, GST_PAD_PROBE_DROP);

	sc = MMF_CAMCORDER_SUBCONTEXT(hcamcorder);
	mmf_return_val_if_fail(sc && sc->info_audio, GST_PAD_PROBE_DROP);
	audioinfo = sc->info_audio;

	if (sc->isMaxtimePausing || sc->isMaxsizePausing) {
		_mmcam_dbg_warn("isMaxtimePausing[%d],isMaxsizePausing[%d]",
			sc->isMaxtimePausing, sc->isMaxsizePausing);
		return GST_PAD_PROBE_DROP;
	}

	buffer_size = gst_buffer_get_size(buffer);

	if (audioinfo->filesize == 0) {
		if (audioinfo->fileformat == MM_FILE_FORMAT_WAV)
			audioinfo->filesize += 44; /* wave header size */
		else if (audioinfo->fileformat == MM_FILE_FORMAT_AMR)
			audioinfo->filesize += 6; /* amr header size */

		audioinfo->filesize += buffer_size;
		return GST_PAD_PROBE_OK;
	}

	if (sc->ferror_send) {
		_mmcam_dbg_warn("file write error, drop frames");
		return GST_PAD_PROBE_DROP;
	}

	/* get trailer size */
	get_trailer_size = (
		audioinfo->fileformat == MM_FILE_FORMAT_3GP ||
		audioinfo->fileformat == MM_FILE_FORMAT_MP4 ||
		audioinfo->fileformat == MM_FILE_FORMAT_AAC) ? TRUE : FALSE;
	if (get_trailer_size) {
		MMCAMCORDER_G_OBJECT_GET(sc->encode_element[_MMCAMCORDER_ENCSINK_MUX].gst, "expected-trailer-size", &trailer_size);
		/*_mmcam_dbg_log("trailer_size %d", trailer_size);*/
	} else {
		trailer_size = 0; /* no trailer */
	}

	/* to minimizing free space check overhead */
	count = count % _MMCAMCORDER_FREE_SPACE_CHECK_INTERVAL;
	if (count++ == 0) {
		gint free_space_ret = 0;
		storage_state_e storage_state = STORAGE_STATE_UNMOUNTABLE;

		/* check free space */
		free_space_ret = _mmcamcorder_get_freespace(hcamcorder->storage_info.type, &free_space);
		if (free_space_ret != 0) {
			_mmcam_dbg_err("Error occured. [%d]", free_space_ret);
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

		if (free_space < (guint64)(_MMCAMCORDER_AUDIO_MINIMUM_SPACE + buffer_size + trailer_size)) {
			_mmcam_dbg_warn("No more space for recording!!!");
			_mmcam_dbg_warn("Free Space : [%" G_GUINT64_FORMAT "], file size : [%" G_GUINT64_FORMAT "]",
				free_space, audioinfo->filesize);

			if (audioinfo->bMuxing)
				MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, "block", TRUE);
			else
				MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_AQUE].gst, "empty-buffers", TRUE);

			sc->isMaxsizePausing = TRUE;
			msg.id = MM_MESSAGE_CAMCORDER_NO_FREE_SPACE;
			_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);

			return GST_PAD_PROBE_DROP; /* skip this buffer */
		}
	}

	if (!GST_CLOCK_TIME_IS_VALID(GST_BUFFER_PTS(buffer))) {
		_mmcam_dbg_err("Buffer timestamp is invalid, check it");
		return GST_PAD_PROBE_DROP;
	}

	rec_pipe_time = GST_TIME_AS_MSECONDS(GST_BUFFER_PTS(buffer));

	/* calculate remained time can be recorded */
	if (audioinfo->max_time > 0 && audioinfo->max_time < (remained_time + rec_pipe_time)) {
		remained_time = audioinfo->max_time - rec_pipe_time;
	} else if (audioinfo->max_size > 0) {
		long double max_size = (long double)audioinfo->max_size;
		long double current_size = (long double)(audioinfo->filesize + buffer_size + trailer_size);

		remained_time = (unsigned long long)((long double)rec_pipe_time * (max_size/current_size)) - rec_pipe_time;
	}

	/*_mmcam_dbg_log("remained time : %u", remained_time);*/

	/* check max size of recorded file */
	if (audioinfo->max_size > 0 &&
			audioinfo->max_size < audioinfo->filesize + buffer_size + trailer_size + _MMCAMCORDER_MMS_MARGIN_SPACE) {
		_mmcam_dbg_warn("Max size!!! Recording is paused.");
		_mmcam_dbg_warn("Max [%" G_GUINT64_FORMAT "], file [%" G_GUINT64_FORMAT "], trailer : [%" G_GUINT64_FORMAT "]", \
			audioinfo->max_size, audioinfo->filesize, trailer_size);

		/* just same as pause status. After blocking two queue, this function will not call again. */
		if (audioinfo->bMuxing)
			MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, "block", TRUE);
		else
			MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_AQUE].gst, "empty-buffers", TRUE);

		msg.id = MM_MESSAGE_CAMCORDER_RECORDING_STATUS;
		msg.param.recording_status.elapsed = (unsigned long long)rec_pipe_time;
		msg.param.recording_status.filesize = (unsigned long long)((audioinfo->filesize + trailer_size) >> 10);
		msg.param.recording_status.remained_time = 0;
		_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);

		_mmcam_dbg_warn("Last filesize sent by message : %d", audioinfo->filesize + trailer_size);

		sc->isMaxsizePausing = TRUE;
		msg.id = MM_MESSAGE_CAMCORDER_MAX_SIZE;
		_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);

		/* skip this buffer */
		return GST_PAD_PROBE_DROP;
	}

	/* check recording time limit and send recording status message */
	if (audioinfo->max_time > 0 && rec_pipe_time > audioinfo->max_time) {
		_mmcam_dbg_warn("Current time : [%" G_GUINT64_FORMAT "], Maximum time : [%" G_GUINT64_FORMAT "]", \
			rec_pipe_time, audioinfo->max_time);

		if (audioinfo->bMuxing)
			MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst, "block", TRUE);
		else
			MMCAMCORDER_G_OBJECT_SET(sc->encode_element[_MMCAMCORDER_ENCSINK_AQUE].gst, "empty-buffers", TRUE);

		sc->isMaxtimePausing = TRUE;
		msg.id = MM_MESSAGE_CAMCORDER_TIME_LIMIT;
		_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);

		/* skip this buffer */
		return GST_PAD_PROBE_DROP;
	}

	/* send message for recording time and recorded file size */
	if (audioinfo->b_commiting == FALSE) {
		audioinfo->filesize += buffer_size;

		msg.id = MM_MESSAGE_CAMCORDER_RECORDING_STATUS;
		msg.param.recording_status.elapsed = (unsigned long long)rec_pipe_time;
		msg.param.recording_status.filesize = (unsigned long long)((audioinfo->filesize + trailer_size) >> 10);
		msg.param.recording_status.remained_time = remained_time;
		_mmcamcorder_send_message((MMHandleType)hcamcorder, &msg);

		return GST_PAD_PROBE_OK;
	} else {
		/* skip this buffer if commit process has been started */
		return GST_PAD_PROBE_DROP;
	}
}

/* START TAG HERE */
static gboolean __mmcamcorder_audio_add_metadata_info_m4a(MMHandleType handle)
{
	FILE *f = NULL;
	guchar buf[4];
	guint64 udta_size = 0;
	gint64 current_pos = 0;
	gint64 moov_pos = 0;
	gint64 udta_pos = 0;
	/* supporting audio geo tag for mobile */
	int gps_enable = 0;
	gdouble longitude = 0;
	gdouble latitude = 0;
	gdouble altitude = 0;
	_MMCamcorderLocationInfo geo_info = {0, 0, 0};
	_MMCamcorderLocationInfo loc_info = {0, 0, 0};

	char err_msg[128] = {'\0',};

	_MMCamcorderAudioInfo *info = NULL;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	sc = MMF_CAMCORDER_SUBCONTEXT(handle);

	mmf_return_val_if_fail(sc, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	mmf_return_val_if_fail(sc->info_audio, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	info = sc->info_audio;
	mm_camcorder_get_attributes(handle, NULL,
		MMCAM_TAG_GPS_ENABLE, &gps_enable,
		NULL);

	if (gps_enable) {
		mm_camcorder_get_attributes(handle, NULL,
			MMCAM_TAG_LATITUDE, &latitude,
			MMCAM_TAG_LONGITUDE, &longitude,
			MMCAM_TAG_ALTITUDE, &altitude,
			NULL);
		loc_info.longitude = _mmcamcorder_double_to_fix(longitude);
		loc_info.latitude = _mmcamcorder_double_to_fix(latitude);
		loc_info.altitude = _mmcamcorder_double_to_fix(altitude);
		geo_info.longitude = longitude *10000;
		geo_info.latitude = latitude *10000;
		geo_info.altitude = altitude *10000;
	}

	f = fopen64(info->filename, "rb+");
	if (f == NULL) {
		strerror_r(errno, err_msg, 128);
		_mmcam_dbg_err("file open failed [%s]", err_msg);
		return FALSE;
	}

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

		/* goto end of udta and write 'smta' box */
		if (fseek(f, (udta_size-4L), SEEK_CUR) != 0)
			goto fail;

		if (gps_enable) {
			if (!_mmcamcorder_write_loci(f, loc_info))
				goto fail;

			if (!_mmcamcorder_write_geodata(f, geo_info))
				goto fail;
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

		if (!_mmcamcorder_write_udta(f, gps_enable, loc_info, geo_info))
			goto fail;
	}

	/* find moov container.
	   update moov container size. */
	if ((current_pos = ftello(f)) < 0)
		goto ftell_fail;

	if (_mmcamcorder_find_fourcc(f, MMCAM_FOURCC('m', 'o', 'o', 'v'), TRUE)) {

		_mmcam_dbg_log("found moov container");
		if (fseek(f, -8L, SEEK_CUR) != 0)
			goto fail;

		moov_pos = ftello(f);
		if (moov_pos < 0)
			goto ftell_fail;

		if (!_mmcamcorder_update_size(f, moov_pos, current_pos))
			goto fail;

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

/* END TAG HERE */

