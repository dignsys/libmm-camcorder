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
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/vfs.h> /* struct statfs */
#include <sys/time.h> /* gettimeofday */
#include <sys/stat.h>
#include <gst/video/video-info.h>
#include <gio/gio.h>

#include "mm_camcorder_internal.h"
#include "mm_camcorder_util.h"
#include "mm_camcorder_sound.h"
#include <mm_util_image.h>
#include <mm_util_imgp.h>
#include <mm_util_jpeg.h>

/*-----------------------------------------------------------------------
|    GLOBAL VARIABLE DEFINITIONS for internal				|
-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
|    LOCAL VARIABLE DEFINITIONS for internal				|
-----------------------------------------------------------------------*/
#define TIME_STRING_MAX_LEN                     64
#define __MMCAMCORDER_CAPTURE_WAIT_TIMEOUT      5
#define __MMCAMCORDER_MAX_WIDTH                 8192
#define __MMCAMCORDER_MAX_HEIGHT                8192

#define FPUTC_CHECK(x_char, x_file) \
{ \
	if (fputc(x_char, x_file) == EOF) { \
		_mmcam_dbg_err("[Critical] fputc() returns fail.\n"); \
		return FALSE; \
	} \
}
#define FPUTS_CHECK(x_str, x_file) \
{ \
	if (fputs(x_str, x_file) == EOF) { \
		_mmcam_dbg_err("[Critical] fputs() returns fail.\n"); \
		SAFE_G_FREE(str); \
		return FALSE; \
	} \
}

/*---------------------------------------------------------------------------
|    LOCAL FUNCTION PROTOTYPES:												|
---------------------------------------------------------------------------*/
/* STATIC INTERNAL FUNCTION */

//static gint            skip_mdat(FILE *f);
static guint16           get_language_code(const char *str);
static gchar*            str_to_utf8(const gchar *str);
static inline gboolean   write_tag(FILE *f, const gchar *tag);
static inline gboolean   write_to_32(FILE *f, guint val);
static inline gboolean   write_to_16(FILE *f, guint val);
static inline gboolean   write_to_24(FILE *f, guint val);
static gboolean _mmcamcorder_convert_YUYV_to_I420(unsigned char *src, guint width, guint height, unsigned char **dst, unsigned int *dst_len);
static gboolean _mmcamcorder_convert_UYVY_to_I420(unsigned char *src, guint width, guint height, unsigned char **dst, unsigned int *dst_len);
static gboolean _mmcamcorder_convert_NV12_to_I420(unsigned char *src, guint width, guint height, unsigned char **dst, unsigned int *dst_len);


/*===========================================================================================
|																							|
|  FUNCTION DEFINITIONS																		|
|																							|
========================================================================================== */
/*---------------------------------------------------------------------------
|    GLOBAL FUNCTION DEFINITIONS:											|
---------------------------------------------------------------------------*/

