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

/**
	@addtogroup CAMCORDER
	@{

	@par
	This part describes the APIs with repect to Multimedia Camcorder.
	Camcorder is for recording audio and video from audio and video input devices, capturing
	still image from video input device, and audio recording from audio input
	device.

	@par
	Camcorder can be reached by calling functions as shown in the following
	figure, "The State of Camcorder".

	@par
	@image html	camcorder_state.png	"The State of Camcorder"	width=12cm
	@image latex	camcorder_state.png	"The State of Camcorder"	width=12cm

	@par
	Between each states there is intermediate state, and in this state,
	any function call which change the camcorder state will be failed.

	@par
	Recording state and paused state exists when the mode of camcorder is
	video-capture or audio-capture mode. In case of image-capture mode, CAPTURING state will
	exsit.

	@par
<div>
<table>
	<tr>
		<td>FUNCTION</td>
		<td>PRE-STATE</td>
		<td>POST-STATE</td>
		<td>SYNC TYPE</td>
	</tr>
	<tr>
		<td>mm_camcorder_create()</td>
		<td>NONE</td>
		<td>NULL</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_destroy()</td>
		<td>NULL</td>
		<td>NONE</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_realize()</td>
		<td>NULL</td>
		<td>READY</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_unrealize()</td>
		<td>READY</td>
		<td>NULL</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_start()</td>
		<td>READY</td>
		<td>PREPARED</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_stop()</td>
		<td>PREPARED</td>
		<td>READY</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_capture_start()</td>
		<td>PREPARED</td>
		<td>CAPTURING</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_capture_stop()</td>
		<td>CAPTURING</td>
		<td>PREPARED</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_record()</td>
		<td>PREPARED/PAUSED</td>
		<td>RECORDING</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_pause()</td>
		<td>RECORDING</td>
		<td>PAUSED</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_commit()</td>
		<td>RECORDING/PAUSED</td>
		<td>PREPARED</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_cancel()</td>
		<td>RECORDING/PAUSED</td>
		<td>PREPARED</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_set_message_callback()</td>
		<td>N/A</td>
		<td>N/A</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_set_video_stream_callback()</td>
		<td>N/A</td>
		<td>N/A</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_set_video_capture_callback()</td>
		<td>N/A</td>
		<td>N/A</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_get_state()</td>
		<td>N/A</td>
		<td>N/A</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_get_attributes()</td>
		<td>N/A</td>
		<td>N/A</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_set_attributes()</td>
		<td>N/A</td>
		<td>N/A</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_get_attribute_info()</td>
		<td>N/A</td>
		<td>N/A</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_init_focusing()</td>
		<td>N/A</td>
		<td>N/A</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_start_focusing()</td>
		<td>N/A</td>
		<td>N/A</td>
		<td>SYNC</td>
	</tr>
	<tr>
		<td>mm_camcorder_stop_focusing()</td>
		<td>N/A</td>
		<td>N/A</td>
		<td>SYNC</td>
	</tr>
</table>
</div>

	@par
	* Attribute @n
	Attribute system is an interface to operate camcorder. Depending on each attribute, camcorder behaves differently.
	Attribute system provides get/set functions. Setting proper attributes, a user can control camcorder as he want. (mm_camcorder_set_attributes())
	Also, a user can comprehend current status of the camcorder, calling getter function(mm_camcorder_get_attributes()).
	Beware, arguments of mm_camcorder_set_attributes() and mm_camcorder_get_attributes() should be finished with 'NULL'.
	This is a rule for the variable argument.
	@par
	Besides its value, each Attribute also has 'type' and 'validity type'. 'type' describes variable type that the attribute can get.
	If you input a value that has wrong type, camcorder will not work properly or be crashed. 'validity' describes array or
	range of values that are able to set to the attribute. 'validity type' defines type of the 'validity'.
	@par
	A user can retrieve these values using mm_camcorder_get_attribute_info().
	Following tables have 'Attribute name', 'Attribute macro', 'Type', and 'Validity type'. You can refer '#MMCamAttrsType' and '#MMCamAttrsValidType'
	for discerning 'Type' and 'Validity type'.


	@par
	Following are the attributes which should be set before initialization (#mm_camcorder_realize):

	@par
<div>
<table>
	<tr>
		<td><center><b>Attribute</b></center></td>
		<td><center><b>Description</b></center></td>
	</tr>
	<tr>
		<td>#MMCAM_MODE</td>
		<td>Mode of camcorder ( still/video/audio )</td>
	</tr>
	<tr>
		<td>#MMCAM_AUDIO_DEVICE</td>
		<td>Audio device ID for capturing audio stream</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_DEVICE_COUNT</td>
		<td>Video device count</td>
	</tr>
	<tr>
		<td>#MMCAM_AUDIO_ENCODER</td>
		<td>Audio codec for encoding audio stream</td>
	</tr>
	<tr>
		<td>#MMCAM_VIDEO_ENCODER</td>
		<td>Video codec for encoding video stream</td>
	</tr>
	<tr>
		<td>#MMCAM_IMAGE_ENCODER</td>
		<td>Image codec for capturing still-image</td>
	</tr>
	<tr>
		<td>#MMCAM_FILE_FORMAT</td>
		<td>File format for recording media stream</td>
	</tr>
	<tr>
		<td>#MMCAM_AUDIO_SAMPLERATE</td>
		<td>Sampling rate of audio stream ( This is an integer field )</td>
	</tr>
	<tr>
		<td>#MMCAM_AUDIO_FORMAT</td>
		<td>Audio format of each sample</td>
	</tr>
	<tr>
		<td>#MMCAM_AUDIO_CHANNEL</td>
		<td>Channels of each sample ( This is an integer field )</td>
	</tr>
	<tr>
		<td>#MMCAM_AUDIO_INPUT_ROUTE(deprecated)</td>
		<td>Set audio input route</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_FORMAT</td>
		<td>Format of video stream. This is an integer field</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_FPS</td>
		<td>Frames per second ( This is an integer field )</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_WIDTH</td>
		<td>Width of input video stream</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_HEIGHT</td>
		<td>Height of input video stream</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_FPS_AUTO</td>
		<td>FPS Auto. When you set true to this attribute, FPS will vary depending on the amount of the light.</td>
	</tr>
	<tr>
		<td>#MMCAM_DISPLAY_HANDLE</td>
		<td>Pointer of display buffer or ID of xwindow</td>
	</tr>
	<tr>
		<td>#MMCAM_DISPLAY_DEVICE</td>
		<td>Device of display</td>
	</tr>
	<tr>
		<td>#MMCAM_DISPLAY_SURFACE</td>
		<td>Surface of display</td>
	</tr>
	<tr>
		<td>#MMCAM_DISPLAY_SOURCE_X</td>
		<td>X position of source rectangle. When you want to crop the source, you can set the area with this value.</td>
	</tr>
	<tr>
		<td>#MMCAM_DISPLAY_SOURCE_Y</td>
		<td>Y position of source rectangle. When you want to crop the source, you can set the area with this value.</td>
	</tr>
	<tr>
		<td>#MMCAM_DISPLAY_SOURCE_WIDTH</td>
		<td>Width of source rectangle. When you want to crop the source, you can set the area with this value.</td>
	</tr>
	<tr>
		<td>#MMCAM_DISPLAY_SOURCE_HEIGHT</td>
		<td>Height of source rectangle. When you want to crop the source, you can set the area with this value.</td>
	</tr>
	<tr>
		<td>#MMCAM_DISPLAY_ROTATION</td>
		<td>Rotation of display</td>
	</tr>
	<tr>
		<td>#MMCAM_DISPLAY_VISIBLE</td>
		<td>Visible of display</td>
	</tr>
	<tr>
		<td>#MMCAM_DISPLAY_SCALE</td>
		<td>A scale of displayed image</td>
	</tr>
	<tr>
		<td>#MMCAM_DISPLAY_GEOMETRY_METHOD</td>
		<td>A method that describes a form of geometry for display</td>
	</tr>
</table>
</div>

	@par
	Following are the attributes which should be set before recording (mm_camcorder_record()):

	@par
<div>
<table>
	<tr>
		<td><center><b>Attribute</b></center></td>
		<td><center><b>Description</b></center></td>
	</tr>
	<tr>
		<td>#MMCAM_AUDIO_ENCODER_BITRATE</td>
		<td>Bitrate of Audio Encoder</td>
	</tr>
	<tr>
		<td>#MMCAM_VIDEO_ENCODER_BITRATE</td>
		<td>Bitrate of Video Encoder</td>
	</tr>
	<tr>
		<td>#MMCAM_TARGET_FILENAME</td>
		<td>Target filename. Only used in Audio/Video recording. This is not used for capturing.</td>
	</tr>
	<tr>
		<td>#MMCAM_TARGET_MAX_SIZE</td>
		<td>Maximum size of recording file(Kbyte). If the size of file reaches this value.</td>
	</tr>
	<tr>
		<td>#MMCAM_TARGET_TIME_LIMIT</td>
		<td>Time limit of recording file. If the elapsed time of recording reaches this value.</td>
	</tr>
</table>
</div>

	@par
	Following are the attributes which should be set before capturing (mm_camcorder_capture_start()):

	@par
<div>
<table>
	<tr>
		<td><center><b>Attribute</b></center></td>
		<td><center><b>Description</b></center></td>
	</tr>
	<tr>
		<td>#MMCAM_IMAGE_ENCODER_QUALITY</td>
		<td>Encoding quality of Image codec</td>
	</tr>
	<tr>
		<td>#MMCAM_CAPTURE_FORMAT</td>
		<td>Pixel format that you want to capture</td>
	</tr>
	<tr>
		<td>#MMCAM_CAPTURE_WIDTH</td>
		<td>Width of the image that you want to capture</td>
	</tr>
	<tr>
		<td>#MMCAM_CAPTURE_HEIGHT</td>
		<td>Height of the image that you want to capture</td>
	</tr>
	<tr>
		<td>#MMCAM_CAPTURE_COUNT</td>
		<td>Total count of capturing</td>
	</tr>
	<tr>
		<td>#MMCAM_CAPTURE_INTERVAL</td>
		<td>Interval between each capturing on Multishot ( MMCAM_CAPTURE_COUNT > 1 )</td>
	</tr>
</table>
</div>

	@par
	Following are the attributes which can be set anytime:

	@par
<div>
<table>
	<tr>
		<td><center><b>Attribute</b></center></td>
		<td><center><b>Description</b></center></td>
	</tr>
	<tr>
		<td>#MMCAM_AUDIO_VOLUME</td>
		<td>Input volume of audio source ( double value )</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_DIGITAL_ZOOM</td>
		<td>Digital zoom level</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_OPTICAL_ZOOM</td>
		<td>Optical zoom level</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_FOCUS_MODE</td>
		<td>Focus mode</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_AF_SCAN_RANGE</td>
		<td>AF Scan range</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_AF_TOUCH_X</td>
		<td>X coordinate of touching position</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_AF_TOUCH_Y</td>
		<td>Y coordinate of touching position</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_AF_TOUCH_WIDTH</td>
		<td>Width of touching area</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_AF_TOUCH_HEIGHT</td>
		<td>Height of touching area</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_EXPOSURE_MODE</td>
		<td>Exposure mode</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_EXPOSURE_VALUE</td>
		<td>Exposure value</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_F_NUMBER</td>
		<td>f number of camera</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_SHUTTER_SPEED</td>
		<td>Shutter speed</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_ISO</td>
		<td>ISO of capturing image</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_WDR</td>
		<td>Wide dynamic range</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_ANTI_HANDSHAKE</td>
		<td>Anti Handshake</td>
	</tr>
	<tr>
		<td>#MMCAM_CAMERA_FOCAL_LENGTH</td>
		<td>Focal length of camera lens</td>
	</tr>
	<tr>
		<td>#MMCAM_FILTER_BRIGHTNESS</td>
		<td>Brightness level</td>
	</tr>
	<tr>
		<td>#MMCAM_FILTER_CONTRAST</td>
		<td>Contrast level</td>
	</tr>
	<tr>
		<td>#MMCAM_FILTER_WB</td>
		<td>White balance</td>
	</tr>
	<tr>
		<td>#MMCAM_FILTER_COLOR_TONE</td>
		<td>Color tone (Color effect)</td>
	</tr>
	<tr>
		<td>#MMCAM_FILTER_SCENE_MODE</td>
		<td>Scene mode (Program mode)</td>
	</tr>
	<tr>
		<td>#MMCAM_FILTER_SATURATION</td>
		<td>Saturation level</td>
	</tr>
	<tr>
		<td>#MMCAM_FILTER_HUE</td>
		<td>Hue level</td>
	</tr>
	<tr>
		<td>#MMCAM_FILTER_SHARPNESS</td>
		<td>Sharpness level</td>
	</tr>
	<tr>
		<td>#MMCAM_CAPTURE_BREAK_CONTINUOUS_SHOT</td>
		<td>Set this as true when you want to stop multishot immediately</td>
	</tr>
	<tr>
		<td>#MMCAM_DISPLAY_RECT_X</td>
		<td>X position of display rectangle (This is only available when MMCAM_DISPLAY_GEOMETRY_METHOD is MM_CAMCORDER_CUSTOM_ROI)</td>
	</tr>
	<tr>
		<td>#MMCAM_DISPLAY_RECT_Y</td>
		<td>Y position of display rectangle (This is only available when MMCAM_DISPLAY_GEOMETRY_METHOD is MM_CAMCORDER_CUSTOM_ROI)</td>
	</tr>
	<tr>
		<td>#MMCAM_DISPLAY_RECT_WIDTH</td>
		<td>Width of display rectangle (This is only available when MMCAM_DISPLAY_GEOMETRY_METHOD is MM_CAMCORDER_CUSTOM_ROI)</td>
	</tr>
	<tr>
		<td>#MMCAM_DISPLAY_RECT_HEIGHT</td>
		<td>Height of display rectangle (This is only available when MMCAM_DISPLAY_GEOMETRY_METHOD is MM_CAMCORDER_CUSTOM_ROI)</td>
	</tr>
	<tr>
		<td>#MMCAM_TAG_ENABLE</td>
		<td>Enable to write tags (If this value is FALSE, none of tag information will be written to captured file)</td>
	</tr>
	<tr>
		<td>#MMCAM_TAG_IMAGE_DESCRIPTION</td>
		<td>Image description</td>
	</tr>
	<tr>
		<td>#MMCAM_TAG_ORIENTATION</td>
		<td>Orientation of captured image</td>
	</tr>
	<tr>
		<td>#MMCAM_TAG_VIDEO_ORIENTATION</td>
		<td>Orientation of encoded video</td>
	</tr>
	<tr>
		<td>#MMCAM_TAG_SOFTWARE</td>
		<td>software name and version</td>
	</tr>
	<tr>
		<td>#MMCAM_TAG_LATITUDE</td>
		<td>Latitude of captured postion (GPS information)</td>
	</tr>
	<tr>
		<td>#MMCAM_TAG_LONGITUDE</td>
		<td>Longitude of captured postion (GPS information)</td>
	</tr>
	<tr>
		<td>#MMCAM_TAG_ALTITUDE</td>
		<td>Altitude of captured postion (GPS information)</td>
	</tr>
	<tr>
		<td>#MMCAM_STROBE_CONTROL</td>
		<td>Strobe control</td>
	</tr>
	<tr>
		<td>#MMCAM_STROBE_MODE</td>
		<td>Operation Mode of strobe</td>
	</tr>
	<tr>
		<td>#MMCAM_DETECT_MODE</td>
		<td>Detection mode</td>
	</tr>
	<tr>
		<td>#MMCAM_DETECT_NUMBER</td>
		<td>Total number of detected object</td>
	</tr>
	<tr>
		<td>#MMCAM_DETECT_FOCUS_SELECT</td>
		<td>Select one of detected objects</td>
	</tr>
</table>
</div>
 */



