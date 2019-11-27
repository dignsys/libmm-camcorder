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
#ifndef __MM_CAMCORDER_RM_H__
#define __MM_CAMCORDER_RM_H__

#include <mm_types.h>

/*=======================================================================================
| GLOBAL FUNCTION PROTOTYPES								|
========================================================================================*/

int _mmcamcorder_rm_create(MMHandleType handle);

int _mmcamcorder_rm_allocate(MMHandleType handle);


int _mmcamcorder_rm_deallocate(MMHandleType handle);

int _mmcamcorder_rm_release(MMHandleType handle);

#endif /*__MM_CAMCORDER_RM_H__*/

