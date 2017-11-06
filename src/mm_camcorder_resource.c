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
	"video_overlay",
	"video_encoder"
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

#define RESOURCE_LOG_INFO(fmt, args...) \
do { \
	_mmcam_dbg_log("[%p][ID:%d] "fmt, resource_manager, resource_manager->id, ##args); \
} while (0);

#define RESOURCE_LOG_WARN(fmt, args...) \
do { \
	_mmcam_dbg_warn("[%p][ID:%d] "fmt, resource_manager, resource_manager->id, ##args); \
} while (0);

#define RESOURCE_LOG_ERR(fmt, args...) \
do { \
	_mmcam_dbg_err("[%p][ID:%d] "fmt, resource_manager, resource_manager->id, ##args); \
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
	mmf_camcorder_t *hcamcorder = NULL;
	MMCamcorderResourceManager *resource_manager = (MMCamcorderResourceManager *)user_data;

	mmf_return_if_fail(context);
	mmf_return_if_fail(resource_manager);

	hcamcorder = (mmf_camcorder_t *)resource_manager->hcamcorder;

	mmf_return_if_fail(hcamcorder);

	RESOURCE_LOG_WARN("enter - state %d", context->state);

	_MMCAMCORDER_LOCK_RESOURCE(hcamcorder);

	switch (context->state) {
	case MRP_RES_CONNECTED:
		RESOURCE_LOG_WARN(" - connected to Murphy");
		if ((rset = mrp_res_list_resources(context)) != NULL) {
			mrp_res_string_array_t *resource_names;
			resource_names = mrp_res_list_resource_names(rset);
			if (!resource_names) {
				RESOURCE_LOG_ERR(" - no resources available");
				_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);
				return;
			}
			for (i = 0; i < resource_names->num_strings; i++) {
				resource = mrp_res_get_resource_by_name(rset, resource_names->strings[i]);
				if (resource)
					RESOURCE_LOG_WARN(" - available resource: %s", resource->name);
			}
			mrp_res_free_string_array(resource_names);
		}
		resource_manager->is_connected = TRUE;
		_MMCAMCORDER_RESOURCE_SIGNAL(hcamcorder);
		break;
	case MRP_RES_DISCONNECTED:
		RESOURCE_LOG_ERR(" - disconnected from Murphy : stop camcorder");

		if (resource_manager->rset) {
			mrp_res_delete_resource_set(resource_manager->rset);
			resource_manager->rset = NULL;
		}

		if (resource_manager->context) {
			mrp_res_destroy(resource_manager->context);
			resource_manager->context = NULL;
			resource_manager->is_connected = FALSE;
		}

		_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);

		_MMCAMCORDER_LOCK_ASM(hcamcorder);

		/* Stop the camera */
		__mmcamcorder_force_stop(hcamcorder, _MMCAMCORDER_STATE_CHANGE_BY_RM);

		_MMCAMCORDER_UNLOCK_ASM(hcamcorder);

		_MMCAMCORDER_LOCK_RESOURCE(hcamcorder);
		break;
	}

	_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);

	RESOURCE_LOG_WARN("leave");

	return;
}


