#include "AudioPlayback.h"

#include "Utilities.h"
#include "lauxlib.h"
#include "lua.h"
#include "opus.h"
#include "opus_defines.h"
#include "uthash.h"

#include <stdint.h>
#include <string.h>

static NecroVC_AudioPlayback NecroVC_audioPlayback = {
    NULL,
    {},
    NULL,
    1.0f,
};

static bool NecroVC_audioPlaybackState = false;

static void NecroVC_OnAudioPlaybackDataProcess(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	(void)pInput;

	NecroVC_AudioPlayback* audioPlayback = pDevice->pUserData;
	if (audioPlayback == NULL)
	{
		return;
	}

	opus_int16* output = pOutput;

	for (NecroVC_AudioPlaybackChannels* channel = audioPlayback->channels; channel != NULL; channel = channel->hh.next)
	{
		if (NecroVC_PcmRingBuf_IsEmpty(&channel->value.buffer))
		{
			continue;
		}

		float volume = channel->value.volume;
		opus_int16* pcm = channel->value.buffer.datas[channel->value.buffer.readIndex];
		channel->value.buffer.readIndex = (channel->value.buffer.readIndex + 1) % NecroVC_PlaybackRingSize;

		for (ma_uint32 i = 0; i < frameCount; i++)
		{
			output[i] = (opus_int16)NECROVC_CLAMP(INT16_MIN, (float)output[i] + (float)pcm[i] * volume, INT16_MAX);
		}
	}

	float volume = audioPlayback->volume;
	if (volume != 1.0f)
	{
		for (ma_uint32 i = 0; i < frameCount; i++)
		{
			output[i] = (opus_int16)NECROVC_CLAMP(INT16_MIN, (float)output[i] * volume, INT16_MAX);
		}
	}
}

bool NecroVC_AudioPlayback_IsOpened()
{
	return NecroVC_audioPlaybackState;
}

NecroVC_AudioPlayback* NecroVC_AudioPlayback_Get()
{
	if (NecroVC_audioPlaybackState)
	{
		return &NecroVC_audioPlayback;
	}
	return NULL;
}

bool NecroVC_AudioPlayback_Open()
{
	int opusErr;
	NecroVC_audioPlayback.decoder = opus_decoder_create(NecroVC_SampleRate, 1, &opusErr);
	if (opusErr != OPUS_OK)
	{
		return false; // TODO error code
	}

	NecroVC_audioPlayback.channels = NULL;

	ma_device_config config = ma_device_config_init(ma_device_type_playback);
	config.playback.format = ma_format_s16;
	config.playback.channels = 1;
	config.sampleRate = NecroVC_SampleRate;
	config.periodSizeInFrames = NecroVC_FrameSize;
	config.dataCallback = NecroVC_OnAudioPlaybackDataProcess;

	ma_device* device = &NecroVC_audioPlayback.device;
	if (ma_device_init(NULL, &config, device) != MA_SUCCESS)
	{
		return false; // TODO error code
	}
	if (ma_device_start(device) != MA_SUCCESS)
	{
		ma_device_uninit(device);
		return false; // TODO error code
	}
	device->pUserData = &NecroVC_audioPlayback;

	NecroVC_audioPlaybackState = true;

	return true;
}

void NecroVC_AudioPlayback_Close()
{
	NecroVC_audioPlaybackState = false;
	ma_device* device = &NecroVC_audioPlayback.device;
	device->pUserData = NULL;
	ma_device_stop(device);
	ma_device_uninit(device);
	opus_decoder_destroy(NecroVC_audioPlayback.decoder);
}

float NecroVC_AudioPlayback_GetVolume()
{
	return NecroVC_audioPlayback.volume;
}

bool NecroVC_AudioPlayback_TryGetVolume(float* volume_)
{
	if (NecroVC_audioPlaybackState)
	{
		*volume_ = NecroVC_audioPlayback.volume;
		return true;
	}
	return false;
}

void NecroVC_AudioPlayback_SetVolume(float volume)
{
	NecroVC_audioPlayback.volume = volume;
}

bool NecroVC_AudioPlayback_TrySetVolume(float volume)
{
	if (NecroVC_audioPlaybackState)
	{
		NecroVC_audioPlayback.volume = volume;
		return true;
	}
	return false;
}

bool NecroVC_AudioPlayback_HasChannel(int id)
{
	NecroVC_AudioPlaybackChannels* channel;
	HASH_FIND_INT(NecroVC_audioPlayback.channels, &id, channel);
	return channel != NULL;
}

