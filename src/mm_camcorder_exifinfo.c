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

#include "mm_camcorder_exifinfo.h"
#include "mm_camcorder_exifdef.h"
#include "mm_camcorder_internal.h"

#include <libexif/exif-loader.h>
#include <libexif/exif-utils.h>
#include <libexif/exif-data.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <mm_debug.h>
#include <mm_error.h>

#define MM_EXIFINFO_USE_BINARY_EXIFDATA         1
#define JPEG_MAX_SIZE                           20000000
#define JPEG_THUMBNAIL_MAX_SIZE                 (128*1024)
#define JPEG_DATA_OFFSET                        2
#define JPEG_EXIF_OFFSET                        4
#define EXIF_MARKER_SOI_LENGTH                  2
#define EXIF_MARKER_APP1_LENGTH                 2
#define EXIF_APP1_LENGTH                        2


#if MM_EXIFINFO_USE_BINARY_EXIFDATA
/**
 * Exif Binary Data.
 */
#include <string.h>
#define _EXIF_BIN_SIZE_		((unsigned int)174)
unsigned char g_exif_bin[_EXIF_BIN_SIZE_] = {
	0x45 , 0x78 , 0x69 , 0x66 , 0x00 , 0x00 , 0x49 , 0x49 , 0x2a , 0x00 , 0x08 , 0x00 , 0x00 , 0x00 , 0x05 , 0x00 ,
	0x1a , 0x01 , 0x05 , 0x00 , 0x01 , 0x00 , 0x00 , 0x00 , 0x4a , 0x00 , 0x00 , 0x00 , 0x1b , 0x01 , 0x05 , 0x00 ,
	0x01 , 0x00 , 0x00 , 0x00 , 0x52 , 0x00 , 0x00 , 0x00 , 0x28 , 0x01 , 0x03 , 0x00 , 0x01 , 0x00 , 0x00 , 0x00 ,
	0x02 , 0x00 , 0x00 , 0x00 , 0x13 , 0x02 , 0x03 , 0x00 , 0x01 , 0x00 , 0x00 , 0x00 , 0x01 , 0x00 , 0x00 , 0x00 ,
	0x69 , 0x87 , 0x04 , 0x00 , 0x01 , 0x00 , 0x00 , 0x00 , 0x5a , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 ,
	0x48 , 0x00 , 0x00 , 0x00 , 0x01 , 0x00 , 0x00 , 0x00 , 0x48 , 0x00 , 0x00 , 0x00 , 0x01 , 0x00 , 0x00 , 0x00 ,
	0x06 , 0x00 , 0x00 , 0x90 , 0x07 , 0x00 , 0x04 , 0x00 , 0x00 , 0x00 , 0x30 , 0x32 , 0x31 , 0x30 , 0x01 , 0x91 ,
	0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0xa0 , 0x07 , 0x00 , 0x04 , 0x00 ,
	0x00 , 0x00 , 0x30 , 0x31 , 0x30 , 0x30 , 0x01 , 0xa0 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 ,
	0x00 , 0x00 , 0x02 , 0xa0 , 0x04 , 0x00 , 0x01 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x03 , 0xa0 ,
	0x04 , 0x00 , 0x01 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00
};
#endif

/**
 * Structure for exif entry.
 */
typedef struct  _mm_exif_entry_t {
	ExifTag			tag;			/**< exif tag*/
	ExifFormat		format;			/**< exif format*/
	unsigned long	components;		/**< number of components*/
	unsigned char	*data;			/**< data*/
	unsigned int	size;			/**< size of data*/
} mm_exif_entry_type;


/**
 * local functions.
 */
static void
_exif_set_uint16(int is_motorola, void * out, unsigned short in)
{
	if (is_motorola) {
		((unsigned char *)out)[0] = in & 0x00ff;
		((unsigned char *)out)[1] = in >> 8;
	} else {
		((unsigned char *)out)[0] = in >> 8;
		((unsigned char *)out)[1] = in & 0x00ff;
	}
}


