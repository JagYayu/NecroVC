#pragma once

#ifdef NECROVC_EXPORTS
#	define NECROVC_API __declspec(dllexport)
#else
#	define NECROVC_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct lua_State lua_State;

	NECROVC_API int NecroVC_IsOpened(lua_State* L);

	NECROVC_API int NecroVC_Open(lua_State* L);

	NECROVC_API int NecroVC_Close(lua_State* L);

	NECROVC_API int NecroVC_Pull(lua_State* L);

	NECROVC_API int NecroVC_Push(lua_State* L);

	// NECROVC_API int NecroVC_PushSpatial(lua_State* L);

#ifdef __cplusplus
}
#endif
