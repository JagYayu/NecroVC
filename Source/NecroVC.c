#include "NecroVC.h"
#include "AudioPlayback.h"
#include <math.h>

#define MINIAUDIO_IMPLEMENTATION

#include "AudioCapture.h"
#include "lua.h"
#include "miniaudio.h"

#include <assert.h>
#include <stdbool.h>

#include "winver.h"

#define NECROVC_CLOSED ("NecroVC has not been opened or has been closed already")

typedef enum
{
	NecroVC_OpenOpusError_None = 0,
	NecroVC_OpenOpusError_EncoderInitFail,
	NecroVC_OpenOpusError_DecoderInitFail,
} NecroVC_OpenOpusError;

typedef struct
{
	NecroVC_OpenOpusError error;
	int value;
} NecroVC_OpenOpusResult;

typedef struct
{
	OpusEncoder* encoder;
	OpusDecoder* decoder;
} NecroVC_Opus;

static NecroVC_Opus opus;

NecroVC_OpenOpusResult NecroVC_OpenOpus()
{
	NecroVC_OpenOpusResult result;

	opus.encoder = opus_encoder_create(NecroVC_SampleRate, 1, OPUS_APPLICATION_VOIP, &result.value);
	if (result.value != OPUS_OK)
	{
		result.error = NecroVC_OpenOpusError_EncoderInitFail;
		return result;
	}

	opus.decoder = opus_decoder_create(NecroVC_SampleRate, 1, &result.value);
	if (result.value != OPUS_OK)
	{
		result.error = NecroVC_OpenOpusError_DecoderInitFail;
		return result;
	}

	result.error = NecroVC_OpenOpusError_None;
	return result;
}

void NecroVC_CloseOpus()
{
	opus_decoder_destroy(opus.decoder);
	opus_encoder_destroy(opus.encoder);
}

static bool NecroVC_BeginAudioCapture()
{
	return NecroVC_AudioCapture_Open(1);
}

static void NecroVC_EndAudioCapture()
{
	NecroVC_AudioCapture_Close(1);
}

static bool NecroVC_BeginAudioPlayback()
{
	return NecroVC_AudioPlayback_Open();
}

static void NecroVC_EndAudioPlayback()
{
	NecroVC_AudioPlayback_Close();
}

static bool necroVC = false;

int NecroVC_LIsOpened(lua_State* L)
{
	lua_pushboolean(L, necroVC);
	return 1;
}

static const char* NecroVC_OpusErrorString(int errorCode)
{
	switch (errorCode)
	{
	case OPUS_BAD_ARG:
		return "One or more invalid/out of range arguments";
	case OPUS_BUFFER_TOO_SMALL:
		return "Not enough bytes allocated in the buffer";
	case OPUS_INTERNAL_ERROR:
		return "An internal error was detected";
	case OPUS_INVALID_PACKET:
		return "The compressed data passed is corrupted";
	case OPUS_UNIMPLEMENTED:
		return "Invalid/unsupported request number";
	case OPUS_INVALID_STATE:
		return "An encoder or decoder structure is invalid or already freed";
	case OPUS_ALLOC_FAIL:
		return "Memory allocation has failed";
	default:
		return "";
	}
}

int NecroVC_LOpen(lua_State* L)
{
	if (necroVC)
	{
		lua_pushboolean(L, false);
		lua_pushstring(L, "NecroVC has been opened already");
		return 2;
	}

	// 初始化 opus 的编码器、解码器
	NecroVC_OpenOpusResult result = NecroVC_OpenOpus();
	switch (result.error)
	{
	case NecroVC_OpenOpusError_EncoderInitFail:
		lua_pushboolean(L, false);
		lua_pushfstring(L, "Failed to initialize opus encoder: %d: %s", result.value, NecroVC_OpusErrorString(result.value));
		return 2;
	case NecroVC_OpenOpusError_DecoderInitFail:
		lua_pushboolean(L, false);
		lua_pushfstring(L, "Failed to initialize opus decoder: %d: %s", result.value, NecroVC_OpusErrorString(result.value));
		return 2;
	default:
		break;
	}

	if (!NecroVC_BeginAudioCapture())
	{
		NecroVC_CloseOpus();

		lua_pushboolean(L, false);
		lua_pushstring(L, "Failed to start audio capture device");
		return 2;
	}

	if (!NecroVC_BeginAudioPlayback())
	{
		NecroVC_EndAudioCapture();
		NecroVC_CloseOpus();

		lua_pushboolean(L, false);
		lua_pushstring(L, "Failed to start audio playback device");
		return 2;
	}

	necroVC = true;

	lua_pushboolean(L, true);
	return 1;
}

int NecroVC_LClose(lua_State* L)
{
	if (!necroVC)
	{
		lua_pushboolean(L, false);
		lua_pushstring(L, NECROVC_CLOSED);
		return 2;
	}

	NecroVC_EndAudioPlayback();
	NecroVC_EndAudioCapture();
	NecroVC_CloseOpus();

	necroVC = false;

	lua_pushboolean(L, true);
	return 1;
}

int NecroVC_LPull(lua_State* L)
{
	if (!necroVC)
	{
		lua_pushboolean(L, false);
		lua_pushstring(L, NECROVC_CLOSED);
		return 2;
	}
	if (!NecroVC_AudioCapture_IsOpened(0))
	{
		NecroVC_AudioCapture_Open(0);
	}
	if (lua_gettop(L) < 1)
	{
		lua_pushinteger(L, 0);
	}
	else if (!lua_isnumber(L, 1))
	{
		lua_pushinteger(L, 0);
		lua_replace(L, 1);
	}
	lua_pushinteger(L, 0);
	return NecroVC_AudioCapture_LPull(L);
}

int NecroVC_LPush(lua_State* L)
{
	if (!necroVC)
	{
		lua_pushboolean(L, false);
		lua_pushstring(L, NECROVC_CLOSED);
		return 2;
	}
	if (!NecroVC_AudioPlayback_IsOpened())
	{
		NecroVC_AudioPlayback_Open();
	}
	if (!NecroVC_AudioPlayback_HasChannel(0))
	{
		NecroVC_AudioPlayback_AddChannel(0);
	}
	if (lua_gettop(L) < 2)
	{
		lua_pushinteger(L, 0);
	}
	else if (!lua_isnumber(L, 2))
	{
		lua_pushinteger(L, 0);
		lua_replace(L, 2);
	}
	lua_pushvalue(L, 1);
	lua_pushvalue(L, 2);
	lua_replace(L, 1);
	lua_replace(L, 2);
	return NecroVC_AudioPlayback_LPush(L);
}