#ifdef _MMCAMCORDER_EXIF_GET_JPEG_MARKER_OFFSET
static unsigned long
_exif_get_jpeg_marker_offset(void *jpeg, int jpeg_size, unsigned short marker)
{
	unsigned char *p = NULL;
	unsigned char *src = jpeg;
	int src_sz = jpeg_size;
	unsigned char m[2];
	unsigned long ret;
	int i;

	m[0] = marker >> 8;
	m[1] = marker & 0x00FF;

	_mmcam_dbg_log("marker: 0x%02X 0x%02X", m[0], m[1]);

	if (*src == 0xff && *(src + 1) == 0xd8) {
		p = src + 2; /* SOI(start of image) */
	} else {
		_mmcam_dbg_log("invalid JPEG file.");
		return 0UL;
	}

	for (i = 0; i < src_sz - (1 + 2); i++, p++) {
		if (*p == 0xff) {
			/*marker is 0xFFxx*/
			if (*(p + 1) == m[1]) {
				ret = p - src;
				_mmcam_dbg_log("marker offset: %lu %p %p.", ret, (p+1), src);
				return ret;
			}
		}
	}
	_mmcam_dbg_log("Marker not found.");
	return 0UL;
}
#endif /* _MMCAMCORDER_EXIF_GET_JPEG_MARKER_OFFSET */


ExifData*
mm_exif_get_exif_data_from_data(mm_exif_info_t *info)
{
	ExifData *ed = NULL;

	ed = exif_data_new_from_data(info->data, info->size);
	if (ed == NULL)
		_mmcam_dbg_log("Null exif data. (ed:%p)", ed);

	return ed;
}


ExifData*
mm_exif_get_exif_from_info(mm_exif_info_t *info)
{
	ExifData *ed = NULL;
	ExifLoader *loader = NULL;

	unsigned char size[2];
	unsigned int i;

	/*get ExifData from info*/
	loader = exif_loader_new();

	size[0] = (unsigned char) (info->size);
	size[1] = (unsigned char) (info->size >> 8);
	exif_loader_write(loader, size, 2);

	for (i = 0; i < info->size && exif_loader_write(loader, info->data + i, 1); i++);

	ed = exif_loader_get_data(loader);
	exif_loader_unref(loader);
	return ed;
}


int
mm_exif_set_exif_to_info(mm_exif_info_t *info, ExifData *exif)
{
	unsigned char *eb = NULL;
	unsigned int ebs;

	if (!exif) {
		_mmcam_dbg_log("exif Null");
		return MM_ERROR_CAMCORDER_NOT_INITIALIZED;
	}

	_mmcam_dbg_log("exif(ifd :%p)", exif->ifd);

	if (info->data) {
		free(info->data);
		info->data = NULL;
		info->size = 0;
	}

	exif_data_save_data(exif, &eb, &ebs);
	if (eb == NULL) {
		_mmcam_dbg_log("MM_ERROR_CAMCORDER_LOW_MEMORY");
		return MM_ERROR_CAMCORDER_LOW_MEMORY;
	}
	info->data = eb;
	info->size = ebs;
	return MM_ERROR_NONE;
}


