/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd All Rights Reserved
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
 */


#include <gio/gio.h>
#include "gtests_libmm_camcorder.h"

using namespace std;
using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestCase;

int g_ret;
int g_frame_count;
MMHandleType g_cam_handle;
MMCamPreset g_info;
GDBusConnection *g_dbus_connection;
GMutex g_lock;
GCond g_capture_cond;
GCond g_focus_cond;

static int _message_callback(int id, void *param, void *user_param)
{
	MMMessageParamType *m = (MMMessageParamType *)param;

	cout << "[ENTER]" << endl;

	switch (id) {
	case MM_MESSAGE_CAMCORDER_STATE_CHANGED:
		cout << "[STATE CHANGED] " << m->state.previous << " -> " << m->state.current << endl;
		break;
	case MM_MESSAGE_CAMCORDER_CAPTURED:
		cout << "[CAPTURED MESSAGE] SEND SIGNAL" << endl;
		g_mutex_lock(&g_lock);
		g_cond_signal(&g_capture_cond);
		g_mutex_unlock(&g_lock);
		break;
	case MM_MESSAGE_CAMCORDER_FOCUS_CHANGED:
		cout << "[FOCUS CHANGED] SEND SIGNAL" << endl;
		g_mutex_lock(&g_lock);
		g_cond_signal(&g_focus_cond);
		g_mutex_unlock(&g_lock);
		break;
	default:
		break;
	}

	cout << "[LEAVE]" << endl;

	return 1;
}

static gboolean _get_video_recording_settings(int *video_encoder, int *audio_encoder, int *file_format)
{
	int i = 0;
	int j = 0;
	int tmp_v_enc;
	int tmp_a_enc;
	int tmp_fmt;
	MMCamAttrsInfo info_a_enc;
	MMCamAttrsInfo info_fmt;

	if (!video_encoder || !audio_encoder || !file_format) {
		cout << "NULL pointer" << endl;
		return FALSE;
	}

	/* get supported audio encoder list */
	mm_camcorder_get_attribute_info(g_cam_handle, MMCAM_AUDIO_ENCODER, &info_a_enc);

	/* get supported file format list */
	mm_camcorder_get_attribute_info(g_cam_handle, MMCAM_FILE_FORMAT, &info_fmt);

	/* compatibility check */
	tmp_v_enc = MM_VIDEO_CODEC_MPEG4;

	for (i = 0 ; i < info_fmt.int_array.count ; i++) {
		tmp_fmt = info_fmt.int_array.array[i];
		if (mm_camcorder_check_codec_fileformat_compatibility(MMCAM_VIDEO_ENCODER, tmp_v_enc, tmp_fmt) == MM_ERROR_NONE) {
			for (j = 0 ; info_a_enc.int_array.count ; j++) {
				tmp_a_enc = info_a_enc.int_array.array[j];
				if (mm_camcorder_check_codec_fileformat_compatibility(MMCAM_AUDIO_ENCODER, tmp_a_enc, tmp_fmt) == MM_ERROR_NONE) {
					cout << "[VIDEO RECORDING SETTING FOUND]" << endl;

					*video_encoder = tmp_v_enc;
					*audio_encoder = tmp_a_enc;
					*file_format = tmp_fmt;

					return TRUE;
				}
			}
		}
	}

	cout << "[VIDEO RECORDING SETTING FAILED]" << endl;

	return FALSE;
}

static gboolean _get_audio_recording_settings(int *audio_encoder, int *file_format)
{
	int i = 0;
	int tmp_a_enc;
	int tmp_fmt;
	MMCamAttrsInfo info_a_enc;
	MMCamAttrsInfo info_fmt;

	if (!audio_encoder || !file_format) {
		cout << "NULL pointer" << endl;
		return FALSE;
	}

	/* get supported audio encoder list */
	mm_camcorder_get_attribute_info(g_cam_handle, MMCAM_AUDIO_ENCODER, &info_a_enc);

	/* get supported file format list */
	mm_camcorder_get_attribute_info(g_cam_handle, MMCAM_FILE_FORMAT, &info_fmt);

	/* compatibility check */
	tmp_a_enc = info_a_enc.int_array.array[0];

	for (i = 0 ; i < info_fmt.int_array.count ; i++) {
		tmp_fmt = info_fmt.int_array.array[i];
		if (mm_camcorder_check_codec_fileformat_compatibility(MMCAM_AUDIO_ENCODER, tmp_a_enc, tmp_fmt) == MM_ERROR_NONE) {
			cout << "[AUDIO RECORDING SETTING FOUND]" << endl;

			*audio_encoder = tmp_a_enc;
			*file_format = tmp_fmt;

			return TRUE;
		}
	}

	cout << "[VIDEO RECORDING SETTING FAILED]" << endl;

	return FALSE;
}

