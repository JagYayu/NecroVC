#include "AudioCapture.h"

#include "EncRingBuf.h"

#include "Utilities.h"
#include "lauxlib.h"
#include "lua.h"
#include "opus.h"

#include <stdint.h>
#include <string.h>

static NecroVC_AudioCapture NecroVC_audioCaptures[NecroVC_AudioCapture_MaxCount] = {};

static bool NecroVC_audioCaptureStates[NecroVC_AudioCapture_MaxCount] = {false, false, false, false};

bool NecroVC_AudioCapture_IsOpened(int index)
{
	return index >= 0 && index <= NecroVC_AudioCapture_MaxCount ? NecroVC_audioCaptureStates[index] : false;
}

static void NecroVC_OnAudioCaptureDataProcess(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	(void)pOutput;

	NecroVC_AudioCapture* audioCapture = pDevice->pUserData;
	if (audioCapture == NULL)
	{
		return;
	}
	if (frameCount != (ma_uint32)NecroVC_FrameSize)
	{
		return;
	}

	const opus_int16* pcm = pInput;
	unsigned char data[512];
	int length;

	float volume = audioCapture->volume;
	if (volume == 1.0f)
	{
		length = opus_encode(audioCapture->encoder, pcm, frameCount, data, sizeof(data));
	}
	else
	{
		opus_int16 pcmAdjusted[NecroVC_FrameSize];

		for (ma_uint32 i = 0; i < frameCount; i++)
		{
			pcmAdjusted[i] = (opus_int16)NECROVC_CLAMP(INT16_MIN, (float)pcm[i] * volume, INT16_MAX);
		}

		length = opus_encode(audioCapture->encoder, pcmAdjusted, frameCount, data, sizeof(data));
	}

	if (length <= 0)
	{
		return;
	}

	NecroVC_EncRingBuf* buffer = &audioCapture->buffer;
	if (!NecroVC_EncRingBuf_IsFull(buffer))
	{
		NecroVC_EncRingBufFrame frame = {data, length};
		NecroVC_EncRingBuf_WriteFrame(buffer, frame);
	}
}

static bool NecroVC_AudioCapture_Construct(NecroVC_AudioCapture* audioCapture)
{
	int opusErr;
	audioCapture->encoder = opus_encoder_create(NecroVC_SampleRate, 1, OPUS_APPLICATION_VOIP, &opusErr);
	if (opusErr != OPUS_OK)
	{
		return false; // TODO opus not ok!
	}

	memset(&audioCapture->buffer, 0, sizeof(audioCapture->buffer));

	ma_device_config config = ma_device_config_init(ma_device_type_capture);
	config.capture.format = ma_format_s16;
	config.capture.channels = 1;
	config.sampleRate = NecroVC_SampleRate;
	config.periodSizeInFrames = NecroVC_FrameSize;
	config.dataCallback = NecroVC_OnAudioCaptureDataProcess;

	ma_device* device = &audioCapture->device;
	if (ma_device_init(NULL, &config, device) != MA_SUCCESS)
	{
		return false;
	}
	if (ma_device_start(device) != MA_SUCCESS)
	{
		ma_device_uninit(device);
		return false;
	}
	device->pUserData = audioCapture;

	return true;
}

bool NecroVC_AudioCapture_Open(int index)
{
	NecroVC_AudioCapture* audioCapture = &NecroVC_audioCaptures[index];
	if (NecroVC_AudioCapture_Construct(audioCapture))
	{
		NecroVC_audioCaptureStates[index] = true;
		return true;
	}
	return false;
}

static void NecroVC_AudioCapture_Destruct(NecroVC_AudioCapture* audioCapture)
{
	ma_device* device = &audioCapture->device;
	device->pUserData = NULL;
	ma_device_stop(device);
	ma_device_uninit(device);
	opus_encoder_destroy(audioCapture->encoder);
}

void NecroVC_AudioCapture_Close(int index)
{
	NecroVC_audioCaptureStates[index] = false;
	NecroVC_AudioCapture* audioCapture = &NecroVC_audioCaptures[index];
	NecroVC_AudioCapture_Destruct(audioCapture);
}

float NecroVC_AudioCapture_GetVolume(int index)
{
	return NecroVC_audioCaptures[index].volume;
}

