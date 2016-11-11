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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "mm_camcorder_internal.h"
#include "mm_camcorder_configure.h"

/*-----------------------------------------------------------------------
|    MACRO DEFINITIONS:							|
-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
|    GLOBAL VARIABLE DEFINITIONS					|
-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
|    LOCAL VARIABLE DEFINITIONS						|
-----------------------------------------------------------------------*/
#define DEFAULT_AUDIO_BUFFER_INTERVAL   50


char *get_new_string(char* src_string)
{
	return g_strdup(src_string);
}

void _mmcamcorder_conf_init(MMHandleType handle, int type, camera_conf** configure_info)
{
	int i = 0;
	int info_table_size = sizeof(conf_info_table);

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	/* Videosrc element default value */
	static type_element _videosrc_element_default = {
		"VideosrcElement",
		"videotestsrc",
		NULL,
		0,
		NULL,
		0,
	};

	/* Audiosrc element default value */
	static type_int  ___audiosrc_default_channel = {"Channel", 1};
	static type_int  ___audiosrc_default_bitrate = {"BitRate", 8000};
	static type_int  ___audiosrc_default_depth = {"Depth", 16};
	static type_int  ___audiosrc_default_blocksize = {"BlockSize", 1280};
	static type_int* __audiosrc_default_int_array[] = {
		&___audiosrc_default_channel,
		&___audiosrc_default_bitrate,
		&___audiosrc_default_depth,
		&___audiosrc_default_blocksize,
	};
	static type_string  ___audiosrc_default_device = {"Device", "default"};
	static type_string* __audiosrc_default_string_array[] = {
		&___audiosrc_default_device,
	};
	static type_element _audiosrc_element_default = {
		"AudiosrcElement",
		"audiotestsrc",
		__audiosrc_default_int_array,
		sizeof(__audiosrc_default_int_array) / sizeof(type_int*),
		__audiosrc_default_string_array,
		sizeof(__audiosrc_default_string_array) / sizeof(type_string*),
	};

	static type_element _audiomodemsrc_element_default = {
		"AudiomodemsrcElement",
		"audiotestsrc",
		__audiosrc_default_int_array,
		sizeof(__audiosrc_default_int_array) / sizeof(type_int*),
		__audiosrc_default_string_array,
		sizeof(__audiosrc_default_string_array) / sizeof(type_string*),
	};


	/* Videosink element default value */
	static type_int  ___videosink_default_display_id = {"display-id", 3};
	static type_int  ___videosink_default_x = {"x", 0};
	static type_int  ___videosink_default_y = {"y", 0};
	static type_int  ___videosink_default_width = {"width", 800};
	static type_int  ___videosink_default_height = {"height", 480};
	static type_int  ___videosink_default_rotation = {"rotation", 1};
	static type_int* __videosink_default_int_array[] = {
		&___videosink_default_display_id,
		&___videosink_default_x,
		&___videosink_default_y,
		&___videosink_default_width,
		&___videosink_default_height,
		&___videosink_default_rotation,
	};
	static type_string* __videosink_default_string_array[] = {NULL};
	static type_element _videosink_element_overlay_default = {
		"VideosinkElementOverlay",
		"waylandsink",
		__videosink_default_int_array,
		sizeof(__videosink_default_int_array) / sizeof(type_int*),
		__videosink_default_string_array,
		sizeof(__videosink_default_string_array) / sizeof(type_string*),
	};
	static type_element _videosink_element_evas_default = {
		"VideosinkElementEvas",
		"evasimagesink",
		__videosink_default_int_array,
		sizeof(__videosink_default_int_array) / sizeof(type_int*),
		__videosink_default_string_array,
		sizeof(__videosink_default_string_array) / sizeof(type_string*),
	};
	static type_element _videosink_element_gl_default = {
		"VideosinkElementGL",
		"glimagesink",
		__videosink_default_int_array,
		sizeof(__videosink_default_int_array) / sizeof(type_int*),
		__videosink_default_string_array,
		sizeof(__videosink_default_string_array) / sizeof(type_string*),
	};
	static type_element _videosink_element_remote_default = {
		"VideosinkElementRemote",
		"shmsink",
		__videosink_default_int_array,
		sizeof(__videosink_default_int_array) / sizeof(type_int*),
		__videosink_default_string_array,
		sizeof(__videosink_default_string_array) / sizeof(type_string*),
	};
	static type_element _videosink_element_null_default = {
		"VideosinkElementNull",
		"fakesink",
		__videosink_default_int_array,
		sizeof(__videosink_default_int_array) / sizeof(type_int*),
		__videosink_default_string_array,
		sizeof(__videosink_default_string_array) / sizeof(type_string*),
	};

	/* Videoscale element default value */
	static type_element _videoscale_element_default = {
		"VideoscaleElement",
		"videoscale",
		NULL,
		0,
		NULL,
		0,
	};

	/* Videoconvert element default value */
	static type_element _videoconvert_element_default = {
		"VideoconvertElement",
		"videoconvert",
		NULL,
		0,
		NULL,
		0,
	};

	/* H.264 decoder element default value */
	static type_element _videodecoder_element_h264_default = {
		"VideodecoderElementH264",
		"avdec_h264",
		NULL,
		0,
		NULL,
		0,
	};

	/* Record sink element default value */
	static type_element _recordsink_element_default = {
		"RecordsinkElement",
		"fakesink",
		NULL,
		0,
		NULL,
		0,
	};

	/* H263 element default value */
	static type_element _h263_element_default = {
		"H263",
		"avenc_h263",
		NULL,
		0,
		NULL,
		0,
	};

	/* H264 element default value */
	static type_element _h264_element_default = {
		"H264",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* H26L element default value */
	static type_element _h26l_element_default = {
		"H26L",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* MPEG4 element default value */
	static type_element _mpeg4_element_default = {
		"MPEG4",
		"avenc_mpeg4",
		NULL,
		0,
		NULL,
		0,
	};

	/* MPEG1 element default value */
	static type_element _mpeg1_element_default = {
		"MPEG1",
		"avenc_mpeg1video",
		NULL,
		0,
		NULL,
		0,
	};

	/* THEORA element default value */
	static type_element _theora_element_default = {
		"THEORA",
		"theoraenc",
		NULL,
		0,
		NULL,
		0,
	};

	/* AMR element default value */
	static type_element _amr_element_default = {
		"AMR",
		"amrnbenc",
		NULL,
		0,
		NULL,
		0,
	};

	/* G723_1 element default value */
	static type_element _g723_1_element_default = {
		"G723_1",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* MP3 element default value */
	static type_element _mp3_element_default = {
		"MP3",
		"lamemp3enc",
		NULL,
		0,
		NULL,
		0,
	};

	/* AAC element default value */
	static type_element _aac_element_default = {
		"AAC",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* MMF element default value */
	static type_element _mmf_element_default = {
		"MMF",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* ADPCM element default value */
	static type_element _adpcm_element_default = {
		"ADPCM",
		"avenc_adpcm_ima_qt",
		NULL,
		0,
		NULL,
		0,
	};

	/* WAVE element default value */
	static type_element _wave_element_default = {
		"WAVE",
		"wavenc",
		NULL,
		0,
		NULL,
		0,
	};

	/* VORBIS element default value */
	static type_element _vorbis_element_default = {
		"VORBIS",
		"vorbisenc",
		NULL,
		0,
		NULL,
		0,
	};

	/* MIDI element default value */
	static type_element _midi_element_default = {
		"MIDI",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* IMELODY element default value */
	static type_element _imelody_element_default = {
		"IMELODY",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* JPEG element default value */
	static type_element _jpeg_element_default = {
		"JPEG",
		"jpegenc",
		NULL,
		0,
		NULL,
		0,
	};

	/* PNG element default value */
	static type_element _png_element_default = {
		"PNG",
		"pngenc",
		NULL,
		0,
		NULL,
		0,
	};

	/* BMP element default value */
	static type_element _bmp_element_default = {
		"BMP",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* WBMP element default value */
	static type_element _wbmp_element_default = {
		"WBMP",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* TIFF element default value */
	static type_element _tiff_element_default = {
		"TIFF",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* PCX element default value */
	static type_element _pcx_element_default = {
		"PCX",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* GIF element default value */
	static type_element _gif_element_default = {
		"GIF",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* ICO element default value */
	static type_element _ico_element_default = {
		"ICO",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* RAS element default value */
	static type_element _ras_element_default = {
		"RAS",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* TGA element default value */
	static type_element _tga_element_default = {
		"TGA",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* XBM element default value */
	static type_element _xbm_element_default = {
		"XBM",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* XPM element default value */
	static type_element _xpm_element_default = {
		"XPM",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* 3GP element default value */
	static type_element _3gp_element_default = {
		"3GP",
		"avmux_3gp",
		NULL,
		0,
		NULL,
		0,
	};

	/* AMR mux element default value */
	static type_element _amrmux_element_default = {
		"AMR",
		"avmux_amr",
		NULL,
		0,
		NULL,
		0,
	};

	/* MP4 element default value */
	static type_element _mp4_element_default = {
		"MP4",
		"avmux_mp4",
		NULL,
		0,
		NULL,
		0,
	};

	/* AAC mux element default value */
	static type_element _aacmux_element_default = {
		"AAC",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* MP3 mux element default value */
	static type_element _mp3mux_element_default = {
		"MP3",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* OGG element default value */
	static type_element _ogg_element_default = {
		"OGG",
		"oggmux",
		NULL,
		0,
		NULL,
		0,
	};

	/* WAV element default value */
	static type_element _wav_element_default = {
		"WAV",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* AVI element default value */
	static type_element _avi_element_default = {
		"AVI",
		"avimux",
		NULL,
		0,
		NULL,
		0,
	};

	/* WMA element default value */
	static type_element _wma_element_default = {
		"WMA",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* WMV element default value */
	static type_element _wmv_element_default = {
		"WMV",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* MID element default value */
	static type_element _mid_element_default = {
		"MID",
		NULL,
		NULL,
		0,
		NULL,
		0,
	};

	/* MMF mux element default value */
	static type_element _mmfmux_element_default = {
		"MMF",
		"avmux_mmf",
		NULL,
		0,
		NULL,
		0,
	};

	/* MATROSKA element default value */
	static type_element _matroska_element_default = {
		"MATROSKA",
		"matroskamux",
		NULL,
		0,
		NULL,
		0,
	};

	/* M2TS element default value */
	static type_element _m2ts_element_default = {
		"M2TS",
		"mpegtsmux",
		NULL,
		0,
		NULL,
		0,
	};

	/* [General] matching table */
	static conf_info_table conf_main_general_table[] = {
		{ "SyncStateChange", CONFIGURE_VALUE_INT,           {.value_int = 1} },
		{ "GSTInitOption",   CONFIGURE_VALUE_STRING_ARRAY,  {NULL} },
		{ "ModelName",       CONFIGURE_VALUE_STRING,        {NULL} },
		{ "DisabledAttributes", CONFIGURE_VALUE_STRING_ARRAY,  {NULL} },
	};

	/* [VideoInput] matching table */
	static conf_info_table conf_main_video_input_table[] = {
		{ "UseConfCtrl",        CONFIGURE_VALUE_INT,            {.value_int = 1} },
		{ "ConfCtrlFile0",      CONFIGURE_VALUE_STRING,         {NULL} },
		{ "ConfCtrlFile1",      CONFIGURE_VALUE_STRING,         {NULL} },
		{ "VideosrcElement",    CONFIGURE_VALUE_ELEMENT,        {&_videosrc_element_default} },
		{ "UseVideoscale",      CONFIGURE_VALUE_INT,            {.value_int = 0} },
		{ "VideoscaleElement",  CONFIGURE_VALUE_ELEMENT,        {&_videoscale_element_default} },
		{ "UseZeroCopyFormat",  CONFIGURE_VALUE_INT,            {.value_int = 0} },
		{ "DeviceCount",        CONFIGURE_VALUE_INT,            {.value_int = MM_VIDEO_DEVICE_NUM} },
		{ "SupportMediaPacketPreviewCb",  CONFIGURE_VALUE_INT,  {.value_int = 0} },
	};

	/* [AudioInput] matching table */
	static conf_info_table conf_main_audio_input_table[] = {
		{ "AudioDevice",          CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "AudiosrcElement",      CONFIGURE_VALUE_ELEMENT, {&_audiosrc_element_default} },
		{ "AudiomodemsrcElement", CONFIGURE_VALUE_ELEMENT, {&_audiomodemsrc_element_default} },
		{ "AudioBufferInterval",  CONFIGURE_VALUE_INT,     {.value_int = DEFAULT_AUDIO_BUFFER_INTERVAL} },
	};

	/* [VideoOutput] matching table */
	static conf_info_table conf_main_video_output_table[] = {
		{ "DisplayDevice",           CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "DisplayMode",             CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "Videosink",               CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "VideosinkElementOverlay", CONFIGURE_VALUE_ELEMENT,   {&_videosink_element_overlay_default} },
		{ "VideosinkElementEvas",    CONFIGURE_VALUE_ELEMENT,   {&_videosink_element_evas_default} },
		{ "VideosinkElementGL",      CONFIGURE_VALUE_ELEMENT,   {&_videosink_element_gl_default} },
		{ "VideosinkElementRemote",  CONFIGURE_VALUE_ELEMENT,   {&_videosink_element_remote_default} },
		{ "VideosinkElementNull",    CONFIGURE_VALUE_ELEMENT,   {&_videosink_element_null_default} },
		{ "UseVideoscale",           CONFIGURE_VALUE_INT,       {.value_int = 0} },
		{ "VideoscaleElement",       CONFIGURE_VALUE_ELEMENT,   {&_videoscale_element_default} },
		{ "UseVideoconvert",         CONFIGURE_VALUE_INT,       {.value_int = 0} },
		{ "VideoconvertElement",     CONFIGURE_VALUE_ELEMENT,   {&_videoconvert_element_default} },
		{ "VideodecoderElementH264", CONFIGURE_VALUE_ELEMENT,   {&_videodecoder_element_h264_default} },
		{ "RecreateDecoder",         CONFIGURE_VALUE_INT,       {.value_int = 0} }
	};

	/* [Capture] matching table */
	static conf_info_table conf_main_capture_table[] = {
		{ "UseEncodebin",           CONFIGURE_VALUE_INT,     {.value_int = 0} },
		{ "UseCaptureMode",         CONFIGURE_VALUE_INT,     {.value_int = 0} },
		{ "VideoscaleElement",      CONFIGURE_VALUE_ELEMENT, {&_videoscale_element_default} },
		{ "PlayCaptureSound",       CONFIGURE_VALUE_INT,     {.value_int = 1} },
	};

	/* [Record] matching table */
	static conf_info_table conf_main_record_table[] = {
		{ "UseAudioEncoderQueue",   CONFIGURE_VALUE_INT,     {.value_int = 1} },
		{ "UseVideoEncoderQueue",   CONFIGURE_VALUE_INT,     {.value_int = 1} },
		{ "VideoProfile",           CONFIGURE_VALUE_INT,     {.value_int = 0} },
		{ "VideoAutoAudioConvert",  CONFIGURE_VALUE_INT,     {.value_int = 0} },
		{ "VideoAutoAudioResample", CONFIGURE_VALUE_INT,     {.value_int = 0} },
		{ "VideoAutoColorSpace",    CONFIGURE_VALUE_INT,     {.value_int = 0} },
		{ "AudioProfile",           CONFIGURE_VALUE_INT,     {.value_int = 0} },
		{ "AudioAutoAudioConvert",  CONFIGURE_VALUE_INT,     {.value_int = 0} },
		{ "AudioAutoAudioResample", CONFIGURE_VALUE_INT,     {.value_int = 0} },
		{ "AudioAutoColorSpace",    CONFIGURE_VALUE_INT,     {.value_int = 0} },
		{ "ImageProfile",           CONFIGURE_VALUE_INT,     {.value_int = 0} },
		{ "ImageAutoAudioConvert",  CONFIGURE_VALUE_INT,     {.value_int = 0} },
		{ "ImageAutoAudioResample", CONFIGURE_VALUE_INT,     {.value_int = 0} },
		{ "ImageAutoColorSpace",    CONFIGURE_VALUE_INT,     {.value_int = 0} },
		{ "RecordsinkElement",      CONFIGURE_VALUE_ELEMENT, {&_recordsink_element_default} },
		{ "UseNoiseSuppressor",     CONFIGURE_VALUE_INT,     {.value_int = 0} },
		{ "DropVideoFrame",         CONFIGURE_VALUE_INT,     {.value_int = 0} },
		{ "PassFirstVideoFrame",    CONFIGURE_VALUE_INT,     {.value_int = 0} },
		{ "SupportDualStream",      CONFIGURE_VALUE_INT,     {.value_int = FALSE} },
	};

	/* [VideoEncoder] matching table */
	static conf_info_table conf_main_video_encoder_table[] = {
		{ "H263",    CONFIGURE_VALUE_ELEMENT, {&_h263_element_default} },
		{ "H264",    CONFIGURE_VALUE_ELEMENT, {&_h264_element_default} },
		{ "H26L",    CONFIGURE_VALUE_ELEMENT, {&_h26l_element_default} },
		{ "MPEG4",   CONFIGURE_VALUE_ELEMENT, {&_mpeg4_element_default} },
		{ "MPEG1",   CONFIGURE_VALUE_ELEMENT, {&_mpeg1_element_default} },
		{ "THEORA",  CONFIGURE_VALUE_ELEMENT, {&_theora_element_default} },
	};

	/* [AudioEncoder] matching table */
	static conf_info_table conf_main_audio_encoder_table[] = {
		{ "AMR",     CONFIGURE_VALUE_ELEMENT, {&_amr_element_default} },
		{ "G723_1",  CONFIGURE_VALUE_ELEMENT, {&_g723_1_element_default} },
		{ "MP3",     CONFIGURE_VALUE_ELEMENT, {&_mp3_element_default} },
		{ "AAC",     CONFIGURE_VALUE_ELEMENT, {&_aac_element_default} },
		{ "MMF",     CONFIGURE_VALUE_ELEMENT, {&_mmf_element_default} },
		{ "ADPCM",   CONFIGURE_VALUE_ELEMENT, {&_adpcm_element_default} },
		{ "WAVE",    CONFIGURE_VALUE_ELEMENT, {&_wave_element_default} },
		{ "MIDI",    CONFIGURE_VALUE_ELEMENT, {&_midi_element_default} },
		{ "IMELODY", CONFIGURE_VALUE_ELEMENT, {&_imelody_element_default} },
		{ "VORBIS",  CONFIGURE_VALUE_ELEMENT, {&_vorbis_element_default} },
	};

	/* [ImageEncoder] matching table */
	static conf_info_table conf_main_image_encoder_table[] = {
		{ "JPEG", CONFIGURE_VALUE_ELEMENT, {&_jpeg_element_default} },
		{ "PNG",  CONFIGURE_VALUE_ELEMENT, {&_png_element_default} },
		{ "BMP",  CONFIGURE_VALUE_ELEMENT, {&_bmp_element_default} },
		{ "WBMP", CONFIGURE_VALUE_ELEMENT, {&_wbmp_element_default} },
		{ "TIFF", CONFIGURE_VALUE_ELEMENT, {&_tiff_element_default} },
		{ "PCX",  CONFIGURE_VALUE_ELEMENT, {&_pcx_element_default} },
		{ "GIF",  CONFIGURE_VALUE_ELEMENT, {&_gif_element_default} },
		{ "ICO",  CONFIGURE_VALUE_ELEMENT, {&_ico_element_default} },
		{ "RAS",  CONFIGURE_VALUE_ELEMENT, {&_ras_element_default} },
		{ "TGA",  CONFIGURE_VALUE_ELEMENT, {&_tga_element_default} },
		{ "XBM",  CONFIGURE_VALUE_ELEMENT, {&_xbm_element_default} },
		{ "XPM",  CONFIGURE_VALUE_ELEMENT, {&_xpm_element_default} },
	};

	/* [Mux] matching table */
	static conf_info_table conf_main_mux_table[] = {
		{ "3GP",      CONFIGURE_VALUE_ELEMENT, {&_3gp_element_default} },
		{ "AMR",      CONFIGURE_VALUE_ELEMENT, {&_amrmux_element_default} },
		{ "MP4",      CONFIGURE_VALUE_ELEMENT, {&_mp4_element_default} },
		{ "AAC",      CONFIGURE_VALUE_ELEMENT, {&_aacmux_element_default} },
		{ "MP3",      CONFIGURE_VALUE_ELEMENT, {&_mp3mux_element_default} },
		{ "OGG",      CONFIGURE_VALUE_ELEMENT, {&_ogg_element_default} },
		{ "WAV",      CONFIGURE_VALUE_ELEMENT, {&_wav_element_default} },
		{ "AVI",      CONFIGURE_VALUE_ELEMENT, {&_avi_element_default} },
		{ "WMA",      CONFIGURE_VALUE_ELEMENT, {&_wma_element_default} },
		{ "WMV",      CONFIGURE_VALUE_ELEMENT, {&_wmv_element_default} },
		{ "MID",      CONFIGURE_VALUE_ELEMENT, {&_mid_element_default} },
		{ "MMF",      CONFIGURE_VALUE_ELEMENT, {&_mmfmux_element_default} },
		{ "MATROSKA", CONFIGURE_VALUE_ELEMENT, {&_matroska_element_default} },
		{ "M2TS",     CONFIGURE_VALUE_ELEMENT, {&_m2ts_element_default} },
	};


	/*******************
	*  Camera control *
	*******************/

	/* [Camera] matching table */
	static conf_info_table conf_ctrl_camera_table[] = {
		{ "InputIndex",           CONFIGURE_VALUE_INT_ARRAY,      {NULL} },
		{ "DeviceName",           CONFIGURE_VALUE_STRING,         {NULL} },
		{ "PreviewResolution",    CONFIGURE_VALUE_INT_PAIR_ARRAY, {NULL} },
		{ "CaptureResolution",    CONFIGURE_VALUE_INT_PAIR_ARRAY, {NULL} },
		{ "VideoResolution",      CONFIGURE_VALUE_INT_PAIR_ARRAY, {NULL} },
		{ "FPS0",                 CONFIGURE_VALUE_INT_ARRAY,      {NULL} },
		{ "FPS1",                 CONFIGURE_VALUE_INT_ARRAY,      {NULL} },
		{ "FPS2",                 CONFIGURE_VALUE_INT_ARRAY,      {NULL} },
		{ "FPS3",                 CONFIGURE_VALUE_INT_ARRAY,      {NULL} },
		{ "FPS4",                 CONFIGURE_VALUE_INT_ARRAY,      {NULL} },
		{ "FPS5",                 CONFIGURE_VALUE_INT_ARRAY,      {NULL} },
		{ "FPS6",                 CONFIGURE_VALUE_INT_ARRAY,      {NULL} },
		{ "FPS7",                 CONFIGURE_VALUE_INT_ARRAY,      {NULL} },
		{ "FPS8",                 CONFIGURE_VALUE_INT_ARRAY,      {NULL} },
		{ "FPS9",                 CONFIGURE_VALUE_INT_ARRAY,      {NULL} },
		{ "PictureFormat",        CONFIGURE_VALUE_INT_ARRAY,      {NULL} },
		{ "Overlay",              CONFIGURE_VALUE_INT_RANGE,      {NULL} },
		{ "RecommendDisplayRotation", CONFIGURE_VALUE_INT,     {.value_int = 3}    },
		{ "RecommendPreviewFormatCapture", CONFIGURE_VALUE_INT, {.value_int = MM_PIXEL_FORMAT_YUYV} },
		{ "RecommendPreviewFormatRecord",  CONFIGURE_VALUE_INT, {.value_int = MM_PIXEL_FORMAT_NV12} },
		{ "RecommendPreviewResolution", CONFIGURE_VALUE_INT_PAIR_ARRAY, {NULL} },
		{ "FacingDirection",      CONFIGURE_VALUE_INT, {.value_int = MM_CAMCORDER_CAMERA_FACING_DIRECTION_REAR} },
	};

	/* [Strobe] matching table */
	static conf_info_table conf_ctrl_strobe_table[] = {
		{ "StrobeControl",        CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "StrobeMode",           CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "StrobeEV",             CONFIGURE_VALUE_INT_RANGE, {NULL} },
	};

	/* [Effect] matching table */
	static conf_info_table conf_ctrl_effect_table[] = {
		{ "Brightness",           CONFIGURE_VALUE_INT_RANGE, {NULL} },
		{ "BrightnessStepDenominator", CONFIGURE_VALUE_INT, {.value_int = 2} },
		{ "Contrast",             CONFIGURE_VALUE_INT_RANGE, {NULL} },
		{ "Saturation",           CONFIGURE_VALUE_INT_RANGE, {NULL} },
		{ "Sharpness",            CONFIGURE_VALUE_INT_RANGE, {NULL} },
		{ "WhiteBalance",         CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "ColorTone",            CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "Flip",                 CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "Rotation",             CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "WDR",                  CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "PartColorMode",        CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "PartColor",            CONFIGURE_VALUE_INT_ARRAY, {NULL} },
	};

	/* [Photograph] matching table */
	static conf_info_table conf_ctrl_photograph_table[] = {
		{ "LensInit",             CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "DigitalZoom",          CONFIGURE_VALUE_INT_RANGE, {NULL} },
		{ "OpticalZoom",          CONFIGURE_VALUE_INT_RANGE, {NULL} },
		{ "FocusMode",            CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "AFType",               CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "AEType",               CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "ExposureValue",        CONFIGURE_VALUE_INT_RANGE, {NULL} },
		{ "FNumber",              CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "ShutterSpeed",         CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "ISO",                  CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "ProgramMode",          CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "AntiHandshake",        CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "VideoStabilization",   CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "FaceZoomMode",         CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "FaceZoomLevel",        CONFIGURE_VALUE_INT_RANGE, {NULL} },
		{ "PanMecha",             CONFIGURE_VALUE_INT_RANGE, {NULL} },
		{ "PanElec",              CONFIGURE_VALUE_INT_RANGE, {NULL} },
		{ "TiltMecha",            CONFIGURE_VALUE_INT_RANGE, {NULL} },
		{ "TiltElec",             CONFIGURE_VALUE_INT_RANGE, {NULL} },
	};

	/* [Capture] matching table */
	static conf_info_table conf_ctrl_capture_table[] = {
		{ "OutputMode",           CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "JpegQuality",          CONFIGURE_VALUE_INT_RANGE, {NULL} },
		{ "MultishotNumber",      CONFIGURE_VALUE_INT_RANGE, {NULL} },
		{ "SensorEncodedCapture", CONFIGURE_VALUE_INT,       {.value_int = 1} },
		{ "SupportHDR",           CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "SupportZSL",           CONFIGURE_VALUE_INT,       {.value_int = FALSE} },
	};

	/* [Detect] matching table */
	static conf_info_table conf_ctrl_detect_table[] = {
		{ "DetectMode",           CONFIGURE_VALUE_INT_ARRAY, {NULL} },
		{ "DetectNumber",         CONFIGURE_VALUE_INT_RANGE, {NULL} },
		{ "DetectSelect",         CONFIGURE_VALUE_INT_RANGE, {NULL} },
		{ "DetectSelectNumber",   CONFIGURE_VALUE_INT_RANGE, {NULL} },
	};

	if (hcamcorder == NULL) {
		_mmcam_dbg_err("handle is NULL");
		return;
	}

	_mmcam_dbg_log("Entered...");

	if (type == CONFIGURE_TYPE_MAIN) {
		hcamcorder->conf_main_info_table[CONFIGURE_CATEGORY_MAIN_GENERAL]       = conf_main_general_table;
		hcamcorder->conf_main_info_table[CONFIGURE_CATEGORY_MAIN_VIDEO_INPUT]   = conf_main_video_input_table;
		hcamcorder->conf_main_info_table[CONFIGURE_CATEGORY_MAIN_AUDIO_INPUT]   = conf_main_audio_input_table;
		hcamcorder->conf_main_info_table[CONFIGURE_CATEGORY_MAIN_VIDEO_OUTPUT]  = conf_main_video_output_table;
		hcamcorder->conf_main_info_table[CONFIGURE_CATEGORY_MAIN_CAPTURE]       = conf_main_capture_table;
		hcamcorder->conf_main_info_table[CONFIGURE_CATEGORY_MAIN_RECORD]        = conf_main_record_table;
		hcamcorder->conf_main_info_table[CONFIGURE_CATEGORY_MAIN_VIDEO_ENCODER] = conf_main_video_encoder_table;
		hcamcorder->conf_main_info_table[CONFIGURE_CATEGORY_MAIN_AUDIO_ENCODER] = conf_main_audio_encoder_table;
		hcamcorder->conf_main_info_table[CONFIGURE_CATEGORY_MAIN_IMAGE_ENCODER] = conf_main_image_encoder_table;
		hcamcorder->conf_main_info_table[CONFIGURE_CATEGORY_MAIN_MUX]           = conf_main_mux_table;

		hcamcorder->conf_main_category_size[CONFIGURE_CATEGORY_MAIN_GENERAL]       = sizeof(conf_main_general_table) / info_table_size;
		hcamcorder->conf_main_category_size[CONFIGURE_CATEGORY_MAIN_VIDEO_INPUT]   = sizeof(conf_main_video_input_table) / info_table_size;
		hcamcorder->conf_main_category_size[CONFIGURE_CATEGORY_MAIN_AUDIO_INPUT]   = sizeof(conf_main_audio_input_table) / info_table_size;
		hcamcorder->conf_main_category_size[CONFIGURE_CATEGORY_MAIN_VIDEO_OUTPUT]  = sizeof(conf_main_video_output_table) / info_table_size;
		hcamcorder->conf_main_category_size[CONFIGURE_CATEGORY_MAIN_CAPTURE]       = sizeof(conf_main_capture_table) / info_table_size;
		hcamcorder->conf_main_category_size[CONFIGURE_CATEGORY_MAIN_RECORD]        = sizeof(conf_main_record_table) / info_table_size;
		hcamcorder->conf_main_category_size[CONFIGURE_CATEGORY_MAIN_VIDEO_ENCODER] = sizeof(conf_main_video_encoder_table) / info_table_size;
		hcamcorder->conf_main_category_size[CONFIGURE_CATEGORY_MAIN_AUDIO_ENCODER] = sizeof(conf_main_audio_encoder_table) / info_table_size;
		hcamcorder->conf_main_category_size[CONFIGURE_CATEGORY_MAIN_IMAGE_ENCODER] = sizeof(conf_main_image_encoder_table) / info_table_size;
		hcamcorder->conf_main_category_size[CONFIGURE_CATEGORY_MAIN_MUX]           = sizeof(conf_main_mux_table) / info_table_size;

		(*configure_info)->info = (conf_detail**)g_malloc0(sizeof(conf_detail*) * CONFIGURE_CATEGORY_MAIN_NUM);

		for (i = 0 ; i < CONFIGURE_CATEGORY_MAIN_NUM ; i++)
			(*configure_info)->info[i] = NULL;
	} else {
		hcamcorder->conf_ctrl_info_table[CONFIGURE_CATEGORY_CTRL_CAMERA]     = conf_ctrl_camera_table;
		hcamcorder->conf_ctrl_info_table[CONFIGURE_CATEGORY_CTRL_STROBE]     = conf_ctrl_strobe_table;
		hcamcorder->conf_ctrl_info_table[CONFIGURE_CATEGORY_CTRL_EFFECT]     = conf_ctrl_effect_table;
		hcamcorder->conf_ctrl_info_table[CONFIGURE_CATEGORY_CTRL_PHOTOGRAPH] = conf_ctrl_photograph_table;
		hcamcorder->conf_ctrl_info_table[CONFIGURE_CATEGORY_CTRL_CAPTURE]    = conf_ctrl_capture_table;
		hcamcorder->conf_ctrl_info_table[CONFIGURE_CATEGORY_CTRL_DETECT]     = conf_ctrl_detect_table;

		hcamcorder->conf_ctrl_category_size[CONFIGURE_CATEGORY_CTRL_CAMERA]     = sizeof(conf_ctrl_camera_table) / info_table_size;
		hcamcorder->conf_ctrl_category_size[CONFIGURE_CATEGORY_CTRL_STROBE]     = sizeof(conf_ctrl_strobe_table) / info_table_size;
		hcamcorder->conf_ctrl_category_size[CONFIGURE_CATEGORY_CTRL_EFFECT]     = sizeof(conf_ctrl_effect_table) / info_table_size;
		hcamcorder->conf_ctrl_category_size[CONFIGURE_CATEGORY_CTRL_PHOTOGRAPH] = sizeof(conf_ctrl_photograph_table) / info_table_size;
		hcamcorder->conf_ctrl_category_size[CONFIGURE_CATEGORY_CTRL_CAPTURE]    = sizeof(conf_ctrl_capture_table) / info_table_size;
		hcamcorder->conf_ctrl_category_size[CONFIGURE_CATEGORY_CTRL_DETECT]     = sizeof(conf_ctrl_detect_table) / info_table_size;

		(*configure_info)->info = (conf_detail**)g_malloc0(sizeof(conf_detail*) * CONFIGURE_CATEGORY_CTRL_NUM);

		for (i = 0 ; i < CONFIGURE_CATEGORY_CTRL_NUM ; i++)
			(*configure_info)->info[i] = NULL;
	}

	_mmcam_dbg_log("Done.");

	return;
}


int _mmcamcorder_conf_get_info(MMHandleType handle, int type, const char *ConfFile, camera_conf **configure_info)
{
	int ret = MM_ERROR_NONE;
	FILE *fd = NULL;
	char conf_path[60] = {'\0',};

	_mmcam_dbg_log("Opening...[%s]", ConfFile);

	mmf_return_val_if_fail(ConfFile, FALSE);

	snprintf(conf_path, sizeof(conf_path), "%s/multimedia/%s", SYSCONFDIR, ConfFile);
	_mmcam_dbg_log("Try open Configure File[%s]", conf_path);

	fd = fopen(conf_path, "r");
	if (fd == NULL) {
		_mmcam_dbg_warn("File open failed.[%s] retry...", conf_path);

		snprintf(conf_path, sizeof(conf_path), "%s/multimedia/%s", TZ_SYS_ETC, ConfFile);
		_mmcam_dbg_log("Try open Configure File[%s]", conf_path);
		fd = fopen(conf_path, "r");
		if (fd == NULL)
			_mmcam_dbg_warn("open failed.[%s] errno [%d]", conf_path, errno);
	}

	if (fd != NULL) {
		ret = _mmcamcorder_conf_parse_info(handle, type, fd, configure_info);
		fclose(fd);
	} else {
		if (errno == ENOENT)
			ret = MM_ERROR_CAMCORDER_NOT_SUPPORTED;
		else
			ret = MM_ERROR_CAMCORDER_CREATE_CONFIGURE;
	}

	_mmcam_dbg_log("Leave...");

	return ret;
}


int _mmcamcorder_conf_parse_info(MMHandleType handle, int type, FILE* fd, camera_conf** configure_info)
{
	const unsigned int BUFFER_NUM_DETAILS = 256;
	const unsigned int BUFFER_NUM_TOKEN = 20;
	size_t BUFFER_LENGTH_STRING = 256;
	const char* delimiters = " |=,\t\n";

	int category = 0;
	int count_main_category = 0;
	int count_details = 0;
	int length_read = 0;
	int count_token = 0;
	int read_main = 0;

	char *buffer_string = NULL;
	char *buffer_details[BUFFER_NUM_DETAILS];
	char *buffer_token[BUFFER_NUM_TOKEN];
	char *token = NULL;
	char *category_name = NULL;
	char *detail_string = NULL;
	char *user_ptr = NULL;

	_mmcam_dbg_log("");

	camera_conf* new_conf = (camera_conf *)g_malloc0(sizeof(camera_conf));
	if (new_conf == NULL) {
		_mmcam_dbg_err("new_conf alloc failed : %d", sizeof(camera_conf));
		*configure_info = NULL;
		return MM_ERROR_CAMCORDER_LOW_MEMORY;
	}

	buffer_string = (char*)g_malloc0(sizeof(char) * BUFFER_LENGTH_STRING);
	if (buffer_string == NULL) {
		_mmcam_dbg_err("buffer_string alloc failed : %d", sizeof(char) * BUFFER_LENGTH_STRING);
		*configure_info = NULL;
		SAFE_G_FREE(new_conf);
		return MM_ERROR_CAMCORDER_LOW_MEMORY;
	}

	new_conf->type  = type;
	*configure_info = new_conf;

	_mmcamcorder_conf_init(handle, type, &new_conf);

	if (fd == NULL) {
		_mmcam_dbg_err("failed file descriptor fail");
		*configure_info = NULL;
		SAFE_G_FREE(buffer_string);
		SAFE_G_FREE(new_conf);
		return MM_ERROR_CAMCORDER_INVALID_ARGUMENT;
	}

	read_main = 0;
	count_main_category = 0;

	while (!feof(fd)) {
		if (read_main == 0) {
			length_read = getline(&buffer_string, &BUFFER_LENGTH_STRING, fd);
			/*_mmcam_dbg_log( "Read Line : [%s]", buffer_string );*/

			count_token = 0;
			token = strtok_r(buffer_string, delimiters, &user_ptr);

			if ((token) && (token[0] == ';') && (length_read > -1)) {
				/*_mmcam_dbg_log( "Comment - Nothing to do" );*/
				continue;
			}

			while (token) {
				/*_mmcam_dbg_log("token : [%s]", token);*/
				buffer_token[count_token] = token;
				count_token++;
				token = strtok_r(NULL, delimiters, &user_ptr);
			}

			if (count_token == 0)
				continue;
		}

		read_main = 0;

		/* Comment */
		if (*buffer_token[0] == ';') {
			/*_mmcam_dbg_log( "Comment - Nothing to do" );*/
		}
		/* Main Category */
		else if (*buffer_token[0] == '[') {
			category_name = get_new_string(buffer_token[0]);
			if (category_name == NULL) {
				_mmcam_dbg_err("strdup failed for [%s]", buffer_token[0]);
				continue;
			}

			count_main_category++;
			count_details = 0;

			while (!feof(fd)) {
				length_read = getline(&buffer_string, &BUFFER_LENGTH_STRING, fd);
				/*_mmcam_dbg_log("Read Detail Line : [%s], read length : [%d]", buffer_string, length_read);*/

				detail_string = get_new_string(buffer_string);

				token = strtok_r(buffer_string, delimiters, &user_ptr);

				if (token && token[0] != ';' && length_read > -1) {
					/*_mmcam_dbg_log("token : [%s]", token);*/
					if (token[0] == '[') {
						read_main = 1;
						buffer_token[0] = token;
						SAFE_G_FREE(detail_string);
						break;
					}

					buffer_details[count_details++] = detail_string;
				} else {
					SAFE_G_FREE(detail_string);
				}
			}

			/*_mmcam_dbg_log("type : %d, category_name : %s, count : [%d]", type, category_name, count_details);*/

			if (count_details == 0) {
				_mmcam_dbg_warn("category %s has no detail value... skip this category...", category_name);
				SAFE_G_FREE(category_name);
				continue;
			}

			category = -1;

			/* Details */
			if (type == CONFIGURE_TYPE_MAIN) {
				if (!strcmp("[General]", category_name)) {
					category = CONFIGURE_CATEGORY_MAIN_GENERAL;
				} else if (!strcmp("[VideoInput]", category_name)) {
					category = CONFIGURE_CATEGORY_MAIN_VIDEO_INPUT;
				} else if (!strcmp("[AudioInput]", category_name)) {
					category = CONFIGURE_CATEGORY_MAIN_AUDIO_INPUT;
				} else if (!strcmp("[VideoOutput]", category_name)) {
					category = CONFIGURE_CATEGORY_MAIN_VIDEO_OUTPUT;
				} else if (!strcmp("[Capture]", category_name)) {
					category = CONFIGURE_CATEGORY_MAIN_CAPTURE;
				} else if (!strcmp("[Record]", category_name)) {
					category = CONFIGURE_CATEGORY_MAIN_RECORD;
				} else if (!strcmp("[VideoEncoder]", category_name)) {
					category = CONFIGURE_CATEGORY_MAIN_VIDEO_ENCODER;
				} else if (!strcmp("[AudioEncoder]", category_name)) {
					category = CONFIGURE_CATEGORY_MAIN_AUDIO_ENCODER;
				} else if (!strcmp("[ImageEncoder]", category_name)) {
					category = CONFIGURE_CATEGORY_MAIN_IMAGE_ENCODER;
				} else if (!strcmp("[Mux]", category_name)) {
					category = CONFIGURE_CATEGORY_MAIN_MUX;
				}
			} else {
				if (!strcmp("[Camera]", category_name)) {
					category = CONFIGURE_CATEGORY_CTRL_CAMERA;
				} else if (!strcmp("[Strobe]", category_name)) {
					category = CONFIGURE_CATEGORY_CTRL_STROBE;
				} else if (!strcmp("[Effect]", category_name)) {
					category = CONFIGURE_CATEGORY_CTRL_EFFECT;
				} else if (!strcmp("[Photograph]", category_name))	{
					category = CONFIGURE_CATEGORY_CTRL_PHOTOGRAPH;
				} else if (!strcmp("[Capture]", category_name)) {
					category = CONFIGURE_CATEGORY_CTRL_CAPTURE;
				} else if (!strcmp("[Detect]", category_name)) {
					category = CONFIGURE_CATEGORY_CTRL_DETECT;
				}
			}

			if (category != -1) {
				_mmcamcorder_conf_add_info(handle, type, &(new_conf->info[category]),
											buffer_details, category, count_details);
			} else {
				_mmcam_dbg_warn("No matched category[%s],type[%d]... check it.", category_name, type);
			}

			/* Free memory */
			{
				int i;

				for (i = 0 ; i < count_details ; i++) {
					SAFE_G_FREE(buffer_details[i]);
				}
			}
		}

		SAFE_G_FREE(category_name);
	}

	/* (*configure_info) = new_conf; */

	SAFE_G_FREE(buffer_string);

	/*_mmcam_dbg_log("Done.");*/

	return MM_ERROR_NONE;
}


void _mmcamcorder_conf_release_info(MMHandleType handle, camera_conf **configure_info)
{
	int i = 0;
	int j = 0;
	int k = 0;
	int type = CONFIGURE_VALUE_INT;
	int count = 0;
	int category_num = CONFIGURE_CATEGORY_MAIN_NUM;
	camera_conf *temp_conf = (*configure_info);

	type_int2 *temp_int = NULL;
	type_int_range *temp_int_range = NULL;
	type_int_array *temp_int_array = NULL;
	type_int_pair_array *temp_int_pair_array = NULL;
	type_string2 *temp_string = NULL;
	type_string_array *temp_string_array = NULL;
	type_element2 *temp_element = NULL;

	_mmcam_dbg_log("Entered...");

	mmf_return_if_fail(temp_conf);

	if ((*configure_info)->type == CONFIGURE_TYPE_MAIN)
		category_num = CONFIGURE_CATEGORY_MAIN_NUM;
	else
		category_num = CONFIGURE_CATEGORY_CTRL_NUM;

	for (i = 0 ; i < category_num ; i++) {
		if (temp_conf->info[i]) {
			for (j = 0 ; j < temp_conf->info[i]->count ; j++) {
				if (temp_conf->info[i]->detail_info[j] == NULL)
					continue;

				if (_mmcamcorder_conf_get_value_type(handle, temp_conf->type, i, ((type_element*)(temp_conf->info[i]->detail_info[j]))->name, &type)) {
					switch (type) {
					case CONFIGURE_VALUE_INT:
						temp_int = (type_int2*)(temp_conf->info[i]->detail_info[j]);
						SAFE_G_FREE(temp_int->name);
						break;
					case CONFIGURE_VALUE_INT_RANGE:
						temp_int_range = (type_int_range*)(temp_conf->info[i]->detail_info[j]);
						SAFE_G_FREE(temp_int_range->name);
						break;
					case CONFIGURE_VALUE_INT_ARRAY:
						temp_int_array = (type_int_array*)(temp_conf->info[i]->detail_info[j]);
						SAFE_G_FREE(temp_int_array->name);
						SAFE_G_FREE(temp_int_array->value);
						break;
					case CONFIGURE_VALUE_INT_PAIR_ARRAY:
						temp_int_pair_array = (type_int_pair_array*)(temp_conf->info[i]->detail_info[j]);
						SAFE_G_FREE(temp_int_pair_array->name);
						SAFE_G_FREE(temp_int_pair_array->value[0]);
						SAFE_G_FREE(temp_int_pair_array->value[1]);
						break;
					case CONFIGURE_VALUE_STRING:
						temp_string = (type_string2*)(temp_conf->info[i]->detail_info[j]);
						SAFE_G_FREE(temp_string->name);
						SAFE_G_FREE(temp_string->value);
						break;
					case CONFIGURE_VALUE_STRING_ARRAY:
						temp_string_array = (type_string_array*)(temp_conf->info[i]->detail_info[j]);
						SAFE_G_FREE(temp_string_array->name);
						if (temp_string_array->value) {
							count = temp_string_array->count;
							for (k = 0 ; k < count ; k++) {
								SAFE_G_FREE(temp_string_array->value[k]);
							}

							SAFE_G_FREE(temp_string_array->value);
						}
						SAFE_G_FREE(temp_string_array->default_value);
						break;
					case CONFIGURE_VALUE_ELEMENT:
						temp_element = (type_element2*)(temp_conf->info[i]->detail_info[j]);
						SAFE_G_FREE(temp_element->name);
						SAFE_G_FREE(temp_element->element_name);

						if (temp_element->value_int) {
							count = temp_element->count_int;
							for (k = 0 ; k < count ; k++) {
								SAFE_G_FREE(temp_element->value_int[k]->name);
								SAFE_G_FREE(temp_element->value_int[k]);
							}
							SAFE_G_FREE(temp_element->value_int);
						}

						if (temp_element->value_string) {
							count = temp_element->count_string;
							for (k = 0 ; k < count ; k++) {
								SAFE_G_FREE(temp_element->value_string[k]->name);
								SAFE_G_FREE(temp_element->value_string[k]->value);
								SAFE_G_FREE(temp_element->value_string[k]);
							}
							SAFE_G_FREE(temp_element->value_string);
						}
						break;
					default:
						_mmcam_dbg_warn("unknown type %d", type);
						break;
					}
				}

				SAFE_G_FREE(temp_conf->info[i]->detail_info[j]);
			}

			SAFE_G_FREE(temp_conf->info[i]->detail_info);

			SAFE_G_FREE(temp_conf->info[i]);
			temp_conf->info[i] = NULL;
		}
	}

	SAFE_G_FREE((*configure_info)->info);
	SAFE_G_FREE((*configure_info));

	_mmcam_dbg_log("Done.");
}

int _mmcamcorder_conf_get_value_type(MMHandleType handle, int type, int category, const char* name, int* value_type)
{
	int i = 0;
	int count_value = 0;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	if (hcamcorder == NULL) {
		_mmcam_dbg_err("handle is NULL");
		return FALSE;
	}

	/*_mmcam_dbg_log("Entered...");*/

	mmf_return_val_if_fail(name, FALSE);

	if (!_mmcamcorder_conf_get_category_size(handle, type, category, &count_value)) {
		_mmcam_dbg_warn("No matched category... check it... categoty[%d]", category);
		return FALSE;
	}

	/*_mmcam_dbg_log("Number of value : [%d]", count_value);*/

	if (type == CONFIGURE_TYPE_MAIN) {
		for (i = 0 ; i < count_value ; i++) {
			if (!strcmp(hcamcorder->conf_main_info_table[category][i].name, name)) {
				*value_type = hcamcorder->conf_main_info_table[category][i].value_type;
				/*_mmcam_dbg_log("Category[%d],Name[%s],Type[%d]", category, name, *value_type);*/
				return TRUE;
			}
		}
	} else {
		for (i = 0 ; i < count_value ; i++) {
			if (!strcmp(hcamcorder->conf_ctrl_info_table[category][i].name, name)) {
				*value_type = hcamcorder->conf_ctrl_info_table[category][i].value_type;
				/*_mmcam_dbg_log("Category[%d],Name[%s],Type[%d]", category, name, *value_type);*/
				return TRUE;
			}
		}
	}

	return FALSE;
}


int _mmcamcorder_conf_add_info(MMHandleType handle, int type, conf_detail** info, char** buffer_details, int category, int count_details)
{
	const int BUFFER_NUM_TOKEN = 256;

	int i = 0;
	int j = 0;
	int count_token;
	int value_type;
	char *token = NULL;
	char *buffer_token[BUFFER_NUM_TOKEN];
	char *user_ptr = NULL;

	const char *delimiters     = " |=,\t\n";
	const char *delimiters_sub = " |\t\n";
	const char *delimiters_3rd = "|\n";

	mmf_return_val_if_fail(buffer_details, FALSE);

	(*info) = (conf_detail*)g_malloc0(sizeof(conf_detail));
	if (*info == NULL) {
		_mmcam_dbg_err("allocation failed");
		return FALSE;
	}
	(*info)->detail_info = (void**)g_malloc0(sizeof(void*) * count_details);
	(*info)->count = count_details;

	for (i = 0 ; i < count_details ; i++) {
		/*_mmcam_dbg_log("Read line\"%s\"", buffer_details[i]);*/
		count_token = 0;
		token = strtok_r(buffer_details[i], delimiters, &user_ptr);

		if (token) {
			buffer_token[count_token] = token;
			count_token++;
		} else {
			(*info)->detail_info[i] = NULL;
			_mmcam_dbg_warn("No token... check it.[%s]", buffer_details[i]);
			continue;
		}

		if (!_mmcamcorder_conf_get_value_type(handle, type, category, buffer_token[0], &value_type)) {
			(*info)->detail_info[i] = NULL;
			_mmcam_dbg_warn("Failed to get value type... check it. Category[%d],Name[%s]", category, buffer_token[0]);
			continue;
		}

		if (value_type != CONFIGURE_VALUE_STRING && value_type != CONFIGURE_VALUE_STRING_ARRAY) {
			token = strtok_r(NULL, delimiters, &user_ptr);
			while (token) {
				buffer_token[count_token] = token;
				/*_mmcam_dbg_log("token : [%s]", buffer_token[count_token]);*/
				count_token++;
				token = strtok_r(NULL, delimiters, &user_ptr);
			}

			if (count_token < 2) {
				(*info)->detail_info[i] = NULL;
				_mmcam_dbg_warn("Number of token is too small... check it.[%s]", buffer_details[i]);
				continue;
			}
		} else { /* CONFIGURE_VALUE_STRING or CONFIGURE_VALUE_STRING_ARRAY */
			/* skip "=" */
			strtok_r(NULL, delimiters_sub, &user_ptr);

			if (value_type == CONFIGURE_VALUE_STRING_ARRAY) {
				token = strtok_r(NULL, delimiters_sub, &user_ptr);
				while (token) {
					buffer_token[count_token] = token;
					/*_mmcam_dbg_log("token : [%s]", buffer_token[count_token]);*/
					count_token++;
					token = strtok_r(NULL, delimiters_sub, &user_ptr);
				}
			} else { /* CONFIGURE_VALUE_STRING */
				token = strtok_r(NULL, delimiters_3rd, &user_ptr);
				if (token) {
					g_strchug(token);
					buffer_token[count_token] = token;
					/*_mmcam_dbg_log("token : [%s]", buffer_token[count_token]);*/
					count_token++;
				}
			}

			if (count_token < 2) {
				(*info)->detail_info[i] = NULL;
				_mmcam_dbg_warn("No string value... check it.[%s]", buffer_details[i]);
				continue;
			}
		}

		switch (value_type) {
		case CONFIGURE_VALUE_INT:
		{
			type_int2* new_int;

			new_int = (type_int2*)g_malloc0(sizeof(type_int2));
			if (new_int == NULL) {
				_mmcam_dbg_err("allocation failed");
				break;
			}
			new_int->name  = get_new_string(buffer_token[0]);
			new_int->value = atoi(buffer_token[1]);
			(*info)->detail_info[i] = (void*)new_int;
			/*_mmcam_dbg_log("INT - name[%s],value[%d]", new_int->name, new_int->value);*/
			break;
		}
		case CONFIGURE_VALUE_INT_RANGE:
		{
			type_int_range* new_int_range;

			new_int_range = (type_int_range*)g_malloc0(sizeof(type_int_range));
			if (new_int_range == NULL) {
				_mmcam_dbg_err("allocation failed");
				break;
			}
			new_int_range->name          = get_new_string(buffer_token[0]);
			new_int_range->min           = atoi(buffer_token[1]);
			new_int_range->max           = atoi(buffer_token[2]);
			new_int_range->default_value = atoi(buffer_token[3]);
			(*info)->detail_info[i]      = (void*)new_int_range;
			/*
			_mmcam_dbg_log("INT RANGE - name[%s],min[%d],max[%d],default[%d]",
				new_int_range->name,
				new_int_range->min,
				new_int_range->max,
				new_int_range->default_value);
			*/
			break;
		}
		case CONFIGURE_VALUE_INT_ARRAY:
		{
			int count_value = count_token - 2;
			type_int_array* new_int_array;

			new_int_array = (type_int_array*)g_malloc0(sizeof(type_int_array));
			if (new_int_array == NULL) {
				_mmcam_dbg_err("allocation failed");
				break;
			}
			new_int_array->name = get_new_string(buffer_token[0]);
			new_int_array->value = (int*)g_malloc0(sizeof(int)*count_value);
			if (new_int_array->value == NULL) {
				SAFE_G_FREE(new_int_array->name);
				SAFE_G_FREE(new_int_array);
				_mmcam_dbg_err("allocation failed");
				break;
			}
			new_int_array->count = count_value;

			/*_mmcam_dbg_log("INT ARRAY - name[%s]", new_int_array->name);*/
			for (j = 0 ; j < count_value ; j++) {
				new_int_array->value[j] = atoi(buffer_token[j+1]);
				/*_mmcam_dbg_log("   index[%d] - value[%d]", j, new_int_array->value[j]);*/
			}

			new_int_array->default_value = atoi(buffer_token[count_token-1]);
			/*_mmcam_dbg_log("   default value[%d]", new_int_array->default_value);*/

			(*info)->detail_info[i] = (void*)new_int_array;
			break;
		}
		case CONFIGURE_VALUE_INT_PAIR_ARRAY:
		{
			int count_value = (count_token - 3) >> 1;
			type_int_pair_array* new_int_pair_array;

			new_int_pair_array = (type_int_pair_array*)g_malloc0(sizeof(type_int_pair_array));
			if (new_int_pair_array == NULL) {
				_mmcam_dbg_err("allocation failed");
				break;
			}
			new_int_pair_array->name = get_new_string(buffer_token[0]);
			new_int_pair_array->value[0] = (int*)g_malloc0(sizeof(int)*(count_value));
			if (new_int_pair_array->value[0] == NULL) {
				SAFE_G_FREE(new_int_pair_array->name);
				SAFE_G_FREE(new_int_pair_array);
				_mmcam_dbg_err("allocation failed");
				break;
			}
			new_int_pair_array->value[1] = (int*)g_malloc0(sizeof(int)*(count_value));
			if (new_int_pair_array->value[1] == NULL) {
				SAFE_G_FREE(new_int_pair_array->value[0]);
				SAFE_G_FREE(new_int_pair_array->name);
				SAFE_G_FREE(new_int_pair_array);
				_mmcam_dbg_err("allocation failed");
				break;
			}
			new_int_pair_array->count    = count_value;

			/*_mmcam_dbg_log("INT PAIR ARRAY - name[%s],count[%d]", new_int_pair_array->name, count_value);*/
			for (j = 0 ; j < count_value ; j++) {
				new_int_pair_array->value[0][j] = atoi(buffer_token[(j<<1)+1]);
				new_int_pair_array->value[1][j] = atoi(buffer_token[(j<<1)+2]);
				/*
				_mmcam_dbg_log("   index[%d] - value[%d,%d]", j,
					new_int_pair_array->value[0][j],
					new_int_pair_array->value[1][j]);
				*/
			}

			new_int_pair_array->default_value[0] = atoi(buffer_token[count_token-2]);
			new_int_pair_array->default_value[1] = atoi(buffer_token[count_token-1]);
			/*
			_mmcam_dbg_log("   default value[%d,%d]",
				new_int_pair_array->default_value[0],
				new_int_pair_array->default_value[1]);
			*/
			(*info)->detail_info[i] = (void*)new_int_pair_array;
			break;
		}
		case CONFIGURE_VALUE_STRING:
		{
			type_string2* new_string;

			new_string = (type_string2*)g_malloc0(sizeof(type_string2));
			if (new_string == NULL) {
				_mmcam_dbg_err("allocation failed");
				break;
			}
			new_string->name  = get_new_string(buffer_token[0]);
			new_string->value = get_new_string(buffer_token[1]);
			(*info)->detail_info[i]	= (void*)new_string;

			/*_mmcam_dbg_log("STRING - name[%s],value[%s]", new_string->name, new_string->value);*/
			break;
		}
		case CONFIGURE_VALUE_STRING_ARRAY:
		{
			int count_value = count_token - 2;
			type_string_array* new_string_array;

			new_string_array = (type_string_array*)g_malloc0(sizeof(type_string_array));
			if (new_string_array == NULL)
				break;

			new_string_array->name  = get_new_string(buffer_token[0]);
			new_string_array->count = count_value;
			new_string_array->value = (char**)g_malloc0(sizeof(char*)*count_value);
			if (new_string_array->value == NULL) {
				SAFE_G_FREE(new_string_array->name);
				SAFE_G_FREE(new_string_array);
				_mmcam_dbg_err("allocation failed");
				break;
			}
			/*_mmcam_dbg_log("STRING ARRAY - name[%s]", new_string_array->name);*/
			for (j = 0 ; j < count_value ; j++) {
				new_string_array->value[j] = get_new_string(buffer_token[j+1]);
				/*_mmcam_dbg_log("   index[%d] - value[%s]", j, new_string_array->value[j]);*/
			}

			new_string_array->default_value = get_new_string(buffer_token[count_token-1]);
			/*_mmcam_dbg_log("   default value[%s]", new_string_array->default_value);*/
			(*info)->detail_info[i]	= (void*)new_string_array;
			break;
		}
		case CONFIGURE_VALUE_ELEMENT:
		{
			type_element2* new_element;

			new_element = (type_element2*)g_malloc0(sizeof(type_element2));
			if (new_element == NULL) {
				_mmcam_dbg_err("allocation failed");
				break;
			}
			new_element->name         = get_new_string(buffer_token[0]);
			new_element->element_name = get_new_string(buffer_token[1]);
			new_element->count_int    = atoi(buffer_token[2]);
			new_element->value_int    = NULL;
			new_element->count_string = atoi(buffer_token[3]);
			new_element->value_string = NULL;
			/*
			_mmcam_dbg_log("Element - name[%s],element_name[%s],count_int[%d],count_string[%d]",
				new_element->name, new_element->element_name, new_element->count_int, new_element->count_string);
			*/

			/* add int values */
			if (new_element->count_int > 0 && new_element->count_int <= 30) {
				new_element->value_int = (type_int2**)g_malloc0(sizeof(type_int2*)*(new_element->count_int));
				if (new_element->value_int) {
					for (j = 0 ; j < new_element->count_int ; j++) {
						new_element->value_int[j] = (type_int2*)g_malloc0(sizeof(type_int2));
						if (new_element->value_int[j]) {
							new_element->value_int[j]->name  = get_new_string(buffer_token[4+(j<<1)]);
							new_element->value_int[j]->value = atoi(buffer_token[5+(j<<1)]);
						} else {
							_mmcam_dbg_err("allocation failed");
						}
						/*
						_mmcam_dbg_log("   Element INT[%d] - name[%s],value[%d]",
							j, new_element->value_int[j]->name, new_element->value_int[j]->value);
						*/
					}
				} else {
					_mmcam_dbg_err("allocation failed");
				}
			} else {
				/*_mmcam_dbg_err("invalid count - %d", new_element->count_int);*/
				new_element->value_int = NULL;
			}

			/* add string values */
			if (new_element->count_string > 0 && new_element->count_string <= 30 &&
				new_element->count_int >= 0 && new_element->count_int <= 30) {
				new_element->value_string = (type_string2**)g_malloc0(sizeof(type_string2*)*(new_element->count_string));
				if (new_element->value_string) {
					for ( ; j < new_element->count_string + new_element->count_int ; j++) {
						new_element->value_string[j-new_element->count_int]	= (type_string2*)g_malloc0(sizeof(type_string2));
						if (new_element->value_string[j-new_element->count_int]) {
							new_element->value_string[j-new_element->count_int]->name	= get_new_string(buffer_token[4+(j<<1)]);
							new_element->value_string[j-new_element->count_int]->value	= get_new_string(buffer_token[5+(j<<1)]);
							/*
							_mmcam_dbg_log("   Element STRING[%d] - name[%s],value[%s]",
								j-new_element->count_int, new_element->value_string[j-new_element->count_int]->name, new_element->value_string[j-new_element->count_int]->value);
							*/
						} else {
							_mmcam_dbg_err("allocation failed");
						}
					}
				} else {
					_mmcam_dbg_err("malloc failed : %d", sizeof(type_string2*)*(new_element->count_string));
				}
			} else {
				/*_mmcam_dbg_err("invalid count - %d", new_element->count_string);*/
				new_element->value_string = NULL;
			}

			(*info)->detail_info[i] = (void*)new_element;
			break;
		}
		default:
			break;
		}
	}

	return TRUE;
}


int _mmcamcorder_conf_get_value_int(MMHandleType handle, camera_conf* configure_info, int category, const char* name, int* value)
{
	int i, count;
	conf_detail* info;

	/* _mmcam_dbg_log("Entered... category[%d],name[%s]", category, name); */

	mmf_return_val_if_fail(configure_info, FALSE);
	mmf_return_val_if_fail(name, FALSE);

	if (configure_info->info[category]) {
		count = configure_info->info[category]->count;
		info = configure_info->info[category];

		for (i = 0 ; i < count ; i++) {
			if (info->detail_info[i] == NULL)
				continue;

			if (!strcmp(((type_int*)(info->detail_info[i]))->name , name)) {
				*value = ((type_int*)(info->detail_info[i]))->value;
				/* _mmcam_dbg_log("Get[%s] int[%d]", name, *value); */
				return TRUE;
			}
		}
	}

	if (_mmcamcorder_conf_get_default_value_int(handle, configure_info->type, category, name, value)) {
		 /*_mmcam_dbg_log("Get[%s] int[%d]", name, *value);*/
		return TRUE;
	}

	_mmcam_dbg_err("Faild to get int... check it...Category[%d],Name[%s]", category, name);

	return FALSE;
}

int
_mmcamcorder_conf_get_value_int_range(camera_conf* configure_info, int category, const char* name, type_int_range** value)
{
	int i, count;
	conf_detail* info;

	/* _mmcam_dbg_log("Entered... category[%d],name[%s]", category, name); */

	mmf_return_val_if_fail(configure_info, FALSE);
	mmf_return_val_if_fail(name, FALSE);

	if (configure_info->info[category]) {
		count = configure_info->info[category]->count;
		info = configure_info->info[category];

		for (i = 0 ; i < count ; i++) {
			if (info->detail_info[i] == NULL)
				continue;

			if (!strcmp(((type_int_range*)(info->detail_info[i]))->name , name)) {
				*value = (type_int_range*)(info->detail_info[i]);
				/*
				_mmcam_dbg_log("Get[%s] int range - min[%d],max[%d],default[%d]",
					name, (*value)->min, (*value)->max, (*value)->default_value);
				*/
				return TRUE;
			}
		}
	}

	*value = NULL;

	/*_mmcam_dbg_warn( "Faild to get int range... check it...Category[%d],Name[%s]", category, name );*/

	return FALSE;
}

int
_mmcamcorder_conf_get_value_int_array(camera_conf* configure_info, int category, const char* name, type_int_array** value)
{
	int i, count;
	conf_detail* info;

	/*_mmcam_dbg_log("Entered... category[%d],name[%s]", category, name);*/

	mmf_return_val_if_fail(configure_info, FALSE);
	mmf_return_val_if_fail(name, FALSE);

	if (configure_info->info[category]) {
		count	= configure_info->info[category]->count;
		info	= configure_info->info[category];

		for (i = 0 ; i < count ; i++) {
			if (info->detail_info[i] == NULL)
				continue;

			if (!strcmp(((type_int_array*)(info->detail_info[i]))->name , name)) {
				*value = (type_int_array*)(info->detail_info[i]);
				/*
				_mmcam_dbg_log("Get[%s] int array - [%x],count[%d],default[%d]",
					name, (*value)->value, (*value)->count, (*value)->default_value);
				*/
				return TRUE;
			}
		}
	}

	*value = NULL;

	/*_mmcam_dbg_warn( "Faild to get int array... check it...Category[%d],Name[%s]", category, name );*/

	return FALSE;
}

int
_mmcamcorder_conf_get_value_int_pair_array(camera_conf* configure_info, int category, const char* name, type_int_pair_array** value)
{
	int i, count;
	conf_detail* info;

	/* _mmcam_dbg_log("Entered... category[%d],name[%s]", category, name); */

	mmf_return_val_if_fail(configure_info, FALSE);
	mmf_return_val_if_fail(name, FALSE);

	if (configure_info->info[category]) {
		count = configure_info->info[category]->count;
		info = configure_info->info[category];

		for (i = 0 ; i < count ; i++) {
			if (info->detail_info[i] == NULL)
				continue;

			if (!strcmp(((type_int_pair_array*)(info->detail_info[i]))->name , name)) {
				*value = (type_int_pair_array*)(info->detail_info[i]);
				/*
				_mmcam_dbg_log("Get[%s] int pair array - [%x][%x],count[%d],default[%d][%d]",
					name, (*value)->value[0], (*value)->value[1], (*value)->count,
					(*value)->default_value[0], (*value)->default_value[1]);
				*/
				return TRUE;
			}
		}
	}

	*value = NULL;

	/*_mmcam_dbg_warn("Faild to get int pair array... check it...Category[%d],Name[%s]", category, name);*/

	return FALSE;
}

int _mmcamcorder_conf_get_value_string(MMHandleType handle, camera_conf* configure_info, int category, const char* name, const char** value)
{
	int i, count;
	conf_detail* info;

	/* _mmcam_dbg_log("Entered... category[%d],name[%s]", category, name); */

	mmf_return_val_if_fail(configure_info, FALSE);
	mmf_return_val_if_fail(name, FALSE);

	if (configure_info->info[category]) {
		count = configure_info->info[category]->count;
		info = configure_info->info[category];

		for (i = 0 ; i < count ; i++) {
			if (info->detail_info[i] == NULL)
				continue;

			if (!strcmp(((type_string*)(info->detail_info[i]))->name , name)) {
				*value = ((type_string*)(info->detail_info[i]))->value;
				 /*_mmcam_dbg_log( "Get[%s] string[%s]", name, *value ? *value : "NULL" );*/
				return TRUE;
			}
		}
	}

	if (_mmcamcorder_conf_get_default_value_string(handle, configure_info->type, category, name, value)) {
		/*_mmcam_dbg_log( "Get[%s] string[%s]", name, *value ? *value : "NULL" );*/
		return TRUE;
	}

	_mmcam_dbg_err("Faild to get string... check it...Category[%d],Name[%s]", category, name);

	return FALSE;
}

int
_mmcamcorder_conf_get_value_string_array(camera_conf* configure_info, int category, const char* name, type_string_array** value)
{
	int i, count;
	conf_detail* info;

	/* _mmcam_dbg_log("Entered... category[%d],name[%s]", category, name); */

	mmf_return_val_if_fail(configure_info, FALSE);
	mmf_return_val_if_fail(name, FALSE);

	if (configure_info->info[category]) {
		count = configure_info->info[category]->count;
		info = configure_info->info[category];

		for (i = 0 ; i < count ; i++) {
			if (info->detail_info[i] == NULL)
				continue;

			if (!strcmp(((type_string_array*)(info->detail_info[i]))->name , name)) {
				*value = (type_string_array*)(info->detail_info[i]);
				/*
				_mmcam_dbg_log("Get[%s] string array - [%x],count[%d],default[%s]",
					name, (*value)->value, (*value)->count, (*value)->default_value);
				*/
				return TRUE;
			}
		}
	}

	*value = NULL;

	/*_mmcam_dbg_warn("Faild to get string array... check it...Category[%d],Name[%s]", category, name);*/

	return FALSE;
}

int _mmcamcorder_conf_get_element(MMHandleType handle, camera_conf* configure_info, int category, const char* name, type_element** element)
{
	int i, count;
	conf_detail* info;

	/* _mmcam_dbg_log("Entered... category[%d], ame[%s]", category, name); */

	mmf_return_val_if_fail(configure_info, FALSE);
	mmf_return_val_if_fail(name, FALSE);

	if (configure_info->info[category]) {
		count = configure_info->info[category]->count;
		info = configure_info->info[category];

		for (i = 0 ; i < count ; i++) {
			if (info->detail_info[i] == NULL)
				continue;

			if (!strcmp(((type_element*)(info->detail_info[i]))->name , name)) {
				*element = (type_element*)(info->detail_info[i]);
				/* _mmcam_dbg_log("Get[%s] element[%x]", name, *element); */
				return TRUE;
			}
		}
	}

	if (_mmcamcorder_conf_get_default_element(handle, configure_info->type, category, name, element)) {
		/* _mmcam_dbg_log("Get[%s] element[%x]", name, *element); */
		return TRUE;
	}

	_mmcam_dbg_warn("Faild to get element name... check it...Category[%d],Name[%s]", category, name);

	return FALSE;
}

int
_mmcamcorder_conf_get_value_element_name(type_element* element, const char** value)
{
	/* _mmcam_dbg_log("Entered..."); */

	mmf_return_val_if_fail(element, FALSE);

	*value = element->element_name;

	/* _mmcam_dbg_log("Get element name : [%s]", *value); */

	return TRUE;
}

int
_mmcamcorder_conf_get_value_element_int(type_element* element, const char* name, int* value)
{
	int i;

	/* _mmcam_dbg_log( "Entered..." ); */

	mmf_return_val_if_fail(element, FALSE);
	mmf_return_val_if_fail(name, FALSE);

	for (i = 0 ; i < element->count_int ; i++) {
		if (!strcmp(element->value_int[i]->name, name)) {
			*value = element->value_int[i]->value;
			/* _mmcam_dbg_log("Get[%s] element int[%d]", name, *value); */
			return TRUE;
		}
	}

	_mmcam_dbg_warn("Faild to get int in element... ElementName[%p],Name[%s],Count[%d]",
		element->name, name, element->count_int);

	return FALSE;
}

int
_mmcamcorder_conf_get_value_element_string(type_element* element, const char* name, const char** value)
{
	int i;

	/* _mmcam_dbg_log("Entered..."); */

	mmf_return_val_if_fail(element, FALSE);
	mmf_return_val_if_fail(name, FALSE);

	for (i = 0 ; i < element->count_string ; i++) {
		if (!strcmp(element->value_string[i]->name, name)) {
			*value = element->value_string[i]->value;
			/*_mmcam_dbg_log("Get[%s] element string[%s]", name, *value);*/
			return TRUE;
		}
	}

	_mmcam_dbg_warn("Faild to get int in element... ElementName[%p],Name[%s],Count[%d]",
		element->name, name, element->count_string);

	return FALSE;
}

int
_mmcamcorder_conf_set_value_element_property(GstElement* gst, type_element* element)
{
	int i;

	/* _mmcam_dbg_log("Entered..."); */

	mmf_return_val_if_fail(gst, FALSE);
	mmf_return_val_if_fail(element, FALSE);

	if (element->count_int == 0) {
		/*_mmcam_dbg_log( "There is no integer property to set in INI file[%s].", element->name );*/
	} else {
		if (element->value_int == NULL) {
			_mmcam_dbg_warn("count_int[%d] is NOT zero, but value_int is NULL", element->count_int);
			return FALSE;
		}

		for (i = 0 ; i < element->count_int ; i++) {
			MMCAMCORDER_G_OBJECT_SET(gst, element->value_int[i]->name, element->value_int[i]->value);

			/*
			_mmcam_dbg_log("Element[%s] Set[%s] -> integer[%d]",
				element->element_name,
				element->value_int[i]->name,
				element->value_int[i]->value);
			*/
		}
	}

	if (element->count_string == 0) {
		_mmcam_dbg_log("There is no string property to set in INI file[%s].", element->name);
	} else {
		if (element->value_string == NULL) {
			_mmcam_dbg_warn("count_string[%d] is NOT zero, but value_string is NULL", element->count_string);
			return FALSE;
		}

		for (i = 0 ; i < element->count_string ; i++) {
			MMCAMCORDER_G_OBJECT_SET_POINTER(gst, element->value_string[i]->name, element->value_string[i]->value);

			_mmcam_dbg_log("Element[%s] Set[%s] -> string[%s]",
				element->element_name,
				element->value_string[i]->name,
				element->value_string[i]->value);
		}
	}

	/* _mmcam_dbg_log( "Done." ); */

	return TRUE;
}

int
_mmcamcorder_conf_get_default_value_int(MMHandleType handle, int type, int category, const char* name, int* value)
{
	int i = 0;
	int count_value = 0;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	if (hcamcorder == NULL) {
		_mmcam_dbg_err("handle is NULL");
		return FALSE;
	}

	/* _mmcam_dbg_log("Entered..."); */

	mmf_return_val_if_fail(name, FALSE);

	if (!_mmcamcorder_conf_get_category_size(handle, type, category, &count_value)) {
		_mmcam_dbg_warn("No matched category... check it... categoty[%d]", category);
		return FALSE;
	}

	if (type == CONFIGURE_TYPE_MAIN) {
		for (i = 0 ; i < count_value ; i++) {
			if (!strcmp(hcamcorder->conf_main_info_table[category][i].name, name)) {
				*value = hcamcorder->conf_main_info_table[category][i].value_int;
				return TRUE;
			}
		}
	} else {
		for (i = 0 ; i < count_value ; i++) {
			if (!strcmp(hcamcorder->conf_ctrl_info_table[category][i].name, name)) {
				*value = hcamcorder->conf_ctrl_info_table[category][i].value_int;
				return TRUE;
			}
		}
	}

	_mmcam_dbg_err("Failed to get default int... check it... Type[%d],Category[%d],Name[%s]", type, category, name);

	return FALSE;
}

int _mmcamcorder_conf_get_default_value_string(MMHandleType handle, int type, int category, const char* name, const char** value)
{
	int i = 0;
	int count_value = 0;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	if (hcamcorder == NULL) {
		_mmcam_dbg_err("handle is NULL");
		return FALSE;
	}

	/* _mmcam_dbg_log("Entered..."); */

	mmf_return_val_if_fail(name, FALSE);

	if (!_mmcamcorder_conf_get_category_size(handle, type, category, &count_value)) {
		_mmcam_dbg_warn("No matched category... check it... categoty[%d]", category);
		return FALSE;
	}

	if (type == CONFIGURE_TYPE_MAIN) {
		for (i = 0 ; i < count_value ; i++) {
			if (!strcmp(hcamcorder->conf_main_info_table[category][i].name, name)) {
				*value = hcamcorder->conf_main_info_table[category][i].value_string;
				/*_mmcam_dbg_log("Get[%s] default string[%s]", name, *value ? *value : "NULL");*/
				return TRUE;
			}
		}
	} else {
		for (i = 0 ; i < count_value ; i++) {
			if (!strcmp(hcamcorder->conf_ctrl_info_table[category][i].name, name)) {
				*value = hcamcorder->conf_ctrl_info_table[category][i].value_string;
				/*_mmcam_dbg_log("Get[%s] default string[%s]", name, *value ? *value : "NULL");*/
				return TRUE;
			}
		}
	}

	_mmcam_dbg_warn("Failed to get default string... check it... Type[%d],Category[%d],Name[%s]", type, category, name);

	return FALSE;
}

int _mmcamcorder_conf_get_default_element(MMHandleType handle, int type, int category, const char* name, type_element** element)
{
	int i = 0;
	int count_value = 0;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	if (hcamcorder == NULL) {
		_mmcam_dbg_err("handle is NULL");
		return FALSE;
	}

	/* _mmcam_dbg_log("Entered..."); */

	mmf_return_val_if_fail(name, FALSE);

	if (!_mmcamcorder_conf_get_category_size(handle, type, category, &count_value)) {
		_mmcam_dbg_warn("No matched category... check it... categoty[%d]", category);
		return FALSE;
	}

	if (type == CONFIGURE_TYPE_MAIN) {
		for (i = 0 ; i < count_value ; i++) {
			if (!strcmp(hcamcorder->conf_main_info_table[category][i].name, name)) {
				*element = hcamcorder->conf_main_info_table[category][i].value_element;
				_mmcam_dbg_log("Get[%s] element[%p]", name, *element);
				return TRUE;
			}
		}
	} else {
		for (i = 0 ; i < count_value ; i++) {
			if (!strcmp(hcamcorder->conf_ctrl_info_table[category][i].name, name)) {
				*element = hcamcorder->conf_ctrl_info_table[category][i].value_element;
				_mmcam_dbg_log("Get[%s] element[%p]", name, *element);
				return TRUE;
			}
		}
	}

	_mmcam_dbg_warn("Failed to get default element... check it... Type[%d],Category[%d],Name[%s]", type, category, name);

	return FALSE;
}

int _mmcamcorder_conf_get_category_size(MMHandleType handle, int type, int category, int* size)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	if (hcamcorder == NULL) {
		_mmcam_dbg_err("handle is NULL");
		return FALSE;
	}

	/*
	mmf_return_val_if_fail(conf_main_category_size, FALSE);
	mmf_return_val_if_fail(conf_ctrl_category_size, FALSE);
	*/

	if (type == CONFIGURE_TYPE_MAIN) {
		mmf_return_val_if_fail(category < CONFIGURE_CATEGORY_MAIN_NUM, FALSE);

		*size = (int)hcamcorder->conf_main_category_size[category];
	} else {
		mmf_return_val_if_fail(category < CONFIGURE_CATEGORY_CTRL_NUM, FALSE);

		*size = (int)hcamcorder->conf_ctrl_category_size[category];
	}

	return TRUE;
}

void
_mmcamcorder_conf_print_info(MMHandleType handle, camera_conf** configure_info)
{
	int i, j, k, type, category_type;

	type_int                *temp_int;
	type_int_range          *temp_int_range;
	type_int_array          *temp_int_array;
	type_int_pair_array     *temp_int_pair_array;
	type_string             *temp_string;
	type_element            *temp_element;

	g_print("[ConfigureInfoPrint] : Entered.\n");

	mmf_return_if_fail(*configure_info);

	if ((*configure_info)->type == CONFIGURE_TYPE_MAIN)
		category_type = CONFIGURE_CATEGORY_MAIN_NUM;
	else
		category_type = CONFIGURE_CATEGORY_CTRL_NUM;

	for (i = 0 ; i < category_type ; i++) {

		if ((*configure_info)->info[i]) {
			g_print("[ConfigureInfo] : Category [%d]\n", i);

			for (j = 0 ; j < (*configure_info)->info[i]->count ; j++) {

				if (_mmcamcorder_conf_get_value_type(handle, (*configure_info)->type, i, ((type_int*)((*configure_info)->info[i]->detail_info[j]))->name, &type)) {
					switch (type) {
					case CONFIGURE_VALUE_INT:
						temp_int = (type_int*)((*configure_info)->info[i]->detail_info[j]);
						g_print("[ConfigureInfo] : INT - Name[%s],Value [%d]\n", temp_int->name, temp_int->value);
						break;
					case CONFIGURE_VALUE_INT_RANGE:
						temp_int_range = (type_int_range*)((*configure_info)->info[i]->detail_info[j]);
						g_print("[ConfigureInfo] : INT_RANGE - Name[%s],Value [%d]~[%d], default [%d]\n",
							temp_int_range->name, temp_int_range->min, temp_int_range->max, temp_int_range->default_value);
						break;
					case CONFIGURE_VALUE_INT_ARRAY:
						temp_int_array = (type_int_array*)((*configure_info)->info[i]->detail_info[j]);
						g_print("[ConfigureInfo] : INT_ARRAY - Name[%s], default [%d]\n                            - ",
							temp_int_array->name, temp_int_array->default_value);

						for (k = 0 ; k < temp_int_array->count ; k++)
							g_print("[%d] ", temp_int_array->value[k]);

						g_print("\n");
						break;
					case CONFIGURE_VALUE_INT_PAIR_ARRAY:
						temp_int_pair_array = (type_int_pair_array*)((*configure_info)->info[i]->detail_info[j]);
						g_print("[ConfigureInfo] : INT_PAIR_ARRAY - Name[%s], default [%d][%d]\n                            - ",
							temp_int_pair_array->name, temp_int_pair_array->default_value[0], temp_int_pair_array->default_value[0]);

						for (k = 0 ; k < temp_int_pair_array->count ; k++)
							g_print("[%d,%d] ", temp_int_pair_array->value[0][k], temp_int_pair_array->value[1][k]);

						g_print("\n");
						break;
					case CONFIGURE_VALUE_STRING:
						temp_string = (type_string*)((*configure_info)->info[i]->detail_info[j]);
						g_print("[ConfigureInfo] : STRING - Name[%s],Value[%s]\n", temp_string->name, temp_string->value);
						break;
					case CONFIGURE_VALUE_ELEMENT:
						temp_element = (type_element*)((*configure_info)->info[i]->detail_info[j]);
						g_print("[ConfigureInfo] : Element - Name[%s],Element_Name[%s]\n", temp_element->name, temp_element->element_name);

						for (k = 0 ; k < temp_element->count_int ; k++)
							g_print("                          - INT[%d] Name[%s],Value[%d]\n", k, temp_element->value_int[k]->name, temp_element->value_int[k]->value);

						for (k = 0 ; k < temp_element->count_string ; k++)
							g_print("                          - STRING[%d] Name[%s],Value[%s]\n", k, temp_element->value_string[k]->name, temp_element->value_string[k]->value);

						break;
					default:
						g_print("[ConfigureInfo] : Not matched value type... So can not print data... check it... Name[%s],type[%d]\n", ((type_int*)((*configure_info)->info[i]->detail_info[j]))->name, type);
						break;
					}
				} else {
					g_print("[ConfigureInfo] : Failed to get value type.");
				}
			}
		}
	}

	g_print("[ConfigureInfoPrint] : Done.\n");
}


static type_element *
__mmcamcorder_get_audio_codec_element(MMHandleType handle)
{
	type_element *telement = NULL;
	const char *codec_type_str = NULL;
	int codec_type = MM_AUDIO_CODEC_INVALID;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder, NULL);
	sc = MMF_CAMCORDER_SUBCONTEXT(handle);

	mmf_return_val_if_fail(sc, NULL);

	_mmcam_dbg_log("");

	/* Check element availability */
	mm_camcorder_get_attributes(handle, NULL, MMCAM_AUDIO_ENCODER, &codec_type, NULL);

	switch (codec_type) {
	case MM_AUDIO_CODEC_AMR:
		codec_type_str = "AMR";
		break;
	case MM_AUDIO_CODEC_G723_1:
		codec_type_str = "G723_1";
		break;
	case MM_AUDIO_CODEC_MP3:
		codec_type_str = "MP3";
		break;
	case MM_AUDIO_CODEC_AAC:
		codec_type_str = "AAC";
		break;
	case MM_AUDIO_CODEC_MMF:
		codec_type_str = "MMF";
		break;
	case MM_AUDIO_CODEC_ADPCM:
		codec_type_str = "ADPCM";
		break;
	case MM_AUDIO_CODEC_WAVE:
		codec_type_str = "WAVE";
		break;
	case MM_AUDIO_CODEC_MIDI:
		codec_type_str = "MIDI";
		break;
	case MM_AUDIO_CODEC_IMELODY:
		codec_type_str = "IMELODY";
		break;
	case MM_AUDIO_CODEC_VORBIS:
		codec_type_str = "VORBIS";
		break;
	default:
		_mmcam_dbg_err("Not supported audio codec[%d]", codec_type);
		return NULL;
	}

	_mmcamcorder_conf_get_element(handle, hcamcorder->conf_main,
		CONFIGURE_CATEGORY_MAIN_AUDIO_ENCODER,
		codec_type_str,
		&telement);

	return telement;
}


static type_element *
__mmcamcorder_get_video_codec_element(MMHandleType handle)
{
	type_element *telement = NULL;
	const char *codec_type_str = NULL;
	int codec_type = MM_VIDEO_CODEC_INVALID;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder, NULL);
	sc = MMF_CAMCORDER_SUBCONTEXT(handle);

	mmf_return_val_if_fail(sc, NULL);

	_mmcam_dbg_log("");

	/* Check element availability */
	mm_camcorder_get_attributes(handle, NULL, MMCAM_VIDEO_ENCODER, &codec_type, NULL);

	switch (codec_type) {
	case MM_VIDEO_CODEC_H263:
		codec_type_str = "H263";
		break;
	case MM_VIDEO_CODEC_H264:
		codec_type_str = "H264";
		break;
	case MM_VIDEO_CODEC_H26L:
		codec_type_str = "H26L";
		break;
	case MM_VIDEO_CODEC_MPEG4:
		codec_type_str = "MPEG4";
		break;
	case MM_VIDEO_CODEC_MPEG1:
		codec_type_str = "MPEG1";
		break;
	case MM_VIDEO_CODEC_THEORA:
		codec_type_str = "THEORA";
		break;
	default:
		_mmcam_dbg_err("Not supported video codec[%d]", codec_type);
		return NULL;
	}

	_mmcamcorder_conf_get_element(handle, hcamcorder->conf_main,
		CONFIGURE_CATEGORY_MAIN_VIDEO_ENCODER,
		codec_type_str,
		&telement);

	return telement;
}


static type_element *
__mmcamcorder_get_image_codec_element(MMHandleType handle)
{
	type_element *telement = NULL;
	const char *codec_type_str = NULL;
	int codec_type = MM_IMAGE_CODEC_INVALID;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder, NULL);
	sc = MMF_CAMCORDER_SUBCONTEXT(handle);

	mmf_return_val_if_fail(sc, NULL);

	_mmcam_dbg_log("");

	/* Check element availability */
	mm_camcorder_get_attributes(handle, NULL, MMCAM_IMAGE_ENCODER, &codec_type, NULL);

	switch (codec_type) {
	case MM_IMAGE_CODEC_JPEG:
		codec_type_str = "JPEG";
		break;
	case MM_IMAGE_CODEC_SRW:
		codec_type_str = "SRW";
		break;
	case MM_IMAGE_CODEC_JPEG_SRW:
		codec_type_str = "JPEG_SRW";
		break;
	case MM_IMAGE_CODEC_PNG:
		codec_type_str = "PNG";
		break;
	case MM_IMAGE_CODEC_BMP:
		codec_type_str = "BMP";
		break;
	case MM_IMAGE_CODEC_WBMP:
		codec_type_str = "WBMP";
		break;
	case MM_IMAGE_CODEC_TIFF:
		codec_type_str = "TIFF";
		break;
	case MM_IMAGE_CODEC_PCX:
		codec_type_str = "PCX";
		break;
	case MM_IMAGE_CODEC_GIF:
		codec_type_str = "GIF";
		break;
	case MM_IMAGE_CODEC_ICO:
		codec_type_str = "ICO";
		break;
	case MM_IMAGE_CODEC_RAS:
		codec_type_str = "RAS";
		break;
	case MM_IMAGE_CODEC_TGA:
		codec_type_str = "TGA";
		break;
	case MM_IMAGE_CODEC_XBM:
		codec_type_str = "XBM";
		break;
	case MM_IMAGE_CODEC_XPM:
		codec_type_str = "XPM";
		break;
	default:
		_mmcam_dbg_err("Not supported image codec[%d]", codec_type);
		return NULL;
	}

	_mmcamcorder_conf_get_element(handle, hcamcorder->conf_main,
		CONFIGURE_CATEGORY_MAIN_IMAGE_ENCODER,
		codec_type_str,
		&telement);

	return telement;
}


static type_element *
__mmcamcorder_get_file_format_element(MMHandleType handle)
{
	type_element *telement = NULL;
	const char *mux_type_str = NULL;
	int file_type = MM_FILE_FORMAT_INVALID;
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	_MMCamcorderSubContext *sc = NULL;

	mmf_return_val_if_fail(hcamcorder, NULL);
	sc = MMF_CAMCORDER_SUBCONTEXT(handle);

	mmf_return_val_if_fail(sc, NULL);

	_mmcam_dbg_log("");

	/* Check element availability */
	mm_camcorder_get_attributes(handle, NULL, MMCAM_FILE_FORMAT, &file_type, NULL);

	switch (file_type) {
	case MM_FILE_FORMAT_3GP:
		mux_type_str = "3GP";
		break;
	case MM_FILE_FORMAT_AMR:
		mux_type_str = "AMR";
		break;
	case MM_FILE_FORMAT_MP4:
		mux_type_str = "MP4";
		break;
	case MM_FILE_FORMAT_AAC:
		mux_type_str = "AAC";
		break;
	case MM_FILE_FORMAT_MP3:
		mux_type_str = "MP3";
		break;
	case MM_FILE_FORMAT_OGG:
		mux_type_str = "OGG";
		break;
	case MM_FILE_FORMAT_WAV:
		mux_type_str = "WAV";
		break;
	case MM_FILE_FORMAT_AVI:
		mux_type_str = "AVI";
		break;
	case MM_FILE_FORMAT_WMA:
		mux_type_str = "WMA";
		break;
	case MM_FILE_FORMAT_WMV:
		mux_type_str = "WMV";
		break;
	case MM_FILE_FORMAT_MID:
		mux_type_str = "MID";
		break;
	case MM_FILE_FORMAT_MMF:
		mux_type_str = "MMF";
		break;
	case MM_FILE_FORMAT_MATROSKA:
		mux_type_str = "MATROSKA";
		break;
	case MM_FILE_FORMAT_M2TS:
		mux_type_str = "M2TS";
		break;
	default:
		_mmcam_dbg_err("Not supported file format[%d]", file_type);
		return NULL;
	}

	_mmcamcorder_conf_get_element(handle, hcamcorder->conf_main,
		CONFIGURE_CATEGORY_MAIN_MUX,
		mux_type_str,
		&telement);

	return telement;
}


type_element *
_mmcamcorder_get_type_element(MMHandleType handle, int type)
{
	type_element *telement = NULL;

	_mmcam_dbg_log("type:%d", type);

	switch (type) {
	case MM_CAM_AUDIO_ENCODER:
		telement = __mmcamcorder_get_audio_codec_element(handle);
		break;
	case MM_CAM_VIDEO_ENCODER:
		telement = __mmcamcorder_get_video_codec_element(handle);
		break;
	case MM_CAM_IMAGE_ENCODER:
		telement = __mmcamcorder_get_image_codec_element(handle);
		break;
	case MM_CAM_FILE_FORMAT:
		telement = __mmcamcorder_get_file_format_element(handle);
		break;
	default:
		_mmcam_dbg_log("Can't get element type form this profile.(%d)", type);
	}

	return telement;
}


int _mmcamcorder_get_audio_codec_format(MMHandleType handle, const char *name)
{
	int codec_index = MM_AUDIO_CODEC_INVALID;

	if (!name) {
		_mmcam_dbg_err("name is NULL");
		return MM_AUDIO_CODEC_INVALID;
	}

	if (!strcmp(name, "AMR")) {
		codec_index = MM_AUDIO_CODEC_AMR;
	} else if (!strcmp(name, "G723_1")) {
		codec_index = MM_AUDIO_CODEC_G723_1;
	} else if (!strcmp(name, "MP3")) {
		codec_index = MM_AUDIO_CODEC_MP3;
	} else if (!strcmp(name, "AAC")) {
		codec_index = MM_AUDIO_CODEC_AAC;
	} else if (!strcmp(name, "MMF")) {
		codec_index = MM_AUDIO_CODEC_MMF;
	} else if (!strcmp(name, "ADPCM")) {
		codec_index = MM_AUDIO_CODEC_ADPCM;
	} else if (!strcmp(name, "WAVE")) {
		codec_index = MM_AUDIO_CODEC_WAVE;
	} else if (!strcmp(name, "MIDI")) {
		codec_index = MM_AUDIO_CODEC_MIDI;
	} else if (!strcmp(name, "IMELODY")) {
		codec_index = MM_AUDIO_CODEC_IMELODY;
	} else if (!strcmp(name, "VORBIS")) {
		codec_index = MM_AUDIO_CODEC_VORBIS;
	}

	/*_mmcam_dbg_log("audio codec index %d", codec_index);*/

	return codec_index;
}



int _mmcamcorder_get_video_codec_format(MMHandleType handle, const char *name)
{
	int codec_index = MM_VIDEO_CODEC_INVALID;

	if (!name) {
		_mmcam_dbg_err("name is NULL");
		return MM_VIDEO_CODEC_INVALID;
	}

	if (!strcmp(name, "H263")) {
		codec_index = MM_VIDEO_CODEC_H263;
	} else if (!strcmp(name, "H264")) {
		codec_index = MM_VIDEO_CODEC_H264;
	} else if (!strcmp(name, "H26L")) {
		codec_index = MM_VIDEO_CODEC_H26L;
	} else if (!strcmp(name, "MPEG4")) {
		codec_index = MM_VIDEO_CODEC_MPEG4;
	} else if (!strcmp(name, "MPEG1")) {
		codec_index = MM_VIDEO_CODEC_MPEG1;
	} else if (!strcmp(name, "THEORA")) {
		codec_index = MM_VIDEO_CODEC_THEORA;
	}

	/*_mmcam_dbg_log("video codec index %d", codec_index);*/

	return codec_index;
}



int _mmcamcorder_get_image_codec_format(MMHandleType handle, const char *name)
{
	int codec_index = MM_IMAGE_CODEC_INVALID;

	if (!name) {
		_mmcam_dbg_err("name is NULL");
		return MM_IMAGE_CODEC_INVALID;
	}

	if (!strcmp(name, "JPEG")) {
		codec_index = MM_IMAGE_CODEC_JPEG;
	} else if (!strcmp(name, "PNG")) {
		codec_index = MM_IMAGE_CODEC_PNG;
	} else if (!strcmp(name, "BMP")) {
		codec_index = MM_IMAGE_CODEC_BMP;
	} else if (!strcmp(name, "WBMP")) {
		codec_index = MM_IMAGE_CODEC_WBMP;
	} else if (!strcmp(name, "TIFF")) {
		codec_index = MM_IMAGE_CODEC_TIFF;
	} else if (!strcmp(name, "PCX")) {
		codec_index = MM_IMAGE_CODEC_PCX;
	} else if (!strcmp(name, "GIF")) {
		codec_index = MM_IMAGE_CODEC_GIF;
	} else if (!strcmp(name, "ICO")) {
		codec_index = MM_IMAGE_CODEC_ICO;
	} else if (!strcmp(name, "RAS")) {
		codec_index = MM_IMAGE_CODEC_RAS;
	} else if (!strcmp(name, "TGA")) {
		codec_index = MM_IMAGE_CODEC_TGA;
	} else if (!strcmp(name, "XBM")) {
		codec_index = MM_IMAGE_CODEC_XBM;
	} else if (!strcmp(name, "XPM")) {
		codec_index = MM_IMAGE_CODEC_XPM;
	}

	_mmcam_dbg_log("image codec index %d", codec_index);

	return codec_index;
}


int _mmcamcorder_get_mux_format(MMHandleType handle, const char *name)
{
	int mux_index = MM_FILE_FORMAT_INVALID;

	if (!name) {
		_mmcam_dbg_err("name is NULL");
		return MM_FILE_FORMAT_INVALID;
	}

	if (!strcmp(name, "3GP")) {
		mux_index = MM_FILE_FORMAT_3GP;
	} else if (!strcmp(name, "AMR")) {
		mux_index = MM_FILE_FORMAT_AMR;
	} else if (!strcmp(name, "MP4")) {
		mux_index = MM_FILE_FORMAT_MP4;
	} else if (!strcmp(name, "AAC")) {
		mux_index = MM_FILE_FORMAT_AAC;
	} else if (!strcmp(name, "MP3")) {
		mux_index = MM_FILE_FORMAT_MP3;
	} else if (!strcmp(name, "OGG")) {
		mux_index = MM_FILE_FORMAT_OGG;
	} else if (!strcmp(name, "WAV")) {
		mux_index = MM_FILE_FORMAT_WAV;
	} else if (!strcmp(name, "AVI")) {
		mux_index = MM_FILE_FORMAT_AVI;
	} else if (!strcmp(name, "WMA")) {
		mux_index = MM_FILE_FORMAT_WMA;
	} else if (!strcmp(name, "WMV")) {
		mux_index = MM_FILE_FORMAT_WMV;
	} else if (!strcmp(name, "MID")) {
		mux_index = MM_FILE_FORMAT_MID;
	} else if (!strcmp(name, "MMF")) {
		mux_index = MM_FILE_FORMAT_MMF;
	} else if (!strcmp(name, "MATROSKA")) {
		mux_index = MM_FILE_FORMAT_MATROSKA;
	} else if (!strcmp(name, "M2TS")) {
		mux_index = MM_FILE_FORMAT_M2TS;
	}

	/*_mmcam_dbg_log("mux index %d", mux_index);*/

	return mux_index;
}


int
_mmcamcorder_get_format(MMHandleType handle, int conf_category, const char *name)
{
	int fmt = -1;

	mmf_return_val_if_fail(name, -1);

	switch (conf_category) {
	case CONFIGURE_CATEGORY_MAIN_AUDIO_ENCODER:
		fmt = _mmcamcorder_get_audio_codec_format(handle, name);
		break;
	case CONFIGURE_CATEGORY_MAIN_VIDEO_ENCODER:
		fmt = _mmcamcorder_get_video_codec_format(handle, name);
		break;
	case CONFIGURE_CATEGORY_MAIN_IMAGE_ENCODER:
		fmt = _mmcamcorder_get_image_codec_format(handle, name);
		break;
	case CONFIGURE_CATEGORY_MAIN_MUX:
		fmt = _mmcamcorder_get_mux_format(handle, name);
		break;
	default:
		_mmcam_dbg_log("Can't get format from this category.(%d)", conf_category);
	}

	return fmt;
}

int
_mmcamcorder_get_available_format(MMHandleType handle, int conf_category, int ** format)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	camera_conf* configure_info = NULL;
	int *arr = NULL;
	int total_count = 0;

	mmf_return_val_if_fail(hcamcorder, 0);

	/*_mmcam_dbg_log("conf_category:%d", conf_category);*/

	configure_info = hcamcorder->conf_main;

	if (configure_info->info[conf_category]) {
		int i = 0;
		int fmt = 0;
		const char *name = NULL;
		int count = configure_info->info[conf_category]->count;
		conf_detail *info = configure_info->info[conf_category];

		/*_mmcam_dbg_log("count[%d], info[%p]", count, info);*/

		if (count <= 0 || !info)
			return total_count;

		arr = (int*) g_malloc0(count * sizeof(int));
		if (arr == NULL) {
			_mmcam_dbg_err("malloc failed : %d", count * sizeof(int));
			return 0;
		}

		for (i = 0 ; i < count ; i++) {
			if (info->detail_info[i] == NULL)
				continue;

			name = ((type_element*)(info->detail_info[i]))->name;
			fmt = _mmcamcorder_get_format(handle, conf_category, name);
			if (fmt >= 0)
				arr[total_count++] = fmt;

			/*_mmcam_dbg_log("name:%s, fmt:%d", name, fmt);*/
		}
	}

	*format = arr;

	return total_count;
}