static gboolean _video_stream_callback(MMCamcorderVideoStreamDataType *stream, void *user_param)
{
	cout << "[VIDEO_STREAM_CALLBACK]" << endl;
	return TRUE;
}

static gboolean _audio_stream_callback(MMCamcorderAudioStreamDataType *stream, void *user_param)
{
	cout << "[AUDIO_STREAM_CALLBACK]" << endl;
	return TRUE;
}

static gboolean _muxed_stream_callback(MMCamcorderMuxedStreamDataType *stream, void *user_param)
{
	cout << "[MUXED_STREAM_CALLBACK]" << endl;

	g_frame_count++;

	return TRUE;
}

static gboolean _video_capture_callback(MMCamcorderCaptureDataType *frame, MMCamcorderCaptureDataType *thumbnail, void *user_data)
{
	cout << "[CAPTURE_CALLBACK]" << endl;
	return TRUE;
}

static int _start_preview(MMHandleType handle)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_realize(handle);
	if (ret != MM_ERROR_NONE) {
		cout << "[REALIZE failed]" << endl;
		return ret;
	}

	ret = mm_camcorder_start(handle);
	if (ret != MM_ERROR_NONE) {
		cout << "[START failed]" << endl;
		mm_camcorder_unrealize(handle);
	}

	return ret;
}

static void _stop_preview(MMHandleType handle)
{
	mm_camcorder_stop(handle);
	mm_camcorder_unrealize(handle);
}


class MMCamcorderTest : public ::testing::Test {
	protected:
		void SetUp() {
			cout << "[SetUp]" << endl;

			if (g_dbus_connection) {
				g_object_unref(g_dbus_connection);
				g_dbus_connection = NULL;
			}

			g_dbus_connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);

			if (g_cam_handle) {
				mm_camcorder_destroy(g_cam_handle);
				g_cam_handle = NULL;
			}

			g_info.videodev_type = MM_VIDEO_DEVICE_CAMERA0;

			g_ret = mm_camcorder_create(&g_cam_handle, &g_info);

			cout << "handle " << g_cam_handle << ", dbus connection " << g_dbus_connection << endl;

			/* set gdbus connection */
			if (g_cam_handle && g_dbus_connection) {
				mm_camcorder_set_attributes(g_cam_handle, NULL,
					MMCAM_GDBUS_CONNECTION, g_dbus_connection, 4,
					NULL);
			}

			/* set message callback */
			if (mm_camcorder_set_message_callback(g_cam_handle, _message_callback, g_cam_handle) != MM_ERROR_NONE)
				cout << "[FAILED] set message callback" << endl;

			return;
		}

		void TearDown() {
			cout << "[TearDown]" << endl << endl;

			if (g_cam_handle) {
				mm_camcorder_destroy(g_cam_handle);
				g_cam_handle = NULL;
			}

			if (g_dbus_connection) {
				g_object_unref(g_dbus_connection);
				g_dbus_connection = NULL;
			}

			return;
		}
};

TEST_F(MMCamcorderTest, CreateP)
{
	EXPECT_EQ(g_ret, MM_ERROR_NONE);
}

TEST_F(MMCamcorderTest, CreateN)
{
	int ret = mm_camcorder_create(NULL, NULL);

	EXPECT_EQ(ret, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);
}

TEST_F(MMCamcorderTest, DestroyP)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_destroy(g_cam_handle);
	ASSERT_EQ(ret, MM_ERROR_NONE);

	g_cam_handle = NULL;
}

TEST_F(MMCamcorderTest, DestroyN)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_destroy(NULL);
	EXPECT_EQ(ret, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);
}