NecroVC_AudioPlaybackChannel* NecroVC_AudioPlayback_GetChannel(int id)
{
	NecroVC_AudioPlaybackChannels* channel;
	HASH_FIND_INT(NecroVC_audioPlayback.channels, &id, channel);
	return &channel->value;
}

NecroVC_AudioPlaybackChannel* NecroVC_AudioPlayback_TryGetChannel(int id)
{
	NecroVC_AudioPlaybackChannels* channel;
	HASH_FIND_INT(NecroVC_audioPlayback.channels, &id, channel);
	if (channel != NULL)
	{
		return &channel->value;
	}
	return NULL;
}

void NecroVC_AudioPlayback_AddChannel(int id)
{
	NecroVC_AudioPlaybackChannels* channel = malloc(sizeof(NecroVC_AudioPlaybackChannels));
	channel->id = id;
	channel->value.volume = 1.0f;
	memset(&channel->value.buffer, 0, sizeof(channel->value.buffer));
	HASH_ADD_INT(NecroVC_audioPlayback.channels, id, channel);
}

bool NecroVC_AudioPlayback_TryAddChannel(int id)
{
	NecroVC_AudioPlaybackChannels* channel;
	HASH_FIND_INT(NecroVC_audioPlayback.channels, &id, channel);
	if (channel == NULL)
	{
		channel = malloc(sizeof(NecroVC_AudioPlaybackChannels));
		channel->id = id;
		channel->value.volume = 1.0f;
		memset(&channel->value.buffer, 0, sizeof(channel->value.buffer));
		HASH_ADD_INT(NecroVC_audioPlayback.channels, id, channel);
		return true;
	}
	return false;
}

void NecroVC_AudioPlayback_RemoveChannel(int id)
{
	NecroVC_AudioPlaybackChannels* channel;
	HASH_FIND_INT(NecroVC_audioPlayback.channels, &id, channel);
	HASH_DEL(NecroVC_audioPlayback.channels, channel);
	free(channel);
}

bool NecroVC_AudioPlayback_TryRemoveChannel(int id)
{
	NecroVC_AudioPlaybackChannels* channel;
	HASH_FIND_INT(NecroVC_audioPlayback.channels, &id, channel);
	if (channel != NULL)
	{
		HASH_DEL(NecroVC_audioPlayback.channels, channel);
		free(channel);
		return true;
	}
	return false;
}

float NecroVC_AudioPlayback_GetChannelVolume(int id)
{
	NecroVC_AudioPlaybackChannels* channel;
	HASH_FIND_INT(NecroVC_audioPlayback.channels, &id, channel);
	return channel->value.volume;
}

bool NecroVC_AudioPlayback_TryChannelGetVolume(int id, float* volume_)
{
	NecroVC_AudioPlaybackChannels* channel;
	HASH_FIND_INT(NecroVC_audioPlayback.channels, &id, channel);
	if (channel != NULL)
	{
		*volume_ = channel->value.volume;
		return true;
	}
	return false;
}

void NecroVC_AudioPlayback_SetChannelVolume(int id, float volume)
{
	NecroVC_AudioPlaybackChannels* channel;
	HASH_FIND_INT(NecroVC_audioPlayback.channels, &id, channel);
	channel->value.volume = volume;
}

bool NecroVC_AudioPlayback_TryChannelSetVolume(int id, float volume)
{
	NecroVC_AudioPlaybackChannels* channel;
	HASH_FIND_INT(NecroVC_audioPlayback.channels, &id, channel);
	if (channel != NULL)
	{
		channel->value.volume = volume;
		return true;
	}
	return false;
}

bool NecroVC_AudioPlayback_Push(int channelID, const unsigned char* data, opus_int32 length)
{
	opus_int16 pcm[NecroVC_FrameSize];
	int decLen = opus_decode(NecroVC_audioPlayback.decoder, (const unsigned char*)data, (opus_int32)length, pcm, NecroVC_FrameSize, 0);
	if (decLen <= 0)
	{
		return false;
	}

	NecroVC_AudioPlaybackChannels* channel;
	HASH_FIND_INT(NecroVC_audioPlayback.channels, &channelID, channel);
	NecroVC_PcmRingBuf_WriteFrame(&channel->value.buffer, pcm);
	return true;
}

