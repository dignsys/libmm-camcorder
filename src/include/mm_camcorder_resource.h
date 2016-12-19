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

typedef enum {
	MM_CAMCORDER_RESOURCE_TYPE_CAMERA,
	MM_CAMCORDER_RESOURCE_TYPE_VIDEO_OVERLAY,
	MM_CAMCORDER_RESOURCE_MAX
} MMCamcorderResourceType;

typedef struct {
	mrp_mainloop_t *mloop;
	mrp_res_context_t *context;
	mrp_res_resource_set_t *rset;
	bool is_connected;
	void *user_data;
	int acquire_count;
	int acquire_remain;
} MMCamcorderResourceManager;

int _mmcamcorder_resource_manager_init(MMCamcorderResourceManager *resource_manager, void *user_data);
int _mmcamcorder_resource_create_resource_set(MMCamcorderResourceManager *resource_manager);
int _mmcamcorder_resource_manager_prepare(MMCamcorderResourceManager *resource_manager, MMCamcorderResourceType resource_type);
int _mmcamcorder_resource_manager_acquire(MMCamcorderResourceManager *resource_manager);
int _mmcamcorder_resource_manager_release(MMCamcorderResourceManager *resource_manager);
int _mmcamcorder_resource_manager_deinit(MMCamcorderResourceManager *resource_manager);

#ifdef __cplusplus
}
#endif

#endif /* __MM_PLAYER_RESOURCE_H__ */