TEST_F(MMCamcorderTest, RealizeP)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_realize(g_cam_handle);

	ASSERT_EQ(ret, MM_ERROR_NONE);

	mm_camcorder_unrealize(g_cam_handle);
}

TEST_F(MMCamcorderTest, RealizeN)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_realize(NULL);

	EXPECT_EQ(ret, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);
}

TEST_F(MMCamcorderTest, UnrealizeP)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_realize(g_cam_handle);

	ASSERT_EQ(ret, MM_ERROR_NONE);

	ret = mm_camcorder_unrealize(g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_NONE);
}

TEST_F(MMCamcorderTest, UnrealizeN)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_realize(g_cam_handle);

	ASSERT_EQ(ret, MM_ERROR_NONE);

	ret = mm_camcorder_unrealize(NULL);
	EXPECT_EQ(ret, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	mm_camcorder_unrealize(g_cam_handle);
}

TEST_F(MMCamcorderTest, StartP)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_realize(g_cam_handle);
	ASSERT_EQ(ret, MM_ERROR_NONE);

	ret = mm_camcorder_start(g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_NONE);

	mm_camcorder_stop(g_cam_handle);
	mm_camcorder_unrealize(g_cam_handle);
}

TEST_F(MMCamcorderTest, StartN)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_realize(g_cam_handle);
	ASSERT_EQ(ret, MM_ERROR_NONE);

	ret = mm_camcorder_start(NULL);
	EXPECT_EQ(ret, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	mm_camcorder_unrealize(g_cam_handle);
}

TEST_F(MMCamcorderTest, StopP)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_realize(g_cam_handle);
	ret |= mm_camcorder_start(g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_NONE);

	if (ret == MM_ERROR_NONE) {
		ret = mm_camcorder_stop(g_cam_handle);
		EXPECT_EQ(ret, MM_ERROR_NONE);
	}

	mm_camcorder_unrealize(g_cam_handle);
}

TEST_F(MMCamcorderTest, StopN)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_realize(g_cam_handle);
	ret |= mm_camcorder_start(g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_NONE);

	if (ret == MM_ERROR_NONE) {
		ret = mm_camcorder_stop(NULL);
		EXPECT_EQ(ret, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);
	}

	mm_camcorder_stop(g_cam_handle);
	mm_camcorder_unrealize(g_cam_handle);
}

TEST_F(MMCamcorderTest, CaptureStartP)
{
	int ret = MM_ERROR_NONE;
	gboolean ret_wait = FALSE;
	gint64 end_time = 0;

	ASSERT_EQ(_start_preview(g_cam_handle), MM_ERROR_NONE);

	g_mutex_lock(&g_lock);

	if (mm_camcorder_set_video_capture_callback(g_cam_handle, _video_capture_callback, g_cam_handle) != MM_ERROR_NONE)
		cout << "[FAILED] set video capture callback" << endl;

	ret = mm_camcorder_capture_start(g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_NONE);

	if (ret == MM_ERROR_NONE) {
		end_time = g_get_monotonic_time() + 3 * G_TIME_SPAN_SECOND;

		ret_wait = g_cond_wait_until(&g_capture_cond, &g_lock, end_time);

		EXPECT_EQ(ret_wait, TRUE);

		mm_camcorder_capture_stop(g_cam_handle);
	}

	g_mutex_unlock(&g_lock);

	_stop_preview(g_cam_handle);
}

TEST_F(MMCamcorderTest, CaptureStartN1)
{
	int ret = MM_ERROR_NONE;

	ASSERT_EQ(_start_preview(g_cam_handle), MM_ERROR_NONE);

	ret = mm_camcorder_capture_start(NULL);
	EXPECT_EQ(ret, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);

	_stop_preview(g_cam_handle);
}

TEST_F(MMCamcorderTest, CaptureStartN2)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_realize(g_cam_handle);
	ASSERT_EQ(ret, MM_ERROR_NONE);

	ret = mm_camcorder_capture_start(g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_CAMCORDER_INVALID_STATE);

	mm_camcorder_unrealize(g_cam_handle);
}