static void __mmcamcorder_resource_set_state_callback(mrp_res_context_t *cx, const mrp_res_resource_set_t *rs, void *user_data)
{
	int i = 0;
	mmf_camcorder_t *hcamcorder = NULL;
	MMCamcorderResourceManager *resource_manager = (MMCamcorderResourceManager *)user_data;
	mrp_res_resource_t *res = NULL;

	mmf_return_if_fail(resource_manager && resource_manager->hcamcorder);

	hcamcorder = (mmf_camcorder_t *)resource_manager->hcamcorder;

	RESOURCE_LOG_WARN("start");

	_MMCAMCORDER_LOCK_RESOURCE(hcamcorder);

	if (!mrp_res_equal_resource_set(rs, resource_manager->rset)) {
		RESOURCE_LOG_WARN("- resource set(%p) is not same as this handle's(%p)", rs, resource_manager->rset);
		_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);
		return;
	}

	RESOURCE_LOG_INFO(" - resource set state is changed to [%s]", __mmcamcorder_resource_state_to_str(rs->state));

	for (i = 0; i < MM_CAMCORDER_RESOURCE_MAX; i++) {
		res = mrp_res_get_resource_by_name(rs, mm_camcorder_resource_str[i]);
		if (res == NULL) {
			RESOURCE_LOG_WARN(" -- %s not present in resource set", mm_camcorder_resource_str[i]);
		} else {
			RESOURCE_LOG_WARN(" -- resource name [%s] -> [%s]",
				res->name, __mmcamcorder_resource_state_to_str(res->state));

			if (res->state == MRP_RES_RESOURCE_ACQUIRED) {
				resource_manager->acquire_remain--;

				if (resource_manager->acquire_remain <= 0) {
					RESOURCE_LOG_WARN("send signal - resource acquire done");
					_MMCAMCORDER_RESOURCE_SIGNAL(hcamcorder);
				} else {
					RESOURCE_LOG_WARN("remained acquire count %d",
						resource_manager->acquire_remain);
				}
			} else if (res->state == MRP_RES_RESOURCE_LOST) {
				resource_manager->acquire_remain++;

				if (resource_manager->acquire_remain >= resource_manager->acquire_count) {
					RESOURCE_LOG_WARN("resource release done");

					if (hcamcorder->state > MM_CAMCORDER_STATE_NULL) {
						RESOURCE_LOG_WARN("send resource signal");
						_MMCAMCORDER_RESOURCE_SIGNAL(hcamcorder);
					} else {
						RESOURCE_LOG_WARN("skip resource signal - state %d", hcamcorder->state);
					}
				} else {
					RESOURCE_LOG_WARN("acquired %d, lost %d",
						resource_manager->acquire_count, resource_manager->acquire_remain);
				}
			}
		}
	}

	mrp_res_delete_resource_set(resource_manager->rset);
	resource_manager->rset = mrp_res_copy_resource_set(rs);

	_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);

	RESOURCE_LOG_WARN("done");

	return;
}


static void __mmcamcorder_resource_release_cb(mrp_res_context_t *cx, const mrp_res_resource_set_t *rs, void *user_data)
{
	int i = 0;
	int current_state = MM_CAMCORDER_STATE_NONE;
	mmf_camcorder_t *hcamcorder = NULL;
	MMCamcorderResourceManager *resource_manager = (MMCamcorderResourceManager *)user_data;
	mrp_res_resource_t *res = NULL;

	mmf_return_if_fail(resource_manager && resource_manager->hcamcorder);

	hcamcorder = (mmf_camcorder_t *)resource_manager->hcamcorder;

	current_state = _mmcamcorder_get_state((MMHandleType)hcamcorder);
	if (current_state <= MM_CAMCORDER_STATE_NONE ||
	    current_state >= MM_CAMCORDER_STATE_NUM) {
		RESOURCE_LOG_ERR("Abnormal state %d", current_state);
		return;
	}

	RESOURCE_LOG_WARN("enter");

	_MMCAMCORDER_LOCK_RESOURCE(hcamcorder);

	if (!mrp_res_equal_resource_set(rs, resource_manager->rset)) {
		_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);
		RESOURCE_LOG_WARN("- resource set(%p) is not same as this handle's(%p)", rs, resource_manager->rset);
		return;
	}

	/* set flag for resource release callback */
	resource_manager->is_release_cb_calling = TRUE;

	RESOURCE_LOG_INFO(" - resource set state is changed to [%s]", __mmcamcorder_resource_state_to_str(rs->state));

	for (i = 0; i < MM_CAMCORDER_RESOURCE_MAX; i++) {
		res = mrp_res_get_resource_by_name(rs, mm_camcorder_resource_str[i]);
		if (res) {
			RESOURCE_LOG_WARN(" -- resource name [%s] -> [%s]",
				res->name, __mmcamcorder_resource_state_to_str(res->state));
		} else {
			RESOURCE_LOG_WARN(" -- %s not present in resource set", mm_camcorder_resource_str[i]);
		}
	}

	_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);

	_MMCAMCORDER_LOCK_ASM(hcamcorder);

	if (resource_manager->id == MM_CAMCORDER_RESOURCE_ID_MAIN) {
		/* Stop camera */
		__mmcamcorder_force_stop(hcamcorder, _MMCAMCORDER_STATE_CHANGE_BY_RM);
	} else {
		/* Stop video recording */
		if (_mmcamcorder_commit((MMHandleType)hcamcorder) != MM_ERROR_NONE) {
			RESOURCE_LOG_ERR("commit failed, cancel it");
			_mmcamcorder_cancel((MMHandleType)hcamcorder);
		}
	}

	_MMCAMCORDER_UNLOCK_ASM(hcamcorder);

	_MMCAMCORDER_LOCK_RESOURCE(hcamcorder);

	/* restore flag for resource release callback */
	resource_manager->is_release_cb_calling = FALSE;

	_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);

	RESOURCE_LOG_WARN("leave");

	return;
}