int
mm_exif_set_add_entry(ExifData *exif, ExifIfd ifd, ExifTag tag, ExifFormat format, unsigned long components, const char* data)
{
	ExifData *ed = (ExifData *)exif;
	ExifEntry *e = NULL;

	if (exif == NULL || format <= 0 || components <= 0 || data == NULL) {
		_mmcam_dbg_err("invalid argument exif=%p format=%d, components=%lu data=%p!",
					   exif, format, components, data);
		return MM_ERROR_CAMCORDER_INVALID_ARGUMENT;
	}

	/*remove same tag in EXIF*/
	exif_content_remove_entry(ed->ifd[ifd], exif_content_get_entry(ed->ifd[ifd], tag));

	/*create new tag*/
	e = exif_entry_new();
	if (e == NULL) {
		_mmcam_dbg_err("entry create error!");
		return MM_ERROR_CAMCORDER_LOW_MEMORY;
	}

	exif_entry_initialize(e, tag);

	e->tag = tag;
	e->format = format;
	e->components = components;

	if (e->size == 0) {
		e->data = NULL;
		e->data = malloc(exif_format_get_size(format) * e->components);
		if (!e->data) {
			exif_entry_unref(e);
			return MM_ERROR_CAMCORDER_LOW_MEMORY;
		}

		if (format == EXIF_FORMAT_ASCII)
			memset(e->data, '\0', exif_format_get_size(format) * e->components);
	}

	e->size = exif_format_get_size(format) * e->components;
	memcpy(e->data, data, e->size);
	exif_content_add_entry(ed->ifd[ifd], e);
	exif_entry_unref(e);

	return MM_ERROR_NONE;
}




/**
 * global functions.
 */


int
mm_exif_create_exif_info(mm_exif_info_t **info)
{
	mm_exif_info_t *x = NULL;
#if (MM_EXIFINFO_USE_BINARY_EXIFDATA == 0)
	ExifData *ed = NULL;
	unsigned char *eb = NULL;
	unsigned int ebs;
#endif
	_mmcam_dbg_log("");

	if (!info) {
		_mmcam_dbg_err("NULL pointer");
		return MM_ERROR_CAMCORDER_INVALID_ARGUMENT;
	}

	x = malloc(sizeof(mm_exif_info_t));
	if (!x) {
		_mmcam_dbg_err("malloc error");
		return MM_ERROR_CAMCORDER_LOW_MEMORY;
	}
#if MM_EXIFINFO_USE_BINARY_EXIFDATA
	x->data = NULL;
	x->data = malloc(_EXIF_BIN_SIZE_);
	if (!x->data) {
		_mmcam_dbg_err("malloc error");
		free(x);
		return MM_ERROR_CAMCORDER_LOW_MEMORY;
	}
	memcpy(x->data, g_exif_bin, _EXIF_BIN_SIZE_);
	x->size = _EXIF_BIN_SIZE_;
#else
	ed = exif_data_new();
	if (!ed) {
		_mmcam_dbg_err("exif data new error");
		return MM_ERROR_CAMCORDER_LOW_MEMORY;
	}

	exif_data_set_byte_order(ed, EXIF_BYTE_ORDER_INTEL);
	exif_data_set_data_type(ed, EXIF_DATA_TYPE_COMPRESSED);
	exif_data_set_option(ed, EXIF_DATA_OPTION_FOLLOW_SPECIFICATION);

	exif_data_fix(ed);

	exif_data_save_data(ed, &eb, &ebs);
	if (eb == NULL) {
		_mmcam_dbg_err("exif_data_save_data error");
		free(x->data);
		free(x);
		exif_data_unref(ed);
		return MM_ERROR_CAMCORDER_INTERNAL;
	}
	exif_data_unref(ed);

	x->data = eb;
	x->size = ebs;
#endif
	*info = x;

	_mmcam_dbg_log("Data:%p Size:%d", x->data, x->size);

	return MM_ERROR_NONE;
}

void
mm_exif_destory_exif_info(mm_exif_info_t *info)
{
	/* _mmcam_dbg_log( ""); */

#if MM_EXIFINFO_USE_BINARY_EXIFDATA
	if (info) {
		if (info->data)
			free(info->data);
		free(info);
	}
#else
	if (info) {
		if (info->data)
			exif_mem_free(info->data);
		free(info);
	}
#endif
}


