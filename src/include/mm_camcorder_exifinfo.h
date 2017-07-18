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

#ifndef _MM_EXIFINFO_H_
#define _MM_EXIFINFO_H_

/*=======================================================================================
| INCLUDE FILES										|
========================================================================================*/
#include <libexif/exif-ifd.h>
#include <libexif/exif-tag.h>
#include <libexif/exif-format.h>
#include <libexif/exif-data.h>

#ifdef __cplusplus
extern "C" {
#endif

/*=======================================================================================
| STRUCTURE DEFINITIONS									|
========================================================================================*/
/**
 * Structure for exif data infomation.
 */
typedef struct {
	void *data;			/**< saved exif data*/
	unsigned int size;		/**< size of saved exif data*/
} mm_exif_info_t;

/*=======================================================================================
| GLOBAL FUNCTION PROTOTYPES								|
========================================================================================*/
/**
 * Create exif info
 *  @param[in] info exif info.
 * @return return int.
 */
int mm_exif_create_exif_info(mm_exif_info_t **info);

/**
 * Destroy exif info
 *  @param[in] info exif info.
 *  @return void
 */
void mm_exif_destory_exif_info(mm_exif_info_t *info);

/**
 * get exif data information from data buffer
 *  @param[in] info exif info.
 *  @return return ExifData.
 */
ExifData *mm_exif_get_exif_data_from_data(mm_exif_info_t *info);

/**
 * get exif data information from exif info
 *  @param[in] info exif info.
 *  @param[out] Exif tag .
 *  @return return int.
 */
ExifData *mm_exif_get_exif_from_info(mm_exif_info_t *info);

/**
 * Write exif data information into exif info
 *  @param[in] info exif info.
 *  @param[in] Exif tag .
 *  @param[in] Exif tag value to be written.
 *  @return return int.
 */
int mm_exif_set_exif_to_info(mm_exif_info_t *info, ExifData *exif);

/**
 * add one tag information into exif
 *  @param[in] info exif info.
 *  @param[in] Exif tag .
 *  @param[in] tag content category.
 *  @param[in] tag format.
 *  @param[in] the number of the component .
 *  @param[in] the pointer of the tag data.
 *  @return return int.
 */
int mm_exif_set_add_entry(ExifData *exif, ExifIfd ifd, ExifTag tag,
			  ExifFormat format, unsigned long components,
			  const char *data);

/**
 * Write thumbnail data into exif info.
 * @param[in] info exif info.
 * @param[in] thumbnail image thumbnail data.
 * @param[in] width width of thumbnail image.
 * @param[in] height height of thumbnail image.
 * @param[in] length length of thumbnail image.
 * @return return int.
 */
int mm_exif_add_thumbnail_info(mm_exif_info_t *info, void *thumbnail,
			       int width, int height, int len);

/**
 * Write exif info into a jpeg memory buffer.
 * @param[in/out] mem image data buffer.
 * @param[in/out] length image data length.
 * @param[in] info exif info.
 * @param[in] jpeg jpeg image data.
 * @param[in] length length of jpeg image.
 * @return return int.
 */
int mm_exif_write_exif_jpeg_to_memory(void **mem, unsigned int *length,
				      mm_exif_info_t *info, void *jpeg,
				      unsigned int jpeg_len);

/**
 * Load exif info from a jpeg memory buffer.
 * @param[out] info exif info.
 * @param[in] jpeg jpeg image data.
 * @param[in] length length of jpeg image.
 * @return return int.
 */
int mm_exif_load_exif_info(mm_exif_info_t **info, void *jpeg_data, int jpeg_length);

#ifdef __cplusplus
}
#endif
#endif	/*_MM_EXIFINFO_H_*/
