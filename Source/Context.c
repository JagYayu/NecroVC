#include "Context.h"

#include "lauxlib.h"
#include "lua.h"

#include <limits.h>
#include <stdlib.h>

static ma_context NecroVC_context = {};

static bool NecroVC_contextState = false;

static ma_device_info* NecroVC_captureDevices = NULL;

static ma_uint32 NecroVC_captureDeviceCount = 0;

static ma_device_info* NecroVC_playbackDevices = NULL;

static ma_uint32 NecroVC_playbackDeviceCount = 0;

bool NecroVC_Context_IsInited()
{
	return NecroVC_contextState;
}

void NecroVC_Context_Init()
{
	ma_context_init(NULL, 0, NULL, &NecroVC_context);
	ma_context_get_devices(&NecroVC_context, &NecroVC_playbackDevices, &NecroVC_playbackDeviceCount, &NecroVC_captureDevices, &NecroVC_captureDeviceCount);
	NecroVC_contextState = true;
}

void NecroVC_Context_Deinit()
{
	NecroVC_contextState = false;
	ma_context_uninit(&NecroVC_context);
}

ma_context* NecroVC_Context_Get()
{
	return &NecroVC_context;
}

const ma_device_info* NecroVC_Context_GetCaptureDevices(ma_uint32* count_)
{
	*count_ = NecroVC_captureDeviceCount;
	return NecroVC_captureDevices;
}

const ma_device_info* NecroVC_Context_GetPlaybackDevices(ma_uint32* count_)
{
	*count_ = NecroVC_playbackDeviceCount;
	return NecroVC_playbackDevices;
}

static int NecroVC_Context_LListDevicesImpl(lua_State* L, const ma_device_info* deviceInfos, ma_uint32 count)
{
	if (NECROVC_UNLIKELY(!NecroVC_contextState))
	{
		return luaL_error(L, "Context has not been initialized");
	}

	lua_createtable(L, count, 0);
	for (ma_uint32 i = 0; i < min(count, INT_MAX); i++)
	{
		const ma_device_info* info = &deviceInfos[i];

		lua_newtable(L);

		lua_pushinteger(L, i);
		lua_setfield(L, -2, "index");

		lua_pushboolean(L, info->isDefault);
		lua_setfield(L, -2, "isDefault");

		lua_pushstring(L, info->name);
		lua_setfield(L, -2, "name");

		// TODO
		// lua_push(L, info->id);
		// lua_setfield(L, -2, "id");

		lua_rawseti(L, -2, i + 1);
	}
	return 1;
}

int NecroVC_Context_LIsInited(lua_State* L)
{
	lua_pushboolean(L, NecroVC_contextState);
	return 1;
}

int NecroVC_Context_LInit(lua_State* L)
{
	if (NECROVC_UNLIKELY(NecroVC_contextState))
	{
		return luaL_error(L, "Context has already been initialized");
	}
	NecroVC_Context_Init();
	return 0;
}

int NecroVC_Context_LDeinit(lua_State* L)
{
	if (NECROVC_UNLIKELY(!NecroVC_contextState))
	{
		return luaL_error(L, "Context has already been deinitialized or is uninitialized");
	}
	NecroVC_Context_Deinit();
	return 0;
}

int NecroVC_Context_LListCaptureDevices(lua_State* L)
{
	return NecroVC_Context_LListDevicesImpl(L, NecroVC_captureDevices, NecroVC_captureDeviceCount);
}

int NecroVC_Context_LListPlaybackDevices(lua_State* L)
{
	return NecroVC_Context_LListDevicesImpl(L, NecroVC_playbackDevices, NecroVC_playbackDeviceCount);
}
