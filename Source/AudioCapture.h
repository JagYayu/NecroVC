#pragma once

#include "EncRingBuf.h"
#include "NecroVC.h"

#include "miniaudio.h"
#include "opus.h"

#ifdef __cplusplus
extern "C"
{
#endif

	enum
	{
		NecroVC_AudioCapture_MaxCount = 4, // max local-coop player
	};

	typedef struct
	{
		OpusEncoder* encoder;
		NecroVC_EncRingBuf buffer;
		ma_device device;
		float volume;
	} NecroVC_AudioCapture;

	bool NecroVC_AudioCapture_IsOpened(int index);

	bool NecroVC_AudioCapture_Open(int index);

	void NecroVC_AudioCapture_Close(int index);

	float NecroVC_AudioCapture_GetVolume(int index);

	bool NecroVC_AudioCapture_TryGetVolume(int index, float* volume_);

	void NecroVC_AudioCapture_SetVolume(int index, float volume);

	bool NecroVC_AudioCapture_TrySetVolume(int index, float volume);

	NecroVC_EncRingBufFrame NecroVC_AudioCapture_Pull(size_t index);

	NECROVC_API int NecroVC_AudioCapture_LGetMaxCount(lua_State* L);

	NECROVC_API int NecroVC_AudioCapture_LIsOpened(lua_State* L);

	NECROVC_API int NecroVC_AudioCapture_LOpen(lua_State* L);

	NECROVC_API int NecroVC_AudioCapture_LClose(lua_State* L);

	NECROVC_API int NecroVC_AudioCapture_LGetVolume(lua_State* L);

	NECROVC_API int NecroVC_AudioCapture_LSetVolume(lua_State* L);

	NECROVC_API int NecroVC_AudioCapture_LPull(lua_State* L);

	NECROVC_API int NecroVC_AudioCapture_LPullAll(lua_State* L);

#ifdef __cplusplus
}
#endif