int _mmcamcorder_resource_create_resource_set(MMCamcorderResourceManager *resource_manager)
{
	if (resource_manager->rset) {
		RESOURCE_LOG_WARN(" - resource set was already created, delete it");
		mrp_res_delete_resource_set(resource_manager->rset);
		resource_manager->rset = NULL;
	}

	resource_manager->rset = mrp_res_create_resource_set(resource_manager->context,
		MRP_APP_CLASS_FOR_CAMCORDER, __mmcamcorder_resource_set_state_callback, (void *)resource_manager);

	if (resource_manager->rset == NULL) {
		RESOURCE_LOG_ERR(" - could not create resource set");
		return MM_ERROR_RESOURCE_INTERNAL;
	}

	if (!mrp_res_set_autorelease(TRUE, resource_manager->rset))
		RESOURCE_LOG_WARN(" - could not set autorelease flag!");

	RESOURCE_LOG_INFO("done");

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
		RESOURCE_LOG_ERR(" - could not include resource[%s]", resource_name);
		return MM_ERROR_RESOURCE_INTERNAL;
	}

	resource_manager->acquire_count++;
	resource_manager->acquire_remain = resource_manager->acquire_count;

	RESOURCE_LOG_INFO(" - count[%d] include resource[%s]",
		resource_manager->acquire_count, resource_name);

	return MM_ERROR_NONE;
}

static int __mmcamcorder_resource_set_release_cb(MMCamcorderResourceManager *resource_manager)
{
	int ret = MM_ERROR_NONE;
	bool mrp_ret = FALSE;

	if (resource_manager->rset) {
		mrp_ret = mrp_res_set_release_callback(resource_manager->rset, __mmcamcorder_resource_release_cb, (void *)resource_manager);
		if (!mrp_ret) {
			RESOURCE_LOG_ERR(" - could not set release callback");
			ret = MM_ERROR_RESOURCE_INTERNAL;
		}
	} else {
		RESOURCE_LOG_ERR(" - resource set is null");
		ret = MM_ERROR_RESOURCE_INVALID_STATE;
	}

	return ret;
}

int _mmcamcorder_resource_manager_init(MMCamcorderResourceManager *resource_manager)
{
	GMainContext *mrp_ctx = NULL;
	GMainLoop *mrp_loop = NULL;

	MMCAMCORDER_CHECK_RESOURCE_MANAGER_INSTANCE(resource_manager);

	RESOURCE_LOG_WARN("start");

	mrp_ctx = g_main_context_new();
	if (!mrp_ctx) {
		RESOURCE_LOG_ERR("failed to get create glib context for mrp");
		return MM_ERROR_RESOURCE_INTERNAL;
	}

	mrp_loop = g_main_loop_new(mrp_ctx, TRUE);

	g_main_context_unref(mrp_ctx);
	mrp_ctx = NULL;

	if (!mrp_loop) {
		RESOURCE_LOG_ERR("failed to get create glib loop for mrp");
		return MM_ERROR_RESOURCE_INTERNAL;
	}

	resource_manager->mloop = mrp_mainloop_glib_get(mrp_loop);

	g_main_loop_unref(mrp_loop);
	mrp_loop = NULL;

	if (!resource_manager->mloop) {
		RESOURCE_LOG_ERR("failed to get mainloop for mrp");
		return MM_ERROR_RESOURCE_INTERNAL;
	}

	RESOURCE_LOG_WARN("mloop %p", resource_manager->mloop);

	resource_manager->context = mrp_res_create(resource_manager->mloop, __mmcamcorder_resource_state_callback, (void *)resource_manager);
	if (!resource_manager->context) {
		RESOURCE_LOG_ERR("could not get context for mrp");

		mrp_mainloop_destroy(resource_manager->mloop);
		resource_manager->mloop = NULL;

		return MM_ERROR_RESOURCE_INTERNAL;
	}

	RESOURCE_LOG_INFO("done");

	return MM_ERROR_NONE;
}


