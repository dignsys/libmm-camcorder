/*
 * libmm-camcorder
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
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

#include "mm_camcorder_internal.h"
#include "mm_camcorder_resource.h"
#include <murphy/common/glib-glue.h>

#define MRP_APP_CLASS_FOR_CAMCORDER   "media"
#define MRP_RESOURCE_TYPE_MANDATORY TRUE
#define MRP_RESOURCE_TYPE_EXCLUSIVE FALSE

const char* mm_camcorder_resource_str[MM_CAMCORDER_RESOURCE_MAX] = {
	"camera",
	"video_overlay"
};

#define MMCAMCORDER_CHECK_RESOURCE_MANAGER_INSTANCE(x_camcorder_resource_manager) \
do { \
	if (!x_camcorder_resource_manager) { \
		_mmcam_dbg_err("no resource manager instance"); \
		return MM_ERROR_INVALID_ARGUMENT; \
	} \
} while (0);

#define MMCAMCORDER_CHECK_CONNECTION_RESOURCE_MANAGER(x_camcorder_resource_manager) \
do { \
	if (!x_camcorder_resource_manager) { \
		_mmcam_dbg_err("no resource manager instance"); \
		return MM_ERROR_INVALID_ARGUMENT; \
	} else { \
		if (!x_camcorder_resource_manager->is_connected) { \
			_mmcam_dbg_err("not connected to resource server yet"); \
			return MM_ERROR_RESOURCE_NOT_INITIALIZED; \
		} \
	} \
} while (0);

static char *__mmcamcorder_resource_state_to_str(mrp_res_resource_state_t st)
{
	char *state = "unknown";
	switch (st) {
	case MRP_RES_RESOURCE_ACQUIRED:
		state = "acquired";
		break;
	case MRP_RES_RESOURCE_LOST:
		state = "lost";
		break;
	case MRP_RES_RESOURCE_AVAILABLE:
		state = "available";
		break;
	case MRP_RES_RESOURCE_PENDING:
		state = "pending";
		break;
	case MRP_RES_RESOURCE_ABOUT_TO_LOOSE:
		state = "about to loose";
		break;
	}
	return state;
}

static void __mmcamcorder_resource_state_callback(mrp_res_context_t *context, mrp_res_error_t err, void *user_data)
{
	int i = 0;
	const mrp_res_resource_set_t *rset;
	mrp_res_resource_t *resource;
	mmf_camcorder_t* camcorder = NULL;

	camcorder = (mmf_camcorder_t*)user_data;

	mmf_return_if_fail((MMHandleType)camcorder);
	mmf_return_if_fail(context);

	_mmcam_dbg_warn("enter - state %d", context->state);

	_MMCAMCORDER_LOCK_RESOURCE(camcorder);

	switch (context->state) {
	case MRP_RES_CONNECTED:
		_mmcam_dbg_warn(" - connected to Murphy");
		if ((rset = mrp_res_list_resources(context)) != NULL) {
			mrp_res_string_array_t *resource_names;
			resource_names = mrp_res_list_resource_names(rset);
			if (!resource_names) {
				_mmcam_dbg_err(" - no resources available");
				_MMCAMCORDER_UNLOCK_RESOURCE(camcorder);
				return;
			}
			for (i = 0; i < resource_names->num_strings; i++) {
				resource = mrp_res_get_resource_by_name(rset, resource_names->strings[i]);
				if (resource)
					_mmcam_dbg_warn(" - available resource: %s", resource->name);
			}
			mrp_res_free_string_array(resource_names);
		}
		camcorder->resource_manager.is_connected = TRUE;
		_MMCAMCORDER_RESOURCE_SIGNAL(camcorder);
		break;
	case MRP_RES_DISCONNECTED:
		_mmcam_dbg_err(" - disconnected from Murphy : stop camera");

		if (camcorder->resource_manager.rset) {
			mrp_res_delete_resource_set(camcorder->resource_manager.rset);
			camcorder->resource_manager.rset = NULL;
		}

		if (camcorder->resource_manager.context) {
			mrp_res_destroy(camcorder->resource_manager.context);
			camcorder->resource_manager.context = NULL;
			camcorder->resource_manager.is_connected = FALSE;
		}

		_MMCAMCORDER_UNLOCK_RESOURCE(camcorder);

		_MMCAMCORDER_LOCK_ASM(camcorder);

		/* set value to inform a status is changed by resource manaer */
		camcorder->state_change_by_system = _MMCAMCORDER_STATE_CHANGE_BY_RM;

		/* Stop the camera */
		__mmcamcorder_force_stop(camcorder);

		/* restore value */
		camcorder->state_change_by_system = _MMCAMCORDER_STATE_CHANGE_NORMAL;

		_MMCAMCORDER_UNLOCK_ASM(camcorder);

		_MMCAMCORDER_LOCK_RESOURCE(camcorder);
		break;
	}

	_MMCAMCORDER_UNLOCK_RESOURCE(camcorder);

	_mmcam_dbg_warn("leave");

	return;
}