TEST_F(MMCamcorderTest, CaptureStopP)
{
	int ret = MM_ERROR_NONE;
	gboolean ret_wait = FALSE;
	gint64 end_time = 0;

	ASSERT_EQ(_start_preview(g_cam_handle), MM_ERROR_NONE);

	g_mutex_lock(&g_lock);

	if (mm_camcorder_set_video_capture_callback(g_cam_handle, _video_capture_callback, g_cam_handle) != MM_ERROR_NONE)
		cout << "[FAILED] set video capture callback" << endl;

	ret = mm_camcorder_capture_start(g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_NONE);

	if (ret == MM_ERROR_NONE) {
		end_time = g_get_monotonic_time() + 3 * G_TIME_SPAN_SECOND;

		ret_wait = g_cond_wait_until(&g_capture_cond, &g_lock, end_time);

		EXPECT_EQ(ret_wait, TRUE);

		ret = mm_camcorder_capture_stop(g_cam_handle);
		EXPECT_EQ(ret, MM_ERROR_NONE);
	}

	g_mutex_unlock(&g_lock);

	_stop_preview(g_cam_handle);
}

TEST_F(MMCamcorderTest, SetMessageCallbackP)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_set_message_callback(g_cam_handle, _message_callback, g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_NONE);
}

TEST_F(MMCamcorderTest, SetMessageCallbackN)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_set_message_callback(NULL, _message_callback, g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);
}

TEST_F(MMCamcorderTest, SetVideoStreamCallbackP)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_set_video_stream_callback(g_cam_handle, _video_stream_callback, g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_NONE);
}

TEST_F(MMCamcorderTest, SetVideoStreamCallbackN)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_set_video_stream_callback(NULL, _video_stream_callback, g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);
}

TEST_F(MMCamcorderTest, SetAudioStreamCallbackP)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_set_audio_stream_callback(g_cam_handle, _audio_stream_callback, g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_NONE);
}

TEST_F(MMCamcorderTest, SetAudioStreamCallbackN)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_set_audio_stream_callback(NULL, _audio_stream_callback, g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);
}

TEST_F(MMCamcorderTest, SetVideoCaptureCallbackP)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_set_video_capture_callback(g_cam_handle, _video_capture_callback, g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_NONE);
}

TEST_F(MMCamcorderTest, SetVideoCaptureCallbackN)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_set_video_capture_callback(NULL, _video_capture_callback, g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);
}

TEST_F(MMCamcorderTest, SetMuxedStreamCallbackP)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_set_muxed_stream_callback(g_cam_handle, _muxed_stream_callback, g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_NONE);
}

TEST_F(MMCamcorderTest, SetMuxedStreamCallbackN)
{
	int ret = MM_ERROR_NONE;

	ret = mm_camcorder_set_muxed_stream_callback(NULL, _muxed_stream_callback, g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_CAMCORDER_INVALID_ARGUMENT);
}

TEST_F(MMCamcorderTest, RecordP)
{
	int ret = MM_ERROR_NONE;
	int video_encoder = 0;
	int audio_encoder = 0;
	int file_format = 0;
	gboolean ret_settings = FALSE;

	ASSERT_EQ(_start_preview(g_cam_handle), MM_ERROR_NONE);

	g_frame_count = 0;

	if (mm_camcorder_set_muxed_stream_callback(g_cam_handle, _muxed_stream_callback, g_cam_handle) != MM_ERROR_NONE)
		cout << "[FAILED] set muxed stream callback" << endl;

	ret_settings = _get_video_recording_settings(&video_encoder, &audio_encoder, &file_format);
	EXPECT_EQ(ret_settings, TRUE);

	ret = mm_camcorder_set_attributes(g_cam_handle, NULL,
		MMCAM_VIDEO_ENCODER, video_encoder,
		MMCAM_AUDIO_ENCODER, audio_encoder,
		MMCAM_FILE_FORMAT, file_format,
		NULL);
	EXPECT_EQ(ret, MM_ERROR_NONE);

	ret = mm_camcorder_record(g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_NONE);

	if (ret == MM_ERROR_NONE) {
		sleep(5);

		EXPECT_EQ(g_frame_count > 30, TRUE);

		mm_camcorder_cancel(g_cam_handle);
	}

	_stop_preview(g_cam_handle);
}

