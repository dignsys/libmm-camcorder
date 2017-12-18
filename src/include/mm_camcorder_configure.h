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

#ifndef __MM_CAMCORDER_CONFIGURE_H__
#define __MM_CAMCORDER_CONFIGURE_H__

#ifdef __cplusplus
extern "C" {
#endif

/*=======================================================================================
| MACRO DEFINITIONS									|
========================================================================================*/
#define SAFE_FREE(x) \
do { \
	if (x) {\
		free(x); \
		x = NULL; \
	} \
} while (0)

#define SAFE_G_FREE(x) \
do { \
	if (x) {\
		g_free(x); \
		x = NULL; \
	} \
} while (0)

#define CONFIGURE_MAIN_FILE             "mmfw_camcorder.ini"
#define CONFIGURE_CTRL_FILE_PREFIX      "mmfw_camcorder_camera"

/*=======================================================================================
| ENUM DEFINITIONS									|
========================================================================================*/
enum ConfigureType {
	CONFIGURE_TYPE_MAIN,
	CONFIGURE_TYPE_CTRL,
};

enum ConfigureValueType {
	CONFIGURE_VALUE_INT,
	CONFIGURE_VALUE_INT_RANGE,
	CONFIGURE_VALUE_INT_ARRAY,
	CONFIGURE_VALUE_INT_PAIR_ARRAY,
	CONFIGURE_VALUE_STRING,
	CONFIGURE_VALUE_STRING_ARRAY,
	CONFIGURE_VALUE_ELEMENT,
	CONFIGURE_VALUE_NUM,
};

enum ConfigureCategoryMain {
	CONFIGURE_CATEGORY_MAIN_GENERAL,
	CONFIGURE_CATEGORY_MAIN_VIDEO_INPUT,
	CONFIGURE_CATEGORY_MAIN_AUDIO_INPUT,
	CONFIGURE_CATEGORY_MAIN_VIDEO_OUTPUT,
	CONFIGURE_CATEGORY_MAIN_CAPTURE,
	CONFIGURE_CATEGORY_MAIN_RECORD,
	CONFIGURE_CATEGORY_MAIN_VIDEO_ENCODER,
	CONFIGURE_CATEGORY_MAIN_AUDIO_ENCODER,
	CONFIGURE_CATEGORY_MAIN_IMAGE_ENCODER,
	CONFIGURE_CATEGORY_MAIN_MUX,
	CONFIGURE_CATEGORY_MAIN_NUM,
};

enum ConfigureCategoryCtrl {
	CONFIGURE_CATEGORY_CTRL_CAMERA,
	CONFIGURE_CATEGORY_CTRL_STROBE,
	CONFIGURE_CATEGORY_CTRL_EFFECT,
	CONFIGURE_CATEGORY_CTRL_PHOTOGRAPH,
	CONFIGURE_CATEGORY_CTRL_CAPTURE,
	CONFIGURE_CATEGORY_CTRL_DETECT,
	CONFIGURE_CATEGORY_CTRL_NUM,
};

/*=======================================================================================
| STRUCTURE DEFINITIONS									|
========================================================================================*/
typedef struct _type_int type_int;
struct _type_int {
	const char *name;
	int value;
};

typedef struct _type_int2 type_int2;
struct _type_int2 {
	char *name;
	int value;
};

typedef struct _type_int_range type_int_range;
struct _type_int_range {
	char *name;
	int min;
	int max;
	int default_value;
};

typedef struct _type_int_array type_int_array;
struct _type_int_array {
	char *name;
	int *value;
	int count;
	int default_value;
};

typedef struct _type_int_pair_array type_int_pair_array;
struct _type_int_pair_array {
	char *name;
	int *value[2];
	int count;
	int default_value[2];
};

typedef struct _type_string type_string;
struct _type_string {
	const char *name;
	const char *value;
};

typedef struct _type_string2 type_string2;
struct _type_string2 {
	char *name;
	char *value;
};

typedef struct _type_string_array type_string_array;
struct _type_string_array {
	char *name;
	char **value;
	int count;
	char *default_value;
};

typedef struct _type_element type_element;
struct _type_element {
	const char *name;
	const char *element_name;
	type_int **value_int;
	int count_int;
	type_string **value_string;
	int count_string;
};

typedef struct _type_element2 type_element2;
struct _type_element2 {
	char *name;
	char *element_name;
	type_int2 **value_int;
	int count_int;
	type_string2 **value_string;
	int count_string;
};

typedef struct _conf_detail conf_detail;
struct _conf_detail {
	int count;
	void **detail_info;
};

typedef struct _conf_info_table conf_info_table;
struct _conf_info_table {
	const char *name;
	int value_type;
	union {
		type_element *value_element;
		int value_int;
		char *value_string;
	};
};

typedef struct _camera_conf camera_conf;
struct _camera_conf {
	int type;
	conf_detail **info;
};

/*=======================================================================================
| MODULE FUNCTION PROTOTYPES								|
========================================================================================*/
/* User function */
/**
 * This function creates configure info structure from ini file.
 *
 * @param[in]	handle		Handle of camcorder.
 * @param[in]	type		configure type(MAIN or CTRL).
 * @param[in]	ConfFile	ini file path.
 * @param[out]	configure_info	configure structure to be got.
 * @return	This function returns MM_ERROR_NONE on success, or others on failure.
 * @remarks
 * @see		_mmcamcorder_conf_release_info()
 *
 */
int _mmcamcorder_conf_get_info(MMHandleType handle, int type, const char *ConfFile, camera_conf **configure_info);

/**
 * This function releases configure info.
 *
 * @param[in]	handle		Handle of camcorder.
 * @param[in]	configure_info	configure structure to be released.
 * @return	void
 * @remarks
 * @see		_mmcamcorder_conf_get_info()
 *
 */
void _mmcamcorder_conf_release_info(MMHandleType handle, camera_conf **configure_info);

/**
 * This function gets integer type value from configure info.
 *
 * @param[in]	handle		Handle of camcorder.
 * @param[in]	configure_info	configure structure created by _mmcamcorder_conf_get_info.
 * @param[in]	category	configure category.
 * @param[in]	name		detail name in category.
 * @param[out]	value		value to be got.
 * @return	This function returns TRUE on success, or FALSE on failure.
 * @remarks
 * @see
 *
 */
int _mmcamcorder_conf_get_value_int(MMHandleType handle, camera_conf *configure_info, int category, const char *name, int *value);

/**
 * This function gets integer-range type value from configure info.
 *
 * @param[in]	configure_info	configure structure created by _mmcamcorder_conf_get_info.
 * @param[in]	category	configure category.
 * @param[in]	name		detail name in category.
 * @param[out]	value		value to be got.
 * @return	This function returns TRUE on success, or FALSE on failure.
 * @remarks
 * @see
 *
 */
int _mmcamcorder_conf_get_value_int_range(camera_conf *configure_info, int category, const char *name, type_int_range **value);

/**
 * This function gets integer-array type value from configure info.
 *
 * @param[in]	configure_info	configure structure created by _mmcamcorder_conf_get_info.
 * @param[in]	category	configure category.
 * @param[in]	name		detail name in category.
 * @param[out]	value		value to be got.
 * @return	This function returns TRUE on success, or FALSE on failure.
 * @remarks
 * @see
 *
 */
int _mmcamcorder_conf_get_value_int_array(camera_conf *configure_info, int category, const char *name, type_int_array **value);

/**
 * This function gets integer-pair-array type value from configure info.
 *
 * @param[in]	configure_info	configure structure created by _mmcamcorder_conf_get_info.
 * @param[in]	category	configure category.
 * @param[in]	name		detail name in category.
 * @param[out]	value		value to be got.
 * @return	This function returns TRUE on success, or FALSE on failure.
 * @remarks
 * @see
 *
 */
int _mmcamcorder_conf_get_value_int_pair_array(camera_conf *configure_info, int category, const char *name, type_int_pair_array **value);

/**
 * This function gets string type value from configure info.
 *
 * @param[in]	handle		Handle of camcorder.
 * @param[in]	configure_info	configure structure created by _mmcamcorder_conf_get_info.
 * @param[in]	category	configure category.
 * @param[in]	name		detail name in category.
 * @param[out]	value		value to be got.
 * @return	This function returns TRUE on success, or FALSE on failure.
 * @remarks
 * @see
 *
 */
int _mmcamcorder_conf_get_value_string(MMHandleType handle, camera_conf *configure_info, int category, const char *name, const char **value);

/**
 * This function gets string-array type value from configure info.
 *
 * @param[in]	configure_info	configure structure created by _mmcamcorder_conf_get_info.
 * @param[in]	category	configure category.
 * @param[in]	name		detail name in category.
 * @param[out]	value		value to be got.
 * @return	This function returns TRUE on success, or FALSE on failure.
 * @remarks
 * @see
 *
 */
int _mmcamcorder_conf_get_value_string_array(camera_conf *configure_info, int category, const char *name, type_string_array **value);

/**
 * This function gets element info from configure info.
 *
 * @param[in]	handle		Handle of camcorder.
 * @param[in]	configure_info	configure structure created by _mmcamcorder_conf_get_info.
 * @param[in]	category	configure category.
 * @param[in]	name		detail name in category.
 * @param[out]	element		element info to be got.
 * @return	This function returns TRUE on success, or FALSE on failure.
 * @remarks
 * @see
 *
 */
int _mmcamcorder_conf_get_element(MMHandleType handle, camera_conf *configure_info, int category, const char *name, type_element **element);

/**
 * This function gets element name from element info.
 *
 * @param[in]	element		element info.
 * @param[out]	name		element name to be got.
 * @return	This function returns TRUE on success, or FALSE on failure.
 * @remarks
 * @see
 *
 */
int _mmcamcorder_conf_get_value_element_name(type_element *element, const char **value);

/**
 * This function gets integer value of element's named property from element info.
 *
 * @param[in]	element		element info.
 * @param[in]	name		property name.
 * @param[out]	value		value to be got.
 * @return	This function returns TRUE on success, or FALSE on failure.
 * @remarks
 * @see
 *
 */
int _mmcamcorder_conf_get_value_element_int(type_element *element, const char *name, int *value);

/**
 * This function gets string value of element's named property from element info.
 *
 * @param[in]	element		element info.
 * @param[in]	name		property name.
 * @param[out]	value		value to be got.
 * @return	This function returns TRUE on success, or FALSE on failure.
 * @remarks
 * @see
 *
 */
int _mmcamcorder_conf_get_value_element_string(type_element *element, const char *name, const char **value);

/**
 * This function sets all property of element info.
 *
 * @param[in]	gst		gstreamer element.
 * @param[in]	element		element info.
 * @return	This function returns TRUE on success, or FALSE on failure.
 * @remarks
 * @see
 *
 */
int _mmcamcorder_conf_set_value_element_property(GstElement *gst, type_element *element);

/**
 * This function prints all values of configure info.
 *
 * @param[in]	configure_info	configure structure created by _mmcamcorder_conf_get_info.
 * @return	This function returns TRUE on success, or FALSE on failure.
 * @remarks
 * @see
 *
 */
void _mmcamcorder_conf_print_info(MMHandleType handle, camera_conf **configure_info);

type_element *_mmcamcorder_get_type_element(MMHandleType handle, int type);
int _mmcamcorder_get_available_format(MMHandleType handle, int conf_category, int **format);

/* Internal function */
int _mmcamcorder_conf_init(MMHandleType handle, int type, camera_conf *configure_info);
int _mmcamcorder_conf_parse_info(MMHandleType handle, int type, FILE *fd, camera_conf **configure_info);
int _mmcamcorder_conf_get_value_type(MMHandleType handle, int type, int category, const char *name, int *value_type);
int _mmcamcorder_conf_add_info(MMHandleType handle, int type, conf_detail **info, char **buffer_details, int category, int count_details);
int _mmcamcorder_conf_get_default_value_int(MMHandleType handle, int type, int category, const char *name, int *value);
int _mmcamcorder_conf_get_default_value_string(MMHandleType handle, int type, int category, const char *name, const char **value);
int _mmcamcorder_conf_get_default_element(MMHandleType handle, int type, int category, const char *name, type_element **element);
int _mmcamcorder_conf_get_category_size(MMHandleType handle, int type, int category, int *size);

#ifdef __cplusplus
}
#endif
#endif /* __MM_CAMCORDER_CONFIGURE_H__ */
