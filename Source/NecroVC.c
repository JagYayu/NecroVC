#include "NecroVC.h"

#include "lauxlib.h"
#include "lua.h"
#include "miniaudio.h"
#include "opus.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

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

enum
{
	NecroVC_SampleRate = 16000,     // 16kHz
	NecroVC_FrameSize = 320,        // 20ms
	NecroVC_EncodeBufferSize = 512, //
	NecroVC_CaptureRingSize = 25,   // 25*20ms=640ms
	NecroVC_PlaybackRingSize = 16,  // 16*20ms=320ms
};

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

typedef struct
{
	unsigned char datas[NecroVC_CaptureRingSize][NecroVC_EncodeBufferSize]; // 编码后的数据
	int lengths[NecroVC_CaptureRingSize];                                   // 每帧实际长度
	volatile int readIndex;
	volatile int writeIndex;
} NecroVC_EncRingBuf;

typedef struct
{
	const unsigned char* data;
	int length;
} NecroVC_EncRingBufFrame;

bool NecroVC_EncRingBuf_IsEmpty(const NecroVC_EncRingBuf* ringBuffer)
{
	return ringBuffer->readIndex == ringBuffer->writeIndex;
}

bool NecroVC_EncRingBuf_IsFull(const NecroVC_EncRingBuf* ringBuffer)
{
	int next = (ringBuffer->writeIndex + 1) % NecroVC_CaptureRingSize;
	return next == ringBuffer->readIndex;
}

NecroVC_EncRingBufFrame NecroVC_EncRingBuf_ReadFrame(NecroVC_EncRingBuf* ringBuffer)
{
	int i = ringBuffer->readIndex;
	NecroVC_EncRingBufFrame frame = {
	    (const unsigned char*)ringBuffer->datas[i],
	    ringBuffer->lengths[i],
	};
	ringBuffer->readIndex = (i + 1) % NecroVC_CaptureRingSize;
	return frame;
}

typedef struct
{
	ma_device device;
	NecroVC_EncRingBuf buffer;
} NecroVC_AudioCapture;

static void NecroVC_OnAudioCaptureDataProcess(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	(void)pOutput; // 采集设备不需要输出

	NecroVC_AudioCapture* audioCapture = pDevice->pUserData;
	if (audioCapture == NULL)
	{
		return;
	}

	if (frameCount != (ma_uint32)NecroVC_FrameSize)
	{
		return;
	}

	const opus_int16* pcm = pInput; // 假设 miniaudio 配置为 S16
	unsigned char encoded[512];
	int encLen;

	// Opus 编码
	encLen = opus_encode(opus.encoder, pcm, frameCount, encoded, sizeof(encoded));
	if (encLen <= 0)
	{
		// 编码错误，丢弃
		return;
	}

	NecroVC_EncRingBuf* buffer = &audioCapture->buffer;
	int next = (buffer->writeIndex + 1) % NecroVC_CaptureRingSize;
	if (next != buffer->readIndex)
	{
		memcpy(buffer->datas[buffer->writeIndex], encoded, encLen);
		buffer->lengths[buffer->writeIndex] = encLen;
		buffer->writeIndex = next;
	}
	else
	{
		// 缓冲区已满则丢帧
	}
}

static NecroVC_AudioCapture NecroVC_audioCapture;

static bool NecroVC_BeginAudioCapture()
{
	memset(&NecroVC_audioCapture.buffer, 0, sizeof(NecroVC_audioCapture.buffer));

	ma_device_config config = ma_device_config_init(ma_device_type_capture);
	config.capture.format = ma_format_s16;
	config.capture.channels = 1;
	config.sampleRate = NecroVC_SampleRate;
	config.periodSizeInFrames = NecroVC_FrameSize;
	config.dataCallback = NecroVC_OnAudioCaptureDataProcess;

	ma_device* device = &NecroVC_audioCapture.device;
	if (ma_device_init(NULL, &config, device) != MA_SUCCESS)
	{
		return false;
	}
	if (ma_device_start(device) != MA_SUCCESS)
	{
		ma_device_uninit(device);
		return false;
	}
	device->pUserData = &NecroVC_audioCapture;

	return true;
}

static void NecroVC_EndAudioCapture()
{
	ma_device* device = &NecroVC_audioCapture.device;
	device->pUserData = NULL;
	ma_device_stop(device);
	ma_device_uninit(device);
}

typedef struct
{
	opus_int16 datas[NecroVC_PlaybackRingSize][NecroVC_FrameSize]; // PCM 数据
	volatile int readIndex;
	volatile int writeIndex;
} NecroVC_PcmRingBuf;

static bool NecroVC_PcmRingBuf_IsEmpty(const NecroVC_PcmRingBuf* ring)
{
	return ring->readIndex == ring->writeIndex;
}

