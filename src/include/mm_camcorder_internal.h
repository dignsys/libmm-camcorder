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

#ifndef __MM_CAMCORDER_INTERNAL_H__
#define __MM_CAMCORDER_INTERNAL_H__

/*=======================================================================================
| INCLUDE FILES										|
========================================================================================*/
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>

#include <mm_types.h>
#include <mm_attrs.h>
#include <mm_attrs_private.h>
#include <mm_message.h>
#include <mm_session.h>
#include <mm_session_private.h>
#include <mm_sound_focus.h>
#include <vconf.h>
#include <gst/video/video-format.h>
#include <ttrace.h>
#include <errno.h>
#include <restriction.h> /* device policy manager */

#include "mm_camcorder.h"
#include "mm_debug.h"

#ifdef _MMCAMCORDER_MURPHY_SUPPORT
#include "mm_camcorder_resource.h"
#endif /* _MMCAMCORDER_MURPHY_SUPPORT */

/* camcorder sub module */
#include "mm_camcorder_attribute.h"
#include "mm_camcorder_platform.h"
#include "mm_camcorder_stillshot.h"
#include "mm_camcorder_videorec.h"
#include "mm_camcorder_audiorec.h"
#include "mm_camcorder_gstcommon.h"
#include "mm_camcorder_exifinfo.h"
#include "mm_camcorder_util.h"
#include "mm_camcorder_configure.h"
#include "mm_camcorder_sound.h"

#ifdef _MMCAMCORDER_RM_SUPPORT
/* rm (resource manager)*/
#include <rm_api.h>
#endif /* _MMCAMCORDER_RM_SUPPORT */

