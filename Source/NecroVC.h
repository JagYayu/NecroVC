#pragma once

#ifdef NECROVC_EXPORTS
#	define NECROVC_API __declspec(dllexport)
#else
#	define NECROVC_API __declspec(dllimport)
#endif

#if defined(__GNUC__) || defined(__clang__)
#	define NECROVC_LIKELY(x)   __builtin_expect(!!(x), 1)
#	define NECROVC_UNLIKELY(x) __builtin_expect(!!(x), 0)
#elif defined(_MSC_VER)
#	define NECROVC_LIKELY(x)   (x)
#	define NECROVC_UNLIKELY(x) (x)
#else
#	define NECROVC_LIKELY(x)   (x)
#	define NECROVC_UNLIKELY(x) (x)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

	enum
	{
		NecroVC_MaxAudioCaptures = 4,   // 4 local-coop players
		NecroVC_SampleRate = 16000,     // 16kHz
		NecroVC_FrameSize = 320,        // 20ms
		NecroVC_EncodeBufferSize = 512, //
		NecroVC_CaptureRingSize = 25,   // 25 * 20ms = 640ms
		NecroVC_PlaybackRingSize = 16,  // 16 * 20ms = 320ms
	};

	typedef struct lua_State lua_State;

	NECROVC_API int NecroVC_LIsOpened(lua_State* L);

	NECROVC_API int NecroVC_LOpen(lua_State* L);

	NECROVC_API int NecroVC_LClose(lua_State* L);

	NECROVC_API int NecroVC_LPull(lua_State* L);

	NECROVC_API int NecroVC_LPush(lua_State* L);

	// NECROVC_API int NecroVC_PushSpatial(lua_State* L);

#ifdef __cplusplus
}
#endif