static void __mmcamcorder_resource_set_state_callback(mrp_res_context_t *cx, const mrp_res_resource_set_t *rs, void *user_data)
{
	int i = 0;
	mmf_camcorder_t *camcorder = (mmf_camcorder_t *)user_data;
	mrp_res_resource_t *res;

	mmf_return_if_fail((MMHandleType)camcorder);

	_MMCAMCORDER_LOCK_RESOURCE(camcorder);

	if (!mrp_res_equal_resource_set(rs, camcorder->resource_manager.rset)) {
		_mmcam_dbg_warn("- resource set(%p) is not same as this camcorder handle's(%p)", rs, camcorder->resource_manager.rset);
		_MMCAMCORDER_UNLOCK_RESOURCE(camcorder);
		return;
	}

	_mmcam_dbg_log(" - resource set state of camcorder(%p) is changed to [%s]",
		camcorder, __mmcamcorder_resource_state_to_str(rs->state));

	for (i = 0; i < MM_CAMCORDER_RESOURCE_MAX; i++) {
		res = mrp_res_get_resource_by_name(rs, mm_camcorder_resource_str[i]);
		if (res == NULL) {
			_mmcam_dbg_warn(" -- %s not present in resource set", mm_camcorder_resource_str[i]);
		} else {
			_mmcam_dbg_warn(" -- resource name [%s] -> [%s]",
				res->name, __mmcamcorder_resource_state_to_str(res->state));

			if (res->state == MRP_RES_RESOURCE_ACQUIRED) {
				camcorder->resource_manager.acquire_remain--;

				if (camcorder->resource_manager.acquire_remain <= 0) {
					_mmcam_dbg_warn("send signal - resource acquire done");
					_MMCAMCORDER_RESOURCE_SIGNAL(camcorder);
				} else {
					_mmcam_dbg_warn("remained acquire count %d",
						camcorder->resource_manager.acquire_remain);
				}
			} else if (res->state == MRP_RES_RESOURCE_LOST) {
				camcorder->resource_manager.acquire_remain++;

				if (camcorder->resource_manager.acquire_remain >= camcorder->resource_manager.acquire_count) {
					_mmcam_dbg_warn("resource release done");

					if (camcorder->state > MM_CAMCORDER_STATE_NULL) {
						_mmcam_dbg_warn("send resource signal");
						_MMCAMCORDER_RESOURCE_SIGNAL(camcorder);
					} else {
						_mmcam_dbg_warn("skip resource signal - state %d", camcorder->state);
					}
				} else {
					_mmcam_dbg_warn("acquired %d, lost %d",
						camcorder->resource_manager.acquire_count,
						camcorder->resource_manager.acquire_remain);
				}
			}
		}
	}

	mrp_res_delete_resource_set(camcorder->resource_manager.rset);
	camcorder->resource_manager.rset = mrp_res_copy_resource_set(rs);

	_MMCAMCORDER_UNLOCK_RESOURCE(camcorder);

	return;
}