int NecroVC_AudioPlayback_LIsOpened(lua_State* L)
{
	lua_pushboolean(L, NecroVC_audioPlaybackState);
	return 1;
}

int NecroVC_AudioPlayback_LOpen(lua_State* L)
{
	if (NECROVC_UNLIKELY(NecroVC_AudioPlayback_IsOpened()))
	{
		return luaL_error(L, "AudioPlayback is already been opened");
	}
	bool success = NecroVC_AudioPlayback_Open();
	if (NECROVC_UNLIKELY(!success))
	{
		return luaL_error(L, "failed...");
	}
	return 0;
}

int NecroVC_AudioPlayback_LClose(lua_State* L)
{
	if (NECROVC_UNLIKELY(!NecroVC_AudioPlayback_IsOpened()))
	{
		return luaL_error(L, "AudioPlayback is already been closed or is not opened");
	}
	NecroVC_AudioPlayback_Close();
	return 0;
}

int NecroVC_AudioPlayback_LGetVolume(lua_State* L)
{
	if (NECROVC_UNLIKELY(!NecroVC_AudioPlayback_IsOpened()))
	{
		return luaL_error(L, "AudioPlayback is already been closed or is not opened");
	}
	float volume = NecroVC_AudioPlayback_GetVolume();
	lua_pushnumber(L, volume);
	return 1;
}

int NecroVC_AudioPlayback_LSetVolume(lua_State* L)
{
	lua_Number volume = luaL_checknumber(L, 1);
	if (NECROVC_UNLIKELY(!NecroVC_AudioPlayback_IsOpened()))
	{
		return luaL_error(L, "AudioPlayback is already been closed or is not opened");
	}
	NecroVC_AudioPlayback_SetVolume((float)volume);
	return 0;
}

int NecroVC_AudioPlayback_LHasChannel(lua_State* L)
{
	int channelID = luaL_checkint(L, 1);
	bool value = NecroVC_AudioPlayback_HasChannel(channelID);
	lua_pushboolean(L, value);
	return 1;
}

int NecroVC_AudioPlayback_LAddChannel(lua_State* L)
{
	int channelID = luaL_checkint(L, 1);
	bool added = NecroVC_AudioPlayback_TryAddChannel(channelID);
	if (!added)
	{
		luaL_error(L, "AudioPlayback has channel-%d already", channelID);
	}
	return 0;
}

int NecroVC_AudioPlayback_LRemoveChannel(lua_State* L)
{
	int channelID = luaL_checkint(L, 1);
	bool removed = NecroVC_AudioPlayback_TryRemoveChannel(channelID);
	if (!removed)
	{
		luaL_error(L, "AudioPlayback has no channel-%d", channelID);
	}
	return 0;
}

int NecroVC_AudioPlayback_LGetChannelVolume(lua_State* L)
{
	int channelID = luaL_checkint(L, 1);
	if (NECROVC_UNLIKELY(!NecroVC_AudioPlayback_HasChannel(channelID)))
	{
		return luaL_error(L, "AudioPlayback has no channel-%d", channelID);
	}
	float volume = NecroVC_AudioPlayback_GetChannelVolume(channelID);
	lua_pushnumber(L, volume);
	return 1;
}

int NecroVC_AudioPlayback_LSetChannelVolume(lua_State* L)
{
	int channelID = luaL_checkint(L, 1);
	lua_Number volume = luaL_checknumber(L, 2);
	if (NECROVC_UNLIKELY(!NecroVC_AudioPlayback_HasChannel(channelID)))
	{
		return luaL_error(L, "AudioPlayback has no channel-%d", channelID);
	}
	NecroVC_AudioPlayback_SetChannelVolume(channelID, (float)volume);
	return 0;
}

int NecroVC_AudioPlayback_LPush(lua_State* L)
{
	int channelID = luaL_checkint(L, 1);
	size_t length;
	const char* data = luaL_checklstring(L, 2, &length);
	if (NECROVC_UNLIKELY(length > INT32_MAX))
	{
		length = INT32_MAX;
	}
	if (NECROVC_UNLIKELY(!NecroVC_AudioPlayback_HasChannel(channelID)))
	{
		return luaL_error(L, "AudioPlayback has no channel-%d", channelID);
	}
	bool success = NecroVC_AudioPlayback_Push(channelID, (const unsigned char*)data, (opus_int32)length);
	if (NECROVC_UNLIKELY(!success))
	{
		return luaL_error(L, "AudioPlayback failed to decode voice data");
	}
	return 0;
}
