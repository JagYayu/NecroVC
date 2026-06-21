#pragma once

#include "NecroVC.h"
#include "miniaudio.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

	bool NecroVC_Context_IsInited();

	void NecroVC_Context_Init();

	void NecroVC_Context_Deinit();

	ma_context* NecroVC_Context_Get();

	const ma_device_info* NecroVC_Context_GetCaptureDevices(ma_uint32* count_);

	const ma_device_info* NecroVC_Context_GetPlaybackDevices(ma_uint32* count_);

	NECROVC_API int NecroVC_Context_LIsInited(lua_State* L);

	NECROVC_API int NecroVC_Context_LInit(lua_State* L);

	NECROVC_API int NecroVC_Context_LDeinit(lua_State* L);

	NECROVC_API int NecroVC_Context_LListCaptureDevices(lua_State* L);

	NECROVC_API int NecroVC_Context_LListPlaybackDevices(lua_State* L);

#ifdef __cplusplus
}
#endif