int _mmcamcorder_resource_wait_for_connection(MMCamcorderResourceManager *resource_manager)
{
	int ret = MM_ERROR_NONE;
	void *hcamcorder = NULL;

	mmf_return_val_if_fail(resource_manager && resource_manager->hcamcorder, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	hcamcorder = resource_manager->hcamcorder;

	_MMCAMCORDER_LOCK_RESOURCE(hcamcorder);

	if (resource_manager->is_connected == FALSE) {
		gint64 end_time = 0;

		/* wait for resource manager connected */
		RESOURCE_LOG_WARN("not connected. wait for signal...");

		end_time = g_get_monotonic_time() + (__MMCAMCORDER_RESOURCE_WAIT_TIME * G_TIME_SPAN_SECOND);

		if (_MMCAMCORDER_RESOURCE_WAIT_UNTIL(hcamcorder, end_time)) {
			RESOURCE_LOG_WARN("signal received");
			ret = MM_ERROR_NONE;
		} else {
			RESOURCE_LOG_ERR("connection timeout");
			ret = MM_ERROR_RESOURCE_INTERNAL;
		}
	} else {
		RESOURCE_LOG_WARN("already connected");
	}

	_MMCAMCORDER_UNLOCK_RESOURCE(hcamcorder);

	return ret;
}


int _mmcamcorder_resource_check_connection(MMCamcorderResourceManager *resource_manager)
{
	int ret = MM_ERROR_NONE;

	mmf_return_val_if_fail(resource_manager, MM_ERROR_CAMCORDER_NOT_INITIALIZED);

	if (resource_manager->is_connected == FALSE) {
		RESOURCE_LOG_WARN("resource manager disconnected before, try to reconnect");

		/* release remained resource */
		_mmcamcorder_resource_manager_deinit(resource_manager);

		/* init resource manager and wait for connection */
		ret = _mmcamcorder_resource_manager_init(resource_manager);
		if (ret != MM_ERROR_NONE) {
			RESOURCE_LOG_ERR("failed to initialize resource manager");
			return ret;
		}

		ret = _mmcamcorder_resource_wait_for_connection(resource_manager);
		if (ret != MM_ERROR_NONE) {
			RESOURCE_LOG_ERR("failed to connect resource manager");
			return ret;
		}
	}

	RESOURCE_LOG_WARN("done");

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
		RESOURCE_LOG_ERR("- could not acquire resource, resource set is null");
		ret = MM_ERROR_RESOURCE_INVALID_STATE;
	} else {
		ret = __mmcamcorder_resource_set_release_cb(resource_manager);
		if (ret) {
			RESOURCE_LOG_ERR("- could not set resource release cb, ret(%d)", ret);
			ret = MM_ERROR_RESOURCE_INTERNAL;
		} else {
			ret = mrp_res_acquire_resource_set(resource_manager->rset);
			if (ret) {
				RESOURCE_LOG_ERR("- could not acquire resource, ret(%d)", ret);
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
		RESOURCE_LOG_ERR("- could not release resource, resource set is null");
		ret = MM_ERROR_RESOURCE_INVALID_STATE;
	} else {
		if (resource_manager->rset->state != MRP_RES_RESOURCE_ACQUIRED) {
			RESOURCE_LOG_ERR("- could not release resource, resource set state is [%s]",
				__mmcamcorder_resource_state_to_str(resource_manager->rset->state));
			ret = MM_ERROR_RESOURCE_INVALID_STATE;
		} else {
			ret = mrp_res_release_resource_set(resource_manager->rset);
			if (ret) {
				RESOURCE_LOG_ERR("- could not release resource, ret(%d)", ret);
				ret = MM_ERROR_RESOURCE_INTERNAL;
			} else {
				RESOURCE_LOG_INFO("resource release done");
			}
		}
	}

	return ret;
}


int _mmcamcorder_resource_manager_deinit(MMCamcorderResourceManager *resource_manager)
{
	MMCAMCORDER_CHECK_RESOURCE_MANAGER_INSTANCE(resource_manager);

	RESOURCE_LOG_WARN("rset %p, context %p, mloop %p",
		resource_manager->rset, resource_manager->context, resource_manager->mloop);

	if (resource_manager->rset) {
		if (resource_manager->rset->state == MRP_RES_RESOURCE_ACQUIRED) {
			RESOURCE_LOG_WARN("resource is still acquired. release...");
			if (mrp_res_release_resource_set(resource_manager->rset))
				RESOURCE_LOG_ERR("- could not release resource");
		}

		RESOURCE_LOG_WARN("delete resource set");

		mrp_res_delete_resource_set(resource_manager->rset);
		resource_manager->rset = NULL;
	}

	if (resource_manager->context) {
		RESOURCE_LOG_WARN("destroy resource context");

		mrp_res_destroy(resource_manager->context);
		resource_manager->context = NULL;
	}

	if (resource_manager->mloop) {
		RESOURCE_LOG_WARN("destroy resource mainloop");

		mrp_mainloop_quit(resource_manager->mloop, 0);
		mrp_mainloop_destroy(resource_manager->mloop);
		resource_manager->mloop = NULL;
	}

	RESOURCE_LOG_WARN("done");

	return MM_ERROR_NONE;
}