#ifndef __MM_CAMCORDER_H__
#define __MM_CAMCORDER_H__


/*=======================================================================================
| INCLUDE FILES										|
========================================================================================*/
#include <glib.h>

#include <mm_types.h>
#include <mm_error.h>
#include <mm_message.h>

#ifdef __cplusplus
extern "C" {
#endif

/*=======================================================================================
| GLOBAL DEFINITIONS AND DECLARATIONS FOR CAMCORDER					|
========================================================================================*/

/*=======================================================================================
| MACRO DEFINITIONS									|
========================================================================================*/
/**
 * Get numerator. Definition for fraction setting, such as MMCAM_CAMERA_SHUTTER_SPEED and MMCAM_CAMERA_EXPOSURE_VALUE.
 */
#define MM_CAMCORDER_GET_NUMERATOR(x)					((int)(((int)(x) >> 16) & 0xFFFF))
/**
 * Get denominator. Definition for fraction setting, such as MMCAM_CAMERA_SHUTTER_SPEED and MMCAM_CAMERA_EXPOSURE_VALUE.
 */
#define MM_CAMCORDER_GET_DENOMINATOR(x)					((int)(((int)(x)) & 0xFFFF))
/**
 * Set fraction value. Definition for fraction setting, such as MMCAM_CAMERA_SHUTTER_SPEED and MMCAM_CAMERA_EXPOSURE_VALUE.
 */
#define MM_CAMCORDER_SET_FRACTION(numerator, denominator)	((int)((((int)(numerator)) << 16) | (int)(denominator)))

/* Attributes Macros */
/**
 * Mode of camcorder (still/video/audio).
 * @see		MMCamcorderModeType
 */
#define MMCAM_MODE                              "mode"

/**
 * Audio device ID for capturing audio stream.
 * @see		MMAudioDeviceType (in mm_types.h)
 */
#define MMCAM_AUDIO_DEVICE                      "audio-device"

/**
 * Video device count.
 */
#define MMCAM_CAMERA_DEVICE_COUNT               "camera-device-count"

/**
 * Facing direction of camera device.
 * @see		MMCamcorderCameraFacingDirection
 */
#define MMCAM_CAMERA_FACING_DIRECTION           "camera-facing-direction"

/**
 * Audio codec for encoding audio stream.
 * @see		MMAudioCodecType  (in mm_types.h)
 */
#define MMCAM_AUDIO_ENCODER                     "audio-encoder"

/**
 * Video codec for encoding video stream.
 * @see		MMVideoCodecType (in mm_types.h)
 */
#define MMCAM_VIDEO_ENCODER                     "video-encoder"

/**
 * Image codec for capturing still-image.
 * @see		MMImageCodecType (in mm_types.h)
 */
#define MMCAM_IMAGE_ENCODER                     "image-encoder"

/**
 * File format for recording media stream.
 * @see		MMFileFormatType (in mm_types.h)
 */
#define MMCAM_FILE_FORMAT                       "file-format"

/**
 * Sampling rate of audio stream. This is an integer field.
 */
#define MMCAM_AUDIO_SAMPLERATE                  "audio-samplerate"

/**
 * Audio format of each sample.
 * @see		MMCamcorderAudioFormat
 */
#define MMCAM_AUDIO_FORMAT                      "audio-format"

/**
 * Channels of each sample. This is an integer field.
 */
#define MMCAM_AUDIO_CHANNEL                     "audio-channel"

/**
 * Input volume of audio source. Double value.
 */
#define MMCAM_AUDIO_VOLUME                      "audio-volume"

/**
 * Disable Audio stream when record.
 */
#define MMCAM_AUDIO_DISABLE                     "audio-disable"

/**
 * Set audio input route
 * @remarks	Deprecated. This will be removed soon.
 * @see		MMAudioRoutePolicy (in mm_types.h)
 */
#define MMCAM_AUDIO_INPUT_ROUTE                 "audio-input-route"

/**
 * Format of video stream. This is an integer field
 * @see		MMPixelFormatType (in mm_types.h)
 */
#define MMCAM_CAMERA_FORMAT                     "camera-format"

/**
 * Slow motion rate when video recording
 * @remarks	Deprecated
 */
#define MMCAM_CAMERA_SLOW_MOTION_RATE           "camera-slow-motion-rate"

/**
 * Motion rate when video recording
 * @remarks	This should be bigger than 0(zero).
 *		Default value is 1 and it's for normal video recording.
 *		If the value is smaller than 1, recorded file will be played slower,
 *		otherwise, recorded file will be played faster.
 */
#define MMCAM_CAMERA_RECORDING_MOTION_RATE      "camera-recording-motion-rate"

/**
 * Frames per second. This is an integer field
 *
 */
#define MMCAM_CAMERA_FPS                        "camera-fps"

/**
 * Width of preview stream.
 */
#define MMCAM_CAMERA_WIDTH                      "camera-width"

/**
 * Height of preview stream.
 */
#define MMCAM_CAMERA_HEIGHT                     "camera-height"

/**
 * Width of video stream.
 */
#define MMCAM_VIDEO_WIDTH                       "video-width"

/**
 * Height of video stream.
 */
#define MMCAM_VIDEO_HEIGHT                      "video-height"

/**
 * Digital zoom level.
 */
#define MMCAM_CAMERA_DIGITAL_ZOOM               "camera-digital-zoom"

/**
 * Optical zoom level.
 */
#define MMCAM_CAMERA_OPTICAL_ZOOM               "camera-optical-zoom"

/**
 * Focus mode
 * @see		MMCamcorderFocusMode
 */
#define MMCAM_CAMERA_FOCUS_MODE                 "camera-focus-mode"

/**
 * AF Scan range
 * @see		MMCamcorderAutoFocusType
 */
#define MMCAM_CAMERA_AF_SCAN_RANGE              "camera-af-scan-range"

/**
 * X coordinate of touching position. Only available when you set '#MM_CAMCORDER_AUTO_FOCUS_TOUCH' to '#MMCAM_CAMERA_AF_SCAN_RANGE'.
 * @see		MMCamcorderAutoFocusType
 */
#define MMCAM_CAMERA_AF_TOUCH_X                 "camera-af-touch-x"

/**
 * Y coordinate of touching position. Only available when you set '#MM_CAMCORDER_AUTO_FOCUS_TOUCH' to '#MMCAM_CAMERA_AF_SCAN_RANGE'.
 * @see		MMCamcorderAutoFocusType
 */
#define MMCAM_CAMERA_AF_TOUCH_Y                 "camera-af-touch-y"

/**
 * Width of touching area. Only available when you set '#MM_CAMCORDER_AUTO_FOCUS_TOUCH' to '#MMCAM_CAMERA_AF_SCAN_RANGE'.
 * @see		MMCamcorderAutoFocusType
 */
#define MMCAM_CAMERA_AF_TOUCH_WIDTH             "camera-af-touch-width"

/**
 * Height of touching area. Only available when you set '#MM_CAMCORDER_AUTO_FOCUS_TOUCH' to '#MMCAM_CAMERA_AF_SCAN_RANGE'.
 * @see		MMCamcorderAutoFocusType
 */
#define MMCAM_CAMERA_AF_TOUCH_HEIGHT            "camera-af-touch-height"

/**
 * Exposure mode
 * @see		MMCamcorderAutoExposureType
 */
#define MMCAM_CAMERA_EXPOSURE_MODE              "camera-exposure-mode"

/**
 * Exposure value
 */
#define MMCAM_CAMERA_EXPOSURE_VALUE             "camera-exposure-value"

/**
 * f number of camera
 */
#define MMCAM_CAMERA_F_NUMBER                   "camera-f-number"

/**
 * Shutter speed
 */
#define MMCAM_CAMERA_SHUTTER_SPEED              "camera-shutter-speed"

/**
 * ISO of capturing image
 * @see		MMCamcorderISOType
 */
#define MMCAM_CAMERA_ISO                        "camera-iso"

/**
 * Wide dynamic range.
 * @see		MMCamcorderWDRMode
 */
#define MMCAM_CAMERA_WDR                        "camera-wdr"

/**
 * Focal length of camera lens.
 */
#define MMCAM_CAMERA_FOCAL_LENGTH               "camera-focal-length"

/**
 * Anti Handshake
 * @see		MMCamcorderAHSMode
 */
#define MMCAM_CAMERA_ANTI_HANDSHAKE             "camera-anti-handshake"

/**
 * Video Stabilization
 * @see		MMCamcorderVideoStabilizationMode
 */
#define MMCAM_CAMERA_VIDEO_STABILIZATION        "camera-video-stabilization"

/**
 * FPS Auto. When you set true to this attribute, FPS will vary depending on the amount of the light.
 */
#define MMCAM_CAMERA_FPS_AUTO                   "camera-fps-auto"

/**
 * Rotation angle of video input stream.
 * @see		MMVideoInputRotationType (in mm_types.h)
 */
#define MMCAM_CAMERA_ROTATION                   "camera-rotation"

/**
 * HDR(High Dynamic Range) Capture mode
 * @see		MMCamcorderHDRMode
 */
#define MMCAM_CAMERA_HDR_CAPTURE                "camera-hdr-capture"

/**
 * Bitrate of Audio Encoder
 */
#define MMCAM_AUDIO_ENCODER_BITRATE             "audio-encoder-bitrate"

/**
 * Bitrate of Video Encoder
 */
#define MMCAM_VIDEO_ENCODER_BITRATE             "video-encoder-bitrate"

/**
 * Encoding quality of Image codec
 */
#define MMCAM_IMAGE_ENCODER_QUALITY             "image-encoder-quality"

/**
 * Brightness level
 */
#define MMCAM_FILTER_BRIGHTNESS                 "filter-brightness"

/**
 * Contrast level
 */
#define MMCAM_FILTER_CONTRAST                   "filter-contrast"

/**
 * White balance
 * @see		MMCamcorderWhiteBalanceType
 */
#define MMCAM_FILTER_WB                         "filter-wb"

/**
 * Color tone. (Color effect)
 * @see		MMCamcorderColorToneType
 */
#define MMCAM_FILTER_COLOR_TONE                 "filter-color-tone"

/**
 * Scene mode (Program mode)
 * @see		MMCamcorderSceneModeType
 */
#define MMCAM_FILTER_SCENE_MODE                 "filter-scene-mode"

/**
 * Saturation  level
 */
#define MMCAM_FILTER_SATURATION                 "filter-saturation"

/**
 * Hue  level
 */
#define MMCAM_FILTER_HUE                        "filter-hue"

/**
 * Sharpness  level
 */
#define MMCAM_FILTER_SHARPNESS                  "filter-sharpness"

/**
 * Pixel format that you want to capture. If you set MM_PIXEL_FORMAT_ENCODED,
 * the result will be encoded by image codec specified in #MMCAM_IMAGE_ENCODER.
 * If not, the result will be raw data.
 *
 * @see		MMPixelFormatType (in mm_types.h)
 */
#define MMCAM_CAPTURE_FORMAT                    "capture-format"

/**
 * Width of the image that you want to capture
 */
#define MMCAM_CAPTURE_WIDTH                     "capture-width"

/**
 * Height of the image that you want to capture

 */
#define MMCAM_CAPTURE_HEIGHT                    "capture-height"

/**
 * Total count of capturing. If you set this, it will caputre multiple time.
 */
#define MMCAM_CAPTURE_COUNT                     "capture-count"

/**
 * Interval between each capturing on Multishot.
 */
#define MMCAM_CAPTURE_INTERVAL                  "capture-interval"

/**
 * Set this when you want to stop multishot immediately.
 */
#define MMCAM_CAPTURE_BREAK_CONTINUOUS_SHOT     "capture-break-cont-shot"

/**
 * Raw data of captured image which resolution is same as preview.
 * This is READ-ONLY attribute and only available in capture callback.
 * This should be used after casted as MMCamcorderCaptureDataType.
 */
#define MMCAM_CAPTURED_SCREENNAIL               "captured-screennail"

/**
 * Raw data of EXIF. This is READ-ONLY attribute and only available in capture callback.
 */
#define MMCAM_CAPTURED_EXIF_RAW_DATA            "captured-exif-raw-data"

/**
 * Pointer of display buffer or ID of xwindow.
 */
#define MMCAM_DISPLAY_HANDLE                    "display-handle"

/**
 * Device of display.
 * @see		MMDisplayDeviceType (in mm_types.h)
 */
#define MMCAM_DISPLAY_DEVICE                    "display-device"

/**
 * Surface of display.
 * @see		MMDisplaySurfaceType (in mm_types.h)
 */
#define MMCAM_DISPLAY_SURFACE                    "display-surface"

/**
 * Mode of display.
 * @see		MMDisplayModeType (in mm_types.h)
 */
#define MMCAM_DISPLAY_MODE                       "display-mode"

/**
 * X position of display rectangle.
 * This is only available when #MMCAM_DISPLAY_GEOMETRY_METHOD is MM_CAMCORDER_CUSTOM_ROI.
 * @see		MMCamcorderGeometryMethod
 */
#define MMCAM_DISPLAY_RECT_X                    "display-rect-x"

/**
 * Y position of display rectangle
 * This is only available when #MMCAM_DISPLAY_GEOMETRY_METHOD is MM_CAMCORDER_CUSTOM_ROI.
 * @see		MMCamcorderGeometryMethod
 */
#define MMCAM_DISPLAY_RECT_Y                    "display-rect-y"

/**
 * Width of display rectangle
 * This is only available when #MMCAM_DISPLAY_GEOMETRY_METHOD is MM_CAMCORDER_CUSTOM_ROI.
 * @see		MMCamcorderGeometryMethod
 */
#define MMCAM_DISPLAY_RECT_WIDTH                "display-rect-width"

/**
 * Height of display rectangle
 * This is only available when #MMCAM_DISPLAY_GEOMETRY_METHOD is MM_CAMCORDER_CUSTOM_ROI.
 * @see		MMCamcorderGeometryMethod
 */
#define MMCAM_DISPLAY_RECT_HEIGHT               "display-rect-height"

/**
 * X position of source rectangle. When you want to crop the source, you can set the area with this value.
 */
#define MMCAM_DISPLAY_SOURCE_X                  "display-src-x"

/**
 * Y position of source rectangle. When you want to crop the source, you can set the area with this value.
 */
#define MMCAM_DISPLAY_SOURCE_Y                  "display-src-y"

/**
 * Width of source rectangle. When you want to crop the source, you can set the area with this value.
 */
#define MMCAM_DISPLAY_SOURCE_WIDTH              "display-src-width"

/**
 * Height of source rectangle. When you want to crop the source, you can set the area with this value.
 */
#define MMCAM_DISPLAY_SOURCE_HEIGHT             "display-src-height"

/**
 * Rotation angle of display.
 * @see		MMDisplayRotationType (in mm_types.h)
 */
#define MMCAM_DISPLAY_ROTATION                  "display-rotation"

/**
 * Flip of display.
 * @see		MMFlipType (in mm_types.h)
 */
#define MMCAM_DISPLAY_FLIP                      "display-flip"

/**
 * Visible of display.
 */
#define MMCAM_DISPLAY_VISIBLE                   "display-visible"

/**
 * A scale of displayed image. Available value is like below.
 * @see		MMDisplayScaleType (in mm_types.h)
 */
#define MMCAM_DISPLAY_SCALE                     "display-scale"

/**
 * A method that describes a form of geometry for display.
 * @see		MMCamcorderGeometryMethod
 */
#define MMCAM_DISPLAY_GEOMETRY_METHOD           "display-geometry-method"

/**
 * A videosink name of evas surface.
 * This is READ-ONLY attribute.
 */
#define MMCAM_DISPLAY_EVAS_SURFACE_SINK         "display-evas-surface-sink"

/**
 * This attribute is only available if value of MMCAM_DISPLAY_EVAS_SURFACE_SINK "evaspixmapsink"
 */
#define MMCAM_DISPLAY_EVAS_DO_SCALING           "display-evas-do-scaling"

/**
 * Target filename. Only used in Audio/Video recording. This is not used for capturing.
 */
#define MMCAM_TARGET_FILENAME                   "target-filename"

/**
 * Maximum size(Kbyte) of recording file. If the size of file reaches this value,
 * camcorder will send 'MM_MESSAGE_CAMCORDER_MAX_SIZE' message.
 */
#define MMCAM_TARGET_MAX_SIZE                   "target-max-size"

/**
 * Time limit(Second) of recording file. If the elapsed time of recording reaches this value,
 * camcorder will send 'MM_MESSAGE_CAMCORDER_TIME_LIMIT' message.
 */
#define MMCAM_TARGET_TIME_LIMIT                 "target-time-limit"

/**
 * Enable to write tags. If this value is FALSE, none of tag information will be written to captured file.
 */
#define MMCAM_TAG_ENABLE                        "tag-enable"

/**
 * Image description.
 */
#define MMCAM_TAG_IMAGE_DESCRIPTION             "tag-image-description"

/**
 * Orientation of captured image
 * @see		MMCamcorderTagOrientation
 */
#define MMCAM_TAG_ORIENTATION                   "tag-orientation"

/**
 * Orientation of captured video
 * @see		MMCamcorderTagVideoOrientation
 */
#define MMCAM_TAG_VIDEO_ORIENTATION            "tag-video-orientation"

/**
 * software name and version
 */
#define MMCAM_TAG_SOFTWARE                      "tag-software"

/**
 * Enable to write tags related to GPS. If this value is TRUE, tags related GPS information will be written to captured file.
 */
#define MMCAM_TAG_GPS_ENABLE                    "tag-gps-enable"

/**
 * Latitude of captured postion. GPS information.
 */
#define MMCAM_TAG_LATITUDE                      "tag-latitude"

/**
 * Longitude of captured postion. GPS information.
 */
#define MMCAM_TAG_LONGITUDE                     "tag-longitude"

/**
 * Altitude of captured postion. GPS information.
 */
#define MMCAM_TAG_ALTITUDE                      "tag-altitude"

/**
 * Strobe control
 * @see		MMCamcorderStrobeControl
 */
#define MMCAM_STROBE_CONTROL                    "strobe-control"

/**
 * Operation Mode of strobe
 * @see		MMCamcorderStrobeMode
 */
#define MMCAM_STROBE_MODE                       "strobe-mode"

/**
 * Detection mode
 * @see		MMCamcorderDetectMode
 */
#define MMCAM_DETECT_MODE                       "detect-mode"

/**
 * Total number of detected object
 */
#define MMCAM_DETECT_NUMBER                     "detect-number"

/**
 * You can use this attribute to select one of detected objects.
 */
#define MMCAM_DETECT_FOCUS_SELECT               "detect-focus-select"

/**
 * Recommend preview format for capture
 */
#define MMCAM_RECOMMEND_PREVIEW_FORMAT_FOR_CAPTURE     "recommend-preview-format-for-capture"

/**
 * Recommend preview format for recording
 */
#define MMCAM_RECOMMEND_PREVIEW_FORMAT_FOR_RECORDING   "recommend-preview-format-for-recording"

/**
 * Recommend rotation of display
 */
#define MMCAM_RECOMMEND_DISPLAY_ROTATION        "recommend-display-rotation"

/**
 * Recommend width of camera preview.
 * This attribute can be used with #mm_camcorder_get_attribute_info and #MMCamcorderPreviewType.
 * @see		mm_camcorder_get_attribute_info, MMCamcorderPreviewType
 */
#define MMCAM_RECOMMEND_CAMERA_WIDTH            "recommend-camera-width"

/**
 * Recommend height of camera preview
 * This attribute can be used with #mm_camcorder_get_attribute_info and #MMCamcorderPreviewType.
 * @see		mm_camcorder_get_attribute_info, MMCamcorderPreviewType
 */
#define MMCAM_RECOMMEND_CAMERA_HEIGHT           "recommend-camera-height"

/**
 * Enable to play capture sound
 */
#define MMCAM_CAPTURE_SOUND_ENABLE              "capture-sound-enable"

/**
 * Flip of video input stream.
 * @see		MMFlipType (in mm_types.h)
 */
#define MMCAM_CAMERA_FLIP                       "camera-flip"

/**
 * Support Zero Shutter Lag capture
 */
#define MMCAM_SUPPORT_ZSL_CAPTURE               "support-zsl-capture"

/**
* Support zero copy format
*/
#define MMCAM_SUPPORT_ZERO_COPY_FORMAT          "support-zero-copy-format"

/**
* Support media packet callback
*/
#define MMCAM_SUPPORT_MEDIA_PACKET_PREVIEW_CB   "support-media-packet-preview-cb"

/**
* Support user buffer for zero copy
*/
#define MMCAM_SUPPORT_USER_BUFFER               "support-user-buffer"

/**
* Buffer fd from user
*/
#define MMCAM_USER_BUFFER_FD                    "user-buffer-fd"

/**
 * Enable to write tags for recorded file
 */
#define MMCAM_RECORDER_TAG_ENABLE               "recorder-tag-enable"

/**
 * Determines the socket stream path
 */
#define MMCAM_DISPLAY_SOCKET_PATH               "display-socket-path"

/**
 * Client PID
 */
#define MMCAM_CLIENT_PID                        "client-pid"

/**
 * Root directory of application
 */
#define MMCAM_ROOT_DIRECTORY                    "root-directory"

/**
 * Bitrate for encoded preview stream
 */
#define MMCAM_ENCODED_PREVIEW_BITRATE           "encoded-preview-bitrate"

/**
 * GOP interval for encoded preview stream
 */
#define MMCAM_ENCODED_PREVIEW_GOP_INTERVAL      "encoded-preview-gop-interval"

/**
 * The distance to move the camera horizontally and physically
 */
#define MMCAM_CAMERA_PAN_MECHA                  "camera-pan-mecha"

/**
 * The distance to move the camera horizontally
 */
#define MMCAM_CAMERA_PAN_ELEC                   "camera-pan-elec"

/**
 * The distance to move the camera vertically and physically
 */
#define MMCAM_CAMERA_TILT_MECHA                 "camera-tilt-mecha"

/**
 * The distance to move the camera vertically
 */
#define MMCAM_CAMERA_TILT_ELEC                  "camera-tilt-elec"

/**
 * The type of PTZ(Pan Tilt Zoom). Mechanical or Electonic PTZ.
 */
#define MMCAM_CAMERA_PTZ_TYPE                   "camera-ptz-type"

/**
 * Stream type and index for sound route
 */
#define MMCAM_SOUND_STREAM_TYPE                 "sound-stream-type"
#define MMCAM_SOUND_STREAM_INDEX                "sound-stream-index"

/**
 * The display reuse flag and sink element pointer
 */
#define MMCAM_DISPLAY_REUSE_HINT                "display-reuse-hint"
#define MMCAM_DISPLAY_REUSE_ELEMENT             "display-reuse-element"

/**
 * The GBbus connection from outside
 */
#define MMCAM_GDBUS_CONNECTION                  "gdbus-connection"

/**
 * Replay gain enable
 */
#define MMCAM_AUDIO_REPLAY_GAIN_ENABLE          "audio-replay-gain-enable"

/**
 * Reference level for replay gain
 */
#define MMCAM_AUDIO_REPLAY_GAIN_REFERENCE_LEVEL "audio-replay-gain-reference-level"

/*=======================================================================================
| ENUM DEFINITIONS									|
========================================================================================*/
/**
 * An enumeration for camcorder states.
 */
typedef enum {
	MM_CAMCORDER_STATE_NONE,		/**< Camcorder is not created yet */
	MM_CAMCORDER_STATE_NULL,		/**< Camcorder is created, but not initialized yet */
	MM_CAMCORDER_STATE_READY,		/**< Camcorder is ready to capture */
	MM_CAMCORDER_STATE_PREPARE,		/**< Camcorder is prepared to capture (Preview) */
	MM_CAMCORDER_STATE_CAPTURING,		/**< Camcorder is now capturing still images */
	MM_CAMCORDER_STATE_RECORDING,		/**< Camcorder is now recording */
	MM_CAMCORDER_STATE_PAUSED,		/**< Camcorder is paused while recording */
	MM_CAMCORDER_STATE_NUM,			/**< Number of camcorder states */
} MMCamcorderStateType;

/**
 * An enumeration for camcorder mode.
 */
typedef enum {
	MM_CAMCORDER_MODE_VIDEO_CAPTURE = 0,    /**< Video recording and Image capture mode */
	MM_CAMCORDER_MODE_AUDIO,                /**< Audio recording mode */
} MMCamcorderModeType;

/**
 * An enumeration for facing direction.
 */
typedef enum {
	MM_CAMCORDER_CAMERA_FACING_DIRECTION_REAR = 0, /**< Facing direction of camera is REAR */
	MM_CAMCORDER_CAMERA_FACING_DIRECTION_FRONT,    /**< Facing direction of camera is FRONT */
} MMCamcorderCameraFacingDirection;


/**
 * An enumeration of Audio Format.
 */
typedef enum {
	MM_CAMCORDER_AUDIO_FORMAT_PCM_U8 = 0,		/**< unsigned 8bit audio */
	MM_CAMCORDER_AUDIO_FORMAT_PCM_S16_LE = 2,	/**< signed 16bit audio. Little endian. */
} MMCamcorderAudioFormat;


/**
 * An enumeration for color tone. Color tone provides an impression of
 * seeing through a tinted glass.
 */
enum MMCamcorderColorToneType {
	MM_CAMCORDER_COLOR_TONE_NONE = 0,               /**< None */
	MM_CAMCORDER_COLOR_TONE_MONO,                   /**< Mono */
	MM_CAMCORDER_COLOR_TONE_SEPIA,                  /**< Sepia */
	MM_CAMCORDER_COLOR_TONE_NEGATIVE,               /**< Negative */
	MM_CAMCORDER_COLOR_TONE_BLUE,                   /**< Blue */
	MM_CAMCORDER_COLOR_TONE_GREEN,                  /**< Green */
	MM_CAMCORDER_COLOR_TONE_AQUA,                   /**< Aqua */
	MM_CAMCORDER_COLOR_TONE_VIOLET,                 /**< Violet */
	MM_CAMCORDER_COLOR_TONE_ORANGE,                 /**< Orange */
	MM_CAMCORDER_COLOR_TONE_GRAY,                   /**< Gray */
	MM_CAMCORDER_COLOR_TONE_RED,                    /**< Red */
	MM_CAMCORDER_COLOR_TONE_ANTIQUE,                /**< Antique */
	MM_CAMCORDER_COLOR_TONE_WARM,                   /**< Warm */
	MM_CAMCORDER_COLOR_TONE_PINK,                   /**< Pink */
	MM_CAMCORDER_COLOR_TONE_YELLOW,                 /**< Yellow */
	MM_CAMCORDER_COLOR_TONE_PURPLE,                 /**< Purple */
	MM_CAMCORDER_COLOR_TONE_EMBOSS,                 /**< Emboss */
	MM_CAMCORDER_COLOR_TONE_OUTLINE,                /**< Outline */
	MM_CAMCORDER_COLOR_TONE_SOLARIZATION,           /**< Solarization */
	MM_CAMCORDER_COLOR_TONE_SKETCH,                 /**< Sketch */
	MM_CAMCORDER_COLOR_TONE_WASHED,                 /**< Washed */
	MM_CAMCORDER_COLOR_TONE_VINTAGE_WARM,           /**< Vintage warm */
	MM_CAMCORDER_COLOR_TONE_VINTAGE_COLD,           /**< Vintage cold */
	MM_CAMCORDER_COLOR_TONE_POSTERIZATION,          /**< Posterization */
	MM_CAMCORDER_COLOR_TONE_CARTOON,                /**< Cartoon */
	MM_CAMCORDER_COLOR_TONE_SELECTIVE_RED,          /**< Selective color - Red */
	MM_CAMCORDER_COLOR_TONE_SELECTIVE_GREEN,        /**< Selective color - Green */
	MM_CAMCORDER_COLOR_TONE_SELECTIVE_BLUE,         /**< Selective color - Blue */
	MM_CAMCORDER_COLOR_TONE_SELECTIVE_YELLOW,       /**< Selective color - Yellow */
	MM_CAMCORDER_COLOR_TONE_SELECTIVE_RED_YELLOW,   /**< Selective color - Red and Yellow */
};


/**
 * An enumeration for white balance. White Balance is the control that adjusts
 * the camcorder's color sensitivity to match the prevailing color of white
 * outdoor light, yellower indoor light, or (sometimes) greenish fluorescent
 * light. White balance may be set either automatically or manually. White balance
 * may be set "incorrectly" on purpose to achieve special effects.
 */
enum MMCamcorderWhiteBalanceType {
	MM_CAMCORDER_WHITE_BALANCE_NONE = 0,		/**< None */
	MM_CAMCORDER_WHITE_BALANCE_AUTOMATIC,		/**< Automatic */
	MM_CAMCORDER_WHITE_BALANCE_DAYLIGHT,		/**< Daylight */
	MM_CAMCORDER_WHITE_BALANCE_CLOUDY,		/**< Cloudy */
	MM_CAMCORDER_WHITE_BALANCE_FLUOROSCENT,		/**< Fluorescent */
	MM_CAMCORDER_WHITE_BALANCE_INCANDESCENT,	/**< Incandescent */
	MM_CAMCORDER_WHITE_BALANCE_SHADE,		/**< Shade */
	MM_CAMCORDER_WHITE_BALANCE_HORIZON,		/**< Horizon */
	MM_CAMCORDER_WHITE_BALANCE_FLASH,		/**< Flash */
	MM_CAMCORDER_WHITE_BALANCE_CUSTOM,		/**< Custom */
};


/**
 * An enumeration for scene mode. Scene mode gives the environment condition
 * for operating camcorder. The mode of operation can be in daylight, night and
 * backlight. It can be an automatic setting also.
 */
enum MMCamcorderSceneModeType {
	MM_CAMCORDER_SCENE_MODE_NORMAL = 0,     /**< Normal */
	MM_CAMCORDER_SCENE_MODE_PORTRAIT,       /**< Portrait */
	MM_CAMCORDER_SCENE_MODE_LANDSCAPE,      /**< Landscape */
	MM_CAMCORDER_SCENE_MODE_SPORTS,         /**< Sports */
	MM_CAMCORDER_SCENE_MODE_PARTY_N_INDOOR, /**< Party & indoor */
	MM_CAMCORDER_SCENE_MODE_BEACH_N_INDOOR, /**< Beach & indoor */
	MM_CAMCORDER_SCENE_MODE_SUNSET,         /**< Sunset */
	MM_CAMCORDER_SCENE_MODE_DUSK_N_DAWN,    /**< Dusk & dawn */
	MM_CAMCORDER_SCENE_MODE_FALL_COLOR,     /**< Fall */
	MM_CAMCORDER_SCENE_MODE_NIGHT_SCENE,    /**< Night scene */
	MM_CAMCORDER_SCENE_MODE_FIREWORK,       /**< Firework */
	MM_CAMCORDER_SCENE_MODE_TEXT,           /**< Text */
	MM_CAMCORDER_SCENE_MODE_SHOW_WINDOW,    /**< Show window */
	MM_CAMCORDER_SCENE_MODE_CANDLE_LIGHT,   /**< Candle light */
	MM_CAMCORDER_SCENE_MODE_BACKLIGHT,      /**< Backlight */
	MM_CAMCORDER_SCENE_MODE_AQUA,           /**< Aqua */
};


/**
 * An enumeration for focusing .
 */
enum MMCamcorderFocusMode {
	MM_CAMCORDER_FOCUS_MODE_NONE = 0,	/**< Focus mode is None */
	MM_CAMCORDER_FOCUS_MODE_PAN,		/**< Pan focus mode*/
	MM_CAMCORDER_FOCUS_MODE_AUTO,		/**< Autofocus mode*/
	MM_CAMCORDER_FOCUS_MODE_MANUAL,		/**< Manual focus mode*/
	MM_CAMCORDER_FOCUS_MODE_TOUCH_AUTO,	/**< Touch Autofocus mode*/
	MM_CAMCORDER_FOCUS_MODE_CONTINUOUS,	/**< Continuous Autofocus mode*/
};


/**
 * An enumeration for auto focus scan range (af scan range)
 */
enum MMCamcorderAutoFocusType {
	MM_CAMCORDER_AUTO_FOCUS_NONE = 0,	/**< Scan autofocus is not set */
	MM_CAMCORDER_AUTO_FOCUS_NORMAL,		/**< Scan autofocus normally*/
	MM_CAMCORDER_AUTO_FOCUS_MACRO,		/**< Scan autofocus in macro mode(close distance)*/
	MM_CAMCORDER_AUTO_FOCUS_FULL,		/**< Scan autofocus in full mode(all range scan, limited by dev spec)*/
};


/**
 * An enumeration for focus state.
 * When 'MM_MESSAGE_CAMCORDER_FOCUS_CHANGED' is delievered through 'MMMessageCallback',
 * this enumeration will be set to 'code' of MMMessageParamType.
 */
enum MMCamcorderFocusStateType {
	MM_CAMCORDER_FOCUS_STATE_RELEASED = 0,	/**< Focus released.*/
	MM_CAMCORDER_FOCUS_STATE_ONGOING,	/**< Focus in pregress*/
	MM_CAMCORDER_FOCUS_STATE_FOCUSED,	/**< Focus success*/
	MM_CAMCORDER_FOCUS_STATE_FAILED,	/**< Focus failed*/
};


/**
 * An enumeration for ISO.
 */
enum MMCamcorderISOType {
	MM_CAMCORDER_ISO_AUTO = 0,		/**< ISO auto mode*/
	MM_CAMCORDER_ISO_50,			/**< ISO 50*/
	MM_CAMCORDER_ISO_100,			/**< ISO 100*/
	MM_CAMCORDER_ISO_200,			/**< ISO 200*/
	MM_CAMCORDER_ISO_400,			/**< ISO 400*/
	MM_CAMCORDER_ISO_800,			/**< ISO 800*/
	MM_CAMCORDER_ISO_1600,			/**< ISO 1600*/
	MM_CAMCORDER_ISO_3200,			/**< ISO 3200*/
	MM_CAMCORDER_ISO_6400,			/**< ISO 6400*/
	MM_CAMCORDER_ISO_12800,			/**< ISO 12800*/
};

/**
 * An enumeration for Automatic exposure.
 */
enum MMCamcorderAutoExposureType {
	MM_CAMCORDER_AUTO_EXPOSURE_OFF = 0,	/**< AE off*/
	MM_CAMCORDER_AUTO_EXPOSURE_ALL,		/**< AE on, XXX mode*/
	MM_CAMCORDER_AUTO_EXPOSURE_CENTER_1,	/**< AE on, XXX mode*/
	MM_CAMCORDER_AUTO_EXPOSURE_CENTER_2,	/**< AE on, XXX mode*/
	MM_CAMCORDER_AUTO_EXPOSURE_CENTER_3,	/**< AE on, XXX mode*/
	MM_CAMCORDER_AUTO_EXPOSURE_SPOT_1,	/**< AE on, XXX mode*/
	MM_CAMCORDER_AUTO_EXPOSURE_SPOT_2,	/**< AE on, XXX mode*/
	MM_CAMCORDER_AUTO_EXPOSURE_CUSTOM_1,	/**< AE on, XXX mode*/
	MM_CAMCORDER_AUTO_EXPOSURE_CUSTOM_2,	/**< AE on, XXX mode*/
};


/**
 * An enumeration for WDR mode .
 */
enum MMCamcorderWDRMode {
	MM_CAMCORDER_WDR_OFF = 0,		/**< WDR OFF*/
	MM_CAMCORDER_WDR_ON,			/**< WDR ON*/
	MM_CAMCORDER_WDR_AUTO,			/**< WDR AUTO*/
};


/**
 * An enumeration for HDR capture mode
 */
enum MMCamcorderHDRMode {
	MM_CAMCORDER_HDR_OFF = 0,               /**< HDR OFF */
	MM_CAMCORDER_HDR_ON,                    /**< HDR ON and no original data - capture callback will be come once */
	MM_CAMCORDER_HDR_ON_AND_ORIGINAL,       /**< HDR ON and original data - capture callback will be come twice(1st:Original, 2nd:HDR) */
};


/**
 * An enumeration for Anti-handshake mode .
 */
enum MMCamcorderAHSMode {
	MM_CAMCORDER_AHS_OFF = 0,		/**< AHS OFF*/
	MM_CAMCORDER_AHS_ON,			/**< AHS ON*/
	MM_CAMCORDER_AHS_AUTO,			/**< AHS AUTO*/
	MM_CAMCORDER_AHS_MOVIE,			/**< AHS MOVIE*/
};


/**
 * An enumeration for Video stabilization mode
 */
enum MMCamcorderVideoStabilizationMode {
	MM_CAMCORDER_VIDEO_STABILIZATION_OFF = 0,	/**< Video Stabilization OFF*/
	MM_CAMCORDER_VIDEO_STABILIZATION_ON,		/**< Video Stabilization ON*/
};


/**
 * Geometry method for camcorder display.
 */
enum MMCamcorderGeometryMethod {
	MM_CAMCORDER_LETTER_BOX = 0,		/**< Letter box*/
	MM_CAMCORDER_ORIGIN_SIZE,		/**< Origin size*/
	MM_CAMCORDER_FULL,			/**< full-screen*/
	MM_CAMCORDER_CROPPED_FULL,		/**< Cropped full-screen*/
	MM_CAMCORDER_ORIGIN_OR_LETTER,		/**< Origin size or Letter box*/
	MM_CAMCORDER_CUSTOM_ROI,		/**< Explicitely described destination ROI*/
};


/**
 * An enumeration for orientation values of tag .
 */
enum MMCamcorderTagOrientation {
	MM_CAMCORDER_TAG_ORT_NONE = 0,		/**< No Orientation.*/
	MM_CAMCORDER_TAG_ORT_0R_VT_0C_VL,	/**< The 0th row is at the visual top of the image, and the 0th column is the visual left-hand side.*/
	MM_CAMCORDER_TAG_ORT_0R_VT_0C_VR,	/**< The 0th row is at the visual top of the image, and the 0th column is the visual right-hand side.*/
	MM_CAMCORDER_TAG_ORT_0R_VB_0C_VR,	/**< The 0th row is at the visual bottom of the image, and the 0th column is the visual right-hand side.*/
	MM_CAMCORDER_TAG_ORT_0R_VB_0C_VL,	/**< The 0th row is at the visual bottom of the image, and the 0th column is the visual left-hand side.*/
	MM_CAMCORDER_TAG_ORT_0R_VL_0C_VT,	/**< The 0th row is the visual left-hand side of the image, and the 0th column is the visual top.*/
	MM_CAMCORDER_TAG_ORT_0R_VR_0C_VT,	/**< The 0th row is the visual right-hand side of the image, and the 0th column is the visual top.*/
	MM_CAMCORDER_TAG_ORT_0R_VR_0C_VB,	/**< The 0th row is the visual right-hand side of the image, and the 0th column is the visual bottom.*/
	MM_CAMCORDER_TAG_ORT_0R_VL_0C_VB,	/**< The 0th row is the visual left-hand side of the image, and the 0th column is the visual bottom.*/
};

/**
 * An enumeration for captured video orientation values of tag .
 */
enum MMCamcorderTagVideoOrientation {
	MM_CAMCORDER_TAG_VIDEO_ORT_NONE = 0,	/**< No Orientation.*/
	MM_CAMCORDER_TAG_VIDEO_ORT_90,		/**< 90 degree */
	MM_CAMCORDER_TAG_VIDEO_ORT_180,	/**< 180 degree */
	MM_CAMCORDER_TAG_VIDEO_ORT_270,	/**< 270 degree */
};



/**
 * An enumeration for Strobe mode.
 */
enum MMCamcorderStrobeMode {
	MM_CAMCORDER_STROBE_MODE_OFF = 0,		/**< Always off */
	MM_CAMCORDER_STROBE_MODE_ON,			/**< Always splashes */
	MM_CAMCORDER_STROBE_MODE_AUTO,			/**< Depending on intensity of light, strobe starts to flash. */
	MM_CAMCORDER_STROBE_MODE_REDEYE_REDUCTION,	/**< Red eye reduction. Multiple flash before capturing. */
	MM_CAMCORDER_STROBE_MODE_SLOW_SYNC,		/**< Slow sync. A type of curtain synchronization. */
	MM_CAMCORDER_STROBE_MODE_FRONT_CURTAIN,		/**< Front curtain. A type of curtain synchronization. */
	MM_CAMCORDER_STROBE_MODE_REAR_CURTAIN,		/**< Rear curtain. A type of curtain synchronization. */
	MM_CAMCORDER_STROBE_MODE_PERMANENT,		/**< keep turned on until turning off */
};


/**
 * An enumeration for Strobe Control.
 */
enum MMCamcorderStrobeControl {
	MM_CAMCORDER_STROBE_CONTROL_OFF = 0,	/**< turn off the flash light */
	MM_CAMCORDER_STROBE_CONTROL_ON,		/**< turn on the flash light */
	MM_CAMCORDER_STROBE_CONTROL_CHARGE,	/**< charge the flash light */
};


/**
 * An enumeration for Detection mode.
 */
enum MMCamcorderDetectMode {
	MM_CAMCORDER_DETECT_MODE_OFF = 0,	/**< turn detection off */
	MM_CAMCORDER_DETECT_MODE_ON,		/**< turn detection on */
};


/**
 * An enumeration for recommended preview resolution.
 */
enum MMCamcorderPreviewType {
	MM_CAMCORDER_PREVIEW_TYPE_NORMAL = 0,   /**< normal ratio like 4:3 */
	MM_CAMCORDER_PREVIEW_TYPE_WIDE,         /**< wide ratio like 16:9 */
	MM_CAMCORDER_PREVIEW_TYPE_SQUARE,       /**< square ratio like 1:1 */
};


/**********************************
*          Attribute info         *
**********************************/
/**
 * An enumeration for attribute values types.
 */
typedef enum {
	MM_CAM_ATTRS_TYPE_INVALID = -1,		/**< Type is invalid */
	MM_CAM_ATTRS_TYPE_INT,			/**< Integer type attribute */
	MM_CAM_ATTRS_TYPE_DOUBLE,		/**< Double type attribute */
	MM_CAM_ATTRS_TYPE_STRING,		/**< UTF-8 String type attribute */
	MM_CAM_ATTRS_TYPE_DATA,			/**< Pointer type attribute */
} MMCamAttrsType;


/**
 * An enumeration for attribute validation type.
 */
typedef enum {
	MM_CAM_ATTRS_VALID_TYPE_INVALID = -1,	/**< Invalid validation type */
	MM_CAM_ATTRS_VALID_TYPE_NONE,		/**< Do not check validity */
	MM_CAM_ATTRS_VALID_TYPE_INT_ARRAY,	/**< validity checking type of integer array */
	MM_CAM_ATTRS_VALID_TYPE_INT_RANGE,	/**< validity checking type of integer range */
	MM_CAM_ATTRS_VALID_TYPE_DOUBLE_ARRAY,	/**< validity checking type of double array */
	MM_CAM_ATTRS_VALID_TYPE_DOUBLE_RANGE,	/**< validity checking type of double range */
} MMCamAttrsValidType;


/**
 * An enumeration for attribute access flag.
 */
typedef enum {
	MM_CAM_ATTRS_FLAG_DISABLED = 0,		/**< None flag is set. This means the attribute is not allowed to use.  */
	MM_CAM_ATTRS_FLAG_READABLE = 1 << 0,	/**< Readable */
	MM_CAM_ATTRS_FLAG_WRITABLE = 1 << 1,	/**< Writable */
	MM_CAM_ATTRS_FLAG_MODIFIED = 1 << 2,	/**< Modified */
	MM_CAM_ATTRS_FLAG_RW = MM_CAM_ATTRS_FLAG_READABLE | MM_CAM_ATTRS_FLAG_WRITABLE,	/**< Readable and Writable */
} MMCamAttrsFlag;


/**********************************
*          Stream data            *
**********************************/
/**
 * An enumeration for stream data type.
 */
typedef enum {
	MM_CAM_STREAM_DATA_YUV420 = 0,          /**< YUV420 Packed type - 1 plane */
	MM_CAM_STREAM_DATA_YUV422,              /**< YUV422 Packed type - 1 plane */
	MM_CAM_STREAM_DATA_YUV420SP,            /**< YUV420 SemiPlannar type - 2 planes */
	MM_CAM_STREAM_DATA_YUV420P,             /**< YUV420 Plannar type - 3 planes */
	MM_CAM_STREAM_DATA_YUV422P,             /**< YUV422 Plannar type - 3 planes */
	MM_CAM_STREAM_DATA_ENCODED,             /**< Encoded data type - 1 plane */
	MM_CAM_STREAM_DATA_DEPTH,               /**< Depth data type - 1 plane */
	MM_CAM_STREAM_DATA_RGB                  /**< RGB data type - 1 plane */
} MMCamStreamData;


/*=======================================================================================
| STRUCTURE DEFINITIONS									|
========================================================================================*/
/**
 * A structure for attribute information
 */
typedef struct {
	MMCamAttrsType type;
	MMCamAttrsFlag flag;
	MMCamAttrsValidType validity_type;

	/**
	 * A union that describes validity of the attribute.
	 * Only when type is 'MM_CAM_ATTRS_TYPE_INT' or 'MM_CAM_ATTRS_TYPE_DOUBLE',
	 * the attribute can have validity.
	 */
	union {
		/**
		 * Validity structure for integer array.
		 */
		 struct {
			int *array;		/**< a pointer of array */
			int count;		/**< size of array */
			int def;		/**< default value. Real value not index of array */
		} int_array;

		/**
		 * Validity structure for integer range.
		 */
		struct {
			int min;		/**< minimum range */
			int max;		/**< maximum range */
			int def;		/**< default value */
		} int_range;

		/**
		 * Validity structure for double array.
		 */
		 struct {
			double *array;		/**< a pointer of array */
			int count;		/**< size of array */
			double def;		/**< default value. Real value not index of array */
		} double_array;

		/**
		 * Validity structure for double range.
		 */
		struct {
			double min;		/**< minimum range */
			double max;		/**< maximum range */
			double def;		/**< default value */
		} double_range;
	};
} MMCamAttrsInfo;


/* General Structure */
/**
 * Structure for capture data.
 */
typedef struct {
	void *data;			/**< pointer of captured image */
	unsigned int length;		/**< length of captured image (in byte)*/
	MMPixelFormatType format;	/**< image format */
	int width;			/**< width of captured image */
	int height;			/**< height of captured image */
	int encoder_type;		/**< encoder type */
} MMCamcorderCaptureDataType;


/**
 * Structure for video stream data.
 */
#define BUFFER_MAX_PLANE_NUM 4

typedef struct {
	union {
		struct {
			unsigned char *yuv;
			unsigned int length_yuv;
		} yuv420, yuv422;
		struct {
			unsigned char *y;
			unsigned int length_y;
			unsigned char *uv;
			unsigned int length_uv;
		} yuv420sp;
		struct {
			unsigned char *y;
			unsigned int length_y;
			unsigned char *u;
			unsigned int length_u;
			unsigned char *v;
			unsigned int length_v;
		} yuv420p, yuv422p;
		struct {
			unsigned char *data;
			unsigned int length_data;
		} encoded, depth, rgb;
	} data;                         /**< pointer of captured stream */
	MMCamStreamData data_type;      /**< data type */
	unsigned int length_total;      /**< total length of stream buffer (in byte)*/
	unsigned int num_planes;        /**< number of planes */
	MMPixelFormatType format;       /**< image format */
	int width;                      /**< width of video buffer */
	int height;                     /**< height of video buffer */
	unsigned int timestamp;         /**< timestamp of stream buffer (msec)*/
	void *bo[BUFFER_MAX_PLANE_NUM]; /**< TBM buffer object */
	void *internal_buffer;          /**< Internal buffer pointer */
	int stride[BUFFER_MAX_PLANE_NUM];    /**< Stride of each plane */
	int elevation[BUFFER_MAX_PLANE_NUM]; /**< Elevation of each plane */
} MMCamcorderVideoStreamDataType;


/**
 * Structure for audio stream data.
 */
typedef struct {
	void *data;				/**< pointer of captured stream */
	unsigned int length;			/**< length of stream buffer (in byte)*/
	MMCamcorderAudioFormat format;		/**< audio format */
	int channel;				/**< number of channel of the stream */
	unsigned int timestamp;			/**< timestamp of stream buffer (msec)*/
	float volume_dB;			/**< dB value of audio stream */
} MMCamcorderAudioStreamDataType;


/**
 * Structure for muxed stream data.
 */
typedef struct {
	void *data;             /**< pointer of muxed stream */
	unsigned int length;    /**< length of stream buffer (in byte) */
	unsigned long long offset;  /**< current offset for data */
} MMCamcorderMuxedStreamDataType;


/**
  * Prerequisite information for mm_camcorder_create()
  * The information to set prior to create.
  */
typedef struct {
	enum MMVideoDeviceType videodev_type;	/**< Video device type */
	/* For future use */
	int reserved[4];			/**< reserved fields */
} MMCamPreset;


/**
 * Report structure of recording file
 */
typedef struct {
	char *recording_filename;		/**< File name of stored recording file. Please free after using. */
} MMCamRecordingReport; /**< report structure definition of recording file */


/**
 * Face detect defailed information
 */
typedef struct _MMCamFaceInfo {
	int id;                                 /**< id of each face */
	int score;                              /**< score of each face */
	MMRectType rect;                        /**< area of face */
} MMCamFaceInfo;

/**
 * Face detect information
 */
typedef struct _MMCamFaceDetectInfo {
	int num_of_faces;                       /**< number of detected faces */
	MMCamFaceInfo *face_info;               /**< face information, this should be freed after use it. */
} MMCamFaceDetectInfo;

/**
 * Window information
 */
typedef struct _MMCamWindowInfo {
	int surface_id;
	MMRectType rect;
} MMCamWindowInfo;


/*=======================================================================================
| TYPE DEFINITIONS									|
========================================================================================*/
/**
 *	Function definition for video stream callback.
 *  Be careful! In this function, you can't call functions that change the state of camcorder such as mm_camcorder_stop(),
 *  mm_camcorder_unrealize(), mm_camcorder_record(), mm_camcorder_commit(), and mm_camcorder_cancel(), etc.
 *  Please don't hang this function long. It may cause low performance of preview or occur timeout error from video source.
 *  Also, you're not allowed to call mm_camcorder_stop() even in other context, while you're hanging this function.
 *  I recommend to you releasing this function ASAP.
 *
 *	@param[in]	stream			Reference pointer to video stream data
 *	@param[in]	user_param		User parameter which is received from user when callback function was set
 *	@return		This function returns true on success, or false on failure.
 *	@remarks		This function is issued in the context of gstreamer (video sink thread).
 */
typedef gboolean (*mm_camcorder_video_stream_callback)(MMCamcorderVideoStreamDataType *stream, void *user_param);


/**
 *	Function definition for audio stream callback.
 *  Be careful! In this function, you can't call functions that change the state of camcorder such as mm_camcorder_stop(),
 *  mm_camcorder_unrealize(), mm_camcorder_record(), mm_camcorder_commit(), and mm_camcorder_cancel(), etc.
 *  Please don't hang this function long. It may cause low performance of camcorder or occur timeout error from audio source.
 *  I recommend to you releasing this function ASAP.
 *
 *	@param[in]	stream			Reference pointer to audio stream data
 *	@param[in]	user_param		User parameter which is received from user when callback function was set
 *	@return		This function returns true on success, or false on failure.
 *	@remarks
 */
typedef gboolean (*mm_camcorder_audio_stream_callback)(MMCamcorderAudioStreamDataType *stream, void *user_param);


/**
 *	Function definition for muxed stream callback.
 *  Be careful! In this function, you can't call functions that change the state of camcorder such as mm_camcorder_stop(),
 *  mm_camcorder_unrealize(), mm_camcorder_record(), mm_camcorder_commit(), and mm_camcorder_cancel(), etc.
 *  Please don't hang this function long. It may cause low performance of camcorder or occur timeout error from encoding pipeline.
 *  I recommend to you releasing this function ASAP.
 *
 *	@param[in]	stream			Reference pointer to muxed stream data
 *	@param[in]	user_param		User parameter which is received from user when callback function was set
 *	@return		This function returns true on success, or false on failure.
 *	@remarks
 */
typedef gboolean (*mm_camcorder_muxed_stream_callback)(MMCamcorderMuxedStreamDataType *stream, void *user_param);


/**
 *	Function definition for video capture callback.
 *  Like '#mm_camcorder_video_stream_callback', you can't call mm_camcorder_stop() while you are hanging this function.
 *
 *	@param[in]	frame			Reference pointer to captured data
 *	@param[in]	thumbnail		Reference pointer to thumbnail data
 *	@param[in]	user_param		User parameter which is received from user when callback function was set
 *	@return		This function returns true on success, or false on failure.
 *	@remarks		This function is issued in the context of gstreamer (video src thread).
 */
typedef gboolean (*mm_camcorder_video_capture_callback)(MMCamcorderCaptureDataType *frame, MMCamcorderCaptureDataType *thumbnail, void *user_param);


/*=======================================================================================
| GLOBAL FUNCTION PROTOTYPES								|
========================================================================================*/
/**
 *    mm_camcorder_create:\n
 *  Create camcorder object. This is the function that an user who wants to use mm_camcorder calls first.
 *  This function creates handle structure and initialize mutex, attributes, gstreamer.
 *  When this function success, it will return  a handle of newly created object.
 *  A user have to put the handle when he calls every function of mm_camcorder. \n
 *  Second argument of this function is the field to decribe pre-setting information of mm_camcorder such as which camera device it will use.
 *  Normally, MM_VIDEO_DEVICE_CAMERA0 is for Main camera(or Mega camera, Back camera),
 *  and MM_VIDEO_DEVICE_CAMERA1 is for VGA camera (or Front camera). If you want audio recording,
 *  please set MM_VIDEO_DEVICE_NONE. (No camera device is needed.)
 *
 *	@param[out]	camcorder	A handle of camcorder.
 *	@param[in]	info		Information for camera device. Depending on this information,
 *					camcorder opens different camera devices.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@see		mm_camcorder_destroy
 *	@pre		None
 *	@post		Next state of mm-camcorder will be MM_CAMCORDER_STATE_NULL
 *	@remarks	You can create multiple handles on a context at the same time. However,
 *			camcorder cannot guarantee proper operation because of limitation of resources, such as
 *			camera device, audio device, and display device.
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean initialize_camcorder()
{
	int err;
	MMCamPreset cam_info;
#if 1
	cam_info.videodev_type = MM_VIDEO_DEVICE_CAMERA0;
#else
	// when you want to record audio only, enable this.
	cam_info.videodev_type = MM_VIDEO_DEVICE_NONE;
#endif

	err = mm_camcorder_create(&hcam, &cam_info);

	if (err != MM_ERROR_NONE) {
		printf("Fail to call mm_camcorder_create = %x\n", err);
		return FALSE;
	}

	return TRUE;
}

 *	@endcode
 */
int mm_camcorder_create(MMHandleType *camcorder, MMCamPreset *info);


/**
 *    mm_camcorder_destroy:\n
 *  Destroy camcorder object. Release handle and all of the resources that were created in mm_camcorder_create().\n
 *  This is the finalizing function of mm_camcorder. If this function is not called or fails to call, the handle isn't released fully.
 *  This function releases attributes, mutexes, sessions, and handle itself. This function also removes all of remaining messages.
 *  So if your application should wait a certain message of mm_camcorder, please wait to call this function till getting the message.
 *
 *
 *	@param[in]	camcorder	A handle of camcorder.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@see		mm_camcorder_create
 *	@pre		Previous state of mm-camcorder should be MM_CAMCORDER_STATE_NULL
 *	@post		Because the handle is not valid, you can't check the state.
 *	@remarks	None
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean destroy_camcorder()
{
	int err;

	//Destroy camcorder handle
	err = mm_camcorder_destroy(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_destroy  = %x\n", err);
		return FALSE;
	}

	return TRUE;
}

 *	@endcode
 */
int mm_camcorder_destroy(MMHandleType camcorder);


/**
 *    mm_camcorder_realize:\n
 *  Allocate resources for camcorder and initialize it.
 *  This also creates streamer pipeline. So you have to set attributes that are pivotal to create
 *  the pipeline before calling this function. This function also takes a roll to manage confliction
 *  between different applications which use camcorder. For example, if you try to use camcorder when
 *  other application that is more important such as call application, this function will return
 *  'MM_ERROR_POLICY_BLOCKED'. On the contrary, if your application that uses camcorder starts to launch
 *  while another application that uses speaker and has lower priority, your application will kick
 *  another application.
 *
 *	@param[in]	camcorder	A handle of camcorder.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@see		mm_camcorder_unrealize
 *	@pre		Previous state of mm-camcorder should be MM_CAMCORDER_STATE_NULL
 *	@post		Next state of mm-camcorder will be MM_CAMCORDER_STATE_READY
 *	@remarks	None
 *	@par example
 *	@code

#include <mm_camcorder.h>

//For image capturing
gboolean initialize_image_capture()
{
	int err;
	MMCamPreset cam_info;
	char *err_attr_name = NULL;
	void * hdisplay = NULL;
	int hsize = 0;

	//Set video device as 'camera0' (main camera device)
	cam_info.videodev_type = MM_VIDEO_DEVICE_CAMERA0;

	err = mm_camcorder_create(&hcam, &cam_info);

	if (err != MM_ERROR_NONE) {
		printf("Fail to call mm_camcorder_create = %x\n", err);
		return FALSE;
	}

	mm_camcorder_set_message_callback(hcam,(MMMessageCallback)msg_callback, (void*)hcam);
	mm_camcorder_set_video_capture_callback(hcam,(mm_camcorder_video_capture_callback)camcordertest_video_capture_cb, (void*)hcam);

	hdisplay = &ad.xid;		//xid of xwindow. This value can be different depending on your environment.
	hsize = sizeof(ad.xid);		//size of xid structure.

	// camcorder attribute setting
	err = mm_camcorder_set_attributes((MMHandleType)hcam, &err_attr_name,
					  MMCAM_MODE, MM_CAMCORDER_MODE_IMAGE,
					  MMCAM_IMAGE_ENCODER, MM_IMAGE_CODEC_JPEG,
					  MMCAM_CAMERA_WIDTH, 640,
					  MMCAM_CAMERA_HEIGHT, 480,
					  MMCAM_CAMERA_FORMAT, MM_PIXEL_FORMAT_YUYV,
					  MMCAM_CAMERA_FPS, 30,
					  MMCAM_DISPLAY_ROTATION, MM_DISPLAY_ROTATION_270,
					  MMCAM_DISPLAY_HANDLE, (void*) hdisplay,          hsize,
					  MMCAM_CAPTURE_FORMAT, MM_PIXEL_FORMAT_ENCODED,
					  MMCAM_CAPTURE_WIDTH, 640,
					  MMCAM_CAPTURE_HEIGHT, 480,
					  NULL);

	if (err < 0) {
		printf("Set attrs fail. (%s:%x)\n", err_attr_name, err);
		if (err_attr_name) {
			free(err_attr_name);
			err_attr_name = NULL;
			return FALSE;
		}
	}

	err =  mm_camcorder_realize(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_realize  = %x\n", err);
		return FALSE;
	}

	return TRUE;
}

//For A/V capturing
gboolean initialize_video_capture()
{
	int err;
	MMCamPreset cam_info;
	char *err_attr_name = NULL;
	void * hdisplay = NULL;
	int hsize = 0;

	//Set video device as 'camera0' (main camera device)
	cam_info.videodev_type = MM_VIDEO_DEVICE_CAMERA0;

	err = mm_camcorder_create(&hcam, &cam_info);

	if (err != MM_ERROR_NONE) {
		printf("Fail to call mm_camcorder_create = %x\n", err);
		return FALSE;
	}

	mm_camcorder_set_message_callback(hcam,(MMMessageCallback)msg_callback, hcam);

	hdisplay = &ad.xid;		//xid of xwindow. This value can be different depending on your environment.
	hsize = sizeof(ad.xid);		//size of xid structure.

	// camcorder attribute setting
	err = mm_camcorder_set_attributes((MMHandleType)hcam, &err_attr_name,
					  MMCAM_MODE, MM_CAMCORDER_MODE_VIDEO,
					  MMCAM_AUDIO_DEVICE, MM_AUDIO_DEVICE_MIC,
					  MMCAM_AUDIO_ENCODER, MM_AUDIO_CODEC_AAC,
					  MMCAM_VIDEO_ENCODER, MM_VIDEO_CODEC_MPEG4,
					  MMCAM_FILE_FORMAT, MM_FILE_FORMAT_3GP,
					  MMCAM_CAMERA_WIDTH, 1280,
					  MMCAM_CAMERA_HEIGHT, 720,
					  MMCAM_CAMERA_FORMAT, MM_PIXEL_FORMAT_NV12,
					  MMCAM_CAMERA_FPS, 30,
					  MMCAM_AUDIO_SAMPLERATE, 44100,
					  MMCAM_AUDIO_FORMAT, MM_CAMCORDER_AUDIO_FORMAT_PCM_S16_LE,
					  MMCAM_AUDIO_CHANNEL, 2,
					  MMCAM_DISPLAY_ROTATION, MM_DISPLAY_ROTATION_270,
					  MMCAM_DISPLAY_HANDLE, (void*) hdisplay,		hsize,
					  MMCAM_TARGET_FILENAME, TARGET_FILENAME, strlen(TARGET_FILENAME),
					  NULL);

	if (err < 0) {
		printf("Set attrs fail. (%s:%x)\n", err_attr_name, err);
		if (err_attr_name) {
			free(err_attr_name);
			err_attr_name = NULL;
			return FALSE;
		}
	}

	err =  mm_camcorder_realize(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_realize  = %x\n", err);
		return FALSE;
	}

	return TRUE;
}

//For audio(only) capturing
gboolean initialize_audio_capture()
{
	int err;
	MMCamPreset cam_info;
	char *err_attr_name = NULL;
	void * hdisplay = NULL;
	int hsize = 0;

	//Set no video device, because audio recording doesn't need video input.
	cam_info.videodev_type = MM_VIDEO_DEVICE_NONE;

	err = mm_camcorder_create(&hcam, &cam_info);

	if (err != MM_ERROR_NONE) {
		printf("Fail to call mm_camcorder_create = %x\n", err);
		return FALSE;
	}

	mm_camcorder_set_message_callback(hcam,(MMMessageCallback)msg_callback, (void*)hcam);

	hdisplay = &ad.xid;		//xid of xwindow. This value can be different depending on your environment.
	hsize = sizeof(ad.xid);		//size of xid structure.

	// camcorder attribute setting
	err = mm_camcorder_set_attributes((MMHandleType)hcam, &err_attr_name,
					  MMCAM_MODE, MM_CAMCORDER_MODE_AUDIO,
					  MMCAM_AUDIO_DEVICE, MM_AUDIO_DEVICE_MIC,
					  MMCAM_AUDIO_ENCODER, MM_AUDIO_CODEC_AAC,
					  MMCAM_FILE_FORMAT, MM_FILE_FORMAT_3GP,
					  MMCAM_AUDIO_SAMPLERATE, 44100,
					  MMCAM_AUDIO_FORMAT, MM_CAMCORDER_AUDIO_FORMAT_PCM_S16_LE,
					  MMCAM_AUDIO_CHANNEL, 2,
					  MMCAM_TARGET_FILENAME, TARGET_FILENAME, strlen(TARGET_FILENAME),
					  MMCAM_TARGET_TIME_LIMIT, 360000,
					  NULL);

	if (err < 0) {
		printf("Set attrs fail. (%s:%x)\n", err_attr_name, err);
		if (err_attr_name) {
			free(err_attr_name);
			err_attr_name = NULL;
			return FALSE;
		}
	}

	err =  mm_camcorder_realize(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_realize  = %x\n", err);
		return FALSE;
	}

	return TRUE;
}
 *	@endcode
 */
int mm_camcorder_realize(MMHandleType camcorder);


/**
 *    mm_camcorder_unrealize:\n
 *  Uninitialize camcoder resources and free allocated memory.
 *  Most important resource that is released here is gstreamer pipeline of mm_camcorder.
 *  Because most of resources, such as camera device, video display device, and audio I/O device, are operating on the gstreamer pipeline,
 *  this function should be called to release its resources.
 *
 *	@param[in]	camcorder	A handle of camcorder.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@see		mm_camcorder_realize
 *	@pre		Previous state of mm-camcorder should be MM_CAMCORDER_STATE_READY
 *	@post		Next state of mm-camcorder will be MM_CAMCORDER_STATE_NULL
 *	@remarks	None
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean unrealize_camcorder()
{
	int err;

	//Release all resources of camcorder handle
	err =  mm_camcorder_unrealize(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_unrealize  = %x\n", err);
		return FALSE;
	}

	return TRUE;
}

 *	@endcode
 */
int mm_camcorder_unrealize(MMHandleType camcorder);


/**
 *	mm_camcorder_start:\n
 *   Start previewing. (Image/Video mode)
 *  'mm_camcorder_video_stream_callback' is activated after calling this function.
 *
 *	@param[in]	camcorder	A handle of camcorder.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@see		mm_camcorder_stop
 *	@pre		Previous state of mm-camcorder should be MM_CAMCORDER_STATE_READY
 *	@post		Next state of mm-camcorder will be MM_CAMCORDER_STATE_PREPARE
 *	@remarks	None
 *	@par example
 *	@code

#include <mm_camcorder.h>

//For image capturing
gboolean initialize_image_capture()
{
	int err;
	MMCamPreset cam_info;
	char *err_attr_name = NULL;
	void * hdisplay = NULL;
	int hsize = 0;

	//Set video device as 'camera0' (main camera device)
	cam_info.videodev_type = MM_VIDEO_DEVICE_CAMERA0;

	err = mm_camcorder_create(&hcam, &cam_info);

	if (err != MM_ERROR_NONE) {
			printf("Fail to call mm_camcorder_create = %x\n", err);
			return FALSE;
	}

	mm_camcorder_set_message_callback(hcam,(MMMessageCallback)msg_callback, (void*)hcam);
	mm_camcorder_set_video_capture_callback(hcam,(mm_camcorder_video_capture_callback)camcordertest_video_capture_cb, (void*)hcam);

	hdisplay = &ad.xid;		//xid of xwindow. This value can be different depending on your environment.
	hsize = sizeof(ad.xid);		//size of xid structure.

	// camcorder attribute setting
	err = mm_camcorder_set_attributes((MMHandleType)hcam, &err_attr_name,
					  MMCAM_MODE, MM_CAMCORDER_MODE_IMAGE,
					  MMCAM_IMAGE_ENCODER, MM_IMAGE_CODEC_JPEG,
					  MMCAM_CAMERA_WIDTH, 640,
					  MMCAM_CAMERA_HEIGHT, 480,
					  MMCAM_CAMERA_FORMAT, MM_PIXEL_FORMAT_YUYV,
					  MMCAM_CAMERA_FPS, 30,
					  MMCAM_DISPLAY_ROTATION, MM_DISPLAY_ROTATION_270,
					  MMCAM_DISPLAY_HANDLE, (void*) hdisplay,          hsize,
					  MMCAM_CAPTURE_FORMAT, MM_PIXEL_FORMAT_ENCODED,
					  MMCAM_CAPTURE_WIDTH, 640,
					  MMCAM_CAPTURE_HEIGHT, 480,
					  NULL);

	if (err < 0) {
		printf("Set attrs fail. (%s:%x)\n", err_attr_name, err);
		if (err_attr_name) {
			free(err_attr_name);
			err_attr_name = NULL;
			return FALSE;
		}
	}

	err =  mm_camcorder_realize(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_realize  = %x\n", err);
		return FALSE;
	}

	// start camcorder
	err = mm_camcorder_start(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_start  = %x\n", err);
		return FALSE;
	}

	return TRUE;
}

//For A/V capturing
gboolean initialize_video_capture()
{
	int err;
	MMCamPreset cam_info;
	char *err_attr_name = NULL;
	void * hdisplay = NULL;
	int hsize = 0;

	//Set video device as 'camera0' (main camera device)
	cam_info.videodev_type = MM_VIDEO_DEVICE_CAMERA0;

	err = mm_camcorder_create(&hcam, &cam_info);

	if (err != MM_ERROR_NONE) {
			printf("Fail to call mm_camcorder_create = %x\n", err);
			return FALSE;
	}

	mm_camcorder_set_message_callback(hcam,(MMMessageCallback)msg_callback, hcam);

	hdisplay = &ad.xid;		//xid of xwindow. This value can be different depending on your environment.
	hsize = sizeof(ad.xid);		//size of xid structure.

	// camcorder attribute setting
	err = mm_camcorder_set_attributes((MMHandleType)hcam, &err_attr_name,
					  MMCAM_MODE, MM_CAMCORDER_MODE_VIDEO,
					  MMCAM_AUDIO_DEVICE, MM_AUDIO_DEVICE_MIC,
					  MMCAM_AUDIO_ENCODER, MM_AUDIO_CODEC_AAC,
					  MMCAM_VIDEO_ENCODER, MM_VIDEO_CODEC_MPEG4,
					  MMCAM_FILE_FORMAT, MM_FILE_FORMAT_3GP,
					  MMCAM_CAMERA_WIDTH, 1280,
					  MMCAM_CAMERA_HEIGHT, 720,
					  MMCAM_CAMERA_FORMAT, MM_PIXEL_FORMAT_NV12,
					  MMCAM_CAMERA_FPS, 30,
					  MMCAM_AUDIO_SAMPLERATE, 44100,
					  MMCAM_AUDIO_FORMAT, MM_CAMCORDER_AUDIO_FORMAT_PCM_S16_LE,
					  MMCAM_AUDIO_CHANNEL, 2,
					  MMCAM_DISPLAY_ROTATION, MM_DISPLAY_ROTATION_270,
					  MMCAM_DISPLAY_HANDLE, (void*) hdisplay,		hsize,
					  MMCAM_TARGET_FILENAME, TARGET_FILENAME, strlen(TARGET_FILENAME),
					  NULL);

	if (err < 0) {
		printf("Set attrs fail. (%s:%x)\n", err_attr_name, err);
		if (err_attr_name) {
			free(err_attr_name);
			err_attr_name = NULL;
			return FALSE;
		}
	}

	err =  mm_camcorder_realize(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_realize  = %x\n", err);
		return FALSE;
	}

	// start camcorder
	err = mm_camcorder_start(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_start  = %x\n", err);
		return FALSE;
	}

	return TRUE;
}

//For audio(only) capturing
gboolean initialize_audio_capture()
{
	int err;
	MMCamPreset cam_info;
	char *err_attr_name = NULL;
	void * hdisplay = NULL;
	int hsize = 0;

	//Set no video device, because audio recording doesn't need video input.
	cam_info.videodev_type = MM_VIDEO_DEVICE_NONE;

	err = mm_camcorder_create(&hcam, &cam_info);

	if (err != MM_ERROR_NONE) {
		printf("Fail to call mm_camcorder_create = %x\n", err);
		return FALSE;
	}

	mm_camcorder_set_message_callback(hcam,(MMMessageCallback)msg_callback, (void*)hcam);

	hdisplay = &ad.xid;		//xid of xwindow. This value can be different depending on your environment.
	hsize = sizeof(ad.xid);		//size of xid structure.

	// camcorder attribute setting
	err = mm_camcorder_set_attributes((MMHandleType)hcam, &err_attr_name,
					  MMCAM_MODE, MM_CAMCORDER_MODE_AUDIO,
					  MMCAM_AUDIO_DEVICE, MM_AUDIO_DEVICE_MIC,
					  MMCAM_AUDIO_ENCODER, MM_AUDIO_CODEC_AAC,
					  MMCAM_FILE_FORMAT, MM_FILE_FORMAT_3GP,
					  MMCAM_AUDIO_SAMPLERATE, 44100,
					  MMCAM_AUDIO_FORMAT, MM_CAMCORDER_AUDIO_FORMAT_PCM_S16_LE,
					  MMCAM_AUDIO_CHANNEL, 2,
					  MMCAM_TARGET_FILENAME, TARGET_FILENAME, strlen(TARGET_FILENAME),
					  MMCAM_TARGET_TIME_LIMIT, 360000,
					  NULL);

	if (err < 0) {
		printf("Set attrs fail. (%s:%x)\n", err_attr_name, err);
		if (err_attr_name) {
			free(err_attr_name);
			err_attr_name = NULL;
			return FALSE;
		}
	}

	err =  mm_camcorder_realize(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_realize  = %x\n", err);
		return FALSE;
	}

	// start camcorder
	err = mm_camcorder_start(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_start  = %x\n", err);
		return FALSE;
	}

	return TRUE;
}
 *	@endcode
 */
int mm_camcorder_start(MMHandleType camcorder);


/**
 *    mm_camcorder_stop:\n
 *  Stop previewing. (Image/Video mode)
 *  This function will change the status of pipeline. If an application doesn't return callbacks
 *  of camcorder, this function can be locked. For example, if your application still
 *  holds '#mm_camcorder_video_capture_callback' or '#mm_camcorder_video_stream_callback',
 *  this function could be hung. So users have to return every callback before calling this function.
 *
 *	@param[in]	camcorder	A handle of camcorder.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@see		mm_camcorder_start
 *	@pre		Previous state of mm-camcorder should be MM_CAMCORDER_STATE_PREPARE
 *	@post		Next state of mm-camcorder will be MM_CAMCORDER_STATE_READY
 *	@remarks	None
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean stop_camcorder()
{
	int err;

	//Stop preview
	err =  mm_camcorder_stop(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_stop  = %x\n", err);
		return FALSE;
	}

	return TRUE;
}

 *	@endcode
 */
int mm_camcorder_stop(MMHandleType camcorder);


/**
 *    mm_camcorder_capture_start:\n
 *  Start capturing of still images. (Image mode only)
 *  Captured image will be delievered through 'mm_camcorder_video_capture_callback'.
 *  So basically, the operation is working asynchronously. \n
 *  When a user call this function, MSL will stop to retrieving preview from camera device.
 *  Then set capture resolution, pixel format, and encoding type to camera driver. After resuming,
 *  camera can get still image.  A user will be notified by
 *  'MM_MESSAGE_CAMCORDER_CAPTURED' message when capturing succeed. When a user sets
 *  multishot (by setting multiple number to MMCAM_CAPTURE_COUNT), the message
 *  will be called multiple time. You can get the number of image from 'code' of
 *  'MMMessageParamType'.
 *
 *	@param[in]	camcorder	A handle of camcorder.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@see		mm_camcorder_capture_stop
 *	@pre		Previous state of mm-camcorder should be MM_CAMCORDER_STATE_PREPARE
 *	@post		Next state of mm-camcorder will be MM_CAMCORDER_STATE_CAPTURING
 *	@remarks	To call this function, preview should be started successfully.\n
 *			This function is a pair of mm_camcorder_capture_stop().
 *			So user should call mm_camcorder_capture_stop() after getting captured image.
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean capturing_picture()
{
	int err;

	err =  mm_camcorder_capture_start(hcam);
	if (err < 0)
	{
		printf("Fail to call mm_camcorder_capture_start  = %x\n", err);
		return FALSE;
	}

	//mm_camcorder_capture_stop should be called after getting
	//MM_MESSAGE_CAMCORDER_CAPTURED message.

	return TRUE;
}


 *	@endcode
 */
int mm_camcorder_capture_start(MMHandleType camcorder);


/**
 *    mm_camcorder_capture_stop:\n
 *  Stop capturing of still images. (Image mode only)
 *  This function notifies the end of capturing and launch preview again.
 *  Just as mm_camcorder_capture_start(), this funciton stops still image stream and set preview information such as
 *  resolution, pixel format, and framerate to camera driver. Then it command to start preview.
 *  If you don't call this, preview will not be displayed even though capturing was finished.
 *
 *	@param[in]	camcorder	A handle of camcorder.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@see		mm_camcorder_capture_start
 *	@pre		Previous state of mm-camcorder should be MM_CAMCORDER_STATE_CAPTURING
 *	@post		Next state of mm-camcorder will be MM_CAMCORDER_STATE_PREPARE
 *	@remarks	To call this function, a user has to call mm_camcorder_capture_start() first.\n
 *			This is not a function to stop multishot in the middle of operation. For that,
 *			please use '#MMCAM_CAPTURE_BREAK_CONTINUOUS_SHOT' instead.
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean capturing_picture_stop()
{
	int err;

	err =  mm_camcorder_capture_stop(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_capture_stop  = %x\n", err);
		return FALSE;
	}

	//After calling upper function, preview will start.

	return TRUE;
}

 *	@endcode
 */
int mm_camcorder_capture_stop(MMHandleType camcorder);


/**
 *    mm_camcorder_record:\n
 *  Start recording. (Audio/Video mode only)
 *  Camcorder starts to write a file when you call this function. You can specify the name of file
 *  using '#MMCAM_TARGET_FILENAME'. Beware, if you fail to call mm_camcorder_commit() or mm_camcorder_cancel(),
 *  the recorded file is still on the storage.
 *
 *	@param[in]	camcorder	A handle of camcorder.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@see		mm_camcorder_pause
 *	@pre		Previous state of mm-camcorder should be MM_CAMCORDER_STATE_PREPARE
 *	@post		Next state of mm-camcorder will be MM_CAMCORDER_STATE_RECORDING
 *	@remarks	None
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean record_and_cancel_video_file()
{
	int err;

	// Start recording
	err =  mm_camcorder_record(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_record  = %x\n", err);
		return FALSE;
	}

	return TRUE;
}

 *	@endcode
 */
int mm_camcorder_record(MMHandleType camcorder);


/**
 *    mm_camcorder_pause:\n
 *  Pause A/V recording or Audio recording. (Audio/Video mode only)
 *  On video recording, you can see preview while on pausing. So mm_camcorder cuts video stream path to encoder and keep the flow to preview.
 *  If you call mm_camcorder_commit() while on pausing, the recorded file only has Audio and Video stream which were generated before pause().
 *
 *	@param[in]	camcorder	A handle of camcorder.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@see		mm_camcorder_record
 *	@pre		Previous state of mm-camcorder should be MM_CAMCORDER_STATE_RECORDING
 *	@post		Next state of mm-camcorder will be MM_CAMCORDER_STATE_PAUSED
 *	@remarks	Even though this function is for pausing recording, small amount of buffers could be recorded after pause().
 *			Because the buffers which are existed in the queue were created before pause(), the buffers should be recorded.
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean record_pause_and_resume_recording()
{
	int err;

	// Start recording
	err =  mm_camcorder_record(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_record  = %x\n", err);
		return FALSE;
	}

	// Wait while recording...

	// Pause
	err =  mm_camcorder_pause(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_pause  = %x\n", err);
		return FALSE;
	}

	// Pausing...

	// Resume
	err =  mm_camcorder_record(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_record  = %x\n", err);
		return FALSE;
	}

	return TRUE;
}


 *	@endcode
 */
int mm_camcorder_pause(MMHandleType camcorder);


/**
 *    mm_camcorder_commit:\n
 *  Stop recording and save results.  (Audio/Video mode only)\n
 *  After starting recording, encoded data frame will be stored in the location specified in MMCAM_TARGET_FILENAME.
 *  Some encoder or muxer require a certain type of finalizing such as adding some information to header.
 *  This function takes that roll. So if you don't call this function after recording, the result file may not be playable.\n
 *  After committing successfully, camcorder resumes displaying preview (video recording case).
 *  Because this is the function for saving the recording result, the operation is available
 *  only when the mode of camcorder is MM_CAMCORDER_MODE_AUDIO or MM_CAMCORDER_MODE_VIDEO.
 *
 *	@param[in]	camcorder	A handle of camcorder.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@see		mm_camcorder_cancel
 *	@pre		Previous state of mm-camcorder should be MM_CAMCORDER_STATE_RECORDING
 *	@post		Next state of mm-camcorder will be MM_CAMCORDER_STATE_PREPARE
 *	@remarks	This function can take a few second when recording time is long.
 *			and if there are only quite few input buffer from video src or audio src,
 *			committing could be failed.
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean record_and_save_video_file()
{
	int err;

	// Start recording
	err =  mm_camcorder_record(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_record  = %x\n", err);
		return FALSE;
	}

	// Wait while recording for test...
	// In normal case, mm_camcorder_record() and mm_camcorder_commit() aren't called in the same function.

	// Save file
	err =  mm_camcorder_commit(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_commit  = %x\n", err);
		return FALSE;
	}

	return TRUE;
}

 *	@endcode
 */
int mm_camcorder_commit(MMHandleType camcorder);


/**
 *	mm_camcorder_cancel:\n
 *    Stop recording and discard the result. (Audio/Video mode only)
 *	When a user want to finish recording without saving the result file, this function can be used.
 *	Like mm_camcorder_commit(), this function also stops recording, release related resources(like codec) ,and goes back to preview status.
 *	However, instead of saving file, this function unlinks(delete) the result.\n
 *	Because this is the function for canceling recording, the operation is available
 *	only when mode is MM_CAMCORDER_MODE_AUDIO or MM_CAMCORDER_MODE_VIDEO.
 *
 *	@param[in]	camcorder	A handle of camcorder.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@see		mm_camcorder_commit
 *	@pre		Previous state of mm-camcorder should be MM_CAMCORDER_STATE_RECORDING
 *	@post		Next state of mm-camcorder will be MM_CAMCORDER_STATE_PREPARE
 *	@remarks	None
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean record_and_cancel_video_file()
{
	int err;

	// Start recording
	err =  mm_camcorder_record(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_record  = %x\n", err);
		return FALSE;
	}

	// Wait while recording...

	// Cancel recording
	err =  mm_camcorder_cancel(hcam);
	if (err < 0) {
		printf("Fail to call mm_camcorder_cancel  = %x\n", err);
		return FALSE;
	}

	return TRUE;
}

 *	@endcode
 */
int mm_camcorder_cancel(MMHandleType camcorder);


/**
 *    mm_camcorder_set_message_callback:\n
 *  Set callback for receiving messages from camcorder. Through this callback function, camcorder
 *  sends various message including status changes, asynchronous error, capturing, and limitations.
 *  One thing you have to know is that message callback is working on the main loop of application.
 *  So until releasing the main loop, message callback will not be called.
 *
 *	@param[in]	camcorder	A handle of camcorder.
 *	@param[in]	callback	Function pointer of callback function. Please refer 'MMMessageCallback'.
 *	@param[in]	user_data	User parameter for passing to callback function.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@see		MMMessageCallback
 *	@pre		None
 *	@post		None
 *	@remarks	registered 'callback' is called on main loop of the application. So until the main loop is released, 'callback' will not be called.
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean setting_msg_callback()
{
	//set callback
	mm_camcorder_set_message_callback(hcam,(MMMessageCallback)msg_callback, (void*)hcam);

	return TRUE;
}


 *	@endcode
 */
int mm_camcorder_set_message_callback(MMHandleType camcorder, MMMessageCallback callback, void *user_data);


/**
 *    mm_camcorder_set_video_stream_callback:\n
 *  Set callback for user defined video stream callback function.
 *  Users can retrieve video frame using registered callback.
 *  The callback function holds the same buffer that will be drawed on the display device.
 *  So if an user change the buffer, it will be displayed on the device.
 *
 *	@param[in]	camcorder	A handle of camcorder.
 *	@param[in]	callback	Function pointer of callback function.
 *	@param[in]	user_data	User parameter for passing to callback function.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@see		mm_camcorder_video_stream_callback
 *	@pre		None
 *	@post		None
 *	@remarks	registered 'callback' is called on internal thread of camcorder. Regardless of the status of main loop, this function will be called.
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean setting_video_stream_callback()
{
	//set callback
	mm_camcorder_set_video_stream_callback(hcam, (mm_camcorder_video_stream_callback)camcordertest_video_stream_cb, (void*)hcam);

	return TRUE;
}
 *	@endcode
 */
int mm_camcorder_set_video_stream_callback(MMHandleType camcorder, mm_camcorder_video_stream_callback callback, void *user_data);


/**
 *    mm_camcorder_set_video_capture_callback:\n
 *  Set callback for user defined video capture callback function.  (Image mode only)
 *  mm_camcorder deliever captured image through the callback.\n
 *  Normally, this function provides main captured image and thumnail image. But depending on the environment,
 *  thumnail would not be available. Information related with main captured image and thumnail image is also included
 *  in the argument of the callback function.
 *  For more detail information of callback, please refer 'mm_camcorder_video_capture_callback'.
 *
 *	@param[in]	camcorder	A handle of camcorder.
 *	@param[in]	callback	Function pointer of callback function.
 *	@param[in]	user_data	User parameter for passing to callback function.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@see		mm_camcorder_video_capture_callback
 *	@pre		None
 *	@post		None
 *	@remarks	registered 'callback' is called on internal thread of camcorder. Regardless of the status of main loop, this function will be called.
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean setting_capture_callback()
{
	//set callback
	mm_camcorder_set_video_capture_callback(hcam,(mm_camcorder_video_capture_callback)camcordertest_video_capture_cb, (void*)hcam);

	return TRUE;
}
 *	@endcode
 */
int mm_camcorder_set_video_capture_callback(MMHandleType camcorder, mm_camcorder_video_capture_callback callback, void *user_data);


/**
 *    mm_camcorder_set_audio_stream_callback:\n
 *  Set callback for user defined audio stream callback function.
 *  Users can retrieve audio data using registered callback.
 *  The callback function holds the same buffer that will be recorded.
 *  So if an user change the buffer, the result file will has the buffer.
 *
 *	@param[in]	camcorder	A handle of camcorder.
 *	@param[in]	callback	Function pointer of callback function.
 *	@param[in]	user_data	User parameter for passing to callback function.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@see		mm_camcorder_audio_stream_callback
 *	@pre		None
 *	@post		None
 *	@remarks	registered 'callback' is called on internal thread of camcorder. Regardless of the status of main loop, this function will be called.
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean setting_audio_stream_callback()
{
	//set callback
	mm_camcorder_set_audio_stream_callback(hcam, (mm_camcorder_audio_stream_callback)camcordertest_audio_stream_cb, (void*)hcam);

	return TRUE;
}
 *  @endcode
 */
int mm_camcorder_set_audio_stream_callback(MMHandleType camcorder, mm_camcorder_audio_stream_callback callback, void *user_data);


/**
 *    mm_camcorder_set_muxed_stream_callback:\n
 *  Set callback for user defined muxed stream callback function.
 *  Users can retrieve muxed data using registered callback.
 *  The callback function holds the same buffer that will be recorded.
 *
 *	@param[in]	camcorder	A handle of camcorder.
 *	@param[in]	callback	Function pointer of callback function.
 *	@param[in]	user_data	User parameter for passing to callback function.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@see		mm_camcorder_muxed_stream_callback
 *	@pre		None
 *	@post		None
 *	@remarks	registered 'callback' is called on internal thread of camcorder. Regardless of the status of main loop, this function will be called.
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean setting_muxed_stream_callback()
{
	//set callback
	mm_camcorder_set_muxed_stream_callback(hcam, (mm_camcorder_muxed_stream_callback)camcordertest_muxed_stream_cb, (void*)hcam);

	return TRUE;
}
 *  @endcode
 */
int mm_camcorder_set_muxed_stream_callback(MMHandleType camcorder, mm_camcorder_muxed_stream_callback callback, void *user_data);


/**
 *    mm_camcorder_get_state:\n
 *  Get the current state of camcorder.
 *  mm_camcorder is working on the base of its state. An user should check the state of mm_camcorder before calling its functions.
 *  If the handle is avaiable, user can retrieve the value.
 *
 *	@param[in]	camcorder	A handle of camcorder.
 *	@param[out]	state		On return, it contains current state of camcorder.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@see		MMCamcorderStateType
 *	@pre		None
 *	@post		None
 *	@remarks	None
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean get_state_of_camcorder()
{
	MMCamcorderStateType state;

	//Get state of camcorder
	mm_camcorder_get_state(hcam, &state);
	printf("Current status is %d\n", state);

	return TRUE;
}

 *	@endcode
 */
int mm_camcorder_get_state(MMHandleType camcorder, MMCamcorderStateType *state);
int mm_camcorder_get_state2(MMHandleType camcorder, MMCamcorderStateType *state, MMCamcorderStateType *old_state);


/**
 *    mm_camcorder_get_attributes:\n
 *  Get attributes of camcorder with given attribute names. This function can get multiple attributes
 *  simultaneously. If one of attribute fails, this function will stop at the point.
 *  'err_attr_name' let you know the name of the attribute.
 *
 *	@param[in]	camcorder	Specifies the camcorder  handle.
 *	@param[out]	err_attr_name	Specifies the name of attributes that made an error. If the function doesn't make an error, this will be null. @n
 *					Free this variable after using.
 *	@param[in]	attribute_name	attribute name that user want to get.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@pre		None
 *	@post		None
 *	@remarks	You can retrieve multiple attributes at the same time.  @n
 *			This function must finish with 'NULL' argument.  @n
 *			ex) mm_camcorder_get_attributes(....... , NULL);
 *	@see		mm_camcorder_set_attributes
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean getting_attribute()
{
	int err;
	MMCamPreset cam_info;
	char *err_attr_name = NULL;
	void * hdisplay = NULL;
	int hsize = 0;

	hdisplay = &ad.xid;		//xid of xwindow. This value can be different depending on your environment.
	hsize = sizeof(ad.xid);		//size of xid structure.

	// camcorder attribute setting
	err = mm_camcorder_get_attributes(hcamcorder, NULL,	//The second is the argument for debugging. But you can skip this.
					  MMCAM_MODE,  &mode,	//You have to input a pointer instead of variable itself.
					  NULL);		//mm_camcorder_set_attributes() should be finished with a NULL argument.

	return TRUE;
}

 *	@endcode
 */
int mm_camcorder_get_attributes(MMHandleType camcorder,  char **err_attr_name, const char *attribute_name, ...) G_GNUC_NULL_TERMINATED;



/**
 *    mm_camcorder_set_attributes:\n
 *  Set attributes of camcorder with given attribute names. This function can set multiple attributes
 *  simultaneously. If one of attribute fails, this function will stop at the point.
 *  'err_attr_name' let you know the name of the attribute.
 *
 *	@param[in]	camcorder	Specifies the camcorder  handle.
 *	@param[out]	err_attr_name	Specifies the name of attributes that made an error. If the function doesn't make an error, this will be null. @n
 *					Free this variable after using.
 *	@param[in]	attribute_name	attribute name that user want to set.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@pre		None
 *	@post		None
 *	@remarks	You can put multiple attributes to camcorder at the same time.  @n
 *			This function must finish with 'NULL' argument.  @n
 *			ex) mm_camcorder_set_attributes(....... , NULL);
 *	@see		mm_camcorder_get_attributes
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean setting_attribute()
{
	int err;
	MMCamPreset cam_info;
	char *err_attr_name = NULL;
	void * hdisplay = NULL;
	int hsize = 0;

	hdisplay = &ad.xid;		//xid of xwindow. This value can be different depending on your environment.
	hsize = sizeof(ad.xid);		//size of xid structure.

	// camcorder attribute setting
	err = mm_camcorder_set_attributes((MMHandleType)hcam, &err_attr_name,		//The second is the argument for debugging.
					  MMCAM_MODE, MM_CAMCORDER_MODE_IMAGE,
					  MMCAM_IMAGE_ENCODER, MM_IMAGE_CODEC_JPEG,
					  MMCAM_CAMERA_WIDTH, 640,
					  MMCAM_CAMERA_HEIGHT, 480,
					  MMCAM_CAMERA_FORMAT, MM_PIXEL_FORMAT_YUYV,
					  MMCAM_CAMERA_FPS, 30,
					  MMCAM_DISPLAY_ROTATION, MM_DISPLAY_ROTATION_270,
					  MMCAM_DISPLAY_HANDLE, (void*) hdisplay,          hsize,		//Beware some types require 'size' value, too. (STRING, DATA type attributes)
					  MMCAM_CAPTURE_FORMAT, MM_PIXEL_FORMAT_ENCODED,
					  MMCAM_CAPTURE_WIDTH, 640,
					  MMCAM_CAPTURE_HEIGHT, 480,
					  NULL);		//mm_camcorder_set_attributes() should be finished with a NULL argument.

	if (err < 0) {
		printf("Set attrs fail. (%s:%x)\n", err_attr_name, err);		//When the function failed, 'err_attr_name' has the name of attr that made the error.
		if (err_attr_name) {
			free(err_attr_name);			//Please free 'err_attr_name', after using the argument.
			err_attr_name = NULL;
			return FALSE;
		}
	}

	return TRUE;
}
 *	@endcode
 */
int mm_camcorder_set_attributes(MMHandleType camcorder,  char **err_attr_name, const char *attribute_name, ...) G_GNUC_NULL_TERMINATED;


/**
 *    mm_camcorder_get_attribute_info:\n
 *  Get detail information of the attribute. To manager attributes, an user may want to know the exact character of the attribute,
 *  such as type, flag, and validity. This is the function to provide such information.
 *  Depending on the 'validity_type', validity union would be different. To know about the type of union, please refer 'MMCamAttrsInfo'.
 *
 *	@param[in]	camcorder	Specifies the camcorder  handle.
 *	@param[in]	attribute_name	attribute name that user want to get information.
 *	@param[out]	info		a structure that holds information related with the attribute.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@pre		None
 *	@post		None
 *	@remarks	If the function succeeds, 'info' holds detail information about the attribute, such as type,
 *			flag, validity_type, validity_values, and default values.
 *	@see		mm_camcorder_get_attributes, mm_camcorder_set_attributes
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean getting_info_from_attribute()
{
	MMCamAttrsInfo info;
	int err;

	err = mm_camcorder_get_attribute_info(handle, MMCAM_CAPTURE_HEIGHT, &info);
	if (err < 0) {
		printf("Fail to call mm_camcorder_get_attribute_info()");
		return FALSE;
	}

	//Now 'info' has many information about 'MMCAM_CAPTURE_HEIGHT'

	return TRUE;
}
 *	@endcode
 */
int mm_camcorder_get_attribute_info(MMHandleType camcorder, const char *attribute_name, MMCamAttrsInfo *info);


/**
 *    mm_camcorder_get_fps_list_by_resolution:\n
 *  Get detail information of the fps configure. To manager fps, an user may want to know the supported fps list by the current preview resolution,
 *  Gives attribute information structure, from the configure data.
 *  Depending on the 'validity_type', validity union would be different. To know about the type of union, please refer 'MMCamAttrsInfo'.
 *
 *	@param[in]	camcorder	Specifies the camcorder  handle.
 *	@param[in]	width	width value of the current Preview resolution.
 *	@param[in]	height	height value of the current Preview resolution.
 *	@param[out]	fps_info		a structure that holds information related with the attribute.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@pre		None
 *	@post		None
 *	@remarks	If the function succeeds, 'info' holds detail information about the attribute, such as type,
 *			flag, validity_type, validity_values, and default values.
 *	@see		mm_camcorder_get_attributes, mm_camcorder_set_attributes
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean getting_info_from_attribute()
{
	MMCamAttrsInfo info;
	int err;

	err = mm_camcorder_get_fps_list_by_resolution(handle, width, height, &info);
	if (err < 0) {
		printf("Fail to call mm_camcorder_get_attribute_info()");
		return FALSE;
	}

	//Now 'info' has many information about 'MMCAM_CAPTURE_HEIGHT'

	return TRUE;
}
 *	@endcode
 */
int mm_camcorder_get_fps_list_by_resolution(MMHandleType camcorder, int width, int height, MMCamAttrsInfo *fps_info);


/**
 *    mm_camcorder_init_focusing:\n
 *  Initialize focusing. \n
 *  This function stops focusing action and adjust the camera lens to initial position.
 *  Some camera applciation requires to initialize its lens position after releasing half shutter. In that case,
 *  this should be a good choice. Comparing with mm_camcorder_stop_focusing, this function not only stops focusing,
 *  but also initialize the lens. Preview image might be out-focused after calling this function.
 *	@param[in]	camcorder  A handle of camcorder.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@pre		The status of camcorder should be MM_CAMCORDER_STATE_PREPARE, MM_CAMCORDER_STATE_RECORDING, or MM_CAMCORDER_STATE_PAUSED.
 *	@post		None
 *	@remarks	None
 *	@see		mm_camcorder_start_focusing, mm_camcorder_stop_focusing
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean start_autofocus()
{
	int err;
	char * err_attr_name = NULL;

	// Set focus mode to 'AUTO' and scan range to 'AF Normal'.
	//You just need to set these values one time. After that, just call mm_camcorder_start_focusing().
	err = mm_camcorder_set_attributes((MMHandleType)hcam, &err_attr_name,
					  MMCAM_CAMERA_FOCUS_MODE, MM_CAMCORDER_FOCUS_MODE_AUTO,
					  MMCAM_CAMERA_AF_SCAN_RANGE, MM_CAMCORDER_AUTO_FOCUS_NORMAL,
					  NULL);

	if (err < 0) {
		printf("Set attrs fail. (%s:%x)\n", err_attr_name, err);
		if (err_attr_name) {
			free(err_attr_name);
			err_attr_name = NULL;
			return FALSE;
		}
	}

	mm_camcorder_init_focusing(hcam);
	mm_camcorder_start_focusing(hcam);
	printf("Waiting for adjusting focus\n");

	// Waiting for 'MM_MESSAGE_CAMCORDER_FOCUS_CHANGED'

	return TRUE;
}

 *	@endcode
 */
int mm_camcorder_init_focusing(MMHandleType camcorder);


/**
 *    mm_camcorder_start_focusing:\n
 *  Start focusing. \n
 *  This function command to start focusing opeartion. Because focusing operation depends on mechanic or electric module,
 *  it may take small amount of time. (For ex, 500ms ~ 3sec). \n
 *  This function works asynchronously. When an user call this function,  it will return immediately.
 *  However, focusing operation will continue until it gets results.
 *  After finishing operation, you can get 'MM_MESSAGE_CAMCORDER_FOCUS_CHANGED' message.
 *  'param.code' of the message structure describes the fucusing was success or not.
 *
 *	@param[in]	camcorder	A handle of camcorder.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@pre		None
 *	@post		None
 *	@remarks	None
 *	@see		mm_camcorder_init_focusing, mm_camcorder_stop_focusing
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean start_autofocus()
{
	int err;
	char * err_attr_name = NULL;

	// Set focus mode to 'AUTO' and scan range to 'AF Normal'.
	//You just need to set these values one time. After that, just call mm_camcorder_start_focusing().
	err = mm_camcorder_set_attributes((MMHandleType)hcam, &err_attr_name,
					  MMCAM_CAMERA_FOCUS_MODE, MM_CAMCORDER_FOCUS_MODE_AUTO,
					  MMCAM_CAMERA_AF_SCAN_RANGE, MM_CAMCORDER_AUTO_FOCUS_NORMAL,
					  NULL);

	if (err < 0) {
		printf("Set attrs fail. (%s:%x)\n", err_attr_name, err);
		if (err_attr_name) {
			free(err_attr_name);
			err_attr_name = NULL;
			return FALSE;
		}
	}

	mm_camcorder_init_focusing(hcam);
	mm_camcorder_start_focusing(hcam);
	printf("Waiting for adjusting focus\n");

	// Waiting for 'MM_MESSAGE_CAMCORDER_FOCUS_CHANGED'

	return TRUE;
}

 *	@endcode
 */
int mm_camcorder_start_focusing(MMHandleType camcorder);


/**
 *    mm_camcorder_stop_focusing:\n
 *  Stop focusing. This function halts focusing operation.\n
 *  This is the function to stop focusing in the middle of the operation. So if focusing is already finished or not started yet,
 *  this function will do nothing.
 *
 *	@param[in]	camcorder	A handle of camcorder.
 *	@return		This function returns zero(MM_ERROR_NONE) on success, or negative value with error code.\n
 *			Please refer 'mm_error.h' to know the exact meaning of the error.
 *	@see		mm_camcorder_init_focusing, mm_camcorder_start_focusing
 *	@pre		mm_camcorder_start_focusing() should be called before calling this function.
 *	@post		None
 *	@remarks	None
 *	@par example
 *	@code

#include <mm_camcorder.h>

gboolean stop_autofocus()
{
	int err;

	//Stop focusing
	mm_camcorder_stop_focusing(hcam);

	return TRUE;
}

 *	@endcode
 */
int mm_camcorder_stop_focusing(MMHandleType camcorder);

/* emit signal */
void mm_camcorder_emit_signal(MMHandleType camcorder, const char *object_name,
	const char *interface_name, const char *signal_name, int value);

/* check compatability between codec and file format */
int mm_camcorder_check_codec_fileformat_compatibility(const char *codec_type, int codec, int file_format);

/* external storage state management */
int mm_camcorder_manage_external_storage_state(MMHandleType camcorder, int storage_state);

/**
	@}
 */

#ifdef __cplusplus
}
#endif

#endif /* __MM_CAMCORDER_H__ */
