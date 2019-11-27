/*
 * libmm-camcorder
 *
 * Copyright (c) 2019 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Hyuntae Kim <ht1211.kim@samsung.com>
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
========================================================================================*/
#ifdef _MMCAMCORDER_RM_SUPPORT
#include <aul.h>
#include <rm_api.h>
#include "mm_camcorder_rm.h"
#include "mm_camcorder_internal.h"

static rm_cb_result __mmcamcorder_rm_callback(int handle, rm_callback_type event_src,
	rm_device_request_s *info, void* cb_data)
{
	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(cb_data);
	int current_state = MM_CAMCORDER_STATE_NONE;
	rm_cb_result cb_res = RM_CB_RESULT_OK;

	mmf_return_val_if_fail((MMHandleType)hcamcorder, RM_CB_RESULT_OK);

	current_state = _mmcamcorder_get_state((MMHandleType)hcamcorder);

	_mmcam_dbg_warn("current state %d (handle %p)", current_state, hcamcorder);

	_MMCAMCORDER_LOCK_INTERRUPT(hcamcorder);

	/* set RM event code for sending it to application */
	hcamcorder->interrupt_code = event_src;

	_mmcam_dbg_log("RM conflict callback : event code 0x%x", event_src);
	switch (event_src) {
	case RM_CALLBACK_TYPE_RESOURCE_CONFLICT:
	case RM_CALLBACK_TYPE_RESOURCE_CONFLICT_UD:
		__mmcamcorder_force_stop(hcamcorder, _MMCAMCORDER_STATE_CHANGE_BY_RM);
		break;
	default:
		break;
	}

	_MMCAMCORDER_UNLOCK_INTERRUPT(hcamcorder);

	return cb_res;
}



int _mmcamcorder_rm_create(MMHandleType handle)
{
	int ret = RM_OK;
	rm_consumer_info rci;
	int app_pid = 0;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	if (!hcamcorder) {
		_mmcam_dbg_err("Not initialized");
		return MM_ERROR_CAMCORDER_NOT_INITIALIZED;
	}

	mm_camcorder_get_attributes(handle, NULL,
		MMCAM_CLIENT_PID, &app_pid,
		NULL);
	rci.app_pid = app_pid;
	aul_app_get_appid_bypid(rci.app_pid, rci.app_id, sizeof(rci.app_id));

	/* RM register */
	if (hcamcorder->rm_handle == 0) {
		ret = rm_register((rm_resource_cb)__mmcamcorder_rm_callback, (void*)hcamcorder, &(hcamcorder->rm_handle), &rci);
		if (ret != RM_OK) {
			_mmcam_dbg_err("rm_register fail ret = %d",ret);
			return MM_ERROR_RESOURCE_INTERNAL;
		}
	}
	return MM_ERROR_NONE;
}