static void __mmcamcorder_resource_release_cb(mrp_res_context_t *cx, const mrp_res_resource_set_t *rs, void *user_data)
{
	int i = 0;
	int current_state = MM_CAMCORDER_STATE_NONE;
	mmf_camcorder_t* camcorder = (mmf_camcorder_t*)user_data;
	mrp_res_resource_t *res;

	mmf_return_if_fail((MMHandleType)camcorder);

	current_state = _mmcamcorder_get_state((MMHandleType)camcorder);
	if (current_state <= MM_CAMCORDER_STATE_NONE ||
	    current_state >= MM_CAMCORDER_STATE_NUM) {
		_mmcam_dbg_err("Abnormal state. Or null handle. (%p, %d)", camcorder, current_state);
		return;
	}

	_mmcam_dbg_warn("enter");

	_MMCAMCORDER_LOCK_RESOURCE(camcorder);

	if (!mrp_res_equal_resource_set(rs, camcorder->resource_manager.rset)) {
		_MMCAMCORDER_UNLOCK_RESOURCE(camcorder);
		_mmcam_dbg_warn("- resource set(%p) is not same as this camcorder handle's(%p)", rs, camcorder->resource_manager.rset);
		return;
	}

	/* set flag for resource release callback */
	camcorder->resource_release_cb_calling = TRUE;

	_mmcam_dbg_log(" - resource set state of camcorder(%p) is changed to [%s]",
		camcorder, __mmcamcorder_resource_state_to_str(rs->state));

	for (i = 0; i < MM_CAMCORDER_RESOURCE_MAX; i++) {
		res = mrp_res_get_resource_by_name(rs, mm_camcorder_resource_str[i]);
		if (res == NULL) {
			_mmcam_dbg_warn(" -- %s not present in resource set", mm_camcorder_resource_str[i]);
		} else {
			_mmcam_dbg_warn(" -- resource name [%s] -> [%s]", res->name, __mmcamcorder_resource_state_to_str(res->state));
		}
	}

	_MMCAMCORDER_UNLOCK_RESOURCE(camcorder);

	_MMCAMCORDER_LOCK_ASM(camcorder);

	/* set value to inform a status is changed by resource manager */
	camcorder->state_change_by_system = _MMCAMCORDER_STATE_CHANGE_BY_RM;

	/* Stop the camera */
	__mmcamcorder_force_stop(camcorder);

	/* restore value */
	camcorder->state_change_by_system = _MMCAMCORDER_STATE_CHANGE_NORMAL;

	_MMCAMCORDER_UNLOCK_ASM(camcorder);

	_MMCAMCORDER_LOCK_RESOURCE(camcorder);

	/* restore flag for resource release callback */
	camcorder->resource_release_cb_calling = FALSE;

	_MMCAMCORDER_UNLOCK_RESOURCE(camcorder);

	_mmcam_dbg_warn("leave");

	return;
}

int _mmcamcorder_resource_create_resource_set(MMCamcorderResourceManager *resource_manager)
{
	if (resource_manager->rset) {
		_mmcam_dbg_warn(" - resource set was already created, delete it");
		mrp_res_delete_resource_set(resource_manager->rset);
		resource_manager->rset = NULL;
	}

	resource_manager->rset = mrp_res_create_resource_set(resource_manager->context,
		MRP_APP_CLASS_FOR_CAMCORDER, __mmcamcorder_resource_set_state_callback, (void*)resource_manager->user_data);

	if (resource_manager->rset == NULL) {
		_mmcam_dbg_err(" - could not create resource set");
		return MM_ERROR_RESOURCE_INTERNAL;
	}

	if (!mrp_res_set_autorelease(TRUE, resource_manager->rset))
		_mmcam_dbg_warn(" - could not set autorelease flag!");

	_mmcam_dbg_log("done");

	return MM_ERROR_NONE;
}

static int __mmcamcorder_resource_include_resource(MMCamcorderResourceManager *resource_manager, const char *resource_name)
{
	mrp_res_resource_t *resource = NULL;
	resource = mrp_res_create_resource(resource_manager->rset,
		resource_name,
		MRP_RESOURCE_TYPE_MANDATORY,
		MRP_RESOURCE_TYPE_EXCLUSIVE);
	if (resource == NULL) {
		_mmcam_dbg_err(" - could not include resource[%s]", resource_name);
		return MM_ERROR_RESOURCE_INTERNAL;
	}

	resource_manager->acquire_count++;
	resource_manager->acquire_remain = resource_manager->acquire_count;

	_mmcam_dbg_log(" - count[%d] include resource[%s]",
		resource_manager->acquire_count, resource_name);

	return MM_ERROR_NONE;
}