static bool NecroVC_PcmRingBuf_IsFull(const NecroVC_PcmRingBuf* ring)
{
	int next = (ring->writeIndex + 1) % NecroVC_PlaybackRingSize;
	return next == ring->readIndex;
}

void NecroVC_PcmRingBuf_WriteFrame(NecroVC_PcmRingBuf* buf, opus_int16 pcm[NecroVC_FrameSize])
{
	if (NecroVC_PcmRingBuf_IsFull(buf))
	{
		// 丢弃最老的一帧
		buf->readIndex = (buf->readIndex + 1) % NecroVC_PlaybackRingSize;
	}
	memcpy(buf->datas[buf->writeIndex], pcm, NecroVC_FrameSize * sizeof(opus_int16));
	buf->writeIndex = (buf->writeIndex + 1) % NecroVC_PlaybackRingSize;
}

typedef struct
{
	ma_device device;
	NecroVC_PcmRingBuf buffer;
} NecroVC_AudioPlayback;

static void NecroVC_OnAudioPlaybackDataProcess(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	(void)pInput;

	NecroVC_AudioPlayback* audioPlayback = pDevice->pUserData;
	if (audioPlayback == NULL)
	{
		return;
	}

	if (NecroVC_PcmRingBuf_IsEmpty(&audioPlayback->buffer))
	{
		// 没有数据，输出静音
		memset(pOutput, 0, frameCount * sizeof(opus_int16));
		return;
	}
	else
	{
		// 输出音频
		memcpy(pOutput, audioPlayback->buffer.datas[audioPlayback->buffer.readIndex], frameCount * sizeof(opus_int16));
		audioPlayback->buffer.readIndex = (audioPlayback->buffer.readIndex + 1) % NecroVC_PlaybackRingSize;
	}

	(void)pDevice;
}

static NecroVC_AudioPlayback NecroVC_audioPlayback;

static bool NecroVC_BeginAudioPlayback()
{
	memset(&NecroVC_audioPlayback.buffer, 0, sizeof(NecroVC_audioPlayback.buffer));

	ma_device_config config = ma_device_config_init(ma_device_type_playback);
	config.playback.format = ma_format_s16;
	config.playback.channels = 1;
	config.sampleRate = NecroVC_SampleRate;
	config.periodSizeInFrames = NecroVC_FrameSize;
	config.dataCallback = NecroVC_OnAudioPlaybackDataProcess;

	ma_device* device = &NecroVC_audioPlayback.device;
	if (ma_device_init(NULL, &config, device) != MA_SUCCESS)
	{
		return false;
	}
	if (ma_device_start(device) != MA_SUCCESS)
	{
		ma_device_uninit(device);
		return false;
	}
	device->pUserData = &NecroVC_audioPlayback;

	return true;
}

static void NecroVC_EndAudioPlayback()
{
	ma_device* device = &NecroVC_audioPlayback.device;
	device->pUserData = NULL;
	ma_device_stop(device);
	ma_device_uninit(device);
}

static bool necroVC = false;

int NecroVC_IsOpened(lua_State* L)
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

int NecroVC_Open(lua_State* L)
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

	// 初始化 音频捕获器
	if (!NecroVC_BeginAudioCapture())
	{
		NecroVC_CloseOpus();

		lua_pushboolean(L, false);
		lua_pushstring(L, "Failed to start audio capture device");
		return 2;
	}

	// 初始化 音频播放器
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

NECROVC_API int NecroVC_Close(lua_State* L)
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

int NecroVC_Pull(lua_State* L)
{
	if (!necroVC)
	{
		lua_pushnil(L);
		lua_pushstring(L, NECROVC_CLOSED);
		return 2;
	}

	if (NecroVC_EncRingBuf_IsEmpty(&NecroVC_audioCapture.buffer))
	{
		lua_pushstring(L, "");
		return 1;
	}

	NecroVC_EncRingBufFrame frame = NecroVC_EncRingBuf_ReadFrame(&NecroVC_audioCapture.buffer);
	lua_pushlstring(L, (const char*)frame.data, frame.length);
	return 1;
}

int NecroVC_Push(lua_State* L)
{
	size_t len;
	const char* data = luaL_checklstring(L, 1, &len);

	if (!necroVC)
	{
		lua_pushnil(L);
		lua_pushstring(L, NECROVC_CLOSED);
		return 2;
	}

	opus_int16 pcm[NecroVC_FrameSize];
	int decLen = opus_decode(opus.decoder, (const unsigned char*)data, (opus_int32)len, pcm, NecroVC_FrameSize, 0);
	if (decLen <= 0)
	{
		lua_pushboolean(L, false);
		lua_pushfstring(L, "Decode error: %d", decLen);
		return 2;
	}

	NecroVC_PcmRingBuf_WriteFrame(&NecroVC_audioPlayback.buffer, pcm);

	lua_pushboolean(L, true);
	return 1;
}