int
mm_exif_add_thumbnail_info(mm_exif_info_t *info, void *thumbnail, int width, int height, int len)
{
	ExifData *ed = NULL;
	static ExifLong elong[10];

	unsigned char *p_compressed = NULL;
	int ret = MM_ERROR_NONE;
	int cntl = 0;

	_mmcam_dbg_log("Thumbnail size:%d, width:%d, height:%d", len, width, height);

	if (len > JPEG_THUMBNAIL_MAX_SIZE) {
		_mmcam_dbg_err("Thumbnail size[%d] over!!! Skip inserting thumbnail...", len);
		return MM_ERROR_NONE;
	}

	/* get ExifData from info*/
	ed = mm_exif_get_exif_from_info(info);
	if (ed == NULL) {
		_mmcam_dbg_err("mm_exif_get_exif_from_info failed");
		return MM_ERROR_CAMCORDER_INTERNAL;
	}

	ed->data = thumbnail;
	ed->size = len;

	/* set thumbnail data */
	p_compressed = (unsigned char *)malloc(sizeof(ExifShort));
	if (p_compressed != NULL) {
		exif_set_short(p_compressed, exif_data_get_byte_order(ed), 6);
		ret = mm_exif_set_add_entry(ed, EXIF_IFD_1, EXIF_TAG_COMPRESSION, EXIF_FORMAT_SHORT, 1, (const char *)p_compressed);
		if (ret != MM_ERROR_NONE)
			goto exit;
	} else {
		ret = MM_ERROR_CAMCORDER_LOW_MEMORY;
		goto exit;
	}

	/* set thumbnail size */
	exif_set_long((unsigned char *)&elong[cntl], exif_data_get_byte_order(ed), width);
	ret = mm_exif_set_add_entry(ed, EXIF_IFD_1, EXIF_TAG_IMAGE_WIDTH, EXIF_FORMAT_LONG, 1, (const char *)&elong[cntl++]);
	if (ret != MM_ERROR_NONE)
		goto exit;

	exif_set_long((unsigned char *)&elong[cntl], exif_data_get_byte_order(ed), height);
	ret = mm_exif_set_add_entry(ed, EXIF_IFD_1, EXIF_TAG_IMAGE_LENGTH, EXIF_FORMAT_LONG, 1, (const char *)&elong[cntl++]);
	if (ret != MM_ERROR_NONE)
		goto exit;

	ret = mm_exif_set_exif_to_info(info, ed);
	if (ret != MM_ERROR_NONE)
		goto exit;

	ed->data = NULL;
	ed->size = 0;
	exif_data_unref(ed);

exit:
	if (p_compressed != NULL)
		free(p_compressed);

	return ret;
}