static int __mmcamcorder_resource_set_release_cb(MMCamcorderResourceManager *resource_manager)
{
	int ret = MM_ERROR_NONE;
	bool mrp_ret = FALSE;

	if (resource_manager->rset) {
		mrp_ret = mrp_res_set_release_callback(resource_manager->rset, __mmcamcorder_resource_release_cb, resource_manager->user_data);
		if (!mrp_ret) {
			_mmcam_dbg_err(" - could not set release callback");
			ret = MM_ERROR_RESOURCE_INTERNAL;
		}
	} else {
		_mmcam_dbg_err(" - resource set is null");
		ret = MM_ERROR_RESOURCE_INVALID_STATE;
	}

	return ret;
}

int _mmcamcorder_resource_manager_init(MMCamcorderResourceManager *resource_manager, void *user_data)
{
	GMainContext *mrp_ctx = NULL;
	GMainLoop *mrp_loop = NULL;

	MMCAMCORDER_CHECK_RESOURCE_MANAGER_INSTANCE(resource_manager);

	_mmcam_dbg_warn("start");

	mrp_ctx = g_main_context_new();
	if (!mrp_ctx) {
		_mmcam_dbg_err("failed to get create glib context for mrp");
		return MM_ERROR_RESOURCE_INTERNAL;
	}

	mrp_loop = g_main_loop_new(mrp_ctx, TRUE);

	g_main_context_unref(mrp_ctx);
	mrp_ctx = NULL;

	if (!mrp_loop) {
		_mmcam_dbg_err("failed to get create glib loop for mrp");
		return MM_ERROR_RESOURCE_INTERNAL;
	}

	resource_manager->mloop = mrp_mainloop_glib_get(mrp_loop);

	g_main_loop_unref(mrp_loop);
	mrp_loop = NULL;

	if (!resource_manager->mloop) {
		_mmcam_dbg_err("failed to get mainloop for mrp");
		return MM_ERROR_RESOURCE_INTERNAL;
	}

	_mmcam_dbg_warn("mloop %p", resource_manager->mloop);

	resource_manager->context = mrp_res_create(resource_manager->mloop, __mmcamcorder_resource_state_callback, user_data);
	if (!resource_manager->context) {
		_mmcam_dbg_err("could not get context for mrp");

		mrp_mainloop_destroy(resource_manager->mloop);
		resource_manager->mloop = NULL;

		return MM_ERROR_RESOURCE_INTERNAL;
	}

	resource_manager->user_data = user_data;

	_mmcam_dbg_log("done");

	return MM_ERROR_NONE;
}


int _mmcamcorder_resource_wait_for_connection(MMCamcorderResourceManager *resource_manager, void *hcamcorder)
{
	int ret = MM_ERROR_NONE;

	mmf_return_val_if_fail(resource_manager && hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	_MMCAMCORDER_LOCK_RESOURCE(hcamcorder);

	if (resource_manager->is_connected == FALSE) {
		gint64 end_time = 0;

		/* wait for resource manager connected */
		_mmcam_dbg_warn("not connected. wait for signal...");

		end_time = g_get_monotonic_time() + (__MMCAMCORDER_RESOURCE_WAIT_TIME * G_TIME_SPAN_SECOND);

		if (_MMCAMCORDER_RESOURCE_WAIT_UNTIL(hcamcorder, end_time)) {
			_mmcam_dbg_warn("signal received");
			ret = MM_ERROR_NONE;
		} else {
			_mmcam_dbg_err("connection timeout");
			ret = MM_ERROR_RESOURCE_INTERNAL;
		}
	} else {
		_mmcam_dbg_warn("already connected");
	}

	_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);

	return ret;
}