int _mmcamcorder_rm_allocate(MMHandleType handle)
{
	int iret = RM_OK;
	int preview_format = MM_PIXEL_FORMAT_NV12;
	int qret = RM_OK;
	int qret_avail = 0; /* 0: not available, 1: available */
	int resource_count = 0;
	int display_surface_type = MM_DISPLAY_SURFACE_OVERLAY;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);
	if (!hcamcorder) {
		_mmcam_dbg_err("Not initialized");
		return MM_ERROR_CAMCORDER_NOT_INITIALIZED;
	}

	mm_camcorder_get_attributes(handle, NULL,
		MMCAM_DISPLAY_SURFACE, &display_surface_type,
		NULL);

	if (display_surface_type != MM_DISPLAY_SURFACE_NULL) {
		mm_camcorder_get_attributes(handle, NULL,
			MMCAM_CAMERA_FORMAT, &preview_format,
			NULL);

		resource_count = 0;
		memset(&hcamcorder->request_resources, 0x0, sizeof(rm_category_request_s));
		memset(&hcamcorder->returned_devices, 0x0, sizeof(rm_device_return_s));

		if (preview_format == MM_PIXEL_FORMAT_ENCODED_H264) {
			hcamcorder->request_resources.state[resource_count] = RM_STATE_EXCLUSIVE;
			hcamcorder->request_resources.category_id[resource_count] = RM_CATEGORY_VIDEO_DECODER;

			_mmcam_dbg_log("request dec rsc - category 0x%x", RM_CATEGORY_VIDEO_DECODER);

			resource_count++;
		}

		if (display_surface_type == MM_DISPLAY_SURFACE_OVERLAY) {
			hcamcorder->request_resources.state[resource_count] = RM_STATE_EXCLUSIVE;
			hcamcorder->request_resources.category_id[resource_count] = RM_CATEGORY_SCALER;

			_mmcam_dbg_log("request scaler rsc - category 0x%x", RM_CATEGORY_SCALER);

			resource_count++;
		}

		hcamcorder->request_resources.request_num = resource_count;

		if (resource_count > 0) {
			qret = rm_query(hcamcorder->rm_handle, RM_QUERY_ALLOCATION, &(hcamcorder->request_resources), &qret_avail);
			if (qret != RM_OK || qret_avail != 1) {
				_mmcam_dbg_log("rm query failed. retry with sub devices");

				resource_count = 0;
				memset(&hcamcorder->request_resources, 0x0, sizeof(rm_category_request_s));
				memset(&hcamcorder->returned_devices, 0x0, sizeof(rm_device_return_s));

				if (preview_format == MM_PIXEL_FORMAT_ENCODED_H264) {
					hcamcorder->request_resources.state[resource_count] = RM_STATE_EXCLUSIVE;
					hcamcorder->request_resources.category_id[resource_count] = RM_CATEGORY_VIDEO_DECODER_SUB;
					_mmcam_dbg_log("request dec rsc - category 0x%x", RM_CATEGORY_VIDEO_DECODER_SUB);
					resource_count++;
				}

				if (display_surface_type == MM_DISPLAY_SURFACE_OVERLAY) {
					hcamcorder->request_resources.state[resource_count] = RM_STATE_EXCLUSIVE;
					hcamcorder->request_resources.category_id[resource_count] = RM_CATEGORY_SCALER_SUB;
					_mmcam_dbg_log("request scaler rsc - category 0x%x", RM_CATEGORY_SCALER_SUB);
					resource_count++;
				}
			}
		}
	}

	hcamcorder->request_resources.state[resource_count] = RM_STATE_EXCLUSIVE;
	hcamcorder->request_resources.category_id[resource_count] = RM_CATEGORY_CAMERA;

	hcamcorder->request_resources.request_num = resource_count + 1;
	_mmcam_dbg_log("request camera rsc - category 0x%x", RM_CATEGORY_CAMERA);

	iret = rm_allocate_resources(hcamcorder->rm_handle, &(hcamcorder->request_resources), &hcamcorder->returned_devices);
	if (iret != RM_OK) {
		_mmcam_dbg_err("Resource allocation request failed ret = %d",iret);
		return MM_ERROR_RESOURCE_INTERNAL;
	}

	return MM_ERROR_NONE;
}


int _mmcamcorder_rm_deallocate(MMHandleType handle)
{
	int rm_ret = RM_OK;
	int idx = 0;
	rm_device_request_s requested;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	if (!hcamcorder->rm_handle) {
		_mmcam_dbg_err("Resource is not initialized ");
		return MM_ERROR_RESOURCE_NOT_INITIALIZED;
	}

	if (hcamcorder->returned_devices.allocated_num > 0) {
		memset(&requested, 0x0, sizeof(rm_device_request_s));
		requested.request_num = hcamcorder->returned_devices.allocated_num;
		for (idx = 0; idx < requested.request_num; idx++)
			requested.device_id[idx] = hcamcorder->returned_devices.device_id[idx];

		rm_ret = rm_deallocate_resources(hcamcorder->rm_handle, &requested);
		if (rm_ret != RM_OK)
			_mmcam_dbg_err("Resource deallocation request failed ");
	}

	return MM_ERROR_NONE;
}

int _mmcamcorder_rm_release(MMHandleType handle)
{
	int rm_ret = RM_OK;

	mmf_camcorder_t *hcamcorder = MMF_CAMCORDER(handle);

	if (!hcamcorder->rm_handle) {
		_mmcam_dbg_err("Resource is not initialized ");
		return MM_ERROR_RESOURCE_NOT_INITIALIZED;
	}

	/* unregister RM */
	rm_ret = rm_unregister(hcamcorder->rm_handle);
	if (rm_ret != RM_OK)
		_mmcam_dbg_err("rm_unregister() failed");
	hcamcorder->rm_handle = 0;

	return MM_ERROR_NONE;
}

#endif /* _MMCAMCORDER_RM_SUPPORT*/

