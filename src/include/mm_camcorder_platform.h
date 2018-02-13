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

#ifndef __MM_CAMCORDER_PLATFORM_H__
#define __MM_CAMCORDER_PLATFORM_H__

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
#define MM_CAMCORDER_ATTR_NONE -1


/*=======================================================================================
| MACRO DEFINITIONS									|
========================================================================================*/
/* CAMERA DEFINED VALUE */
/* WQSXGA (5M) */
#define MMF_CAM_W2560		2560
#define MMF_CAM_H1920		1920

/* QXGA (3M) */
#define MMF_CAM_W2048		2048
#define MMF_CAM_H1536		1536

/* UXGA (2M) */
#define MMF_CAM_W1600		1600
#define MMF_CAM_H1200		1200

/* WSXGA (1M) */
#define MMF_CAM_W1280		1280
#define MMF_CAM_H960		960

/* SVGA */
#define MMF_CAM_W800		800
#define MMF_CAM_H600		600

/* WVGA */
#define MMF_CAM_W800		800
#define MMF_CAM_H480		480

/* VGA */
#define MMF_CAM_W640		640
#define MMF_CAM_H480		480

/* CIF */
#define MMF_CAM_W352		352
#define MMF_CAM_H288		288

/* QVGA */
#define MMF_CAM_W320		320
#define MMF_CAM_H240		240

/* QCIF */
#define MMF_CAM_W176		176
#define MMF_CAM_H144		144

/* QQVGA */
#define MMF_CAM_W160		160
#define MMF_CAM_H120		120

/* QQCIF */
#define MMF_CAM_W88		88
#define MMF_CAM_H72		72

/* WQVGA */
#define MMF_CAM_W400		400
#define MMF_CAM_H240		240

/* RQVGA */
#define MMF_CAM_W240		240
#define MMF_CAM_H320		320

/* RWQVGA */
#define MMF_CAM_W240		240
#define MMF_CAM_H400		400

/* Non-specified */
#define MMF_CAM_W400		400
#define MMF_CAM_H300		300

/* HD */
#define MMF_CAM_W1280		1280
#define MMF_CAM_H720		720

//Zero
#define MMF_CAM_W0		0
#define MMF_CAM_H0		0


/* Capture related */
/* High quality resolution */
#define MMFCAMCORDER_HIGHQUALITY_WIDTH		MMF_CAM_W0	/* High quality resolution is not needed, */
#define MMFCAMCORDER_HIGHQUALITY_HEIGHT		MMF_CAM_H0	/* because camsensor has a JPEG encoder inside */

/* VGA (1 : 0.75) */
#define MMF_CROP_VGA_LEFT			0
#define MMF_CROP_VGA_RIGHT			0
#define MMF_CROP_VGA_TOP			0
#define MMF_CROP_VGA_BOTTOM			0

/* QCIF (1 : 0.818) */
#define MMF_CROP_CIF_LEFT			68		/* little bit confusing */
#define MMF_CROP_CIF_RIGHT			68
#define MMF_CROP_CIF_TOP			0
#define MMF_CROP_CIF_BOTTOM			0

/* Camera etc */
#define _MMCAMCORDER_MINIMUM_SPACE		(512*1024)      /* byte */
#define _MMCAMCORDER_MMS_MARGIN_SPACE		(512)           /* byte */

/**
 * Default None value for camera sensor enumeration.
 */
#define _MMCAMCORDER_SENSOR_ENUM_NONE	-255

/* camera information related */
#define CAMINFO_CONVERT_NUM		45


/*=======================================================================================
| ENUM DEFINITIONS									|
========================================================================================*/
/**
 * Enumerations for attribute converting.
 */
typedef enum {
	MM_CAMCONVERT_TYPE_INT,
	MM_CAMCONVERT_TYPE_INT_RANGE,
	MM_CAMCONVERT_TYPE_INT_ARRAY,
	MM_CAMCONVERT_TYPE_INT_PAIR_ARRAY,
	MM_CAMCONVERT_TYPE_STRING,
/*
	MM_CAMCONVERT_TYPE_DOUBLE,
	MM_CAMCONVERT_TYPE_DOUBLE_PAIR,
*/
	MM_CAMCONVERT_TYPE_USER,	/* user define */
} MMCamConvertingType;

typedef enum {
	ENUM_CONVERT_WHITE_BALANCE = 0,
	ENUM_CONVERT_COLOR_TONE,
	ENUM_CONVERT_ISO,
	ENUM_CONVERT_PROGRAM_MODE,
	ENUM_CONVERT_FOCUS_MODE,
	ENUM_CONVERT_AF_RANGE,
	ENUM_CONVERT_EXPOSURE_MODE,
	ENUM_CONVERT_STROBE_MODE,
	ENUM_CONVERT_WDR,
	ENUM_CONVERT_FLIP,
	ENUM_CONVERT_ROTATION,
	ENUM_CONVERT_ANTI_HAND_SHAKE,
	ENUM_CONVERT_VIDEO_STABILIZATION,
	ENUM_CONVERT_NUM
} MMCamConvertingEnum;

typedef enum {
	MM_CAMCONVERT_CATEGORY_CAMERA  = 1 << 0,
	MM_CAMCONVERT_CATEGORY_DISPLAY = 1 << 1,
	MM_CAMCONVERT_CATEGORY_AUDIO   = 1 << 2,
	MM_CAMCONVERT_CATEGORY_ALL = MM_CAMCONVERT_CATEGORY_CAMERA | MM_CAMCONVERT_CATEGORY_DISPLAY | MM_CAMCONVERT_CATEGORY_AUDIO
} MMCamConvertingCategory;

/*=======================================================================================
| STRUCTURE DEFINITIONS									|
========================================================================================*/
/**
 * Structure for enumeration converting.
 */
typedef struct {
	int total_enum_num;		/**< total enumeration count */
	int *enum_arr;			/**< enumeration array */
	int category;			/**< category */
	const char *keyword;			/**< keyword array */
} _MMCamcorderEnumConvert;


/**
 * Converting table of camera configuration.
 */
typedef struct {
	int type;				/**< type of configuration */
	int category;				/**< category of configuration */
	int attr_idx;				/**< attribute index */
	int attr_idx_pair;			/**< attribute index (only for 'pair' type) */
	const char *keyword;
	MMCamConvertingType conv_type;
	_MMCamcorderEnumConvert *enum_convert;	/**< converting value table */
} _MMCamcorderInfoConverting;

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
int _mmcamcorder_convert_msl_to_sensor(MMHandleType handle, int attr_idx, int mslval);
int _mmcamcorder_convert_sensor_to_msl(MMHandleType handle, int attr_idx, int sensval);
int _mmcamcorder_get_fps_array_by_resolution(MMHandleType handle, int width, int height,  MMCamAttrsInfo* fps_info);

int _mmcamcorder_set_converted_value(MMHandleType handle, _MMCamcorderEnumConvert *convert);
int _mmcamcorder_init_convert_table(MMHandleType handle);
int _mmcamcorder_init_attr_from_configure(MMHandleType handle, MMCamConvertingCategory category);

int _mmcamcorder_convert_brightness(int mslVal);
int _mmcamcorder_convert_whitebalance(int mslVal);
int _mmcamcorder_convert_colortone(int mslVal);
double _mmcamcorder_convert_volume(int mslVal);
int  __mmcamcorder_set_info_to_attr(MMHandleType handle, _MMCamcorderInfoConverting *info, int table_size);

#ifdef __cplusplus
}
#endif

#endif /* __MM_CAMCORDER_PLATFORM_H__ */