int
mm_exif_write_exif_jpeg_to_memory(void **mem, unsigned int *length, mm_exif_info_t *info,  unsigned char *jpeg, unsigned int jpeg_len)
{
	unsigned short head[2] = {0,};
	unsigned short head_len = 0;
	unsigned char *eb = NULL;
	unsigned int ebs;
	int jpeg_offset = JPEG_DATA_OFFSET;
	mm_exif_info_t *test_exif_info = NULL;

	/*output*/
	unsigned char *m = NULL;
	int m_len = 0;

	_mmcam_dbg_log("");

	if (info == NULL || jpeg == NULL) {
		_mmcam_dbg_err("MM_ERROR_CAMCORDER_INVALID_ARGUMENT info=%p, jpeg=%p", info, jpeg);
		return MM_ERROR_CAMCORDER_INVALID_ARGUMENT;
	}

	if (jpeg_len > JPEG_MAX_SIZE) {
		_mmcam_dbg_err("jpeg_len is worng jpeg_len=%d", jpeg_len);
		return MM_ERROR_CAMCORDER_DEVICE_WRONG_JPEG;
	}

	eb = info->data;
	ebs = info->size;

	/* check EXIF in JPEG */
	if (mm_exif_load_exif_info(&test_exif_info, jpeg, jpeg_len) == MM_ERROR_NONE) {
		if (test_exif_info) {
			jpeg_offset = test_exif_info->size + JPEG_EXIF_OFFSET;
			if (test_exif_info->data) {
				free(test_exif_info->data);
				test_exif_info->data = NULL;
			}
			free(test_exif_info);
			test_exif_info = NULL;
		} else {
			_mmcam_dbg_err("test_exif_info is NULL");
		}
	} else {
		_mmcam_dbg_warn("no EXIF in JPEG");
	}

	/*length of output image*/
	/*SOI + APP1 + length of APP1 + length of EXIF + IMAGE*/
	m_len = EXIF_MARKER_SOI_LENGTH + EXIF_MARKER_APP1_LENGTH + EXIF_APP1_LENGTH + ebs + (jpeg_len - jpeg_offset);
	/*alloc output image*/
	m = malloc(m_len);
	if (!m) {
		_mmcam_dbg_err("malloc() failed.");
		return MM_ERROR_CAMCORDER_LOW_MEMORY;
	}

	/*set SOI, APP1*/
	_exif_set_uint16(0, &head[0], 0xffd8);
	_exif_set_uint16(0, &head[1], 0xffe1);
	/*set header length*/
	_exif_set_uint16(0, &head_len, (unsigned short)(ebs + 2));
	if (head[0] == 0 || head[1] == 0 || head_len == 0) {
		_mmcam_dbg_err("setting error");
		free(m);
		return MM_ERROR_CAMCORDER_INVALID_ARGUMENT;
	}

	/* Complete JPEG+EXIF */
	/*SOI marker*/
	memcpy(m, &head[0], EXIF_MARKER_SOI_LENGTH);

	/*APP1 marker*/
	memcpy(m + EXIF_MARKER_SOI_LENGTH, &head[1], EXIF_MARKER_APP1_LENGTH);

	/*length of APP1*/
	memcpy(m + EXIF_MARKER_SOI_LENGTH + EXIF_MARKER_APP1_LENGTH, &head_len, EXIF_APP1_LENGTH);

	/*EXIF*/
	memcpy(m + EXIF_MARKER_SOI_LENGTH + EXIF_MARKER_APP1_LENGTH + EXIF_APP1_LENGTH, eb, ebs);

	/*IMAGE*/
	memcpy(m + EXIF_MARKER_SOI_LENGTH + EXIF_MARKER_APP1_LENGTH + EXIF_APP1_LENGTH + ebs,
		jpeg + jpeg_offset, jpeg_len - jpeg_offset);

	_mmcam_dbg_log("JPEG+EXIF Copy DONE(original:%d, offset:%d, copied:%d)",
		jpeg_len, jpeg_offset, jpeg_len - jpeg_offset);

	/*set ouput param*/
	*mem = m;
	*length = m_len;

	return MM_ERROR_NONE;
}


int mm_exif_load_exif_info(mm_exif_info_t **info, void *jpeg_data, int jpeg_length)
{
	ExifLoader *loader = NULL;
	const unsigned char *b = NULL;
	unsigned int s = 0;
	mm_exif_info_t *x = NULL;

	/* TODO : get exif and re-set exif */

	loader = exif_loader_new();
	if (loader) {
		exif_loader_write(loader, jpeg_data, jpeg_length);
		exif_loader_get_buf(loader, &b, &s);
		if (s > 0) {
			x = malloc(sizeof(mm_exif_info_t));
			if (x) {
				x->data = malloc(s);
				if (x->data) {
					memcpy((char*)x->data, b, s);
					x->size = s;
					*info = x;
					_mmcam_dbg_warn("load EXIF : data %p, size %d", x->data, x->size);
				} else {
					_mmcam_dbg_err("mm_exif_info_t malloc failed");
					free(x);
					exif_loader_unref(loader);

					return MM_ERROR_CAMCORDER_LOW_MEMORY;
				}
			} else {
				_mmcam_dbg_err("mm_exif_info_t malloc failed");
			}
		} else {
			_mmcam_dbg_err("exif_loader_get_buf failed");
		}

		/* The loader is no longer needed--free it */
		exif_loader_unref(loader);
		loader = NULL;
	} else {
		_mmcam_dbg_err("exif_loader_new failed");
	}

	if (x)
		return MM_ERROR_NONE;
	else
		return MM_ERROR_CAMCORDER_INTERNAL;
}
