/*
 * libmm-camcorder
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Heechul Jeon <heechul.jeon@samsung.com>
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

#ifndef __MM_CAMCORDER_RESOURCE_H__
#define __MM_CAMCORDER_RESOURCE_H__

#include <murphy/plugins/resource-native/libmurphy-resource/resource-api.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __MMCAMCORDER_RESOURCE_WAIT_TIME        3

typedef enum {
	MM_CAMCORDER_RESOURCE_TYPE_CAMERA,
	MM_CAMCORDER_RESOURCE_TYPE_VIDEO_OVERLAY,
	MM_CAMCORDER_RESOURCE_TYPE_VIDEO_ENCODER,
	MM_CAMCORDER_RESOURCE_MAX
} MMCamcorderResourceType;

typedef enum {
	MM_CAMCORDER_RESOURCE_ID_MAIN,
	MM_CAMCORDER_RESOURCE_ID_SUB
} MMCamcorderResourceID;

typedef struct {
	MMCamcorderResourceID id;
	mrp_mainloop_t *mloop;
	mrp_res_context_t *context;
	mrp_res_resource_set_t *rset;
	gboolean is_connected;
	gboolean is_release_cb_calling;
	int acquire_count;
	int acquire_remain;
	void *hcamcorder;
} MMCamcorderResourceManager;

int _mmcamcorder_resource_manager_init(MMCamcorderResourceManager *resource_manager);
int _mmcamcorder_resource_wait_for_connection(MMCamcorderResourceManager *resource_manager);
int _mmcamcorder_resource_check_connection(MMCamcorderResourceManager *resource_manager);
int _mmcamcorder_resource_create_resource_set(MMCamcorderResourceManager *resource_manager);
int _mmcamcorder_resource_manager_prepare(MMCamcorderResourceManager *resource_manager, MMCamcorderResourceType resource_type);
int _mmcamcorder_resource_manager_acquire(MMCamcorderResourceManager *resource_manager);
int _mmcamcorder_resource_manager_release(MMCamcorderResourceManager *resource_manager);
int _mmcamcorder_resource_manager_deinit(MMCamcorderResourceManager *resource_manager);

#ifdef __cplusplus
}
#endif

#endif /* __MM_PLAYER_RESOURCE_H__ */