int _mmcamcorder_resource_manager_prepare(MMCamcorderResourceManager *resource_manager, MMCamcorderResourceType resource_type)
{
	MMCAMCORDER_CHECK_RESOURCE_MANAGER_INSTANCE(resource_manager);
	MMCAMCORDER_CHECK_CONNECTION_RESOURCE_MANAGER(resource_manager);

	return __mmcamcorder_resource_include_resource(resource_manager, mm_camcorder_resource_str[resource_type]);
}

int _mmcamcorder_resource_manager_acquire(MMCamcorderResourceManager *resource_manager)
{
	int ret = MM_ERROR_NONE;
	MMCAMCORDER_CHECK_RESOURCE_MANAGER_INSTANCE(resource_manager);
	MMCAMCORDER_CHECK_CONNECTION_RESOURCE_MANAGER(resource_manager);

	if (resource_manager->rset == NULL) {
		_mmcam_dbg_err("- could not acquire resource, resource set is null");
		ret = MM_ERROR_RESOURCE_INVALID_STATE;
	} else {
		ret = __mmcamcorder_resource_set_release_cb(resource_manager);
		if (ret) {
			_mmcam_dbg_err("- could not set resource release cb, ret(%d)", ret);
			ret = MM_ERROR_RESOURCE_INTERNAL;
		} else {
			ret = mrp_res_acquire_resource_set(resource_manager->rset);
			if (ret) {
				_mmcam_dbg_err("- could not acquire resource, ret(%d)", ret);
				ret = MM_ERROR_RESOURCE_INTERNAL;
			}
		}
	}

	return ret;
}

int _mmcamcorder_resource_manager_release(MMCamcorderResourceManager *resource_manager)
{
	int ret = MM_ERROR_NONE;
	MMCAMCORDER_CHECK_RESOURCE_MANAGER_INSTANCE(resource_manager);
	MMCAMCORDER_CHECK_CONNECTION_RESOURCE_MANAGER(resource_manager);

	if (resource_manager->rset == NULL) {
		_mmcam_dbg_err("- could not release resource, resource set is null");
		ret = MM_ERROR_RESOURCE_INVALID_STATE;
	} else {
		if (resource_manager->rset->state != MRP_RES_RESOURCE_ACQUIRED) {
			_mmcam_dbg_err("- could not release resource, resource set state is [%s]",
				__mmcamcorder_resource_state_to_str(resource_manager->rset->state));
			ret = MM_ERROR_RESOURCE_INVALID_STATE;
		} else {
			ret = mrp_res_release_resource_set(resource_manager->rset);
			if (ret) {
				_mmcam_dbg_err("- could not release resource, ret(%d)", ret);
				ret = MM_ERROR_RESOURCE_INTERNAL;
			} else {
				_mmcam_dbg_log("resource release done");
			}
		}
	}

	return ret;
}


int _mmcamcorder_resource_manager_deinit(MMCamcorderResourceManager *resource_manager)
{
	MMCAMCORDER_CHECK_RESOURCE_MANAGER_INSTANCE(resource_manager);

	_mmcam_dbg_warn("rset %p, context %p, mloop %p",
		resource_manager->rset, resource_manager->context, resource_manager->mloop);

	if (resource_manager->rset) {
		if (resource_manager->rset->state == MRP_RES_RESOURCE_ACQUIRED) {
			_mmcam_dbg_warn("resource is still acquired. release...");
			if (mrp_res_release_resource_set(resource_manager->rset))
				_mmcam_dbg_err("- could not release resource");
		}

		_mmcam_dbg_warn("delete resource set");

		mrp_res_delete_resource_set(resource_manager->rset);
		resource_manager->rset = NULL;
	}

	if (resource_manager->context) {
		_mmcam_dbg_warn("destroy resource context");

		mrp_res_destroy(resource_manager->context);
		resource_manager->context = NULL;
	}

	if (resource_manager->mloop) {
		_mmcam_dbg_warn("destroy resource mainloop");

		mrp_mainloop_quit(resource_manager->mloop, 0);
		mrp_mainloop_destroy(resource_manager->mloop);
		resource_manager->mloop = NULL;
	}

	_mmcam_dbg_warn("done");

	return MM_ERROR_NONE;
}
