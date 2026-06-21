#pragma once

#include "PcmRingBuf.h"

#include "miniaudio.h"
#include "uthash.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct OpusDecoder OpusDecoder;

	typedef struct
	{
		float volume;
		NecroVC_PcmRingBuf buffer;
	} NecroVC_AudioPlaybackChannel;

	typedef struct
	{
		int id;
		NecroVC_AudioPlaybackChannel value;
		UT_hash_handle hh;
	} NecroVC_AudioPlaybackChannels;

	typedef struct
	{
		OpusDecoder* decoder;
		ma_device device;
		NecroVC_AudioPlaybackChannels* channels;
		float volume;
	} NecroVC_AudioPlayback;

	bool NecroVC_AudioPlayback_IsOpened();

	bool NecroVC_AudioPlayback_Open();

	void NecroVC_AudioPlayback_Close();

	float NecroVC_AudioPlayback_GetVolume();

	bool NecroVC_AudioPlayback_TryGetVolume(float* volume_);

	void NecroVC_AudioPlayback_SetVolume(float volume);

	bool NecroVC_AudioPlayback_TrySetVolume(float volume);

	bool NecroVC_AudioPlayback_HasChannel(int id);

	NecroVC_AudioPlaybackChannel* NecroVC_AudioPlayback_GetChannel(int id);

	NecroVC_AudioPlaybackChannel* NecroVC_AudioPlayback_TryGetChannel(int id);

	void NecroVC_AudioPlayback_AddChannel(int id);

	bool NecroVC_AudioPlayback_TryAddChannel(int id);

	void NecroVC_AudioPlayback_RemoveChannel(int id);

	bool NecroVC_AudioPlayback_TryRemoveChannel(int id);

	float NecroVC_AudioPlayback_GetChannelVolume(int id);

	bool NecroVC_AudioPlayback_TryGetChannelVolume(int id, float* volume_);

	void NecroVC_AudioPlayback_SetChannelVolume(int id, float volume);

	bool NecroVC_AudioPlayback_TrySetChannelVolume(int id, float volume);

	bool NecroVC_AudioPlayback_Push(int channelID, const unsigned char* data, opus_int32 length);

	NECROVC_API int NecroVC_AudioPlayback_LIsOpened(lua_State* L);

	NECROVC_API int NecroVC_AudioPlayback_LOpen(lua_State* L);

	NECROVC_API int NecroVC_AudioPlayback_LClose(lua_State* L);

	NECROVC_API int NecroVC_AudioPlayback_LGetVolume(lua_State* L);

	NECROVC_API int NecroVC_AudioPlayback_LSetVolume(lua_State* L);

	NECROVC_API int NecroVC_AudioPlayback_LHasChannel(lua_State* L);

	NECROVC_API int NecroVC_AudioPlayback_LAddChannel(lua_State* L);

	NECROVC_API int NecroVC_AudioPlayback_LRemoveChannel(lua_State* L);

	NECROVC_API int NecroVC_AudioPlayback_LGetChannelVolume(lua_State* L);

	NECROVC_API int NecroVC_AudioPlayback_LSetChannelVolume(lua_State* L);

	NECROVC_API int NecroVC_AudioPlayback_LPush(lua_State* L);

#ifdef __cplusplus
}
#endif