TEST_F(MMCamcorderTest, CommitP)
{
	int ret = MM_ERROR_NONE;
	int video_encoder = 0;
	int audio_encoder = 0;
	int file_format = 0;
	gboolean ret_settings = FALSE;

	ASSERT_EQ(_start_preview(g_cam_handle), MM_ERROR_NONE);

	if (mm_camcorder_set_muxed_stream_callback(g_cam_handle, _muxed_stream_callback, g_cam_handle) != MM_ERROR_NONE)
		cout << "[FAILED] set muxed stream callback" << endl;

	ret_settings = _get_video_recording_settings(&video_encoder, &audio_encoder, &file_format);
	EXPECT_EQ(ret_settings, TRUE);

	ret = mm_camcorder_set_attributes(g_cam_handle, NULL,
		MMCAM_VIDEO_ENCODER, video_encoder,
		MMCAM_AUDIO_ENCODER, audio_encoder,
		MMCAM_FILE_FORMAT, file_format,
		NULL);
	EXPECT_EQ(ret, MM_ERROR_NONE);

	ret = mm_camcorder_record(g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_NONE);

	if (ret == MM_ERROR_NONE) {
		sleep(5);
		ret = mm_camcorder_commit(g_cam_handle);
		EXPECT_EQ(ret, MM_ERROR_NONE);
	}

	_stop_preview(g_cam_handle);
}

TEST_F(MMCamcorderTest, CancelP)
{
	int ret = MM_ERROR_NONE;
	int video_encoder = 0;
	int audio_encoder = 0;
	int file_format = 0;
	gboolean ret_settings = FALSE;

	ASSERT_EQ(_start_preview(g_cam_handle), MM_ERROR_NONE);

	if (mm_camcorder_set_muxed_stream_callback(g_cam_handle, _muxed_stream_callback, g_cam_handle) != MM_ERROR_NONE)
		cout << "[FAILED] set muxed stream callback" << endl;

	ret_settings = _get_video_recording_settings(&video_encoder, &audio_encoder, &file_format);
	EXPECT_EQ(ret_settings, TRUE);

	ret = mm_camcorder_set_attributes(g_cam_handle, NULL,
		MMCAM_VIDEO_ENCODER, video_encoder,
		MMCAM_AUDIO_ENCODER, audio_encoder,
		MMCAM_FILE_FORMAT, file_format,
		NULL);
	EXPECT_EQ(ret, MM_ERROR_NONE);

	ret = mm_camcorder_record(g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_NONE);

	if (ret == MM_ERROR_NONE) {
		ret = mm_camcorder_cancel(g_cam_handle);
		EXPECT_EQ(ret, MM_ERROR_NONE);
	}

	_stop_preview(g_cam_handle);
}

TEST_F(MMCamcorderTest, PauseP)
{
	int ret = MM_ERROR_NONE;
	int video_encoder = 0;
	int audio_encoder = 0;
	int file_format = 0;
	gboolean ret_settings = FALSE;

	ASSERT_EQ(_start_preview(g_cam_handle), MM_ERROR_NONE);

	if (mm_camcorder_set_muxed_stream_callback(g_cam_handle, _muxed_stream_callback, g_cam_handle) != MM_ERROR_NONE)
		cout << "[FAILED] set muxed stream callback" << endl;

	ret_settings = _get_video_recording_settings(&video_encoder, &audio_encoder, &file_format);
	EXPECT_EQ(ret_settings, TRUE);

	ret = mm_camcorder_set_attributes(g_cam_handle, NULL,
		MMCAM_VIDEO_ENCODER, video_encoder,
		MMCAM_AUDIO_ENCODER, audio_encoder,
		MMCAM_FILE_FORMAT, file_format,
		NULL);
	EXPECT_EQ(ret, MM_ERROR_NONE);

	ret = mm_camcorder_record(g_cam_handle);
	EXPECT_EQ(ret, MM_ERROR_NONE);

	if (ret == MM_ERROR_NONE) {
		sleep(5);

		ret = mm_camcorder_pause(g_cam_handle);
		EXPECT_EQ(ret, MM_ERROR_NONE);

		mm_camcorder_cancel(g_cam_handle);
	}

	_stop_preview(g_cam_handle);
}

TEST_F(MMCamcorderTest, GetStateP)
{
	int ret = MM_ERROR_NONE;
	MMCamcorderStateType state = MM_CAMCORDER_STATE_NONE;

	ret = mm_camcorder_get_state(g_cam_handle, &state);
	ASSERT_EQ(ret, MM_ERROR_NONE);
}

