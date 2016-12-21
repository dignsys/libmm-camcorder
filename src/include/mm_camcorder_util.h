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

#ifndef __MM_CAMCORDER_UTIL_H__
#define __MM_CAMCORDER_UTIL_H__

/*=======================================================================================
| INCLUDE FILES										|
========================================================================================*/
#include <gio/gio.h>
#include <linux/magic.h>
#include <storage.h>


#ifdef __cplusplus
extern "C" {
#endif

/*=======================================================================================
| GLOBAL DEFINITIONS AND DECLARATIONS FOR CAMCORDER					|
========================================================================================*/

/*=======================================================================================
| MACRO DEFINITIONS									|
========================================================================================*/
#ifndef CLEAR
#define CLEAR(x)            memset(&(x), 0, sizeof(x))
#endif

#define MMCAMCORDER_ADD_BUFFER_PROBE(x_pad, x_category, x_callback, x_hcamcorder) \
do { \
	MMCamcorderHandlerItem *item = NULL; \
	item = (MMCamcorderHandlerItem *)g_malloc(sizeof(MMCamcorderHandlerItem)); \
	if (!item) {\
		_mmcam_dbg_err("Cannot connect buffer probe [malloc fail] \n"); \
	} else if (x_category == 0 || !(x_category & _MMCAMCORDER_HANDLER_CATEGORY_ALL)) { \
		_mmcam_dbg_err("Invalid handler category : %x \n", x_category); \
	} else { \
		item->object = G_OBJECT(x_pad); \
		item->category = x_category; \
		item->handler_id = gst_pad_add_probe(x_pad, GST_PAD_PROBE_TYPE_BUFFER, x_callback, x_hcamcorder, NULL); \
		x_hcamcorder->buffer_probes = g_list_append(x_hcamcorder->buffer_probes, item); \
		_mmcam_dbg_log("Adding buffer probe on [%s:%s] - [ID : %lu], [Category : %x] ", \
			GST_DEBUG_PAD_NAME(item->object), item->handler_id, item->category); \
	} \
} while (0);

#define MMCAMCORDER_ADD_EVENT_PROBE(x_pad, x_category, x_callback, x_hcamcorder) \
do { \
	MMCamcorderHandlerItem *item = NULL; \
	item = (MMCamcorderHandlerItem *) g_malloc(sizeof(MMCamcorderHandlerItem)); \
	if (!item) { \
		_mmcam_dbg_err("Cannot connect buffer probe [malloc fail] \n"); \
	} \
	else if (x_category == 0 || !(x_category & _MMCAMCORDER_HANDLER_CATEGORY_ALL)) { \
		_mmcam_dbg_err("Invalid handler category : %x \n", x_category); \
	} else { \
		item->object = G_OBJECT(x_pad); \
		item->category = x_category; \
		item->handler_id = gst_pad_add_probe(x_pad, GST_PAD_PROBE_TYPE_EVENT_BOTH, x_callback, x_hcamcorder, NULL); \
		x_hcamcorder->event_probes = g_list_append(x_hcamcorder->event_probes, item); \
		_mmcam_dbg_log("Adding event probe on [%s:%s] - [ID : %lu], [Category : %x] ", \
			GST_DEBUG_PAD_NAME(item->object), item->handler_id, item->category); \
	} \
} while (0);

#define MMCAMCORDER_SIGNAL_CONNECT(x_object, x_category, x_signal, x_callback, x_hcamcorder) \
do { \
	MMCamcorderHandlerItem* item = NULL; \
	item = (MMCamcorderHandlerItem *) g_malloc(sizeof(MMCamcorderHandlerItem)); \
	if (!item) { \
		_mmcam_dbg_err("Cannot connect signal [%s]\n", x_signal); \
	} else if (x_category == 0 || !(x_category & _MMCAMCORDER_HANDLER_CATEGORY_ALL)) { \
		_mmcam_dbg_err("Invalid handler category : %x \n", x_category); \
	} else { \
		item->object = G_OBJECT(x_object); \
		item->category = x_category; \
		item->handler_id = g_signal_connect(G_OBJECT(x_object), x_signal, \
			G_CALLBACK(x_callback), x_hcamcorder); \
		x_hcamcorder->signals = g_list_append(x_hcamcorder->signals, item); \
		_mmcam_dbg_log("Connecting signal on [%s][%p] - [ID : %lu], [Category : %x] ", \
			GST_OBJECT_NAME(item->object), item->object, item->handler_id, item->category); \
	} \
} while (0);

#define MMCAMCORDER_G_OBJECT_GET(obj, name, value) \
do { \
	if (obj) { \
		if (g_object_class_find_property(G_OBJECT_GET_CLASS(G_OBJECT(obj)), name)) { \
			g_object_get(G_OBJECT(obj), name, value, NULL); \
		} else { \
			_mmcam_dbg_warn("The object doesn't have a property named(%s)", name); \
		} \
	} else { \
		_mmcam_dbg_err("Null object"); \
	} \
} while (0);

#define MMCAMCORDER_G_OBJECT_SET(obj, name, value) \
do { \
	if (obj) { \
		GParamSpec *spec = g_object_class_find_property(G_OBJECT_GET_CLASS(G_OBJECT(obj)), name);\
		if (spec) { \
			if (spec->value_type == G_TYPE_INT64) {\
				g_object_set(G_OBJECT(obj), name, (gint64)value, NULL); \
			} else if (spec->value_type == G_TYPE_UINT64) { \
				g_object_set(G_OBJECT(obj), name, (guint64)value, NULL); \
			} else if (spec->value_type == G_TYPE_FLOAT) { \
				g_object_set(G_OBJECT(obj), name, (float)value, NULL); \
			} else if (spec->value_type == G_TYPE_DOUBLE) { \
				g_object_set(G_OBJECT(obj), name, (double)value, NULL); \
			} else { \
				g_object_set(G_OBJECT(obj), name, value, NULL); \
			} \
		} else { \
			_mmcam_dbg_warn("The object doesn't have a property named(%s)", name); \
		} \
	} else { \
		_mmcam_dbg_err("Null object"); \
	} \
} while (0);

#define MMCAMCORDER_G_OBJECT_SET_POINTER(obj, name, value) \
do { \
	if (obj) { \
		GParamSpec *spec = g_object_class_find_property(G_OBJECT_GET_CLASS(G_OBJECT(obj)), name);\
		if (spec) { \
			g_object_set(G_OBJECT(obj), name, value, NULL); \
		} else { \
			_mmcam_dbg_warn("The object doesn't have a property named(%s)", name); \
		} \
	} else { \
		_mmcam_dbg_err("Null object"); \
	} \
} while (0);

#define MMCAM_FOURCC(a, b, c, d)  (guint32)((a)|(b)<<8|(c)<<16|(d)<<24)
#define MMCAM_FOURCC_ARGS(fourcc) \
		((gchar)((fourcc)&0xff)), \
		((gchar)(((fourcc)>>8)&0xff)), \
		((gchar)(((fourcc)>>16)&0xff)), \
		((gchar)(((fourcc)>>24)&0xff))

#define MMCAM_SEND_MESSAGE(handle, msg_id, msg_code) \
{\
	_MMCamcorderMsgItem msg;\
	msg.id = msg_id;\
	msg.param.code = msg_code;\
	_mmcam_dbg_log("msg id : %x, code : %x", msg_id, msg_code);\
	_mmcamcorder_send_message((MMHandleType)handle, &msg);\
}


/*=======================================================================================
| ENUM DEFINITIONS									|
========================================================================================*/
/**
 * Structure of GDBus Callback.
 */
typedef struct {
	GCond sync_cond;
	GMutex sync_mutex;
	int param;
	int is_playing;
	guint subscribe_id;
	void *mm_handle;
} _MMCamcorderGDbusCbInfo;

/**
 *Type define of util.
 */
typedef enum {
	_MMCAMCORDER_HANDLER_PREVIEW = (1 << 0),
	_MMCAMCORDER_HANDLER_VIDEOREC = (1 << 1),
	_MMCAMCORDER_HANDLER_STILLSHOT = (1 << 2),
	_MMCAMCORDER_HANDLER_AUDIOREC = (1 << 3),
} _MMCamcorderHandlerCategory;

/*=======================================================================================
| STRUCTURE DEFINITIONS									|
========================================================================================*/

/**
 * Structure of location info
 */
typedef struct {
	gint32 longitude;
	gint32 latitude;
	gint32 altitude;
} _MMCamcorderLocationInfo;

/**
 * Structure of handler item
 */
typedef struct {
	GObject *object;
	_MMCamcorderHandlerCategory category;
	gulong handler_id;
} MMCamcorderHandlerItem;

/**
 * Structure of message item
 */
typedef struct {
	MMHandleType handle;        /**< handle */
	int id;                     /**< message id */
	MMMessageParamType param;   /**< message parameter */
	GMutex lock;                /**< mutex for item */
} _MMCamcorderMsgItem;

/**
 * Structure of storage information
 */
typedef struct {
	storage_type_e type;
	int id;
} _MMCamcorderStorageInfo;


/*=======================================================================================
| CONSTANT DEFINITIONS									|
========================================================================================*/
#define G_DBUS_TIMEOUT                          3000
#define FAT32_FILE_SYSTEM_MAX_SIZE              (4294967295UL)     /* 4 GigaByte - 1 byte */
#define NANO_SEC_PER_MILI_SEC                   1000000
#define _MMCAMCORDER_HANDLER_CATEGORY_ALL \
	(_MMCAMCORDER_HANDLER_PREVIEW | _MMCAMCORDER_HANDLER_VIDEOREC |_MMCAMCORDER_HANDLER_STILLSHOT | _MMCAMCORDER_HANDLER_AUDIOREC)


/*=======================================================================================
| GLOBAL FUNCTION PROTOTYPES								|
========================================================================================*/
/* GStreamer */
void _mmcamcorder_remove_buffer_probe(MMHandleType handle, _MMCamcorderHandlerCategory category);
void _mmcamcorder_remove_one_buffer_probe(MMHandleType handle, void *object);
void _mmcamcorder_remove_event_probe(MMHandleType handle, _MMCamcorderHandlerCategory category);
void _mmcamcorder_disconnect_signal(MMHandleType handle, _MMCamcorderHandlerCategory category);
void _mmcamcorder_remove_all_handlers(MMHandleType handle, _MMCamcorderHandlerCategory category);
void _mmcamcorder_element_release_noti(gpointer data, GObject *where_the_object_was);
gboolean _mmcamcorder_add_elements_to_bin(GstBin *bin, GList *element_list);
gboolean _mmcamcorder_link_elements(GList *element_list);

/* Message */
#ifdef _MMCAMCORDER_ENABLE_IDLE_MESSAGE_CALLBACK
gboolean _mmcamcorder_msg_callback(void *data);
#endif /* _MMCAMCORDER_ENABLE_IDLE_MESSAGE_CALLBACK */
gboolean _mmcamcorder_send_message(MMHandleType handle, _MMCamcorderMsgItem *data);
void _mmcamcorder_remove_message_all(MMHandleType handle);

/* Pixel format */
int _mmcamcorder_get_pixel_format(GstCaps *pad);
int _mmcamcorder_get_pixtype(unsigned int fourcc);
unsigned int _mmcamcorder_get_fourcc(int pixtype, int codectype, int use_zero_copy_format);

/* JPEG encode */
gboolean _mmcamcorder_encode_jpeg(void *src_data, unsigned int src_width, unsigned int src_height,
	int src_format, unsigned int src_length, unsigned int jpeg_quality,
	void **result_data, unsigned int *result_length);
/* resize */
gboolean _mmcamcorder_resize_frame(unsigned char *src_data, unsigned int src_width, unsigned int src_height, unsigned int src_length, int src_format,
	unsigned char **dst_data, unsigned int *dst_width, unsigned int *dst_height, unsigned int *dst_length);
gboolean _mmcamcorder_downscale_UYVYorYUYV(unsigned char *src, unsigned int src_width, unsigned int src_height,
	unsigned char **dst, unsigned int dst_width, unsigned int dst_height);

/* Recording */
/* find top level tag only, do not use this function for finding sub level tags.
   tag_fourcc is Four-character-code (FOURCC) */
gint _mmcamcorder_find_tag(FILE *f, guint32 tag_fourcc, gboolean do_rewind);
gboolean _mmcamcorder_find_fourcc(FILE *f, guint32 tag_fourcc, gboolean do_rewind);
gint32 _mmcamcorder_double_to_fix(gdouble d_number);
gboolean _mmcamcorder_update_size(FILE *f, gint64 prev_pos, gint64 curr_pos);
gboolean _mmcamcorder_write_loci(FILE *f, _MMCamcorderLocationInfo info);
gboolean _mmcamcorder_write_geodata(FILE *f, _MMCamcorderLocationInfo info);
gboolean _mmcamcorder_write_udta(FILE *f, int gps_enable, _MMCamcorderLocationInfo info, _MMCamcorderLocationInfo geotag);
guint64 _mmcamcorder_get_container_size(const guchar *size);
guint64 _mmcamcorder_get_container_size64(const guchar *size);
gboolean _mmcamcorder_update_composition_matrix(FILE *f, int orientation);

/* File system */
int _mmcamcorder_get_storage_info(const gchar *path, const gchar *root_directory, _MMCamcorderStorageInfo *storage_info);
int _mmcamcorder_get_freespace(storage_type_e type, guint64 *free_space);
int _mmcamcorder_get_file_size(const char *filename, guint64 *size);
int _mmcamcorder_get_file_system_type(const gchar *path, int *file_system_type);

/* Task */
void *_mmcamcorder_util_task_thread_func(void *data);

/* device */
int _mmcamcorder_get_device_flash_brightness(GDBusConnection *conn, int *brightness);

/* sound play via dbus*/
int _mmcamcorder_send_sound_play_message(GDBusConnection *conn, _MMCamcorderGDbusCbInfo *gdbus_info,
	const char *sample_name, const char *stream_role, const char *volume_gain, int sync_play);

/* request to show pop-up related DPM policy */
void _mmcamcorder_request_dpm_popup(GDBusConnection *conn, const char *restricted_policy);

/* dbus signal emit */
void _mmcamcorder_emit_dbus_signal(GDBusConnection *conn, const char *object_name,
	const char *interface_name, const char *signal_name, int value);

/* audio buffer */
int _mmcamcorder_get_audiosrc_blocksize(int samplerate, int format, int channel, int interval, int *blocksize);

#ifdef __cplusplus
}
#endif

#endif /* __MM_CAMCORDER_UTIL_H__ */