#ifdef __cplusplus
extern "C" {
#endif

/*=======================================================================================
| MACRO DEFINITIONS									|
========================================================================================*/
#define _mmcam_dbg_verb(fmt, args...)  debug_verbose(" "fmt"\n", ##args);
#define _mmcam_dbg_log(fmt, args...)   debug_log(" "fmt"\n", ##args);
#define _mmcam_dbg_warn(fmt, args...)  debug_warning(" "fmt"\n", ##args);
#define _mmcam_dbg_err(fmt, args...)   debug_error(" "fmt"\n", ##args);
#define _mmcam_dbg_crit(fmt, args...)  debug_critical(" "fmt"\n", ##args);

/**
 *	Macro for checking validity and debugging
 */
#define mmf_return_if_fail(expr) \
	if (!(expr)) { \
		_mmcam_dbg_err("failed [%s]", #expr); \
		return; \
	}

/**
 *	Macro for checking validity and debugging
 */
#define mmf_return_val_if_fail(expr, val) \
	if (!(expr)) { \
		_mmcam_dbg_err("failed [%s]", #expr); \
		return (val); \
	}


#ifndef ARRAY_SIZE
/**
 *	Macro for getting array size
 */
#define ARRAY_SIZE(a) (sizeof((a)) / sizeof((a)[0]))
#endif

/* gstreamer element creation macro */
#define _MMCAMCORDER_PIPELINE_MAKE(sub_context, element, eid, name /*char* */, err) \
	if (element[eid].gst != NULL) { \
		_mmcam_dbg_err("The element(Pipeline) is existed. element_id=[%d], name=[%s]", eid, name); \
		gst_object_unref(element[eid].gst); \
	} \
	element[eid].id = eid; \
	element[eid].gst = gst_pipeline_new(name); \
	if (element[eid].gst == NULL) { \
		_mmcam_dbg_err("Pipeline creation fail. element_id=[%d], name=[%s]", eid, name); \
		err = MM_ERROR_CAMCORDER_RESOURCE_CREATION; \
		goto pipeline_creation_error; \
	} else { \
		g_object_weak_ref(G_OBJECT(element[eid].gst), (GWeakNotify)_mmcamcorder_element_release_noti, sub_context); \
	}

#define _MMCAMCORDER_BIN_MAKE(sub_context, element, eid, name /*char* */, err) \
	if (element[eid].gst != NULL) { \
		_mmcam_dbg_err("The element(Bin) is existed. element_id=[%d], name=[%s]", eid, name); \
		gst_object_unref(element[eid].gst); \
	} \
	element[eid].id = eid; \
	element[eid].gst = gst_bin_new(name); \
	if (element[eid].gst == NULL) { \
		_mmcam_dbg_err("Bin creation fail. element_id=[%d], name=[%s]\n", eid, name); \
		err = MM_ERROR_CAMCORDER_RESOURCE_CREATION; \
		goto pipeline_creation_error; \
	} else { \
		g_object_weak_ref(G_OBJECT(element[eid].gst), (GWeakNotify)_mmcamcorder_element_release_noti, sub_context); \
	}

#define _MMCAMCORDER_ELEMENT_MAKE(sub_context, element, eid, name /*char* */, nickname /*char* */, elist, err) \
	if (element[eid].gst != NULL) { \
		_mmcam_dbg_err("The element is existed. element_id=[%d], name=[%s]", eid, name); \
		gst_object_unref(element[eid].gst); \
	} \
	traceBegin(TTRACE_TAG_CAMERA, "MMCAMCORDER:ELEMENT_MAKE:%s", name); \
	element[eid].gst = gst_element_factory_make(name, nickname); \
	traceEnd(TTRACE_TAG_CAMERA); \
	if (element[eid].gst == NULL) { \
		_mmcam_dbg_err("Element creation fail. element_id=[%d], name=[%s]", eid, name); \
		err = MM_ERROR_CAMCORDER_RESOURCE_CREATION; \
		goto pipeline_creation_error; \
	} else { \
		_mmcam_dbg_log("Element creation done. element_id=[%d], name=[%s]", eid, name); \
		element[eid].id = eid; \
		g_object_weak_ref(G_OBJECT(element[eid].gst), (GWeakNotify)_mmcamcorder_element_release_noti, sub_context); \
		err = MM_ERROR_NONE; \
	} \
	elist = g_list_append(elist, &(element[eid]));

#define _MMCAMCORDER_ELEMENT_MAKE2(sub_context, element, eid, name /*char* */, nickname /*char* */, err) \
	if (element[eid].gst != NULL) { \
		_mmcam_dbg_err("The element is existed. element_id=[%d], name=[%s]", eid, name); \
		gst_object_unref(element[eid].gst); \
	} \
	element[eid].gst = gst_element_factory_make(name, nickname); \
	if (element[eid].gst == NULL) { \
		_mmcam_dbg_err("Element creation fail. element_id=[%d], name=[%s]", eid, name); \
		err = MM_ERROR_CAMCORDER_RESOURCE_CREATION; \
	} else { \
		_mmcam_dbg_log("Element creation done. element_id=[%d], name=[%s]", eid, name); \
		element[eid].id = eid; \
		g_object_weak_ref(G_OBJECT(element[eid].gst), (GWeakNotify)_mmcamcorder_element_release_noti, sub_context); \
		err = MM_ERROR_NONE; \
	} \

#define _MMCAMCORDER_ELEMENT_MAKE_IGNORE_ERROR(sub_context, element, eid, name /*char* */, nickname /*char* */, elist) \
	if (element[eid].gst != NULL) { \
		_mmcam_dbg_err("The element is existed. element_id=[%d], name=[%s]", eid, name); \
		gst_object_unref(element[eid].gst); \
	} \
	element[eid].gst = gst_element_factory_make(name, nickname); \
	if (element[eid].gst == NULL) { \
		_mmcam_dbg_err("Element creation fail. element_id=[%d], name=[%s], but keep going...", eid, name); \
	} else { \
		_mmcam_dbg_log("Element creation done. element_id=[%d], name=[%s]", eid, name); \
		element[eid].id = eid; \
		g_object_weak_ref(G_OBJECT(element[eid].gst), (GWeakNotify)_mmcamcorder_element_release_noti, sub_context); \
		elist = g_list_append(elist, &(element[eid])); \
	}

#define _MMCAMCORDER_ELEMENT_ADD(sub_context, element, eid, gst_element, elist, err) \
	if (element[eid].gst != NULL) { \
		_mmcam_dbg_err("The element is existed. element_id=[%d]", eid); \
		gst_object_unref(element[eid].gst); \
	} \
	element[eid].gst = gst_element; \
	if (element[eid].gst == NULL) { \
		_mmcam_dbg_err("Element is NULL. element_id=[%d]", eid); \
		err = MM_ERROR_CAMCORDER_RESOURCE_CREATION; \
		goto pipeline_creation_error; \
	} else { \
		_mmcam_dbg_log("Adding Element is done. element_id=[%d] %p", eid, gst_element); \
		element[eid].id = eid; \
		g_object_weak_ref(G_OBJECT(element[eid].gst), (GWeakNotify)_mmcamcorder_element_release_noti, sub_context); \
		err = MM_ERROR_NONE; \
	} \
	elist = g_list_append(elist, &(element[eid]));

#define _MMCAMCORDER_ENCODEBIN_ELMGET(sub_context, eid, name /*char* */, err) \
	if (sub_context->encode_element[eid].gst != NULL) { \
		_mmcam_dbg_err("The element is existed. element_id=[%d], name=[%s]", eid, name); \
		gst_object_unref(sub_context->encode_element[eid].gst); \
	} \
	sub_context->encode_element[eid].id = eid; \
	g_object_get(G_OBJECT(sub_context->encode_element[_MMCAMCORDER_ENCSINK_ENCBIN].gst), name, &(sub_context->encode_element[eid].gst), NULL); \
	if (sub_context->encode_element[eid].gst == NULL) { \
		_mmcam_dbg_err("Encode Element get fail. element_id=[%d], name=[%s]", eid, name); \
		err = MM_ERROR_CAMCORDER_RESOURCE_CREATION; \
		goto pipeline_creation_error; \
	} else{ \
		gst_object_unref(sub_context->encode_element[eid].gst); \
		g_object_weak_ref(G_OBJECT(sub_context->encode_element[eid].gst), (GWeakNotify)_mmcamcorder_element_release_noti, sub_context); \
	}

/* GStreamer element remove macro */
#define _MMCAMCORDER_ELEMENT_REMOVE(element, eid) \
	if (element[eid].gst != NULL) { \
		gst_object_unref(element[eid].gst); \
	}

#define _MM_GST_ELEMENT_LINK_MANY       gst_element_link_many
#define _MM_GST_ELEMENT_LINK            gst_element_link
#define _MM_GST_ELEMENT_LINK_FILTERED   gst_element_link_filtered
#define _MM_GST_ELEMENT_UNLINK          gst_element_unlink
#define _MM_GST_PAD_LINK                gst_pad_link

#define _MM_GST_PAD_LINK_UNREF(srcpad, sinkpad, err, if_fail_goto) \
{ \
	GstPadLinkReturn ret = GST_PAD_LINK_OK; \
	if (srcpad == NULL || sinkpad == NULL) { \
		if (srcpad == NULL) { \
			_mmcam_dbg_err("srcpad is NULL"); \
		} else { \
			gst_object_unref(srcpad);\
			srcpad = NULL; \
		} \
		if (sinkpad == NULL) { \
			_mmcam_dbg_err("sinkpad is NULL"); \
		} else { \
			gst_object_unref(sinkpad); \
			sinkpad = NULL;\
		} \
		err = MM_ERROR_CAMCORDER_GST_LINK; \
		goto if_fail_goto; \
	} \
	ret = _MM_GST_PAD_LINK(srcpad, sinkpad); \
	if (ret != GST_PAD_LINK_OK) { \
		GstObject *src_parent = gst_pad_get_parent(srcpad); \
		GstObject *sink_parent = gst_pad_get_parent(sinkpad); \
		char *src_name = NULL; \
		char *sink_name = NULL; \
		g_object_get((GObject *)src_parent, "name", &src_name, NULL); \
		g_object_get((GObject *)sink_parent, "name", &sink_name, NULL); \
		_mmcam_dbg_err("src[%s] - sink[%s] link failed", src_name, sink_name); \
		gst_object_unref(src_parent); src_parent = NULL; \
		gst_object_unref(sink_parent); sink_parent = NULL; \
		if (src_name) { \
			free(src_name); src_name = NULL; \
		} \
		if (sink_name) { \
			free(sink_name); sink_name = NULL; \
		} \
		gst_object_unref(srcpad); srcpad = NULL; \
		gst_object_unref(sinkpad); sinkpad = NULL; \
		err = MM_ERROR_CAMCORDER_GST_LINK; \
		goto if_fail_goto;\
	} \
	gst_object_unref(srcpad); srcpad = NULL; \
	gst_object_unref(sinkpad); sinkpad = NULL; \
}

#define _MM_GST_PAD_UNLINK_UNREF(srcpad, sinkpad) \
	if (srcpad && sinkpad) { \
		gst_pad_unlink(srcpad, sinkpad); \
	} else { \
		_mmcam_dbg_warn("some pad(srcpad:%p,sinkpad:%p) is NULL", srcpad, sinkpad); \
	} \
	if (srcpad) { \
		gst_object_unref(srcpad); srcpad = NULL; \
	} \
	if (sinkpad) { \
		gst_object_unref(sinkpad); sinkpad = NULL; \
	}

#define	_MMCAMCORDER_STATE_SET_COUNT		3		/* checking interval */
#define	_MMCAMCORDER_STATE_CHECK_TOTALTIME	5000000L	/* total wating time for state change */
#define	_MMCAMCORDER_STATE_CHECK_INTERVAL	(50*1000)	/* checking interval - 50ms*/

/**
 * Default videosink type
 */
#define _MMCAMCORDER_DEFAULT_VIDEOSINK_TYPE     "VideosinkElementOverlay"

/**
 * Default recording motion rate
 */
#define _MMCAMCORDER_DEFAULT_RECORDING_MOTION_RATE   1.0

/**
 * Total level count of manual focus */
#define _MMFCAMCORDER_FOCUS_TOTAL_LEVEL		8

/**
 *	File name length limit
 */
#define _MMCamcorder_FILENAME_LEN	(512)

/**
 *	Minimum integer value
 */
#define _MMCAMCORDER_MIN_INT	(INT_MIN)

/**
 *	Maximum integer value
 */
#define _MMCAMCORDER_MAX_INT	(INT_MAX)

/**
 *	Minimum double value
 */
#define _MMCAMCORDER_MIN_DOUBLE	(DBL_MIN)

/**
 *	Maximum integer value
 */
#define _MMCAMCORDER_MAX_DOUBLE	(DBL_MAX)

/**
 *	Audio timestamp margin (msec)
 */
#define _MMCAMCORDER_AUDIO_TIME_MARGIN (300)

/**
 *	Functions related with LOCK and WAIT
 */
#define _MMCAMCORDER_CAST_MTSAFE(handle)            (((mmf_camcorder_t*)handle)->mtsafe)
#define _MMCAMCORDER_LOCK_FUNC(mutex)               g_mutex_lock(&mutex)
#define _MMCAMCORDER_TRYLOCK_FUNC(mutex)            g_mutex_trylock(&mutex)
#define _MMCAMCORDER_UNLOCK_FUNC(mutex)             g_mutex_unlock(&mutex)

#define _MMCAMCORDER_GET_LOCK(handle)               (_MMCAMCORDER_CAST_MTSAFE(handle).lock)
#define _MMCAMCORDER_LOCK(handle)                   _MMCAMCORDER_LOCK_FUNC(_MMCAMCORDER_GET_LOCK(handle))
#define _MMCAMCORDER_TRYLOCK(handle)                _MMCAMCORDER_TRYLOCK_FUNC(_MMCAMCORDER_GET_LOCK(handle))
#define _MMCAMCORDER_UNLOCK(handle)                 _MMCAMCORDER_UNLOCK_FUNC(_MMCAMCORDER_GET_LOCK(handle))

#define _MMCAMCORDER_GET_COND(handle)               (_MMCAMCORDER_CAST_MTSAFE(handle).cond)
#define _MMCAMCORDER_WAIT(handle)                   g_cond_wait(&_MMCAMCORDER_GET_COND(handle), &_MMCAMCORDER_GET_LOCK(handle))
#define _MMCAMCORDER_WAIT_UNTIL(handle, end_time)   g_cond_wait_until(&_MMCAMCORDER_GET_COND(handle), &_MMCAMCORDER_GET_LOCK(handle), end_time)
#define _MMCAMCORDER_SIGNAL(handle)                 g_cond_signal(&_MMCAMCORDER_GET_COND(handle));
#define _MMCAMCORDER_BROADCAST(handle)              g_cond_broadcast(&_MMCAMCORDER_GET_COND(handle));

/* for command */
#define _MMCAMCORDER_GET_CMD_LOCK(handle)           (_MMCAMCORDER_CAST_MTSAFE(handle).cmd_lock)
#define _MMCAMCORDER_GET_CMD_COND(handle)           (_MMCAMCORDER_CAST_MTSAFE(handle).cmd_cond)
#define _MMCAMCORDER_LOCK_CMD(handle)               _MMCAMCORDER_LOCK_FUNC(_MMCAMCORDER_GET_CMD_LOCK(handle))
#define _MMCAMCORDER_TRYLOCK_CMD(handle)            _MMCAMCORDER_TRYLOCK_FUNC(_MMCAMCORDER_GET_CMD_LOCK(handle))
#define _MMCAMCORDER_UNLOCK_CMD(handle)             _MMCAMCORDER_UNLOCK_FUNC(_MMCAMCORDER_GET_CMD_LOCK(handle))
#define _MMCAMCORDER_CMD_WAIT(handle)                 g_cond_wait(&_MMCAMCORDER_GET_CMD_COND(handle), &_MMCAMCORDER_GET_CMD_LOCK(handle))
#define _MMCAMCORDER_CMD_WAIT_UNTIL(handle, end_time) g_cond_wait_until(&_MMCAMCORDER_GET_CMD_COND(handle), &_MMCAMCORDER_GET_CMD_LOCK(handle), end_time)
#define _MMCAMCORDER_CMD_SIGNAL(handle)               g_cond_signal(&_MMCAMCORDER_GET_CMD_COND(handle));

/* for ASM */
#define _MMCAMCORDER_GET_ASM_LOCK(handle)           (_MMCAMCORDER_CAST_MTSAFE(handle).asm_lock)
#define _MMCAMCORDER_LOCK_ASM(handle)               _MMCAMCORDER_LOCK_FUNC(_MMCAMCORDER_GET_ASM_LOCK(handle))
#define _MMCAMCORDER_TRYLOCK_ASM(handle)            _MMCAMCORDER_TRYLOCK_FUNC(_MMCAMCORDER_GET_ASM_LOCK(handle))
#define _MMCAMCORDER_UNLOCK_ASM(handle)             _MMCAMCORDER_UNLOCK_FUNC(_MMCAMCORDER_GET_ASM_LOCK(handle))

/* for state change */
#define _MMCAMCORDER_GET_STATE_LOCK(handle)         (_MMCAMCORDER_CAST_MTSAFE(handle).state_lock)
#define _MMCAMCORDER_LOCK_STATE(handle)             _MMCAMCORDER_LOCK_FUNC(_MMCAMCORDER_GET_STATE_LOCK(handle))
#define _MMCAMCORDER_TRYLOCK_STATE(handle)          _MMCAMCORDER_TRYLOCK_FUNC(_MMCAMCORDER_GET_STATE_LOCK(handle))
#define _MMCAMCORDER_UNLOCK_STATE(handle)           _MMCAMCORDER_UNLOCK_FUNC(_MMCAMCORDER_GET_STATE_LOCK(handle))

/* for gstreamer state change */
#define _MMCAMCORDER_GET_GST_STATE_LOCK(handle)     (_MMCAMCORDER_CAST_MTSAFE(handle).gst_state_lock)
#define _MMCAMCORDER_LOCK_GST_STATE(handle)         _MMCAMCORDER_LOCK_FUNC(_MMCAMCORDER_GET_GST_STATE_LOCK(handle))
#define _MMCAMCORDER_TRYLOCK_GST_STATE(handle)      _MMCAMCORDER_TRYLOCK_FUNC(_MMCAMCORDER_GET_GST_STATE_LOCK(handle))
#define _MMCAMCORDER_UNLOCK_GST_STATE(handle)       _MMCAMCORDER_UNLOCK_FUNC(_MMCAMCORDER_GET_GST_STATE_LOCK(handle))

#define _MMCAMCORDER_GET_GST_ENCODE_STATE_LOCK(handle)      (_MMCAMCORDER_CAST_MTSAFE(handle).gst_encode_state_lock)
#define _MMCAMCORDER_LOCK_GST_ENCODE_STATE(handle)          _MMCAMCORDER_LOCK_FUNC(_MMCAMCORDER_GET_GST_ENCODE_STATE_LOCK(handle))
#define _MMCAMCORDER_TRYLOCK_GST_ENCODE_STATE(handle)       _MMCAMCORDER_TRYLOCK_FUNC(_MMCAMCORDER_GET_GST_ENCODE_STATE_LOCK(handle))
#define _MMCAMCORDER_UNLOCK_GST_ENCODE_STATE(handle)        _MMCAMCORDER_UNLOCK_FUNC(_MMCAMCORDER_GET_GST_ENCODE_STATE_LOCK(handle))

/* for setting/calling callback */
#define _MMCAMCORDER_GET_MESSAGE_CALLBACK_LOCK(handle)      (_MMCAMCORDER_CAST_MTSAFE(handle).message_cb_lock)
#define _MMCAMCORDER_LOCK_MESSAGE_CALLBACK(handle)          _MMCAMCORDER_LOCK_FUNC(_MMCAMCORDER_GET_MESSAGE_CALLBACK_LOCK(handle))
#define _MMCAMCORDER_TRYLOCK_MESSAGE_CALLBACK(handle)       _MMCAMCORDER_TRYLOCK_FUNC(_MMCAMCORDER_GET_MESSAGE_CALLBACK_LOCK(handle))
#define _MMCAMCORDER_UNLOCK_MESSAGE_CALLBACK(handle)        _MMCAMCORDER_UNLOCK_FUNC(_MMCAMCORDER_GET_MESSAGE_CALLBACK_LOCK(handle))

#define _MMCAMCORDER_GET_VCAPTURE_CALLBACK_LOCK(handle)     (_MMCAMCORDER_CAST_MTSAFE(handle).vcapture_cb_lock)
#define _MMCAMCORDER_LOCK_VCAPTURE_CALLBACK(handle)         _MMCAMCORDER_LOCK_FUNC(_MMCAMCORDER_GET_VCAPTURE_CALLBACK_LOCK(handle))
#define _MMCAMCORDER_TRYLOCK_VCAPTURE_CALLBACK(handle)      _MMCAMCORDER_TRYLOCK_FUNC(_MMCAMCORDER_GET_VCAPTURE_CALLBACK_LOCK(handle))
#define _MMCAMCORDER_UNLOCK_VCAPTURE_CALLBACK(handle)       _MMCAMCORDER_UNLOCK_FUNC(_MMCAMCORDER_GET_VCAPTURE_CALLBACK_LOCK(handle))

#define _MMCAMCORDER_GET_VSTREAM_CALLBACK_LOCK(handle)      (_MMCAMCORDER_CAST_MTSAFE(handle).vstream_cb_lock)
#define _MMCAMCORDER_LOCK_VSTREAM_CALLBACK(handle)          _MMCAMCORDER_LOCK_FUNC(_MMCAMCORDER_GET_VSTREAM_CALLBACK_LOCK(handle))
#define _MMCAMCORDER_TRYLOCK_VSTREAM_CALLBACK(handle)       _MMCAMCORDER_TRYLOCK_FUNC(_MMCAMCORDER_GET_VSTREAM_CALLBACK_LOCK(handle))
#define _MMCAMCORDER_UNLOCK_VSTREAM_CALLBACK(handle)        _MMCAMCORDER_UNLOCK_FUNC(_MMCAMCORDER_GET_VSTREAM_CALLBACK_LOCK(handle))

#define _MMCAMCORDER_GET_ASTREAM_CALLBACK_LOCK(handle)      (_MMCAMCORDER_CAST_MTSAFE(handle).astream_cb_lock)
#define _MMCAMCORDER_LOCK_ASTREAM_CALLBACK(handle)          _MMCAMCORDER_LOCK_FUNC(_MMCAMCORDER_GET_ASTREAM_CALLBACK_LOCK(handle))
#define _MMCAMCORDER_TRYLOCK_ASTREAM_CALLBACK(handle)       _MMCAMCORDER_TRYLOCK_FUNC(_MMCAMCORDER_GET_ASTREAM_CALLBACK_LOCK(handle))
#define _MMCAMCORDER_UNLOCK_ASTREAM_CALLBACK(handle)        _MMCAMCORDER_UNLOCK_FUNC(_MMCAMCORDER_GET_ASTREAM_CALLBACK_LOCK(handle))

#define _MMCAMCORDER_GET_MSTREAM_CALLBACK_LOCK(handle)      (_MMCAMCORDER_CAST_MTSAFE(handle).mstream_cb_lock)
#define _MMCAMCORDER_LOCK_MSTREAM_CALLBACK(handle)          _MMCAMCORDER_LOCK_FUNC(_MMCAMCORDER_GET_MSTREAM_CALLBACK_LOCK(handle))
#define _MMCAMCORDER_TRYLOCK_MSTREAM_CALLBACK(handle)       _MMCAMCORDER_TRYLOCK_FUNC(_MMCAMCORDER_GET_MSTREAM_CALLBACK_LOCK(handle))
#define _MMCAMCORDER_UNLOCK_MSTREAM_CALLBACK(handle)        _MMCAMCORDER_UNLOCK_FUNC(_MMCAMCORDER_GET_MSTREAM_CALLBACK_LOCK(handle))

#ifdef _MMCAMCORDER_MURPHY_SUPPORT
/* for resource conflict */
#define _MMCAMCORDER_GET_RESOURCE_LOCK(handle)              (_MMCAMCORDER_CAST_MTSAFE(handle).resource_lock)
#define _MMCAMCORDER_GET_RESOURCE_COND(handle)              (_MMCAMCORDER_CAST_MTSAFE(handle).resource_cond)
#define _MMCAMCORDER_LOCK_RESOURCE(handle)                  _MMCAMCORDER_LOCK_FUNC(_MMCAMCORDER_GET_RESOURCE_LOCK(handle))
#define _MMCAMCORDER_TRYLOCK_RESOURCE(handle)               _MMCAMCORDER_TRYLOCK_FUNC(_MMCAMCORDER_GET_RESOURCE_LOCK(handle))
#define _MMCAMCORDER_UNLOCK_RESOURCE(handle)                _MMCAMCORDER_UNLOCK_FUNC(_MMCAMCORDER_GET_RESOURCE_LOCK(handle))
#define _MMCAMCORDER_RESOURCE_WAIT(handle)                  g_cond_wait(&_MMCAMCORDER_GET_RESOURCE_COND(handle), &_MMCAMCORDER_GET_RESOURCE_LOCK(handle))
#define _MMCAMCORDER_RESOURCE_WAIT_UNTIL(handle, end_time)  g_cond_wait_until(&_MMCAMCORDER_GET_RESOURCE_COND(handle), &_MMCAMCORDER_GET_RESOURCE_LOCK(handle), end_time)
#define _MMCAMCORDER_RESOURCE_SIGNAL(handle)                g_cond_signal(&_MMCAMCORDER_GET_RESOURCE_COND(handle));
#endif /* _MMCAMCORDER_MURPHY_SUPPORT */

/**
 * Caster of main handle (camcorder)
 */
#define MMF_CAMCORDER(h) (mmf_camcorder_t *)(h)

/**
 * Caster of subcontext
 */
#define MMF_CAMCORDER_SUBCONTEXT(h) (((mmf_camcorder_t *)(h))->sub_context)

/* LOCAL CONSTANT DEFINITIONS */
/**
 * Total Numbers of Attribute values.
 * If you increase any enum of attribute values, you also have to increase this.
 */
#define MM_CAMCORDER_MODE_NUM			3	/**< Number of mode type */
#define MM_CAMCORDER_COLOR_TONE_NUM		30	/**< Number of color-tone modes */
#define MM_CAMCORDER_WHITE_BALANCE_NUM		10	/**< Number of WhiteBalance modes*/
#define MM_CAMCORDER_SCENE_MODE_NUM		16	/**< Number of program-modes */
#define MM_CAMCORDER_FOCUS_MODE_NUM		6	/**< Number of focus mode*/
#define MM_CAMCORDER_AUTO_FOCUS_NUM		5	/**< Total count of auto focus type*/
#define MM_CAMCORDER_FOCUS_STATE_NUM		4	/**< Number of focus state */
#define MM_CAMCORDER_ISO_NUM			10	/**< Number of ISO */
#define MM_CAMCORDER_AUTO_EXPOSURE_NUM		9	/**< Number of Auto exposure type */
#define MM_CAMCORDER_WDR_NUM			3	/**< Number of wide dynamic range */
#define MM_CAMCORDER_FLIP_NUM			4	/**< Number of Filp mode */
#define MM_CAMCORDER_ROTATION_NUM		4	/**< Number of Rotation mode */
#define MM_CAMCORDER_AHS_NUM			4	/**< Number of anti-handshake */
#define MM_CAMCORDER_VIDEO_STABILIZATION_NUM	2	/**< Number of video stabilization */
#define MM_CAMCORDER_HDR_CAPTURE_NUM		3	/**< Number of HDR capture mode */
#define MM_CAMCORDER_GEOMETRY_METHOD_NUM	5	/**< Number of geometry method */
#define MM_CAMCORDER_TAG_ORT_NUM		8	/**< Number of tag orientation */
#define MM_CAMCORDER_STROBE_MODE_NUM		8	/**< Number of strobe mode type */
#define MM_CAMCORDER_STROBE_CONTROL_NUM		3	/**< Number of strobe control type */
#define MM_CAMCORDER_DETECT_MODE_NUM		2	/**< Number of detect mode type */

/*=======================================================================================
| ENUM DEFINITIONS									|
========================================================================================*/
/**
 * Command for Camcorder.
 */
enum {
	/* Command for Video/Audio recording */
	_MMCamcorder_CMD_RECORD,
	_MMCamcorder_CMD_PAUSE,
	_MMCamcorder_CMD_CANCEL,
	_MMCamcorder_CMD_COMMIT,

	/* Command for Image capture */
	_MMCamcorder_CMD_CAPTURE,

	/* Command for Preview(Video/Image only effective) */
	_MMCamcorder_CMD_PREVIEW_START,
	_MMCamcorder_CMD_PREVIEW_STOP,
};

/**
 * Still-shot type
 */
enum {
	_MMCamcorder_SINGLE_SHOT,
	_MMCamcorder_MULTI_SHOT,
};

/**
 * Enumerations for manual focus direction.
 * If focusing mode is not "MM_CAMCORDER_AF_MODE_MANUAL", this value will be ignored.
 */
enum MMCamcorderMfLensDir {
	MM_CAMCORDER_MF_LENS_DIR_FORWARD = 1,	/**< Focus direction to forward */
	MM_CAMCORDER_MF_LENS_DIR_BACKWARD,	/**< Focus direction to backward */
	MM_CAMCORDER_MF_LENS_DIR_NUM,		/**< Total number of the directions */
};

/**
 * Camcorder Pipeline's Element name.
 * @note index of element.
 */
typedef enum {
	_MMCAMCORDER_NONE = (-1),

	/* Main Pipeline Element */
	_MMCAMCORDER_MAIN_PIPE = 0x00,

	/* Pipeline element of Video input */
	_MMCAMCORDER_VIDEOSRC_SRC,
	_MMCAMCORDER_VIDEOSRC_FILT,
	_MMCAMCORDER_VIDEOSRC_CLS_QUE,
	_MMCAMCORDER_VIDEOSRC_CLS,
	_MMCAMCORDER_VIDEOSRC_CLS_FILT,
	_MMCAMCORDER_VIDEOSRC_QUE,
	_MMCAMCORDER_VIDEOSRC_DECODE,

	/* Pipeline element of Video output */
	_MMCAMCORDER_VIDEOSINK_QUE,
	_MMCAMCORDER_VIDEOSINK_CLS,
	_MMCAMCORDER_VIDEOSINK_SINK,

	_MMCAMCORDER_PIPELINE_ELEMENT_NUM,
} _MMCAMCORDER_PREVIEW_PIPELINE_ELELMENT;

/**
 * Camcorder Pipeline's Element name.
 * @note index of element.
 */
typedef enum {
	_MMCAMCORDER_ENCODE_NONE = (-1),

	/* Main Pipeline Element */
	_MMCAMCORDER_ENCODE_MAIN_PIPE = 0x00,

	/* Pipeline element of Audio input */
	_MMCAMCORDER_AUDIOSRC_BIN,
	_MMCAMCORDER_AUDIOSRC_SRC,
	_MMCAMCORDER_AUDIOSRC_FILT,
	_MMCAMCORDER_AUDIOSRC_QUE,
	_MMCAMCORDER_AUDIOSRC_CONV,
	_MMCAMCORDER_AUDIOSRC_VOL,

	/* Pipeline element of Encodebin */
	_MMCAMCORDER_ENCSINK_BIN,
	_MMCAMCORDER_ENCSINK_SRC,
	_MMCAMCORDER_ENCSINK_FILT,
	_MMCAMCORDER_ENCSINK_ENCBIN,
	_MMCAMCORDER_ENCSINK_AQUE,
	_MMCAMCORDER_ENCSINK_CONV,
	_MMCAMCORDER_ENCSINK_AENC,
	_MMCAMCORDER_ENCSINK_AENC_QUE,
	_MMCAMCORDER_ENCSINK_VQUE,
	_MMCAMCORDER_ENCSINK_VCONV,
	_MMCAMCORDER_ENCSINK_VENC,
	_MMCAMCORDER_ENCSINK_VENC_QUE,
	_MMCAMCORDER_ENCSINK_ITOG,
	_MMCAMCORDER_ENCSINK_ICROP,
	_MMCAMCORDER_ENCSINK_ISCALE,
	_MMCAMCORDER_ENCSINK_IFILT,
	_MMCAMCORDER_ENCSINK_IQUE,
	_MMCAMCORDER_ENCSINK_IENC,
	_MMCAMCORDER_ENCSINK_MUX,
	_MMCAMCORDER_ENCSINK_SINK,

	_MMCAMCORDER_ENCODE_PIPELINE_ELEMENT_NUM,
} _MMCAMCORDER_ENCODE_PIPELINE_ELELMENT;

typedef enum {
	_MMCAMCORDER_TASK_THREAD_STATE_NONE,
	_MMCAMCORDER_TASK_THREAD_STATE_SOUND_PLAY_START,
	_MMCAMCORDER_TASK_THREAD_STATE_SOUND_SOLO_PLAY_START,
	_MMCAMCORDER_TASK_THREAD_STATE_ENCODE_PIPE_CREATE,
	_MMCAMCORDER_TASK_THREAD_STATE_CHECK_CAPTURE_IN_RECORDING,
	_MMCAMCORDER_TASK_THREAD_STATE_EXIT,
} _MMCamcorderTaskThreadState;

/**
 * System state change cause
 */
typedef enum {
	_MMCAMCORDER_STATE_CHANGE_NORMAL = 0,
	_MMCAMCORDER_STATE_CHANGE_BY_FOCUS,
	_MMCAMCORDER_STATE_CHANGE_BY_RM,
	_MMCAMCORDER_STATE_CHANGE_BY_DPM
} _MMCamcorderStateChange;


/*=======================================================================================
| STRUCTURE DEFINITIONS									|
========================================================================================*/
/**
 * MMCamcorder Gstreamer Element
 */
typedef struct {
	unsigned int id;		/**< Gstreamer piplinem element name */
	GstElement *gst;		/**< Gstreamer element */
} _MMCamcorderGstElement;

/**
 * MMCamcorder information for KPI measurement
 */
typedef struct {
	int current_fps;		/**< current fps of this second */
	int average_fps;		/**< average fps  */
	unsigned int video_framecount;	/**< total number of video frame */
	unsigned int last_framecount;	/**< total number of video frame in last measurement */
	struct timeval init_video_time;	/**< time when start to measure */
	struct timeval last_video_time;	/**< last measurement time */
} _MMCamcorderKPIMeasure;

/**
 * MMCamcorder information for Multi-Thread Safe
 */
typedef struct {
	GMutex lock;                    /**< Mutex (for general use) */
	GCond cond;                     /**< Condition (for general use) */
	GMutex cmd_lock;                /**< Mutex (for command) */
	GCond cmd_cond;                 /**< Condition (for command) */
	GMutex asm_lock;                /**< Mutex (for ASM) */
	GMutex state_lock;              /**< Mutex (for state change) */
	GMutex gst_state_lock;          /**< Mutex (for gst pipeline state change) */
	GMutex gst_encode_state_lock;   /**< Mutex (for gst encode pipeline state change) */
	GMutex message_cb_lock;         /**< Mutex (for message callback) */
	GMutex vcapture_cb_lock;        /**< Mutex (for video capture callback) */
	GMutex vstream_cb_lock;         /**< Mutex (for video stream callback) */
	GMutex astream_cb_lock;         /**< Mutex (for audio stream callback) */
	GMutex mstream_cb_lock;         /**< Mutex (for muxed stream callback) */
#ifdef _MMCAMCORDER_MURPHY_SUPPORT
	GCond resource_cond;            /**< Condition (for resource check) */
	GMutex resource_lock;           /**< Mutex (for resource check) */
#endif /* _MMCAMCORDER_MURPHY_SUPPORT */
} _MMCamcorderMTSafe;

/**
 * MMCamcorder Sub Context
 */
typedef struct {
	bool isMaxsizePausing;                  /**< Because of size limit, pipeline is paused. */
	bool isMaxtimePausing;                  /**< Because of time limit, pipeline is paused. */
	int element_num;                        /**< count of element */
	int encode_element_num;                 /**< count of encode element */
	int cam_stability_count;                /**< camsensor stability count. the count of frame will drop */
	GstClockTime pipeline_time;             /**< current time of Gstreamer Pipeline */
	GstClockTime pause_time;                /**< amount of time while pipeline is in PAUSE state.*/
	GstClockTime stillshot_time;            /**< pipeline time of capturing moment*/
	gboolean is_modified_rate;              /**< whether recording motion rate is modified or not */
	gboolean ferror_send;                   /**< file write/seek error **/
	guint ferror_count;                     /**< file write/seek error count **/
	GstClockTime previous_slot_time;
	int display_interval;                   /**< This value is set as 'GST_SECOND / display FPS' */
	gboolean bget_eos;                      /**< Whether getting EOS */
	gboolean bencbin_capture;               /**< Use Encodebin for capturing */
	gboolean audio_disable;                 /**< whether audio is disabled or not when record */
	int videosrc_rotate;                    /**< rotate of videosrc */
	unsigned long long muxed_stream_offset; /**< current offset for muxed stream data */

	/* For dropping video frame when start recording */
	int drop_vframe;                        /**< When this value is bigger than zero and pass_first_vframe is zero, MSL will drop video frame though cam_stability count is bigger then zero. */
	int pass_first_vframe;                  /**< When this value is bigger than zero, MSL won't drop video frame though "drop_vframe" is bigger then zero. */

	/* INI information */
	unsigned int fourcc;                    /**< Get fourcc value of camera INI file */
	_MMCamcorderImageInfo *info_image;      /**< extra information for image capture */
	_MMCamcorderVideoInfo *info_video;      /**< extra information for video recording */
	_MMCamcorderAudioInfo *info_audio;      /**< extra information for audio recording */

	_MMCamcorderGstElement *element;        /**< array of preview element */
	_MMCamcorderGstElement *encode_element; /**< array of encode element */
	_MMCamcorderKPIMeasure kpi;             /**< information related with performance measurement */

	type_element *VideosinkElement;         /**< configure data of videosink element */
	type_element *VideoconvertElement;      /**< configure data of videoconvert element */
	type_element *VideodecoderElementH264;  /**< configure data of video decoder element for H.264 format */
	gboolean SensorEncodedCapture;          /**< whether camera sensor support encoded image capture */
	gboolean internal_encode;               /**< whether use internal encoding function */
} _MMCamcorderSubContext;

/**
  * _MMCamcorderContext
  */
typedef struct mmf_camcorder {
	/* information */
	int type;               /**< mmcamcorder_mode_type */
	int device_type;        /**< device type */
	int state;              /**< state of camcorder */
	int target_state;       /**< Target state that want to set. This is a flag that
							   * stands for async state changing. If this value differ from state,
							   * it means state is changing now asychronously. */

	/* handles */
	MMHandleType attributes;               /**< Attribute handle */
	_MMCamcorderSubContext *sub_context;   /**< sub context */
	mm_exif_info_t *exif_info;             /**< EXIF */
	GList *buffer_probes;                  /**< a list of buffer probe handle */
	GList *event_probes;                   /**< a list of event probe handle */
	GList *signals;                        /**< a list of signal handle */
#ifdef _MMCAMCORDER_ENABLE_IDLE_MESSAGE_CALLBACK
	GList *msg_data;                       /**< a list of msg data */
#endif /* _MMCAMCORDER_ENABLE_IDLE_MESSAGE_CALLBACK */
	camera_conf *conf_main;                /**< Camera configure Main structure */
	camera_conf *conf_ctrl;                /**< Camera configure Control structure */
#ifdef _MMCAMCORDER_RM_SUPPORT
	int rm_handle;                        /**< Resource manager handle */
#endif /* _MMCAMCORDER_RM_SUPPORT */
	guint pipeline_cb_event_id;            /**< Event source ID of pipeline message callback */
	guint encode_pipeline_cb_event_id;     /**< Event source ID of encode pipeline message callback */
	guint setting_event_id;                /**< Event source ID of attributes setting to sensor */
	SOUND_INFO snd_info;                   /**< Sound handle for multishot capture */

	/* callback handlers */
	MMMessageCallback msg_cb;                               /**< message callback */
	void *msg_cb_param;                                     /**< message callback parameter */
	mm_camcorder_video_stream_callback vstream_cb;          /**< Video stream callback */
	void *vstream_cb_param;                                 /**< Video stream callback parameter */
	mm_camcorder_audio_stream_callback astream_cb;          /**< Audio stream callback */
	void *astream_cb_param;                                 /**< Audio stream callback parameter */
	mm_camcorder_muxed_stream_callback mstream_cb;          /**< Muxed stream callback */
	void *mstream_cb_param;                                 /**< Muxed stream callback parameter */
	mm_camcorder_video_capture_callback vcapture_cb;        /**< Video capture callback */
	void *vcapture_cb_param;                                /**< Video capture callback parameter */
	int (*command)(MMHandleType, int);                      /**< camcorder's command */

	/* etc */
	mm_cam_attr_construct_info *cam_attrs_const_info;       /**< attribute info */
	conf_info_table *conf_main_info_table[CONFIGURE_CATEGORY_MAIN_NUM]; /** configure info table - MAIN category */
	conf_info_table *conf_ctrl_info_table[CONFIGURE_CATEGORY_CTRL_NUM]; /** configure info table - CONTROL category */
	int conf_main_category_size[CONFIGURE_CATEGORY_MAIN_NUM]; /** configure info table size - MAIN category */
	int conf_ctrl_category_size[CONFIGURE_CATEGORY_CTRL_NUM]; /** configure info table size - CONTROL category */
	_MMCamcorderMTSafe mtsafe;                              /**< Thread safe */
	int state_change_by_system;                             /**< MSL changes its state by itself because of system */
	GMutex restart_preview_lock;                            /**< Capture sound mutex */
	int use_zero_copy_format;                               /**< Whether use zero copy format for camera input */
	int use_videoconvert;                                   /**< Whether use videoconvert element for display */
	int support_media_packet_preview_cb;                    /**< Whether support zero copy format for camera input */
	int shutter_sound_policy;                               /**< shutter sound policy */
	int brightness_default;                                 /**< default value of brightness */
	int brightness_step_denominator;                        /**< denominator of brightness bias step */
	int support_zsl_capture;                                /**< support Zero Shutter Lag capture */
	char *model_name;                                       /**< model name from system info */
	char *software_version;                                 /**< software_version from system info */
	int capture_sound_count;                                /**< count for capture sound */
	char *root_directory;                                   /**< Root directory for device */
	int resolution_changed;                                 /**< Flag for preview resolution change */
	int sound_focus_register;                               /**< Use sound focus internally */
	int sound_focus_id;                                     /**< id for sound focus */
	int sound_focus_watch_id;                               /**< id for sound focus watch */
	unsigned int sound_focus_subscribe_id;                  /**< subscribe id for sound focus signal */
	int interrupt_code;                                     /**< Interrupt code */
	int acquired_focus;                                     /**< Current acquired focus */
	int session_type;                                       /**< Session type */
	int session_flags;                                      /**< Session flags */
	int recreate_decoder;                                   /**< Flag of decoder element recreation for encoded preview format */

	_MMCamcorderInfoConverting caminfo_convert[CAMINFO_CONVERT_NUM];        /**< converting structure of camera info */
	_MMCamcorderEnumConvert enum_conv[ENUM_CONVERT_NUM];                    /**< enum converting list that is modified by ini info */

	gboolean capture_in_recording;                          /**< Flag for capture while recording */

	gboolean error_occurs;                                  /**< flag for error */
	int error_code;                                         /**< error code for internal gstreamer error */

	/* task thread */
	GThread *task_thread;                                   /**< thread for task */
	GMutex task_thread_lock;                                /**< mutex for task thread */
	GCond task_thread_cond;                                 /**< cond for task thread */
	_MMCamcorderTaskThreadState task_thread_state;          /**< state of task thread */

#ifdef _MMCAMCORDER_MURPHY_SUPPORT
	/* resource manager for H/W resources */
	MMCamcorderResourceManager resource_manager;
#endif /* _MMCAMCORDER_MURPHY_SUPPORT */

	/* gdbus */
	GDBusConnection *gdbus_conn;                            /**< gdbus connection */
	_MMCamcorderGDbusCbInfo gdbus_info_sound;               /**< Informations for the gbus cb of sound play */
	_MMCamcorderGDbusCbInfo gdbus_info_solo_sound;          /**< Informations for the gbus cb of solo sound play */

	/* DPM(device policy manager) */
	device_policy_manager_h dpm_handle;                     /**< DPM handle */
	int dpm_camera_cb_id;                                   /**< DPM camera policy changed callback id */

	/* Storage */
	_MMCamcorderStorageInfo storage_info;                   /**< Storage information */

#ifdef _MMCAMCORDER_RM_SUPPORT
	rm_category_request_s request_resources;
	rm_device_return_s returned_devices;
#endif /* _MMCAMCORDER_RM_SUPPORT */
	int reserved[4];                                        /**< reserved */
} mmf_camcorder_t;

/*=======================================================================================
| EXTERN GLOBAL VARIABLE								|
========================================================================================*/

/*=======================================================================================
| GLOBAL FUNCTION PROTOTYPES								|
========================================================================================*/
/**
 *	This function creates camcorder for capturing still image and recording.
 *
 *	@param[out]	handle		Specifies the camcorder  handle
 *	@param[in]	info		Preset information of camcorder
 *	@return		This function returns zero on success, or negative value with error code.
 *	@remarks	When this function calls successfully, camcorder  handle will be filled with a @n
 *			valid value and the state of  the camcorder  will become MM_CAMCORDER_STATE_NULL.@n
 *			Note that  it's not ready to working camcorder. @n
 *			You should call mmcamcorder_realize before starting camcorder.
 *	@see		_mmcamcorder_create
 */
int _mmcamcorder_create(MMHandleType *handle, MMCamPreset *info);

/**
 *	This function destroys instance of camcorder.
 *
 *	@param[in]	hcamcorder	Specifies the camcorder  handle
 *	@return		This function returns zero on success, or negative value with error code.
 *	@see		_mmcamcorder_create
 */
int _mmcamcorder_destroy(MMHandleType hcamcorder);

/**
 *	This function allocates memory for camcorder.
 *
 *	@param[in]	hcamcorder	Specifies the camcorder  handle
 *	@return		This function returns zero on success, or negative value with error code.
 *	@remarks	This function can  be called successfully when current state is MM_CAMCORDER_STATE_NULL @n
 *			and  the state of the camcorder  will become MM_CAMCORDER_STATE_READY. @n
 *			Otherwise, this function will return MM_ERROR_CAMCORDER_INVALID_CONDITION.
 *	@see		_mmcamcorder_unrealize
 *	@pre		MM_CAMCORDER_STATE_NULL
 *	@post		MM_CAMCORDER_STATE_READY
 */
int _mmcamcorder_realize(MMHandleType hcamcorder);

/**
 *	This function free allocated memory for camcorder.
 *
 *	@param[in]	hcamcorder	Specifies the camcorder  handle
 *	@return		This function returns zero on success, or negative value with error code.
 *	@remarks	This function release all resources which are allocated for the camcorder engine.@n
 *			This function can  be called successfully when current state is MM_CAMCORDER_STATE_READY and  @n
 *			the state of the camcorder  will become MM_CAMCORDER_STATE_NULL. @n
 *			Otherwise, this function will return MM_ERROR_CAMCORDER_INVALID_CONDITION.
 *	@see		_mmcamcorder_realize
 *	@pre		MM_CAMCORDER_STATE_READY
 *	@post		MM_CAMCORDER_STATE_NULL
 */
int _mmcamcorder_unrealize(MMHandleType hcamcorder);

/**
 *	This function is to start previewing.
 *
 *	@param[in]	hcamcorder	Specifies the camcorder  handle
 *	@return		This function returns zero on success, or negative value with error code.
 *	@remarks	This function can  be called successfully when current state is MM_CAMCORDER_STATE_READY and  @n
 *			the state of the camcorder  will become MM_CAMCORDER_STATE_PREPARE. @n
 *			Otherwise, this function will return MM_ERROR_CAMCORDER_INVALID_CONDITION.
 *	@see		_mmcamcorder_stop
 */
int _mmcamcorder_start(MMHandleType hcamcorder);

/**
 *	This function is to stop previewing.
 *
 *	@param[in]	hcamcorder	Specifies the camcorder  handle
 *	@return		This function returns zero on success, or negative value with error code.
 *	@remarks	This function can  be called successfully when current state is MM_CAMCORDER_STATE_PREPARE and  @n
 *			the state of the camcorder  will become MM_CAMCORDER_STATE_READY.@n
 *			Otherwise, this function will return MM_ERROR_CAMCORDER_INVALID_CONDITION.
 *	@see		_mmcamcorder_start
 */
int _mmcamcorder_stop(MMHandleType hcamcorder);

/**
 *	This function to start capturing of still images.
 *
 *	@param[in]	hcamcorder	Specifies the camcorder  handle.
 *	@return		This function returns zero on success, or negative value with error code.
 *	@remarks	This function can  be called successfully when current state is MM_CAMCORDER_STATE_PREPARE and @n
 *			the state of the camcorder  will become MM_CAMCORDER_STATE_CAPTURING. @n
 *			Otherwise, this function will return MM_ERROR_CAMCORDER_INVALID_CONDITION.
 *	@see		_mmcamcorder_capture_stop
 */
int _mmcamcorder_capture_start(MMHandleType hcamcorder);

/**
 *	This function is to stop capturing still images.
 *
 *	@param[in]	hcamcorder	Specifies the camcorder  handle
 *	@return		This function returns zero on success, or negative value with error code.
 *	@remarks	This function can  be called successfully when current state is MM_CAMCORDER_STATE_CAPTURING and @n
 *			the state of the camcorder  will become MM_CAMCORDER_STATE_PREPARE. @n
 *			Otherwise, this function will return MM_ERROR_CAMCORDER_INVALID_CONDITION.
 *	@see		_mmcamcorder_capture_start
 */
int _mmcamcorder_capture_stop(MMHandleType hcamcorder);

/**
 *	This function is to start  video and audio recording.
 *
 *	@param[in]	hcamcorder	Specifies the camcorder  handle
 *	@return		This function returns zero on success, or negative value with error code.
 *	@remarks	This function can  be called successfully when current state is @n
 *			MM_CAMCORDER_STATE_PREPARE or MM_CAMCORDER_STATE_PAUSED and  @n
 *			the state of the camcorder  will become MM_CAMCORDER_STATE_RECORDING.@n
 *			Otherwise, this function will return MM_ERROR_CAMCORDER_INVALID_CONDITION.
 *	@see		_mmcamcorder_pause
 */
int _mmcamcorder_record(MMHandleType hcamcorder);

/**
 *	This function is to pause video and audio recording
 *
 *	@param[in]	hcamcorder	Specifies the camcorder  handle
 *	@return		This function returns zero on success, or negative value with error code.
 *	@remarks	This function can  be called successfully when current state is MM_CAMCORDER_STATE_RECORDING and  @n
 *			the  state of the camcorder  will become MM_CAMCORDER_STATE_PAUSED.@n
 *			Otherwise, this function will return MM_ERROR_CAMCORDER_INVALID_CONDITION.@n
 *	@see		_mmcamcorder_record
 */
int _mmcamcorder_pause(MMHandleType hcamcorder);

/**
 *	This function is to stop video and audio  recording and  save results.
 *
 *	@param[in]	hcamcorder	Specifies the camcorder  handle
 *	@return		This function returns zero on success, or negative value with error code.
 *	@remarks	This function can  be called successfully when current state is @n
 *			MM_CAMCORDER_STATE_PAUSED or MM_CAMCORDER_STATE_RECORDING and  @n
 *			the state of the camcorder  will become MM_CAMCORDER_STATE_PREPARE. @n
 *			Otherwise, this function will return MM_ERROR_CAMCORDER_INVALID_CONDITION
 *	@see		_mmcamcorder_cancel
 */
int _mmcamcorder_commit(MMHandleType hcamcorder);

/**
 *	This function is to stop video and audio recording and do not save results.
 *
 *	@param[in]	hcamcorder	Specifies the camcorder  handle
 *	@return		This function returns zero on success, or negative value with error code.
 *	@remarks	This function can  be called successfully when current state is @n
 *			MM_CAMCORDER_STATE_PAUSED or MM_CAMCORDER_STATE_RECORDING and  @n
 *			the state of the camcorder  will become MM_CAMCORDER_STATE_PREPARE. @n
 *			Otherwise, this function will return MM_ERROR_CAMCORDER_INVALID_CONDITION.
 *	@see		_mmcamcorder_commit
 */
int _mmcamcorder_cancel(MMHandleType hcamcorder);

/**
 *	This function calls after commiting action finished asynchronously.
 *	In this function, remaining process , such as state change, happens.
 *
 *	@param[in]	hcamcorder	Specifies the camcorder  handle
 *	@return		This function returns zero on success, or negative value with error code.
 *	@remarks	This function can  be called successfully when current state is @n
 *			MM_CAMCORDER_STATE_PAUSED or MM_CAMCORDER_STATE_RECORDING and  @n
 *			the state of the camcorder  will become MM_CAMCORDER_STATE_PREPARE. @n
 *			Otherwise, this function will return MM_ERROR_CAMCORDER_INVALID_CONDITION.
 *	@see		_mmcamcorder_commit
 */
int _mmcamcorder_commit_async_end(MMHandleType hcamcorder);

/**
 *	This function is to set callback for receiving messages from camcorder.
 *
 *	@param[in]	hcamcorder	Specifies the camcorder  handle
 *	@param[in]	callback	Specifies the function pointer of callback function
 *	@param[in]	user_data	Specifies the user poiner for passing to callback function
 *
 *	@return		This function returns zero on success, or negative value with error code.
 *	@remarks	typedef bool (*mm_message_callback) (int msg, mm_messageType *param, void *user_param);@n
 *		@n
 *		typedef union				@n
 *		{							@n
 *			int code;				@n
 *			struct					@n
 *			{						@n
 *				int total;			@n
 *				int elapsed;		@n
 *			} time;					@n
 *			struct					@n
 *			{						@n
 *				int previous;		@n
 *				int current;			@n
 *			} state;					@n
 *		} mm_message_type;	@n
 *									@n
 *		If a  message value for mm_message_callback is MM_MESSAGE_STATE_CHANGED, @n
 *		state value in mm_message_type  will be a mmcamcorder_state_type enum value;@n
 *		@n
 *		If  a message value for mm_message_callback is MM_MESSAGE_ERROR,  @n
 *		the code value in mm_message_type will be a mmplayer_error_type enum value;
 *
 *	@see		mm_message_type,  mmcamcorder_state_type,  mmcamcorder_error_type
 */
int _mmcamcorder_set_message_callback(MMHandleType hcamcorder,
				      MMMessageCallback callback,
				      void *user_data);

/**
 *	This function is to set callback for video stream.
 *
 *	@param[in]	hcamcorder	Specifies the camcorder  handle
 *	@param[in]	callback	Specifies the function pointer of callback function
 *	@param[in]	user_data	Specifies the user poiner for passing to callback function
 *
 *	@return		This function returns zero on success, or negative value with error code.
 *	@see		mmcamcorder_error_type
 */
int _mmcamcorder_set_video_stream_callback(MMHandleType hcamcorder,
					   mm_camcorder_video_stream_callback callback,
					   void *user_data);

/**
 *	This function is to set callback for audio stream.
 *
 *	@param[in]	hcamcorder	Specifies the camcorder handle
 *	@param[in]	callback	Specifies the function pointer of callback function
 *	@param[in]	user_data	Specifies the user poiner for passing to callback function
 *
 *	@return		This function returns zero on success, or negative value with error code.
 *	@see		mmcamcorder_error_type
 */
int _mmcamcorder_set_audio_stream_callback(MMHandleType handle,
					   mm_camcorder_audio_stream_callback callback,
					   void *user_data);

/**
 *	This function is to set callback for muxed stream.
 *
 *	@param[in]	hcamcorder	Specifies the camcorder handle
 *	@param[in]	callback	Specifies the function pointer of callback function
 *	@param[in]	user_data	Specifies the user poiner for passing to callback function
 *
 *	@return		This function returns zero on success, or negative value with error code.
 *	@see		mmcamcorder_error_type
 */
int _mmcamcorder_set_muxed_stream_callback(MMHandleType handle,
					   mm_camcorder_muxed_stream_callback callback,
					   void *user_data);

/**
 *	This function is to set callback for video capture.
 *
 *	@param[in]	hcamcorder	Specifies the camcorder  handle
 *	@param[in]	callback	Specifies the function pointer of callback function
 *	@param[in]	user_data	Specifies the user poiner for passing to callback function
 *
 *	@return		This function returns zero on success, or negative value with error code.
 *	@see		mmcamcorder_error_type
 */
int _mmcamcorder_set_video_capture_callback(MMHandleType hcamcorder,
					    mm_camcorder_video_capture_callback callback,
					    void *user_data);

/**
 *	This function returns current state of camcorder, or negative value with error code.
 *
 *	@param[in]	hcamcorder	Specifies the camcorder  handle.
 *	@return		This function returns current state of camcorder, or negative value with error code.
 *	@see		mmcamcorder_state_type
 */
int _mmcamcorder_get_current_state(MMHandleType hcamcorder);

int _mmcamcorder_init_focusing(MMHandleType handle);
int _mmcamcorder_adjust_focus(MMHandleType handle, int direction);
int _mmcamcorder_adjust_manual_focus(MMHandleType handle, int direction);
int _mmcamcorder_adjust_auto_focus(MMHandleType handle);
int _mmcamcorder_stop_focusing(MMHandleType handle);

/**
 * This function gets current state of camcorder.
 *
 * @param	void
 * @return	This function returns state of current camcorder context
 * @remarks
 * @see		_mmcamcorder_set_state()
 *
 */
int _mmcamcorder_streamer_init(void);

/**
 * This function gets current state of camcorder.
 *
 * @param	void
 * @return	This function returns state of current camcorder context
 * @remarks
 * @see		_mmcamcorder_set_state()
 *
 */
int _mmcamcorder_display_init(void);

/**
 * This function gets current state of camcorder.
 *
 * @param[in]	handle		Handle of camcorder context.
 * @return	This function returns state of current camcorder context
 * @remarks
 * @see		_mmcamcorder_set_state()
 *
 */
int _mmcamcorder_get_state(MMHandleType handle);

/**
 * This function sets new state of camcorder.
 *
 * @param[in]	handle		Handle of camcorder context.
 * @param[in]	state		setting state value of camcorder.
 * @return	void
 * @remarks
 * @see		_mmcamcorder_get_state()
 *
 */
void _mmcamcorder_set_state(MMHandleType handle, int state);

/**
 * This function gets asynchronous status of MSL Camcroder.
 *
 * @param[in]	handle		Handle of camcorder context.
 * @param[in]	target_state	setting target_state value of camcorder.
 * @return	This function returns asynchrnous state.
 * @remarks
 * @see		_mmcamcorder_set_async_state()
 *
 */
int _mmcamcorder_get_async_state(MMHandleType handle);

/**
 * This function allocates structure of subsidiary attributes.
 *
 * @param[in]	type		Allocation type of camcorder context.
 * @return	This function returns structure pointer on success, NULL value on failure.
 * @remarks
 * @see		_mmcamcorder_dealloc_subcontext()
 *
 */
_MMCamcorderSubContext *_mmcamcorder_alloc_subcontext(int type);

/**
 * This function releases structure of subsidiary attributes.
 *
 * @param[in]	sc		Handle of camcorder subcontext.
 * @return	void
 * @remarks
 * @see		_mmcamcorder_alloc_subcontext()
 *
 */
void _mmcamcorder_dealloc_subcontext(_MMCamcorderSubContext *sc);

/**
 * This function sets command function according to the type.
 *
 * @param[in]	handle		Handle of camcorder context.
 * @param[in]	type		Allocation type of camcorder context.
 * @return	This function returns MM_ERROR_NONE on success, or other values with error code.
 * @remarks
 * @see		__mmcamcorder_video_command(), __mmcamcorder_audio_command(), __mmcamcorder_image_command()
 *
 */
int _mmcamcorder_set_functions(MMHandleType handle, int type);

/**
 * This function is callback function of main pipeline.
 * Once this function is registered with certain pipeline using gst_bus_add_watch(),
 * this callback will be called every time when there is upcomming message from pipeline.
 * Basically, this function is used as error handling function, now.
 *
 * @param[in]	bus		pointer of buf that called this function.
 * @param[in]	message		callback message from pipeline.
 * @param[in]	data		user data.
 * @return	This function returns true on success, or false value with error
 * @remarks
 * @see		__mmcamcorder_create_preview_pipeline()
 *
 */
gboolean _mmcamcorder_pipeline_cb_message(GstBus *bus, GstMessage *message, gpointer data);

/**
 * This function is callback function of main pipeline.
 * Once this function is registered with certain pipeline using gst_bus_set_sync_handler(),
 * this callback will be called every time when there is upcomming message from pipeline.
 * Basically, this function is used as sync error handling function, now.
 *
 * @param[in]	bus		pointer of buf that called this function.
 * @param[in]	message		callback message from pipeline.
 * @param[in]	data		user data.
 * @return	This function returns true on success, or false value with error
 * @remarks
 * @see		__mmcamcorder_create_preview_pipeline()
 *
 */
GstBusSyncReply _mmcamcorder_pipeline_bus_sync_callback(GstBus *bus, GstMessage *message, gpointer data);

/**
 * This function is callback function of encode pipeline.
 * Once this function is registered with certain pipeline using gst_bus_set_sync_handler(),
 * this callback will be called every time when there is upcomming message from pipeline.
 * Basically, this function is used for EOS and error handling now.
 *
 * @param[in]	bus		pointer of buf that called this function.
 * @param[in]	message		callback message from pipeline.
 * @param[in]	data		user data.
 * @return	This function returns true on success, or false value with error
 * @remarks
 * @see		__mmcamcorder_create_recorder_pipeline()
 * @see		__mmcamcorder_create_audiop_with_encodebin()
 *
 */
GstBusSyncReply _mmcamcorder_encode_pipeline_bus_sync_callback(GstBus *bus, GstMessage *message, gpointer data);

/**
 * This function create main pipeline according to type.
 *
 * @param[in]	handle		Handle of camcorder context.
 * @param[in]	type		Allocation type of camcorder context.
 * @return	This function returns zero on success, or negative value with error code.
 * @remarks
 * @see		_mmcamcorder_destroy_pipeline()
 *
 */
int _mmcamcorder_create_pipeline(MMHandleType handle, int type);

/**
 * This function release all element of main pipeline according to type.
 *
 * @param[in]	handle		Handle of camcorder context.
 * @param[in]	type		Allocation type of camcorder context.
 * @return	void
 * @remarks
 * @see		_mmcamcorder_create_pipeline()
 *
 */
void _mmcamcorder_destroy_pipeline(MMHandleType handle, int type);

/**
 * This function sets gstreamer element status.
 * If the gstreamer fails to set status or returns asynchronous mode,
 * this function waits for state changed until timeout expired.
 *
 * @param[in]	pipeline	Pointer of pipeline
 * @param[in]	target_state	newly setting status
 * @return	This function returns zero on success, or negative value with error code.
 * @remarks
 * @see
 *
 */
int _mmcamcorder_gst_set_state(MMHandleType handle, GstElement *pipeline, GstState target_state);

/**
 * This function sets gstreamer element status, asynchronously.
 * Regardless of processing, it returns immediately.
 *
 * @param[in]	pipeline	Pointer of pipeline
 * @param[in]	target_state	newly setting status
 * @return	This function returns zero on success, or negative value with error code.
 * @remarks
 * @see
 *
 */
int _mmcamcorder_gst_set_state_async(MMHandleType handle, GstElement *pipeline, GstState target_state);

/* For xvimagesink */
GstBusSyncReply __mmcamcorder_sync_callback(GstBus *bus, GstMessage *message, gulong data);

/* For querying capabilities */
int _mmcamcorder_read_vidsrc_info(int videodevidx, camera_conf **configure_info);

/* for performance check */
void _mmcamcorder_video_current_framerate_init(MMHandleType handle);
int _mmcamcorder_video_current_framerate(MMHandleType handle);
int _mmcamcorder_video_average_framerate(MMHandleType handle);

/* sound focus related function */
void __mmcamcorder_force_stop(mmf_camcorder_t *hcamcorder);
void _mmcamcorder_sound_focus_cb(int id, mm_sound_focus_type_e focus_type,
	mm_sound_focus_state_e focus_state, const char *reason_for_change,
	int option, const char *additional_info, void *user_data);
void _mmcamcorder_sound_focus_watch_cb(int id, mm_sound_focus_type_e focus_type, mm_sound_focus_state_e focus_state,
	const char *reason_for_change, const char *additional_info, void *user_data);
void _mmcamcorder_sound_signal_callback(mm_sound_signal_name_t signal, int value, void *user_data);

/* device policy manager */
void _mmcamcorder_dpm_camera_policy_changed_cb(const char *name, const char *value, void *user_data);

/* Emit dbus signal */
void _mmcamcorder_emit_signal(MMHandleType handle, const char *object_name,
	const char *interface_name, const char *signal_name, int value);

#ifdef __cplusplus
}
#endif

#endif /* __MM_CAMCORDER_INTERNAL_H__ */