bool NecroVC_AudioCapture_TryGetVolume(int index, float* volume_)
{
	if (NecroVC_AudioCapture_IsOpened(index))
	{
		*volume_ = NecroVC_audioCaptures[index].volume;
		return true;
	}
	return false;
}

void NecroVC_AudioCapture_SetVolume(int index, float volume)
{
	NecroVC_audioCaptures[index].volume = volume;
}

bool NecroVC_AudioCapture_TrySetVolume(int index, float volume)
{
	if (NecroVC_AudioCapture_IsOpened(index))
	{
		NecroVC_audioCaptures[index].volume = volume;
		return true;
	}
	return false;
}

NecroVC_EncRingBufFrame NecroVC_AudioCapture_Pull(size_t index)
{
	NecroVC_EncRingBuf* buffer = &NecroVC_audioCaptures[index].buffer;
	if (NecroVC_EncRingBuf_IsEmpty(buffer))
	{
		NecroVC_EncRingBufFrame frame = {NULL, 0};
		return frame;
	}
	return NecroVC_EncRingBuf_ReadFrame(buffer);
}

int NecroVC_AudioCapture_LGetMaxCount(lua_State* L)
{
	lua_pushinteger(L, NecroVC_AudioCapture_MaxCount);
	return 1;
}

int NecroVC_AudioCapture_LIsOpened(lua_State* L)
{
	int index = luaL_checkint(L, 1);
	bool value = NecroVC_AudioCapture_IsOpened(index);
	lua_pushboolean(L, value);
	return 1;
}

int NecroVC_AudioCapture_LOpen(lua_State* L)
{
	int index = luaL_checkint(L, 1);
	if (NECROVC_UNLIKELY(NecroVC_AudioCapture_IsOpened(index)))
	{
		return luaL_error(L, "AudioCapture-%d has already been opened", index);
	}
	bool success = NecroVC_AudioCapture_Open(index);
	if (NECROVC_UNLIKELY(!success))
	{
		// TODO add more failure messages
		return luaL_error(L, "AudioCapture-%d failed to open", index);
	}
	return 0;
}

int NecroVC_AudioCapture_LClose(lua_State* L)
{
	int index = luaL_checkint(L, 1);
	if (NECROVC_UNLIKELY(!NecroVC_AudioCapture_IsOpened(index)))
	{
		return luaL_error(L, "AudioCapture-%d has already been closed or is not opened", index);
	}
	NecroVC_AudioCapture_Close(index);
	return 0;
}

int NecroVC_AudioCapture_LGetVolume(lua_State* L)
{
	int index = luaL_checkint(L, 1);
	if (NECROVC_UNLIKELY(!NecroVC_AudioCapture_IsOpened(index)))
	{
		luaL_error(L, "AudioCapture-%d is not opened", index);
	}
	float volume = NecroVC_AudioCapture_GetVolume(index);
	lua_pushnumber(L, volume);
	return 1;
}

int NecroVC_AudioCapture_LSetVolume(lua_State* L)
{
	int index = luaL_checkint(L, 1);
	lua_Number volume = luaL_checknumber(L, 2);
	if (NECROVC_UNLIKELY(!NecroVC_AudioCapture_IsOpened(index)))
	{
		luaL_error(L, "AudioCapture-%d is not opened", index);
	}
	NecroVC_AudioCapture_SetVolume(index, (float)volume);
	return 0;
}

static void NecroVC_AudioCapture_LTryPull(lua_State* L, int index)
{
	if (NecroVC_AudioCapture_IsOpened(index))
	{
		NecroVC_EncRingBufFrame frame = NecroVC_AudioCapture_Pull(index);
		lua_pushlstring(L, (const char*)frame.data, frame.length);
	}
	else
	{
		lua_pushnil(L);
	}
}

int NecroVC_AudioCapture_LPull(lua_State* L)
{
	int index = luaL_checkint(L, 1);
	NecroVC_AudioCapture_LTryPull(L, index);
	if (NECROVC_UNLIKELY(lua_isnil(L, 2)))
	{
		return luaL_error(L, "AudioCapture-%d is not opened", index);
	}
	return 1;
}

int NecroVC_AudioCapture_LPullAll(lua_State* L)
{
	for (int index = 0; index < NecroVC_AudioCapture_MaxCount; ++index)
	{
		NecroVC_AudioCapture_LTryPull(L, index);
	}
	return 4;
}