TEST_F(MMCamcorderTest, GetState2P)
{
	int ret = MM_ERROR_NONE;
	MMCamcorderStateType state = MM_CAMCORDER_STATE_NONE;
	MMCamcorderStateType old_state = MM_CAMCORDER_STATE_NONE;

	ret = mm_camcorder_get_state2(g_cam_handle, &state, &old_state);
	ASSERT_EQ(ret, MM_ERROR_NONE);
}

TEST_F(MMCamcorderTest, GetAttributesP)
{
	int ret = MM_ERROR_NONE;
	int device_count = 0;
	int audio_device = 0;

	ret = mm_camcorder_get_attributes(g_cam_handle, NULL,
		MMCAM_CAMERA_DEVICE_COUNT, &device_count,
		MMCAM_AUDIO_DEVICE, &audio_device,
		NULL);
	ASSERT_EQ(ret, MM_ERROR_NONE);
}

TEST_F(MMCamcorderTest, SetAttributesP)
{
	int ret = MM_ERROR_NONE;
	int audio_encoder = 0;
	int file_format = 0;
	gboolean ret_settings = FALSE;

	ret_settings = _get_audio_recording_settings(&audio_encoder, &file_format);
	ASSERT_EQ(ret_settings, TRUE);

	ret = mm_camcorder_set_attributes(g_cam_handle, NULL,
		MMCAM_MODE, MM_CAMCORDER_MODE_AUDIO,
		MMCAM_AUDIO_ENCODER, audio_encoder,
		MMCAM_FILE_FORMAT, file_format,
		NULL);
	ASSERT_EQ(ret, MM_ERROR_NONE);
}

TEST_F(MMCamcorderTest, GetAttributeInfoP)
{
	int ret = MM_ERROR_NONE;
	MMCamAttrsInfo info;

	ret = mm_camcorder_get_attribute_info(g_cam_handle, MMCAM_MODE, &info);
	ASSERT_EQ(ret, MM_ERROR_NONE);
}

TEST_F(MMCamcorderTest, GetFPSListByResolutionP)
{
	int ret = MM_ERROR_NONE;
	int width = 0;
	int height = 0;
	MMCamAttrsInfo fps_info;

	ret = mm_camcorder_get_attributes(g_cam_handle, NULL,
		MMCAM_CAMERA_WIDTH, &width,
		MMCAM_CAMERA_HEIGHT, &height,
		NULL);
	ASSERT_EQ(ret, MM_ERROR_NONE);

	ret = mm_camcorder_get_fps_list_by_resolution(g_cam_handle, width, height, &fps_info);
	ASSERT_EQ(ret, MM_ERROR_NONE);
}

TEST_F(MMCamcorderTest, InitFocsingP)
{
	ASSERT_EQ(_start_preview(g_cam_handle), MM_ERROR_NONE);

	EXPECT_EQ(mm_camcorder_init_focusing(g_cam_handle), MM_ERROR_NONE);

	_stop_preview(g_cam_handle);
}

TEST_F(MMCamcorderTest, StartFocusingP)
{
	gint64 end_time = 0;

	ASSERT_EQ(_start_preview(g_cam_handle), MM_ERROR_NONE);

	g_mutex_lock(&g_lock);

	EXPECT_EQ(mm_camcorder_start_focusing(g_cam_handle), MM_ERROR_NONE);

	end_time = g_get_monotonic_time() + 3 * G_TIME_SPAN_SECOND;

	EXPECT_EQ(g_cond_wait_until(&g_focus_cond, &g_lock, end_time), TRUE);

	g_mutex_unlock(&g_lock);

	_stop_preview(g_cam_handle);
}

TEST_F(MMCamcorderTest, StopFocusingP)
{
	ASSERT_EQ(_start_preview(g_cam_handle), MM_ERROR_NONE);

	EXPECT_EQ(mm_camcorder_start_focusing(g_cam_handle), MM_ERROR_NONE);
	EXPECT_EQ(mm_camcorder_stop_focusing(g_cam_handle), MM_ERROR_NONE);

	_stop_preview(g_cam_handle);
}


int main(int argc, char **argv)
{
	InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