static int __gdbus_method_call_sync(GDBusConnection *conn, const char *bus_name,
	const char *object, const char *iface, const char *method,
	GVariant *args, GVariant **result, bool is_sync)
{
	int ret = MM_ERROR_NONE;
	GVariant *dbus_reply = NULL;

	if (!conn || !object || !iface || !method) {
		_mmcam_dbg_err("Invalid Argument %p %p %p %p",
			conn, object, iface, method);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	_mmcam_dbg_warn("Dbus call - obj [%s], iface [%s], method [%s]", object, iface, method);

	if (is_sync) {
		dbus_reply = g_dbus_connection_call_sync(conn,
			bus_name, object, iface, method, args, NULL,
			G_DBUS_CALL_FLAGS_NONE, G_DBUS_TIMEOUT, NULL, NULL);
		if (dbus_reply) {
			_mmcam_dbg_warn("Method Call '%s.%s' Success", iface, method);
			*result = dbus_reply;
		} else {
			_mmcam_dbg_err("dbus method call sync reply failed");
			ret = MM_ERROR_CAMCORDER_INTERNAL;
		}
	} else {
		g_dbus_connection_call(conn, bus_name, object, iface, method, args, NULL,
			G_DBUS_CALL_FLAGS_NONE, G_DBUS_TIMEOUT, NULL, NULL, NULL);
	}

	_mmcam_dbg_warn("done");

	return ret;
}

static int __gdbus_subscribe_signal(GDBusConnection *conn,
	const char *object_name, const char *iface_name, const char *signal_name,
	GDBusSignalCallback signal_cb, guint *subscribe_id, void *userdata)
{
	guint subs_id = 0;

	if (!conn || !object_name || !iface_name || !signal_name || !signal_cb || !subscribe_id) {
		_mmcam_dbg_err("Invalid Argument %p %p %p %p %p %p",
			conn, object_name, iface_name, signal_name, signal_cb, subscribe_id);
		return MM_ERROR_INVALID_ARGUMENT;
	}

	_mmcam_dbg_log("subscirbe signal Obj %s, iface_name %s, sig_name %s",
		object_name, iface_name, signal_name);

	subs_id = g_dbus_connection_signal_subscribe(conn,
		NULL, iface_name, signal_name, object_name, NULL,
		G_DBUS_SIGNAL_FLAGS_NONE, signal_cb, userdata, NULL);
	if (!subs_id) {
		_mmcam_dbg_err("g_dbus_connection_signal_subscribe() failed");
		return MM_ERROR_CAMCORDER_INTERNAL;
	} else {
		*subscribe_id = subs_id;
		_mmcam_dbg_log("subs_id %u", subs_id);
	}

	return MM_ERROR_NONE;
}


static void __gdbus_stream_eos_cb(GDBusConnection *connection,
	const gchar *sender_name, const gchar *object_path, const gchar *interface_name,
	const gchar *signal_name, GVariant *param, gpointer user_data)
{
	int played_idx = 0;
	_MMCamcorderGDbusCbInfo *gdbus_info = NULL;
	mmf_camcorder_t *hcamcorder = NULL;

	_mmcam_dbg_warn("entered");

	if (!param || !user_data) {
		_mmcam_dbg_err("invalid parameter %p %p", param, user_data);
		return;
	}

	gdbus_info = (_MMCamcorderGDbusCbInfo *)user_data;
	hcamcorder = (mmf_camcorder_t *)gdbus_info->mm_handle;

	g_variant_get(param, "(i)", &played_idx);

	g_mutex_lock(&gdbus_info->sync_mutex);

	_mmcam_dbg_warn("gdbus_info->param %d, played_idx : %d, handle : %p",
		gdbus_info->param, played_idx, hcamcorder);

	if (gdbus_info->param == played_idx) {
		g_dbus_connection_signal_unsubscribe(connection, gdbus_info->subscribe_id);

		gdbus_info->is_playing = FALSE;
		gdbus_info->subscribe_id = 0;
		gdbus_info->param = 0;

		g_cond_signal(&gdbus_info->sync_cond);
	}

	g_mutex_unlock(&gdbus_info->sync_mutex);

	_mmcam_dbg_warn("done");

	return;
}

static int __gdbus_wait_for_cb_return(_MMCamcorderGDbusCbInfo *gdbus_info, int time_out)
{
	int ret = MM_ERROR_NONE;
	gint64 end_time = 0;

	if (!gdbus_info) {
		_mmcam_dbg_err("invalid info");
		return MM_ERROR_CAMCORDER_INVALID_ARGUMENT;
	}

	g_mutex_lock(&gdbus_info->sync_mutex);

	_mmcam_dbg_warn("entered");

	if (gdbus_info->is_playing == FALSE) {
		_mmcam_dbg_log("callback is already returned");
		g_mutex_unlock(&gdbus_info->sync_mutex);
		return MM_ERROR_NONE;
	}

	end_time = g_get_monotonic_time() + (time_out * G_TIME_SPAN_MILLISECOND);

	if (g_cond_wait_until(&gdbus_info->sync_cond, &gdbus_info->sync_mutex, end_time)) {
		_mmcam_dbg_warn("wait signal received");
	} else {
		_mmcam_dbg_err("wait time is expired");
		ret = MM_ERROR_CAMCORDER_RESPONSE_TIMEOUT;
	}

	g_mutex_unlock(&gdbus_info->sync_mutex);

	_mmcam_dbg_warn("done");

	return ret;
}


gint32 _mmcamcorder_double_to_fix(gdouble d_number)
{
	return (gint32) (d_number * 65536.0);
}

// find top level tag only, do not use this function for finding sub level tags
gint _mmcamcorder_find_tag(FILE *f, guint32 tag_fourcc, gboolean do_rewind)
{
	size_t read_item = 0;
	guchar buf[8];

	if (do_rewind)
		rewind(f);

	while ((read_item = fread(&buf, sizeof(guchar), 8, f)) > 0) {
		uint64_t buf_size = 0;
		uint32_t buf_fourcc = 0;

		if (read_item < 8) {
			_mmcam_dbg_err("fread failed : %zu", read_item);
			break;
		}

		buf_fourcc = MMCAM_FOURCC(buf[4], buf[5], buf[6], buf[7]);

		if (tag_fourcc == buf_fourcc) {
			_mmcam_dbg_log("find tag : %c%c%c%c", MMCAM_FOURCC_ARGS(tag_fourcc));
			return TRUE;
		} else {
			_mmcam_dbg_log("skip [%c%c%c%c] tag", MMCAM_FOURCC_ARGS(buf_fourcc));

			buf_size = _mmcamcorder_get_container_size(buf);

			/* if size of mdat is 1, it means largesize is used.(bigger than 4GB) */
			if (buf_fourcc == MMCAM_FOURCC('m', 'd', 'a', 't') &&
			    buf_size == 1) {
				read_item = fread(&buf, sizeof(guchar), 8, f);
				if (read_item < 8) {
					_mmcam_dbg_err("fread failed");
					return FALSE;
				}

				buf_size = _mmcamcorder_get_container_size64(buf);
				buf_size = buf_size - 16; /* include tag and large file flag(size 1) */
			} else {
				buf_size = buf_size - 8; /* include tag */
			}

			_mmcam_dbg_log("seek %"PRIu64, buf_size);
			if (fseeko(f, (off_t)buf_size, SEEK_CUR) != 0) {
				_mmcam_dbg_err("fseeko() fail");
				return FALSE;
			}
		}
	}

	_mmcam_dbg_log("cannot find tag : %c%c%c%c", MMCAM_FOURCC_ARGS(tag_fourcc));

	return FALSE;
}

gboolean _mmcamcorder_find_fourcc(FILE *f, guint32 tag_fourcc, gboolean do_rewind)
{
	size_t read_item = 0;
	guchar buf[8];

	if (do_rewind)
		rewind(f);

	while ((read_item = fread(&buf, sizeof(guchar), 8, f))  > 0) {
		uint64_t buf_size = 0;
		uint32_t buf_fourcc = 0;

		if (read_item < 8) {
			_mmcam_dbg_err("fread failed : %zu", read_item);
			break;
		}

		buf_fourcc = MMCAM_FOURCC(buf[4], buf[5], buf[6], buf[7]);

		if (tag_fourcc == buf_fourcc) {
			_mmcam_dbg_log("find tag : %c%c%c%c", MMCAM_FOURCC_ARGS(tag_fourcc));
			return TRUE;
		} else if (buf_fourcc == MMCAM_FOURCC('m', 'o', 'o', 'v') &&
				   tag_fourcc != buf_fourcc) {
			if (_mmcamcorder_find_fourcc(f, tag_fourcc, FALSE))
				return TRUE;
			else
				continue;
		} else {
			_mmcam_dbg_log("skip [%c%c%c%c] tag", MMCAM_FOURCC_ARGS(buf_fourcc));

			buf_size = _mmcamcorder_get_container_size(buf);

			/* if size of mdat is 1, it means largesize is used.(bigger than 4GB) */
			if (buf_fourcc == MMCAM_FOURCC('m', 'd', 'a', 't') &&
			    buf_size == 1) {
				read_item = fread(&buf, sizeof(guchar), 8, f);
				if (read_item < 8) {
					_mmcam_dbg_err("fread failed");
					return FALSE;
				}

				buf_size = _mmcamcorder_get_container_size64(buf);
				buf_size = buf_size - 16; /* include tag and large file flag(size 1) */
			} else {
				buf_size = buf_size - 8; /* include tag */
			}

			_mmcam_dbg_log("seek %"PRIu64, buf_size);
			if (fseeko(f, (off_t)buf_size, SEEK_CUR) != 0) {
				_mmcam_dbg_err("fseeko() fail");
				return FALSE;
			}
		}
	}

	_mmcam_dbg_log("cannot find tag : %c%c%c%c", MMCAM_FOURCC_ARGS(tag_fourcc));

	return FALSE;
}

gboolean _mmcamcorder_update_size(FILE *f, gint64 prev_pos, gint64 curr_pos)
{
	_mmcam_dbg_log("size : %"G_GINT64_FORMAT"", curr_pos-prev_pos);
	if (fseeko(f, prev_pos, SEEK_SET) != 0) {
		_mmcam_dbg_err("fseeko() fail");
		return FALSE;
	}

	if (!write_to_32(f, curr_pos -prev_pos))
		return FALSE;

	if (fseeko(f, curr_pos, SEEK_SET) != 0) {
		_mmcam_dbg_err("fseeko() fail");
		return FALSE;
	}

	return TRUE;
}

gboolean _mmcamcorder_write_loci(FILE *f, _MMCamcorderLocationInfo info)
{
	gint64 current_pos, pos;
	gchar *str = NULL;

	_mmcam_dbg_log("");

	if ((pos = ftello(f)) < 0) {
		_mmcam_dbg_err("ftello() returns negative value");
		return FALSE;
	}

	if (!write_to_32(f, 0)) //size
		return FALSE;

	if (!write_tag(f, "loci")) // type
		return FALSE;

	FPUTC_CHECK(0, f);		// version

	if (!write_to_24(f, 0))	// flags
		return FALSE;

	if (!write_to_16(f, get_language_code("eng"))) // language
		return FALSE;

	str = str_to_utf8("location_name");

	FPUTS_CHECK(str, f); // name
	SAFE_G_FREE(str);

	FPUTC_CHECK('\0', f);
	FPUTC_CHECK(0, f);		//role

	if (!write_to_32(f, info.longitude))	// Longitude
		return FALSE;

	if (!write_to_32(f, info.latitude)) // Latitude
		return FALSE;

	if (!write_to_32(f, info.altitude))	// Altitude
		return FALSE;

	str = str_to_utf8("Astronomical_body");
	FPUTS_CHECK(str, f);//Astronomical_body
	SAFE_G_FREE(str);

	FPUTC_CHECK('\0', f);

	str = str_to_utf8("Additional_notes");
	FPUTS_CHECK(str, f); // Additional_notes
	SAFE_G_FREE(str);

	FPUTC_CHECK('\0', f);

	if ((current_pos = ftello(f)) < 0) {
		_mmcam_dbg_err("ftello() returns negative value");
		return FALSE;
	}

	if (!_mmcamcorder_update_size(f, pos, current_pos))
		return FALSE;

	return TRUE;
}

void _mmcamcorder_write_Latitude(FILE *f, int value)
{
	char s_latitude[16];
	int l_decimal = 0;
	int l_below_decimal = 0;

	l_decimal = value / 10000;
	if (value < 0) {
		if (l_decimal == 0)
			snprintf(s_latitude, sizeof(s_latitude), "-%.2d.", l_decimal);
		else
			snprintf(s_latitude, sizeof(s_latitude), "%.2d.", l_decimal);
	} else {
		snprintf(s_latitude, sizeof(s_latitude), "+%.2d.", l_decimal);
	}

	l_below_decimal = value - (l_decimal * 10000);
	if (l_below_decimal < 0)
		l_below_decimal = -l_below_decimal;

	snprintf(&s_latitude[4], sizeof(s_latitude) - 4, "%.4d", l_below_decimal);

	write_tag(f, s_latitude);
}

void _mmcamcorder_write_Longitude(FILE *f, int value)
{
	char s_longitude[24];
	int l_decimal = 0;
	int l_below_decimal = 0;

	l_decimal = value / 10000;
	if (value < 0) {
		if (l_decimal == 0)
			snprintf(s_longitude, sizeof(s_longitude), "-%.3d.", l_decimal);
		else
			snprintf(s_longitude, sizeof(s_longitude), "%.3d.", l_decimal);
	} else {
		snprintf(s_longitude, sizeof(s_longitude), "+%.3d.", l_decimal);
	}

	l_below_decimal = value - (l_decimal * 10000);
	if (l_below_decimal < 0)
		l_below_decimal = -l_below_decimal;

	snprintf(&s_longitude[5], sizeof(s_longitude) - 5, "%.4d", l_below_decimal);

	write_tag(f, s_longitude);
}

#define D_GEOGRAPH "\xA9xyz"
// 0x0012 -> latitude(8) + longitude(9) + seperator(1) = 18
// 0x15c7 -> encode in english
#define D_INFO_GEOGRAPH 0x001215c7

gboolean _mmcamcorder_write_geodata(FILE *f, _MMCamcorderLocationInfo info)
{
	gint64 current_pos, pos;

	_mmcam_dbg_log("");

	if ((pos = ftello(f)) < 0) {
		_mmcam_dbg_err("ftello() returns negative value");
		return FALSE;
	}

	if (!write_to_32(f, 0))			//size
		return FALSE;
	// tag -> .xyz
	if (!write_tag(f, D_GEOGRAPH))	// type
		return FALSE;

	if (!write_to_32(f, D_INFO_GEOGRAPH))
		return FALSE;

	_mmcamcorder_write_Latitude(f, info.latitude);
	_mmcamcorder_write_Longitude(f, info.longitude);

	FPUTC_CHECK(0x2F, f);

	if ((current_pos = ftello(f)) < 0) {
		_mmcam_dbg_err("ftello() returns negative value");
		return FALSE;
	}

	if (!_mmcamcorder_update_size(f, pos, current_pos))
		return FALSE;

	return TRUE;
}


gboolean _mmcamcorder_write_udta(FILE *f, int gps_enable, _MMCamcorderLocationInfo info, _MMCamcorderLocationInfo geotag)
{
	gint64 current_pos, pos;

	_mmcam_dbg_log("gps enable : %d", gps_enable);
	if (gps_enable == FALSE) {
		_mmcam_dbg_log("no need to write udta");
		return TRUE;
	}

	if ((pos = ftello(f)) < 0) {
		_mmcam_dbg_err("ftello() returns negative value");
		return FALSE;
	}

	/* size */
	if (!write_to_32(f, 0)) {
		_mmcam_dbg_err("failed to write size");
		return FALSE;
	}

	/* type */
	if (!write_tag(f, "udta")) {
		_mmcam_dbg_err("failed to write type udta");
		return FALSE;
	}

	if (gps_enable) {
		if (!_mmcamcorder_write_loci(f, info)) {
			_mmcam_dbg_err("failed to write loci");
			return FALSE;
		}

		if (!_mmcamcorder_write_geodata(f, geotag)) {
			_mmcam_dbg_err("failed to write geodata");
			return FALSE;
		}
	}

	if ((current_pos = ftello(f)) < 0) {
		_mmcam_dbg_err("ftello() returns negative value");
		return FALSE;
	}

	if (!_mmcamcorder_update_size(f, pos, current_pos)) {
		_mmcam_dbg_err("failed to update size");
		return FALSE;
	}

	_mmcam_dbg_log("done");

	return TRUE;
}


guint64 _mmcamcorder_get_container_size(const guchar *size)
{
	guint64 result = 0;
	guint64 temp = 0;

	temp = size[0];
	result = temp << 24;
	temp = size[1];
	result = result | (temp << 16);
	temp = size[2];
	result = result | (temp << 8);
	result = result | size[3];

	_mmcam_dbg_log("result : %"G_GUINT64_FORMAT, result);

	return result;
}


guint64 _mmcamcorder_get_container_size64(const guchar *size)
{
	guint64 result = 0;
	guint64 temp = 0;

	temp = size[0];
	result = temp << 56;
	temp = size[1];
	result = result | (temp << 48);
	temp = size[2];
	result = result | (temp << 40);
	temp = size[3];
	result = result | (temp << 32);
	temp = size[4];
	result = result | (temp << 24);
	temp = size[5];
	result = result | (temp << 16);
	temp = size[6];
	result = result | (temp << 8);
	result = result | size[7];

	_mmcam_dbg_log("result : %"G_GUINT64_FORMAT, result);

	return result;
}


gboolean _mmcamcorder_update_composition_matrix(FILE *f, int orientation)
{
	/* for 0 degree */
	guint32 a = 0x00010000;
	guint32 b = 0;
	guint32 c = 0;
	guint32 d = 0x00010000;

	switch (orientation) {
	case MM_CAMCORDER_TAG_VIDEO_ORT_90:/* 90 degree */
		a = 0;
		b = 0x00010000;
		c = 0xffff0000;
		d = 0;
		break;
	case MM_CAMCORDER_TAG_VIDEO_ORT_180:/* 180 degree */
		a = 0xffff0000;
		d = 0xffff0000;
		break;
	case MM_CAMCORDER_TAG_VIDEO_ORT_270:/* 270 degree */
		a = 0;
		b = 0xffff0000;
		c = 0x00010000;
		d = 0;
		break;
	case MM_CAMCORDER_TAG_VIDEO_ORT_NONE:/* 0 degree */
	default:
		break;
	}

	write_to_32(f, a);
	write_to_32(f, b);
	write_to_32(f, 0);
	write_to_32(f, c);
	write_to_32(f, d);
	write_to_32(f, 0);
	write_to_32(f, 0);
	write_to_32(f, 0);
	write_to_32(f, 0x40000000);

	_mmcam_dbg_log("orientation : %d, write data 0x%x 0x%x 0x%x 0x%x",
				   orientation, a, b, c, d);

	return TRUE;
}


static int __mmcamcorder_storage_supported_cb(int storage_id, storage_type_e type,
	storage_state_e state, const char *path, void *user_data)
{
	_MMCamcorderStorageInfo *info = (_MMCamcorderStorageInfo *)user_data;

	if (!info) {
		_mmcam_dbg_err("NULL info");
		return FALSE;
	}

	if (type == info->type) {
		info->id = storage_id;
		return FALSE;
	}

	return TRUE;
}


int _mmcamcorder_get_storage_validity(MMHandleType handle, const char *filename, guint64 min_space, gboolean *storage_validity)
{
	int ret = MM_ERROR_NONE;
	int err = 0;
	char *dir_name = NULL;
	guint64 free_space = 0;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	mmf_return_val_if_fail(hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);
	mmf_return_val_if_fail(storage_validity, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	if (!filename) {
		_mmcam_dbg_warn("NULL filename, but keep going...");
		*storage_validity = TRUE;
		return MM_ERROR_NONE;
	}

	dir_name = g_path_get_dirname(filename);
	mmf_return_val_if_fail(dir_name, MM_ERROR_OUT_OF_STORAGE);

	err = _mmcamcorder_get_storage_info(dir_name, hcamcorder->root_directory, &hcamcorder->storage_info);
	if (err != 0) {
		_mmcam_dbg_err("get storage info failed");
		ret = MM_ERROR_CAMCORDER_INTERNAL;
		goto _CHECK_DONE;
	}

	err = _mmcamcorder_get_freespace(hcamcorder->storage_info.type, &free_space);
	if (err != 0) {
		_mmcam_dbg_err("get free space failed");
		ret = MM_ERROR_CAMCORDER_INTERNAL;
		goto _CHECK_DONE;
	}

	_mmcam_dbg_warn("current free space - %s [%" G_GUINT64_FORMAT "]", dir_name, free_space);

_CHECK_DONE:
	g_free(dir_name);

	if (ret == MM_ERROR_NONE && free_space > min_space) {
		*storage_validity = TRUE;
		_mmcam_dbg_log("validity and free space of storage : OK");
	} else {
		*storage_validity = FALSE;
		_mmcam_dbg_err("OUT of STORAGE [err:%d or free space [%"G_GUINT64_FORMAT"] is smaller than [%"G_GUINT64_FORMAT"]",
			err, free_space, min_space);
	}

	return ret;
}


void _mmcamcorder_adjust_recording_max_size(const char *filename, guint64 *max_size)
{
	int file_system_type = 0;
	char *dir_name = NULL;

	mmf_return_if_fail(max_size);
	mmf_return_if_fail(filename);

	dir_name = g_path_get_dirname(filename);
	mmf_return_if_fail(dir_name);

	if (_mmcamcorder_get_file_system_type(dir_name, &file_system_type) == 0) {
		/* MSDOS_SUPER_MAGIC : 0x4d44 */
		if (file_system_type == MSDOS_SUPER_MAGIC &&
			(*max_size == 0 || *max_size > FAT32_FILE_SYSTEM_MAX_SIZE)) {
			_mmcam_dbg_warn("FAT32 and too large max[%"G_GUINT64_FORMAT"], set max as %lu",
				*max_size, FAT32_FILE_SYSTEM_MAX_SIZE);
			*max_size = FAT32_FILE_SYSTEM_MAX_SIZE;
		} else {
			_mmcam_dbg_warn("file system 0x%x, max size %"G_GUINT64_FORMAT,
				file_system_type, *max_size);
		}
	}

	g_free(dir_name);
}


int _mmcamcorder_get_storage_info(const gchar *path, const gchar *root_directory, _MMCamcorderStorageInfo *info)
{
	int ret = 0;
	struct stat stat_path;
	struct stat stat_root;

	if (!path || !root_directory || !info) {
		_mmcam_dbg_err("invalid parameter %p %p %p", path, root_directory, info);
		return -1;
	}

	if (strlen(root_directory) > 0) {
		if (stat(path, &stat_path) != 0) {
			_mmcam_dbg_err("failed to stat for [%s][errno %d]", path, errno);
			return -1;
		}

		if (stat(root_directory, &stat_root) != 0) {
			_mmcam_dbg_err("failed to stat for [%s][errno %d]", root_directory, errno);
			return -1;
		}

		if (stat_path.st_dev == stat_root.st_dev)
			info->type = STORAGE_TYPE_INTERNAL;
		else
			info->type = STORAGE_TYPE_EXTERNAL;
	} else {
		info->type = STORAGE_TYPE_INTERNAL;
		_mmcam_dbg_warn("invalid length of root directory, assume that it's internal storage.");
	}

	ret = storage_foreach_device_supported((storage_device_supported_cb)__mmcamcorder_storage_supported_cb, info);
	if (ret != STORAGE_ERROR_NONE) {
		_mmcam_dbg_err("storage_foreach_device_supported failed 0x%x", ret);
		return -1;
	}

	_mmcam_dbg_log("storage info - type %d, id %d", info->type, info->id);

	return 0;
}


int _mmcamcorder_get_freespace(storage_type_e type, guint64 *free_space)
{
	int ret = 0;
	struct statvfs vfs;

	if (type == STORAGE_TYPE_INTERNAL)
		ret = storage_get_internal_memory_size(&vfs);
	else
		ret = storage_get_external_memory_size(&vfs);

	if (ret != STORAGE_ERROR_NONE) {
		*free_space = 0;
		_mmcam_dbg_err("get memory size failed [type %d] 0x%x", type, ret);
		return -1;
	}

	*free_space = vfs.f_bsize * vfs.f_bavail;
	/*
	_mmcam_dbg_log("vfs.f_bsize [%lu], vfs.f_bavail [%lu]", vfs.f_bsize, vfs.f_bavail);
	_mmcam_dbg_log("memory size %"G_GUINT64_FORMAT" [%s]", *free_space, path);
	*/
	return 0;
}


int _mmcamcorder_get_file_system_type(const gchar *path, int *file_system_type)
{
	struct statfs fs;

	g_assert(path);

	if (!g_file_test(path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
		_mmcam_dbg_log("File(%s) doesn't exist.", path);
		return -2;
	}

	if (-1 == statfs(path, &fs)) {
		_mmcam_dbg_log("statfs failed.(%s)", path);
		return -1;
	}

	*file_system_type = (int)fs.f_type;

	return 0;
}


int _mmcamcorder_get_device_flash_brightness(GDBusConnection *conn, int *brightness)
{
	int get_value = 0;
	int ret = MM_ERROR_NONE;
	GVariant *params = NULL;
	GVariant *result = NULL;

	ret = __gdbus_method_call_sync(conn, "org.tizen.system.deviced",
		"/Org/Tizen/System/DeviceD/Led", "org.tizen.system.deviced.Led",
		"GetBrightnessForCamera", params, &result, TRUE);
	if (ret != MM_ERROR_NONE) {
		_mmcam_dbg_err("Dbus Call on Client Error");
		return ret;
	}

	if (result) {
		g_variant_get(result, "(i)", &get_value);
		*brightness = get_value;
		_mmcam_dbg_log("flash brightness : %d", *brightness);
	} else {
		_mmcam_dbg_err("replied result is null");
		ret = MM_ERROR_CAMCORDER_INTERNAL;
	}

	return ret;
}


int _mmcamcorder_send_sound_play_message(GDBusConnection *conn, _MMCamcorderGDbusCbInfo *gdbus_info,
	const char *sample_name, const char *stream_role, const char *volume_gain, int sync_play)
{
	int get_value = 0;
	int ret = MM_ERROR_NONE;
	GVariant *params = NULL;
	GVariant *result = NULL;
	guint subs_id = 0;

	if (!conn || !gdbus_info) {
		_mmcam_dbg_err("Invalid parameter %p %p", conn, gdbus_info);
		return MM_ERROR_CAMCORDER_INTERNAL;
	}

	params = g_variant_new("(sss)", sample_name, stream_role, volume_gain);
	result = g_variant_new("(i)", get_value);

	ret = __gdbus_method_call_sync(conn, "org.pulseaudio.Server",
		"/org/pulseaudio/SoundPlayer", "org.pulseaudio.SoundPlayer",
		"SamplePlay", params, &result, TRUE);
	if (ret != MM_ERROR_NONE) {
		_mmcam_dbg_err("Dbus Call on Client Error");
		return ret;
	}

	if (result) {
		g_variant_get(result, "(i)", &get_value);
		_mmcam_dbg_log("played index : %d", get_value);
	} else {
		_mmcam_dbg_err("replied result is null");
		return MM_ERROR_CAMCORDER_INTERNAL;
	}

	g_mutex_lock(&gdbus_info->sync_mutex);

	if (gdbus_info->subscribe_id > 0) {
		_mmcam_dbg_warn("subscribe_id[%u] is remained. remove it.", gdbus_info->subscribe_id);

		g_dbus_connection_signal_unsubscribe(conn, gdbus_info->subscribe_id);

		gdbus_info->subscribe_id = 0;
	}

	gdbus_info->is_playing = TRUE;
	gdbus_info->param = get_value;

	ret = __gdbus_subscribe_signal(conn,
		"/org/pulseaudio/SoundPlayer", "org.pulseaudio.SoundPlayer", "EOS",
		__gdbus_stream_eos_cb, &subs_id, gdbus_info);

	if (ret == MM_ERROR_NONE)
		gdbus_info->subscribe_id = subs_id;

	g_mutex_unlock(&gdbus_info->sync_mutex);

	if (sync_play && ret == MM_ERROR_NONE)
		ret = __gdbus_wait_for_cb_return(gdbus_info, G_DBUS_TIMEOUT);

	return ret;
}


void _mmcamcorder_request_dpm_popup(GDBusConnection *conn, const char *restricted_policy)
{
	int ret = MM_ERROR_NONE;
	gboolean get_value = 0;
	GVariant *params = NULL;
	GVariant *result = NULL;

	if (!conn || !restricted_policy) {
		_mmcam_dbg_err("Invalid parameter %p %p", conn, restricted_policy);
		return;
	}

	params = g_variant_new("(s)", restricted_policy);
	result = g_variant_new("(b)", get_value);

	ret = __gdbus_method_call_sync(conn,
		"org.tizen.DevicePolicyManager",
		"/org/tizen/DevicePolicyManager/Syspopup",
		"org.tizen.DevicePolicyManager.Syspopup",
		"show", params, &result, TRUE);
	if (ret != MM_ERROR_NONE) {
		_mmcam_dbg_err("Dbus Call on Client Error 0x%x", ret);
		return;
	}

	if (result) {
		g_variant_get(result, "(b)", &get_value);
		_mmcam_dbg_log("request result : %d", get_value);
	} else {
		_mmcam_dbg_err("replied result is null");
	}

	return;
}


int _mmcamcorder_get_file_size(const char *filename, guint64 *size)
{
	struct stat buf;

	if (stat(filename, &buf) != 0)
		return -1;
	*size = (guint64)buf.st_size;
	return 1;
}


void _mmcamcorder_remove_buffer_probe(MMHandleType handle, _MMCamcorderHandlerCategory category)
{
	mmf_camcorder_t* hcamcorder = MMF_CAMCORDER(handle);
	GList *list = NULL;
	MMCamcorderHandlerItem *item = NULL;

	mmf_return_if_fail(hcamcorder);

	if (!hcamcorder->buffer_probes) {
		_mmcam_dbg_warn("list for buffer probe is NULL");
		return;
	}

	_mmcam_dbg_log("start - category : 0x%x", category);

	list = hcamcorder->buffer_probes;
	while (list) {
		item = list->data;
		if (!item) {
			_mmcam_dbg_err("Remove buffer probe faild, the item is NULL");
			list = g_list_next(list);
			continue;
		}

		if (item->category & category) {
			if (item->object && GST_IS_PAD(item->object)) {
				_mmcam_dbg_log("Remove buffer probe on [%s:%s] - [ID : %lu], [Category : %x]",
					GST_DEBUG_PAD_NAME(item->object), item->handler_id,  item->category);
				gst_pad_remove_probe(GST_PAD(item->object), item->handler_id);
			} else {
				_mmcam_dbg_warn("Remove buffer probe faild, the pad is null or not pad, just remove item from list and free it");
			}

			list = g_list_next(list);
			hcamcorder->buffer_probes = g_list_remove(hcamcorder->buffer_probes, item);
			SAFE_G_FREE(item);
		} else {
			_mmcam_dbg_log("Skip item : [ID : %lu], [Category : %x] ", item->handler_id, item->category);
			list = g_list_next(list);
		}
	}

	if (category == _MMCAMCORDER_HANDLER_CATEGORY_ALL) {
		g_list_free(hcamcorder->buffer_probes);
		hcamcorder->buffer_probes = NULL;
	}

	_mmcam_dbg_log("done");

	return;
}


void _mmcamcorder_remove_one_buffer_probe(MMHandleType handle, void *object)
{
	mmf_camcorder_t* hcamcorder = MMF_CAMCORDER(handle);
	GList *list = NULL;
	MMCamcorderHandlerItem *item = NULL;

	mmf_return_if_fail(hcamcorder);

	if (!hcamcorder->buffer_probes) {
		_mmcam_dbg_warn("list for buffer probe is NULL");
		return;
	}

	_mmcam_dbg_log("start - object : %p", object);

	list = hcamcorder->buffer_probes;
	while (list) {
		item = list->data;
		if (!item) {
			_mmcam_dbg_err("Remove buffer probe faild, the item is NULL");
			list = g_list_next(list);
			continue;
		}

		if (item->object && item->object == object) {
			if (GST_IS_PAD(item->object)) {
				_mmcam_dbg_log("Remove buffer probe on [%s:%s] - [ID : %lu], [Category : %x]",
					GST_DEBUG_PAD_NAME(item->object), item->handler_id,  item->category);
				gst_pad_remove_probe(GST_PAD(item->object), item->handler_id);
			} else {
				_mmcam_dbg_warn("Remove buffer probe faild, the pad is null or not pad, just remove item from list and free it");
			}

			list = g_list_next(list);
			hcamcorder->buffer_probes = g_list_remove(hcamcorder->buffer_probes, item);
			SAFE_G_FREE(item);

			break;
		} else {
			_mmcam_dbg_log("Skip item : [ID : %lu], [Category : %x] ", item->handler_id, item->category);
			list = g_list_next(list);
		}
	}

	_mmcam_dbg_log("done");

	return;
}


void _mmcamcorder_remove_event_probe(MMHandleType handle, _MMCamcorderHandlerCategory category)
{
	mmf_camcorder_t* hcamcorder = MMF_CAMCORDER(handle);
	GList *list = NULL;
	MMCamcorderHandlerItem *item = NULL;

	mmf_return_if_fail(hcamcorder);

	if (!hcamcorder->event_probes) {
		_mmcam_dbg_warn("list for event probe is NULL");
		return;
	}

	_mmcam_dbg_log("start - category : 0x%x", category);

	list = hcamcorder->event_probes;
	while (list) {
		item = list->data;
		if (!item) {
			_mmcam_dbg_err("Remove event probe faild, the item is NULL");
			list = g_list_next(list);
			continue;
		}

		if (item->category & category) {
			if (item->object && GST_IS_PAD(item->object)) {
				_mmcam_dbg_log("Remove event probe on [%s:%s] - [ID : %lu], [Category : %x]",
					GST_DEBUG_PAD_NAME(item->object), item->handler_id,  item->category);
				gst_pad_remove_probe(GST_PAD(item->object), item->handler_id);
			} else {
				_mmcam_dbg_warn("Remove event probe faild, the pad is null or not pad, just remove item from list and free it");
			}

			list = g_list_next(list);
			hcamcorder->event_probes = g_list_remove(hcamcorder->event_probes, item);
			SAFE_G_FREE(item);
		} else {
			_mmcam_dbg_log("Skip item : [ID : %lu], [Category : %x] ", item->handler_id, item->category);
			list = g_list_next(list);
		}
	}

	if (category == _MMCAMCORDER_HANDLER_CATEGORY_ALL) {
		g_list_free(hcamcorder->event_probes);
		hcamcorder->event_probes = NULL;
	}

	_mmcam_dbg_log("done");

	return;
}


void _mmcamcorder_disconnect_signal(MMHandleType handle, _MMCamcorderHandlerCategory category)
{
	mmf_camcorder_t* hcamcorder = MMF_CAMCORDER(handle);
	GList *list = NULL;
	MMCamcorderHandlerItem *item = NULL;

	mmf_return_if_fail(hcamcorder);

	if (!hcamcorder->signals) {
		_mmcam_dbg_warn("list for signal is NULL");
		return;
	}

	_mmcam_dbg_log("start - category : 0x%x", category);

	list = hcamcorder->signals;
	while (list) {
		item = list->data;
		if (!item) {
			_mmcam_dbg_err("Fail to Disconnecting signal, the item is NULL");
			list = g_list_next(list);
			continue;
		}

		if (item->category & category) {
			if (item->object && GST_IS_ELEMENT(item->object)) {
				if (g_signal_handler_is_connected(item->object, item->handler_id)) {
					_mmcam_dbg_log("Disconnect signal from [%s] : [ID : %lu], [Category : %x]",
						GST_OBJECT_NAME(item->object), item->handler_id, item->category);
					g_signal_handler_disconnect(item->object, item->handler_id);
				} else {
					_mmcam_dbg_warn("Signal was not connected, cannot disconnect it :  [%s]  [ID : %lu], [Category : %x]",
						GST_OBJECT_NAME(item->object), item->handler_id, item->category);
				}
			} else {
				_mmcam_dbg_err("Fail to Disconnecting signal, the element is null or not element, just remove item from list and free it");
			}

			list = g_list_next(list);
			hcamcorder->signals = g_list_remove(hcamcorder->signals, item);
			SAFE_G_FREE(item);
		} else {
			_mmcam_dbg_log("Skip item : [ID : %lu], [Category : %x] ", item->handler_id, item->category);
			list = g_list_next(list);
		}
	}

	if (category == _MMCAMCORDER_HANDLER_CATEGORY_ALL) {
		g_list_free(hcamcorder->signals);
		hcamcorder->signals = NULL;
	}

	_mmcam_dbg_log("done");

	return;
}


void _mmcamcorder_remove_all_handlers(MMHandleType handle,  _MMCamcorderHandlerCategory category)
{
	mmf_camcorder_t* hcamcorder = MMF_CAMCORDER(handle);

	_mmcam_dbg_log("ENTER");

	if (hcamcorder->signals)
		_mmcamcorder_disconnect_signal((MMHandleType)hcamcorder, category);
	if (hcamcorder->event_probes)
		_mmcamcorder_remove_event_probe((MMHandleType)hcamcorder, category);
	if (hcamcorder->buffer_probes)
		_mmcamcorder_remove_buffer_probe((MMHandleType)hcamcorder, category);

	_mmcam_dbg_log("LEAVE");
}


void _mmcamcorder_element_release_noti(gpointer data, GObject *where_the_object_was)
{
	int i = 0;
	_MMCamcorderSubContext *sc = (_MMCamcorderSubContext *)data;

	mmf_return_if_fail(sc);
	mmf_return_if_fail(sc->element);

	for (i = 0 ; i < _MMCAMCORDER_PIPELINE_ELEMENT_NUM ; i++) {
		if (sc->element[i].gst && (G_OBJECT(sc->element[i].gst) == where_the_object_was)) {
			_mmcam_dbg_warn("The element[%d][%p] is finalized", sc->element[i].id, sc->element[i].gst);
			sc->element[i].gst = NULL;
			sc->element[i].id = _MMCAMCORDER_NONE;
			return;
		}
	}

	mmf_return_if_fail(sc->encode_element);

	for (i = 0 ; i < _MMCAMCORDER_ENCODE_PIPELINE_ELEMENT_NUM ; i++) {
		if (sc->encode_element[i].gst && (G_OBJECT(sc->encode_element[i].gst) == where_the_object_was)) {
			_mmcam_dbg_warn("The encode element[%d][%p] is finalized", sc->encode_element[i].id, sc->encode_element[i].gst);
			sc->encode_element[i].gst = NULL;
			sc->encode_element[i].id = _MMCAMCORDER_ENCODE_NONE;
			return;
		}
	}

	_mmcam_dbg_warn("there is no matching element %p", where_the_object_was);

	return;
}


#ifdef _MMCAMCORDER_ENABLE_IDLE_MESSAGE_CALLBACK
gboolean _mmcamcorder_msg_callback(void *data)
{
	_MMCamcorderMsgItem *item = (_MMCamcorderMsgItem*)data;
	mmf_camcorder_t *hcamcorder = NULL;
	mmf_return_val_if_fail(item, FALSE);

	g_mutex_lock(&item->lock);

	hcamcorder = MMF_CAMCORDER(item->handle);
	if (hcamcorder == NULL) {
		_mmcam_dbg_warn("msg id:0x%x, item:%p, handle is NULL", item->id, item);
		goto MSG_CALLBACK_DONE;
	}

	/*_mmcam_dbg_log("msg id:%x, msg_cb:%p, msg_data:%p, item:%p", item->id, hcamcorder->msg_cb, hcamcorder->msg_data, item);*/

	_MMCAMCORDER_LOCK(hcamcorder);

	/* remove item from msg data */
	if (hcamcorder->msg_data) {
		/*_mmcam_dbg_log("remove item %p", item);*/
		hcamcorder->msg_data = g_list_remove(hcamcorder->msg_data, item);
	} else {
		_mmcam_dbg_warn("msg_data is NULL but item[%p] will be removed", item);
	}

	_MMCAMCORDER_UNLOCK(hcamcorder);

	_MMCAMCORDER_LOCK_MESSAGE_CALLBACK(hcamcorder);

	if ((hcamcorder) && (hcamcorder->msg_cb))
		hcamcorder->msg_cb(item->id, (MMMessageParamType*)(&(item->param)), hcamcorder->msg_cb_param);

	_MMCAMCORDER_UNLOCK_MESSAGE_CALLBACK(hcamcorder);

	_MMCAMCORDER_SIGNAL(hcamcorder);

MSG_CALLBACK_DONE:
	/* release allocated memory */
	if (item->id == MM_MESSAGE_CAMCORDER_FACE_DETECT_INFO) {
		MMCamFaceDetectInfo *cam_fd_info = (MMCamFaceDetectInfo *)item->param.data;
		if (cam_fd_info) {
			SAFE_G_FREE(cam_fd_info->face_info);
			SAFE_G_FREE(cam_fd_info);

			item->param.data = NULL;
			item->param.size = 0;
		}
	} else if (item->id == MM_MESSAGE_CAMCORDER_VIDEO_CAPTURED || item->id == MM_MESSAGE_CAMCORDER_AUDIO_CAPTURED) {
		MMCamRecordingReport *report = (MMCamRecordingReport *)item->param.data;
		if (report) {
			SAFE_G_FREE(report->recording_filename);
			g_free(report);
			report = NULL;
			item->param.data = NULL;
		}
	}

	g_mutex_unlock(&item->lock);
	g_mutex_clear(&item->lock);

	SAFE_G_FREE(item);

	/* For not being called again */
	return FALSE;
}
#endif /* _MMCAMCORDER_ENABLE_IDLE_MESSAGE_CALLBACK */


gboolean _mmcamcorder_send_message(MMHandleType handle, _MMCamcorderMsgItem *data)
{
	mmf_camcorder_t* hcamcorder = MMF_CAMCORDER(handle);
#ifdef _MMCAMCORDER_ENABLE_IDLE_MESSAGE_CALLBACK
	_MMCamcorderMsgItem *item = NULL;
#endif /* _MMCAMCORDER_ENABLE_IDLE_MESSAGE_CALLBACK */

	mmf_return_val_if_fail(hcamcorder, FALSE);
	mmf_return_val_if_fail(data, FALSE);

	switch (data->id) {
	case MM_MESSAGE_CAMCORDER_STATE_CHANGED:
	case MM_MESSAGE_CAMCORDER_STATE_CHANGED_BY_RM:
		data->param.union_type = MM_MSG_UNION_STATE;
		break;
	case MM_MESSAGE_CAMCORDER_RECORDING_STATUS:
		data->param.union_type = MM_MSG_UNION_RECORDING_STATUS;
		break;
	case MM_MESSAGE_CAMCORDER_FIRMWARE_UPDATE:
		data->param.union_type = MM_MSG_UNION_FIRMWARE;
		break;
	case MM_MESSAGE_CAMCORDER_CURRENT_VOLUME:
		data->param.union_type = MM_MSG_UNION_REC_VOLUME_DB;
		break;
	case MM_MESSAGE_CAMCORDER_TIME_LIMIT:
	case MM_MESSAGE_CAMCORDER_MAX_SIZE:
	case MM_MESSAGE_CAMCORDER_NO_FREE_SPACE:
	case MM_MESSAGE_CAMCORDER_ERROR:
	case MM_MESSAGE_CAMCORDER_FOCUS_CHANGED:
	case MM_MESSAGE_CAMCORDER_CAPTURED:
	case MM_MESSAGE_CAMCORDER_VIDEO_CAPTURED:
	case MM_MESSAGE_CAMCORDER_AUDIO_CAPTURED:
	case MM_MESSAGE_READY_TO_RESUME:
	default:
		data->param.union_type = MM_MSG_UNION_CODE;
		break;
	}

#ifdef _MMCAMCORDER_ENABLE_IDLE_MESSAGE_CALLBACK
	item = g_malloc(sizeof(_MMCamcorderMsgItem));
	if (item) {
		memcpy(item, data, sizeof(_MMCamcorderMsgItem));
		item->handle = handle;
		g_mutex_init(&item->lock);

		_MMCAMCORDER_LOCK(handle);
		hcamcorder->msg_data = g_list_append(hcamcorder->msg_data, item);
		/*_mmcam_dbg_log("item[%p]", item);*/

		/* Use DEFAULT priority */
		g_idle_add_full(G_PRIORITY_DEFAULT, _mmcamcorder_msg_callback, item, NULL);

		_MMCAMCORDER_UNLOCK(handle);
	} else {
		_mmcam_dbg_err("item[id:0x%x] malloc failed : %d", data->id, sizeof(_MMCamcorderMsgItem));
	}
#else /* _MMCAMCORDER_ENABLE_IDLE_MESSAGE_CALLBACK */
	_MMCAMCORDER_LOCK_MESSAGE_CALLBACK(hcamcorder);

	if (hcamcorder->msg_cb)
		hcamcorder->msg_cb(data->id, (MMMessageParamType*)(&(data->param)), hcamcorder->msg_cb_param);
	else
		_mmcam_dbg_log("message callback is NULL. message id %d", data->id);

	_MMCAMCORDER_UNLOCK_MESSAGE_CALLBACK(hcamcorder);

	/* release allocated memory */
	if (data->id == MM_MESSAGE_CAMCORDER_FACE_DETECT_INFO) {
		MMCamFaceDetectInfo *cam_fd_info = (MMCamFaceDetectInfo *)data->param.data;
		if (cam_fd_info) {
			SAFE_G_FREE(cam_fd_info->face_info);
			data->param.data = NULL;
			g_free(cam_fd_info);
		}
	} else if (data->id == MM_MESSAGE_CAMCORDER_VIDEO_CAPTURED || data->id == MM_MESSAGE_CAMCORDER_AUDIO_CAPTURED) {
		MMCamRecordingReport *report = (MMCamRecordingReport *)data->param.data;
		if (report) {
			SAFE_G_FREE(report->recording_filename);
			data->param.data = NULL;
			g_free(report);
		}
	}
#endif /* _MMCAMCORDER_ENABLE_IDLE_MESSAGE_CALLBACK */

	return TRUE;
}


void _mmcamcorder_remove_message_all(MMHandleType handle)
{
	mmf_camcorder_t* hcamcorder = MMF_CAMCORDER(handle);
	gboolean ret = TRUE;
#ifdef _MMCAMCORDER_ENABLE_IDLE_MESSAGE_CALLBACK
	_MMCamcorderMsgItem *item = NULL;
	GList *list = NULL;
	gint64 end_time = 0;
#endif /* _MMCAMCORDER_ENABLE_IDLE_MESSAGE_CALLBACK */

	mmf_return_if_fail(hcamcorder);

	_MMCAMCORDER_LOCK(handle);

#ifdef _MMCAMCORDER_ENABLE_IDLE_MESSAGE_CALLBACK
	if (!hcamcorder->msg_data) {
		_mmcam_dbg_log("No message data is remained.");
	} else {
		list = hcamcorder->msg_data;

		while (list) {
			item = list->data;
			list = g_list_next(list);

			if (!item) {
				_mmcam_dbg_err("Fail to remove message. The item is NULL");
			} else {
				if (g_mutex_trylock(&(item->lock))) {
					ret = g_idle_remove_by_data(item);

					_mmcam_dbg_log("remove msg item[%p], ret[%d]", item, ret);

					if (ret == FALSE) {
						item->handle = 0;
						_mmcam_dbg_warn("failed to remove msg cb for item %p, it will be called later with NULL handle", item);
					}

					/* release allocated memory */
					if (item->id == MM_MESSAGE_CAMCORDER_FACE_DETECT_INFO) {
						MMCamFaceDetectInfo *cam_fd_info = (MMCamFaceDetectInfo *)item->param.data;
						if (cam_fd_info) {
							SAFE_G_FREE(cam_fd_info->face_info);
							item->param.size = 0;
						}
						SAFE_G_FREE(cam_fd_info);
					} else if (item->id == MM_MESSAGE_CAMCORDER_VIDEO_CAPTURED || item->id == MM_MESSAGE_CAMCORDER_AUDIO_CAPTURED) {
						MMCamRecordingReport *report = (MMCamRecordingReport *)item->param.data;
						if (report)
							SAFE_G_FREE(report->recording_filename);

						SAFE_G_FREE(report);
					}

					hcamcorder->msg_data = g_list_remove(hcamcorder->msg_data, item);

					g_mutex_unlock(&(item->lock));

					if (ret == TRUE) {
						g_mutex_clear(&item->lock);

						SAFE_G_FREE(item);

						_mmcam_dbg_log("remove msg done");
					}
				} else {
					_mmcam_dbg_warn("item lock failed. it's being called...");

					end_time = g_get_monotonic_time() + (100 * G_TIME_SPAN_MILLISECOND);

					if (_MMCAMCORDER_WAIT_UNTIL(handle, end_time))
						_mmcam_dbg_warn("signal received");
					else
						_mmcam_dbg_warn("timeout");
				}
			}
		}

		g_list_free(hcamcorder->msg_data);
		hcamcorder->msg_data = NULL;
	}
#endif /* _MMCAMCORDER_ENABLE_IDLE_MESSAGE_CALLBACK */

	/* remove idle function for playing capture sound */
	do {
		ret = g_idle_remove_by_data(hcamcorder);
		_mmcam_dbg_log("remove idle function for playing capture sound. ret[%d]", ret);
	} while (ret);

	_MMCAMCORDER_UNLOCK(handle);

	return;
}


int _mmcamcorder_get_pixel_format(GstCaps *caps)
{
	const GstStructure *structure;
	const char *media_type;
	GstVideoInfo media_info;
	MMPixelFormatType type = 0;
	unsigned int fourcc = 0;

	mmf_return_val_if_fail(caps != NULL, MM_PIXEL_FORMAT_INVALID);

	structure = gst_caps_get_structure(caps, 0);
	media_type = gst_structure_get_name(structure);
	if (media_type == NULL) {
		_mmcam_dbg_err("failed to get media_type");
		return MM_PIXEL_FORMAT_INVALID;
	}

	gst_video_info_init(&media_info);

	if (!strcmp(media_type, "image/jpeg")) {
		_mmcam_dbg_log("It is jpeg.");
		type = MM_PIXEL_FORMAT_ENCODED;
	} else if (!strcmp(media_type, "video/x-raw") &&
		   gst_video_info_from_caps(&media_info, caps) &&
		   GST_VIDEO_INFO_IS_YUV(&media_info)) {
		_mmcam_dbg_log("It is yuv.");
		fourcc = gst_video_format_to_fourcc(GST_VIDEO_INFO_FORMAT(&media_info));
		type = _mmcamcorder_get_pixtype(fourcc);
	} else if (!strcmp(media_type, "video/x-raw") &&
		   gst_video_info_from_caps(&media_info, caps) &&
		   GST_VIDEO_INFO_IS_RGB(&media_info)) {
		_mmcam_dbg_log("It is rgb.");
		type = MM_PIXEL_FORMAT_RGB888;
	} else if (!strcmp(media_type, "video/x-h264")) {
		_mmcam_dbg_log("It is H264");
		type = MM_PIXEL_FORMAT_ENCODED_H264;
	} else {
		_mmcam_dbg_err("Not supported format [%s]", media_type);
		type = MM_PIXEL_FORMAT_INVALID;
	}

	/*_mmcam_dbg_log( "Type [%d]", type );*/

	return type;
}

unsigned int _mmcamcorder_get_fourcc(int pixtype, int codectype, int use_zero_copy_format)
{
	unsigned int fourcc = 0;

	/*_mmcam_dbg_log("pixtype(%d)", pixtype);*/

	switch (pixtype) {
	case MM_PIXEL_FORMAT_NV12:
		if (use_zero_copy_format)
			fourcc = GST_MAKE_FOURCC('S', 'N', '1', '2');
		else
			fourcc = GST_MAKE_FOURCC('N', 'V', '1', '2');

		break;
	case MM_PIXEL_FORMAT_NV21:
		if (use_zero_copy_format)
			fourcc = GST_MAKE_FOURCC('S', 'N', '2', '1');
		else
			fourcc = GST_MAKE_FOURCC('N', 'V', '2', '1');

		break;
	case MM_PIXEL_FORMAT_YUYV:
		if (use_zero_copy_format)
			fourcc = GST_MAKE_FOURCC('S', 'U', 'Y', 'V');
		else
			fourcc = GST_MAKE_FOURCC('Y', 'U', 'Y', '2');

		break;
	case MM_PIXEL_FORMAT_UYVY:
		if (use_zero_copy_format)
			fourcc = GST_MAKE_FOURCC('S', 'Y', 'V', 'Y');
		else
			fourcc = GST_MAKE_FOURCC('U', 'Y', 'V', 'Y');

		break;
	case MM_PIXEL_FORMAT_I420:
		if (use_zero_copy_format)
			fourcc = GST_MAKE_FOURCC('S', '4', '2', '0');
		else
			fourcc = GST_MAKE_FOURCC('I', '4', '2', '0');

		break;
	case MM_PIXEL_FORMAT_YV12:
		fourcc = GST_MAKE_FOURCC('Y', 'V', '1', '2');
		break;
	case MM_PIXEL_FORMAT_422P:
		fourcc = GST_MAKE_FOURCC('4', '2', '2', 'P');
		break;
	case MM_PIXEL_FORMAT_RGB565:
		fourcc = GST_MAKE_FOURCC('R', 'G', 'B', 'P');
		break;
	case MM_PIXEL_FORMAT_RGB888:
		fourcc = GST_MAKE_FOURCC('R', 'G', 'B', ' ');
		break;
	case MM_PIXEL_FORMAT_RGBA:
		fourcc = GST_MAKE_FOURCC('B', 'G', 'R', 'x');
		break;
	case MM_PIXEL_FORMAT_ARGB:
		fourcc = GST_MAKE_FOURCC('x', 'R', 'G', 'B');
		break;
	case MM_PIXEL_FORMAT_ENCODED:
		if (codectype == MM_IMAGE_CODEC_JPEG) {
			fourcc = GST_MAKE_FOURCC('J', 'P', 'E', 'G');
		} else if (codectype == MM_IMAGE_CODEC_JPEG_SRW) {
			fourcc = GST_MAKE_FOURCC('J', 'P', 'E', 'G'); /*TODO: JPEG+SamsungRAW format */
		} else if (codectype == MM_IMAGE_CODEC_SRW) {
			fourcc = GST_MAKE_FOURCC('J', 'P', 'E', 'G'); /*TODO: SamsungRAW format */
		} else if (codectype == MM_IMAGE_CODEC_PNG) {
			fourcc = GST_MAKE_FOURCC('P', 'N', 'G', ' ');
		} else {
			/* Please let us know what other fourcces are. ex) BMP, GIF?*/
			fourcc = GST_MAKE_FOURCC('J', 'P', 'E', 'G');
		}
		break;
	case MM_PIXEL_FORMAT_ITLV_JPEG_UYVY:
		fourcc = GST_MAKE_FOURCC('I', 'T', 'L', 'V');
		break;
	case MM_PIXEL_FORMAT_ENCODED_H264:
		fourcc = GST_MAKE_FOURCC('H', '2', '6', '4');
		break;
	case MM_PIXEL_FORMAT_INVZ:
		fourcc = GST_MAKE_FOURCC('I', 'N', 'V', 'Z');
		break;
	default:
		_mmcam_dbg_log("Not proper pixel type[%d]. Set default - I420", pixtype);
		if (use_zero_copy_format)
			fourcc = GST_MAKE_FOURCC('S', '4', '2', '0');
		else
			fourcc = GST_MAKE_FOURCC('I', '4', '2', '0');

		break;
	}

	return fourcc;
}


int _mmcamcorder_get_pixtype(unsigned int fourcc)
{
	int pixtype = MM_PIXEL_FORMAT_INVALID;
	/*
	char *pfourcc = (char*)&fourcc;

	_mmcam_dbg_log("fourcc(%c%c%c%c)",
				   pfourcc[0], pfourcc[1], pfourcc[2], pfourcc[3]);
	*/

	switch (fourcc) {
	case GST_MAKE_FOURCC('S', 'N', '1', '2'):
	case GST_MAKE_FOURCC('N', 'V', '1', '2'):
		pixtype = MM_PIXEL_FORMAT_NV12;
		break;
	case GST_MAKE_FOURCC('S', 'N', '2', '1'):
	case GST_MAKE_FOURCC('N', 'V', '2', '1'):
		pixtype = MM_PIXEL_FORMAT_NV21;
		break;
	case GST_MAKE_FOURCC('S', 'U', 'Y', 'V'):
	case GST_MAKE_FOURCC('Y', 'U', 'Y', 'V'):
	case GST_MAKE_FOURCC('Y', 'U', 'Y', '2'):
		pixtype = MM_PIXEL_FORMAT_YUYV;
		break;
	case GST_MAKE_FOURCC('S', 'Y', 'V', 'Y'):
	case GST_MAKE_FOURCC('U', 'Y', 'V', 'Y'):
		pixtype = MM_PIXEL_FORMAT_UYVY;
		break;
	case GST_MAKE_FOURCC('S', '4', '2', '0'):
	case GST_MAKE_FOURCC('I', '4', '2', '0'):
		pixtype = MM_PIXEL_FORMAT_I420;
		break;
	case GST_MAKE_FOURCC('Y', 'V', '1', '2'):
		pixtype = MM_PIXEL_FORMAT_YV12;
		break;
	case GST_MAKE_FOURCC('4', '2', '2', 'P'):
		pixtype = MM_PIXEL_FORMAT_422P;
		break;
	case GST_MAKE_FOURCC('R', 'G', 'B', 'P'):
		pixtype = MM_PIXEL_FORMAT_RGB565;
		break;
	case GST_MAKE_FOURCC('R', 'G', 'B', '3'):
		pixtype = MM_PIXEL_FORMAT_RGB888;
		break;
	case GST_MAKE_FOURCC('A', 'R', 'G', 'B'):
	case GST_MAKE_FOURCC('x', 'R', 'G', 'B'):
		pixtype = MM_PIXEL_FORMAT_ARGB;
		break;
	case GST_MAKE_FOURCC('B', 'G', 'R', 'A'):
	case GST_MAKE_FOURCC('B', 'G', 'R', 'x'):
	case GST_MAKE_FOURCC('S', 'R', '3', '2'):
		pixtype = MM_PIXEL_FORMAT_RGBA;
		break;
	case GST_MAKE_FOURCC('J', 'P', 'E', 'G'):
	case GST_MAKE_FOURCC('P', 'N', 'G', ' '):
		pixtype = MM_PIXEL_FORMAT_ENCODED;
		break;
	/*FIXME*/
	case GST_MAKE_FOURCC('I', 'T', 'L', 'V'):
		pixtype = MM_PIXEL_FORMAT_ITLV_JPEG_UYVY;
		break;
	case GST_MAKE_FOURCC('H', '2', '6', '4'):
		pixtype = MM_PIXEL_FORMAT_ENCODED_H264;
		break;
	case GST_MAKE_FOURCC('I', 'N', 'V', 'Z'):
		pixtype = MM_PIXEL_FORMAT_INVZ;
		break;
	default:
		_mmcam_dbg_log("Not supported fourcc type(%c%c%c%c)", fourcc, fourcc>>8, fourcc>>16, fourcc>>24);
		pixtype = MM_PIXEL_FORMAT_INVALID;
		break;
	}

	return pixtype;
}


gboolean _mmcamcorder_add_elements_to_bin(GstBin *bin, GList *element_list)
{
	GList *local_list = element_list;
	_MMCamcorderGstElement *element = NULL;

	mmf_return_val_if_fail(bin && local_list, FALSE);

	while (local_list) {
		element = (_MMCamcorderGstElement*)local_list->data;
		if (element && element->gst) {
			if (!gst_bin_add(bin, GST_ELEMENT(element->gst))) {
				_mmcam_dbg_err("Add element [%s] to bin [%s] FAILED",
					GST_ELEMENT_NAME(GST_ELEMENT(element->gst)),
					GST_ELEMENT_NAME(GST_ELEMENT(bin)));
				return FALSE;
			} else {
				_mmcam_dbg_log("Add element [%s] to bin [%s] OK",
					GST_ELEMENT_NAME(GST_ELEMENT(element->gst)),
					GST_ELEMENT_NAME(GST_ELEMENT(bin)));
			}
		}
		local_list = local_list->next;
	}

	return TRUE;
}

gboolean _mmcamcorder_link_elements(GList *element_list)
{
	GList                  *local_list  = element_list;
	_MMCamcorderGstElement *element     = NULL;
	_MMCamcorderGstElement *pre_element = NULL;

	mmf_return_val_if_fail(local_list, FALSE);

	pre_element = (_MMCamcorderGstElement*)local_list->data;
	local_list = local_list->next;

	while (local_list) {
		element = (_MMCamcorderGstElement*)local_list->data;
		if (pre_element && pre_element->gst && element && element->gst) {
			if (_MM_GST_ELEMENT_LINK(GST_ELEMENT(pre_element->gst), GST_ELEMENT(element->gst))) {
				_mmcam_dbg_log("Link [%s] to [%s] OK",
					GST_ELEMENT_NAME(GST_ELEMENT(pre_element->gst)),
					GST_ELEMENT_NAME(GST_ELEMENT(element->gst)));
			} else {
				_mmcam_dbg_err("Link [%s] to [%s] FAILED",
					GST_ELEMENT_NAME(GST_ELEMENT(pre_element->gst)),
					GST_ELEMENT_NAME(GST_ELEMENT(element->gst)));
				return FALSE;
			}
		}

		pre_element = element;
		local_list = local_list->next;
	}

	return TRUE;
}

gboolean _mmcamcorder_resize_frame(unsigned char *src_data, unsigned int src_width, unsigned int src_height, unsigned int src_length, int src_format,
	unsigned char **dst_data, unsigned int *dst_width, unsigned int *dst_height, size_t *dst_length)
{
	int mm_ret = MM_ERROR_NONE;
	int input_format = MM_UTIL_COLOR_YUV420;
	mm_util_image_h src_image = NULL;
	mm_util_image_h dst_image = NULL;

	if (!src_data || !dst_data || !dst_width || !dst_height || !dst_length) {
		_mmcam_dbg_err("something is NULL %p,%p,%p,%p,%p",
			src_data, dst_data, dst_width, dst_height, dst_length);
		return FALSE;
	}

	/* set input format for mm-util */
	switch (src_format) {
	case MM_PIXEL_FORMAT_I420:
		input_format = MM_UTIL_COLOR_I420;
		break;
	case MM_PIXEL_FORMAT_YV12:
		input_format = MM_UTIL_COLOR_YUV420;
		break;
	case MM_PIXEL_FORMAT_NV12:
		input_format = MM_UTIL_COLOR_NV12;
		break;
	case MM_PIXEL_FORMAT_YUYV:
		input_format = MM_UTIL_COLOR_YUYV;
		break;
	case MM_PIXEL_FORMAT_UYVY:
		input_format = MM_UTIL_COLOR_UYVY;
		break;
	case MM_PIXEL_FORMAT_RGB888:
		input_format = MM_UTIL_COLOR_RGB24;
		break;
	default:
		_mmcam_dbg_err("NOT supported format [%d]", src_format);
		return FALSE;
	}

	_mmcam_dbg_log("src size %dx%d -> dst size %dx%d", src_width, src_height, *dst_width, *dst_height);

	mm_ret = mm_image_create_image(src_width, src_height, input_format, src_data, (size_t)src_length, &src_image);
	if (mm_ret != MM_ERROR_NONE) {
		_mmcam_dbg_err("mm_image_create_image failed 0x%x", mm_ret);
		return FALSE;
	}

	mm_ret = mm_util_resize_image(src_image, *dst_width, *dst_height, &dst_image);
	mm_image_destroy_image(src_image);
	if (mm_ret != MM_ERROR_NONE) {
		_mmcam_dbg_err("mm_util_resize_image failed 0x%x", mm_ret);
		return FALSE;
	}

	mm_ret = mm_image_get_image(dst_image, dst_width, dst_height, NULL, dst_data, dst_length);
	mm_image_destroy_image(dst_image);
	if (mm_ret != MM_ERROR_NONE) {
		_mmcam_dbg_err("mm_image_get_image failed 0x%x", mm_ret);
		return FALSE;
	}

	_mmcam_dbg_log("resize done %dx%d -> %dx%d, %p, length %zu",
		src_width, src_height, *dst_width, *dst_height, *dst_data, *dst_length);

	return TRUE;
}


gboolean _mmcamcorder_encode_jpeg(void *src_data, unsigned int src_width, unsigned int src_height,
	int src_format, unsigned int src_length, unsigned int jpeg_quality,
	void **result_data, unsigned int *result_length)
{
	int ret = 0;
	gboolean ret_conv = TRUE;
	int jpeg_format = 0;
	unsigned char *converted_src = NULL;
	unsigned int converted_src_size = 0;

	mmf_return_val_if_fail(src_data && result_data && result_length, FALSE);

	switch (src_format) {
	case MM_PIXEL_FORMAT_NV12:
		//jpeg_format = MM_UTIL_COLOR_NV12;
		ret_conv = _mmcamcorder_convert_NV12_to_I420(src_data, src_width, src_height, &converted_src, &converted_src_size);
		jpeg_format = MM_UTIL_COLOR_YUV420;
		break;
	case MM_PIXEL_FORMAT_NV16:
		jpeg_format = MM_UTIL_COLOR_NV16;
		converted_src = src_data;
		break;
	case MM_PIXEL_FORMAT_NV21:
		jpeg_format = MM_UTIL_COLOR_NV21;
		converted_src = src_data;
		break;
	case MM_PIXEL_FORMAT_YUYV:
		//jpeg_format = MM_UTIL_COLOR_YUYV;
		ret_conv = _mmcamcorder_convert_YUYV_to_I420(src_data, src_width, src_height, &converted_src, &converted_src_size);
		jpeg_format = MM_UTIL_COLOR_YUV420;
		break;
	case MM_PIXEL_FORMAT_UYVY:
		//jpeg_format = MM_UTIL_COLOR_UYVY;
		ret_conv = _mmcamcorder_convert_UYVY_to_I420(src_data, src_width, src_height, &converted_src, &converted_src_size);
		jpeg_format = MM_UTIL_COLOR_YUV420;
		break;
	case MM_PIXEL_FORMAT_I420:
		jpeg_format = MM_UTIL_COLOR_YUV420;
		converted_src = src_data;
		break;
	case MM_PIXEL_FORMAT_RGB888:
		jpeg_format = MM_UTIL_COLOR_RGB24;
		converted_src = src_data;
		break;
	case MM_PIXEL_FORMAT_RGBA:
		jpeg_format = MM_UTIL_COLOR_RGBA;
		converted_src = src_data;
		break;
	case MM_PIXEL_FORMAT_ARGB:
		jpeg_format = MM_UTIL_COLOR_ARGB;
		converted_src = src_data;
		break;
	case MM_PIXEL_FORMAT_422P:
		jpeg_format = MM_UTIL_COLOR_YUV422;	// not supported
		return FALSE;
	case MM_PIXEL_FORMAT_NV12T:
	case MM_PIXEL_FORMAT_YV12:
	case MM_PIXEL_FORMAT_RGB565:
	case MM_PIXEL_FORMAT_ENCODED:
	case MM_PIXEL_FORMAT_ITLV_JPEG_UYVY:
	default:
		return FALSE;
	}

	if (ret_conv == FALSE) {
		if (converted_src &&
		    converted_src != src_data) {
			free(converted_src);
			converted_src = NULL;
		}
		_mmcam_dbg_err("color convert error source format[%d], jpeg format[%d]", src_format, jpeg_format);
		return FALSE;
	}

	ret = mm_util_jpeg_encode_to_memory(result_data, result_length,
		converted_src, src_width, src_height, jpeg_format, jpeg_quality);

	if (converted_src && (converted_src != src_data)) {
		free(converted_src);
		converted_src = NULL;
	}

	if (ret != MM_ERROR_NONE) {
		_mmcam_dbg_err("No encoder supports %d format, error code %x", src_format, ret);
		return FALSE;
	}

	return TRUE;
}


/* make UYVY smaller as multiple size. ex: 640x480 -> 320x240 or 160x120 ... */
gboolean _mmcamcorder_downscale_UYVYorYUYV(unsigned char *src, unsigned int src_width, unsigned int src_height,
	unsigned char **dst, unsigned int dst_width, unsigned int dst_height)
{
	unsigned int i = 0;
	int j = 0;
	int k = 0;
	int src_index = 0;
	int ratio_width = 0;
	int ratio_height = 0;
	int line_base = 0;
	int line_width = 0;
	int jump_width = 0;
	unsigned char *result = NULL;

	if (src == NULL || dst == NULL) {
		_mmcam_dbg_err("src[%p] or dst[%p] is NULL", src, dst);
		return FALSE;
	}

	result = (unsigned char *)malloc((dst_width * dst_height)<<1);
	if (!result) {
		_mmcam_dbg_err("failed to alloc dst data");
		return FALSE;
	}

	ratio_width = src_width / dst_width;
	ratio_height = src_height / dst_height;
	line_width = src_width << 1;
	jump_width = ratio_width << 1;

	_mmcam_dbg_warn("[src %dx%d] [dst %dx%d] [line width %d] [ratio width %d, height %d]",
		src_width, src_height, dst_width, dst_height, line_width, ratio_width, ratio_height);

	for (i = 0 ; i < src_height ; i += ratio_height) {
		line_base = i * line_width;
		for (j = 0 ; j < line_width ; j += jump_width) {
			src_index = line_base + j;
			result[k++] = src[src_index];
			result[k++] = src[src_index+1];

			j += jump_width;
			src_index = line_base + j;
			if (src_index % 4 == 0)
				result[k++] = src[src_index+2];
			else
				result[k++] = src[src_index];

			result[k++] = src[src_index+1];
		}
	}

	*dst = result;

	_mmcam_dbg_warn("converting done - result %p", result);

	return TRUE;
}


static guint16 get_language_code(const char *str)
{
	return (guint16)(((str[0]-0x60) & 0x1F) << 10) + (((str[1]-0x60) & 0x1F) << 5) + ((str[2]-0x60) & 0x1F);
}

static gchar * str_to_utf8(const gchar *str)
{
	return g_convert(str, -1, "UTF-8", "ASCII", NULL, NULL, NULL);
}

static inline gboolean write_tag(FILE *f, const gchar *tag)
{
	while (*tag)
		FPUTC_CHECK(*tag++, f);

	return TRUE;
}

static inline gboolean write_to_32(FILE *f, guint val)
{
	FPUTC_CHECK(val >> 24, f);
	FPUTC_CHECK(val >> 16, f);
	FPUTC_CHECK(val >> 8, f);
	FPUTC_CHECK(val, f);
	return TRUE;
}

static inline gboolean write_to_16(FILE *f, guint val)
{
	FPUTC_CHECK(val >> 8, f);
	FPUTC_CHECK(val, f);
	return TRUE;
}

static inline gboolean write_to_24(FILE *f, guint val)
{
	write_to_16(f, val >> 8);
	FPUTC_CHECK(val, f);
	return TRUE;
}

void *_mmcamcorder_util_task_thread_func(void *data)
{
	int ret = MM_ERROR_NONE;
	mmf_camcorder_t *hcamcorder = (mmf_camcorder_t *)data;

	if (!hcamcorder) {
		_mmcam_dbg_err("handle is NULL");
		return NULL;
	}

	_mmcam_dbg_warn("start thread");

	g_mutex_lock(&hcamcorder->task_thread_lock);

	while (hcamcorder->task_thread_state != _MMCAMCORDER_TASK_THREAD_STATE_EXIT) {
		switch (hcamcorder->task_thread_state) {
		case _MMCAMCORDER_TASK_THREAD_STATE_NONE:
			_mmcam_dbg_warn("wait for task signal");
			g_cond_wait(&hcamcorder->task_thread_cond, &hcamcorder->task_thread_lock);
			_mmcam_dbg_warn("task signal received : state %d", hcamcorder->task_thread_state);
			break;
		case _MMCAMCORDER_TASK_THREAD_STATE_SOUND_PLAY_START:
			_mmcamcorder_sound_play((MMHandleType)hcamcorder, _MMCAMCORDER_SAMPLE_SOUND_NAME_CAPTURE02, FALSE);
			hcamcorder->task_thread_state = _MMCAMCORDER_TASK_THREAD_STATE_NONE;
			break;
		case _MMCAMCORDER_TASK_THREAD_STATE_SOUND_SOLO_PLAY_START:
			_mmcamcorder_sound_solo_play((MMHandleType)hcamcorder, _MMCAMCORDER_SAMPLE_SOUND_NAME_CAPTURE01, FALSE);
			hcamcorder->task_thread_state = _MMCAMCORDER_TASK_THREAD_STATE_NONE;
			break;
		case _MMCAMCORDER_TASK_THREAD_STATE_ENCODE_PIPE_CREATE:
			ret = _mmcamcorder_video_prepare_record((MMHandleType)hcamcorder);

			/* Play record start sound */
			_mmcamcorder_sound_solo_play((MMHandleType)hcamcorder, _MMCAMCORDER_SAMPLE_SOUND_NAME_REC_START, FALSE);

			_mmcam_dbg_log("_mmcamcorder_video_prepare_record return 0x%x", ret);
			hcamcorder->task_thread_state = _MMCAMCORDER_TASK_THREAD_STATE_NONE;
			break;
		case _MMCAMCORDER_TASK_THREAD_STATE_CHECK_CAPTURE_IN_RECORDING:
			{
				gint64 end_time = 0;

				_mmcam_dbg_warn("wait for capture data in recording. wait signal...");

				end_time = g_get_monotonic_time() + (5 * G_TIME_SPAN_SECOND);

				if (g_cond_wait_until(&hcamcorder->task_thread_cond, &hcamcorder->task_thread_lock, end_time)) {
					_mmcam_dbg_warn("signal received");
				} else {
					_MMCamcorderMsgItem message;

					memset(&message, 0x0, sizeof(_MMCamcorderMsgItem));

					_mmcam_dbg_err("capture data wait time out, send error message");

					message.id = MM_MESSAGE_CAMCORDER_ERROR;
					message.param.code = MM_ERROR_CAMCORDER_RESPONSE_TIMEOUT;

					_mmcamcorder_send_message((MMHandleType)hcamcorder, &message);

					hcamcorder->capture_in_recording = FALSE;
				}

				hcamcorder->task_thread_state = _MMCAMCORDER_TASK_THREAD_STATE_NONE;
			}
			break;
		default:
			_mmcam_dbg_warn("invalid task thread state %d", hcamcorder->task_thread_state);
			hcamcorder->task_thread_state = _MMCAMCORDER_TASK_THREAD_STATE_EXIT;
			break;
		}
	}

	g_mutex_unlock(&hcamcorder->task_thread_lock);

	_mmcam_dbg_warn("exit thread");

	return NULL;
}


static gboolean _mmcamcorder_convert_YUYV_to_I420(unsigned char *src, guint width, guint height, unsigned char **dst, unsigned int *dst_len)
{
	unsigned int i = 0;
	int j = 0;
	int src_offset = 0;
	int dst_y_offset = 0;
	int dst_u_offset = 0;
	int dst_v_offset = 0;
	int loop_length = 0;
	unsigned int dst_size = 0;
	unsigned char *dst_data = NULL;

	if (!src || !dst || !dst_len) {
		_mmcam_dbg_err("NULL pointer %p, %p, %p", src, dst, dst_len);
		return FALSE;
	}

	dst_size = (width * height * 3) >> 1;

	_mmcam_dbg_log("YUVY -> I420 : %dx%d, dst size %d", width, height, dst_size);

	dst_data = (unsigned char *)malloc(dst_size);
	if (!dst_data) {
		_mmcam_dbg_err("failed to alloc dst_data. size %d", dst_size);
		return FALSE;
	}

	loop_length = width << 1;
	dst_u_offset = width * height;
	dst_v_offset = dst_u_offset + (dst_u_offset >> 2);

	_mmcam_dbg_log("offset y %d, u %d, v %d", dst_y_offset, dst_u_offset, dst_v_offset);

	for (i = 0 ; i < height ; i++) {
		for (j = 0 ; j < loop_length ; j += 2) {
			dst_data[dst_y_offset++] = src[src_offset++]; /*Y*/

			if (i % 2 == 0) {
				if (j % 4 == 0)
					dst_data[dst_u_offset++] = src[src_offset++]; /*U*/
				else
					dst_data[dst_v_offset++] = src[src_offset++]; /*V*/
			} else {
				src_offset++;
			}
		}
	}

	*dst = dst_data;
	*dst_len = dst_size;

	_mmcam_dbg_log("DONE: YUVY -> I420 : %dx%d, dst data %p, size %d", width, height, *dst, dst_size);

	return TRUE;
}


static gboolean _mmcamcorder_convert_UYVY_to_I420(unsigned char *src, guint width, guint height, unsigned char **dst, unsigned int *dst_len)
{
	unsigned int i = 0;
	int j = 0;
	int src_offset = 0;
	int dst_y_offset = 0;
	int dst_u_offset = 0;
	int dst_v_offset = 0;
	int loop_length = 0;
	unsigned int dst_size = 0;
	unsigned char *dst_data = NULL;

	if (!src || !dst || !dst_len) {
		_mmcam_dbg_err("NULL pointer %p, %p, %p", src, dst, dst_len);
		return FALSE;
	}

	dst_size = (width * height * 3) >> 1;

	_mmcam_dbg_log("UYVY -> I420 : %dx%d, dst size %d", width, height, dst_size);

	dst_data = (unsigned char *)malloc(dst_size);
	if (!dst_data) {
		_mmcam_dbg_err("failed to alloc dst_data. size %d", dst_size);
		return FALSE;
	}

	loop_length = width << 1;
	dst_u_offset = width * height;
	dst_v_offset = dst_u_offset + (dst_u_offset >> 2);

	_mmcam_dbg_log("offset y %d, u %d, v %d", dst_y_offset, dst_u_offset, dst_v_offset);

	for (i = 0 ; i < height ; i++) {
		for (j = 0 ; j < loop_length ; j += 2) {
			if (i % 2 == 0) {
				if (j % 4 == 0)
					dst_data[dst_u_offset++] = src[src_offset++]; /*U*/
				else
					dst_data[dst_v_offset++] = src[src_offset++]; /*V*/
			} else {
				src_offset++;
			}

			dst_data[dst_y_offset++] = src[src_offset++]; /*Y*/
		}
	}

	*dst = dst_data;
	*dst_len = dst_size;

	_mmcam_dbg_log("DONE: UYVY -> I420 : %dx%d, dst data %p, size %d", width, height, *dst, dst_size);

	return TRUE;
}


static gboolean _mmcamcorder_convert_NV12_to_I420(unsigned char *src, guint width, guint height, unsigned char **dst, unsigned int *dst_len)
{
	int i = 0;
	int src_offset = 0;
	int dst_y_offset = 0;
	int dst_u_offset = 0;
	int dst_v_offset = 0;
	int loop_length = 0;
	unsigned int dst_size = 0;
	unsigned char *dst_data = NULL;

	if (!src || !dst || !dst_len) {
		_mmcam_dbg_err("NULL pointer %p, %p, %p", src, dst, dst_len);
		return FALSE;
	}

	/* buffer overflow prevention check */
	if (width > __MMCAMCORDER_MAX_WIDTH || height > __MMCAMCORDER_MAX_HEIGHT) {
		_mmcam_dbg_err("too large size %d x %d", width, height);
		return FALSE;
	}

	dst_size = (width * height * 3) >> 1;

	_mmcam_dbg_log("NV12 -> I420 : %dx%d, dst size %d", width, height, dst_size);

	dst_data = (unsigned char *)malloc(dst_size);
	if (!dst_data) {
		_mmcam_dbg_err("failed to alloc dst_data. size %d", dst_size);
		return FALSE;
	}

	loop_length = width << 1;
	dst_u_offset = width * height;
	dst_v_offset = dst_u_offset + (dst_u_offset >> 2);

	_mmcam_dbg_log("offset y %d, u %d, v %d", dst_y_offset, dst_u_offset, dst_v_offset);

	/* memcpy Y */
	memcpy(dst_data, src, dst_u_offset);

	loop_length = dst_u_offset >> 1;
	src_offset = dst_u_offset;

	/* set U and V */
	for (i = 0 ; i < loop_length ; i++) {
		if (i % 2 == 0)
			dst_data[dst_u_offset++] = src[src_offset++];
		else
			dst_data[dst_v_offset++] = src[src_offset++];
	}

	*dst = dst_data;
	*dst_len = dst_size;

	_mmcam_dbg_log("DONE: NV12 -> I420 : %dx%d, dst data %p, size %d", width, height, *dst, dst_size);

	return TRUE;
}


void _mmcamcorder_emit_dbus_signal(GDBusConnection *conn, const char *object_name,
	const char *interface_name, const char *signal_name, int value)
{
	if (!conn || !object_name || !interface_name || !signal_name) {
		_mmcam_dbg_err("NULL pointer %p %p %p %p",
			conn, object_name, interface_name, signal_name);
		return;
	}

	if (!g_dbus_connection_emit_signal(conn, NULL,
		object_name, interface_name, signal_name,
		g_variant_new("(i)", value), NULL)) {
		_mmcam_dbg_warn("failed to emit signal");
	} else {
		_mmcam_dbg_log("emit signal done - value 0x%.8x", value);
		g_dbus_connection_flush(conn, NULL, NULL, NULL);
		_mmcam_dbg_log("signal flush done");
	}

	return;
}


int _mmcamcorder_get_audiosrc_blocksize(int samplerate, int format, int channel, int interval, int *blocksize)
{
	int depth = 8;

	if (!blocksize) {
		_mmcam_dbg_err("NULL ptr");
		return FALSE;
	}

	if (samplerate == 0 || channel == 0 || interval == 0) {
		_mmcam_dbg_err("invalid param %d %d %d", samplerate, channel, interval);
		return FALSE;
	}

	if (format == MM_CAMCORDER_AUDIO_FORMAT_PCM_S16_LE)
		depth = 16;

	*blocksize = samplerate * depth * channel * interval / 8000;

	return TRUE;
}
