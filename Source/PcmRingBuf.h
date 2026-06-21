#pragma once

#include "NecroVC.h"
#include "Position.h"

#include "opus_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct
	{
		opus_int16 datas[NecroVC_PlaybackRingSize][NecroVC_FrameSize]; // PCM 数据
		NecroVC_Position positions[NecroVC_PlaybackRingSize];          // 支持空间音效
		volatile int readIndex;
		volatile int writeIndex;
	} NecroVC_PcmRingBuf;

	bool NecroVC_PcmRingBuf_IsEmpty(const NecroVC_PcmRingBuf* ring);

	bool NecroVC_PcmRingBuf_IsFull(const NecroVC_PcmRingBuf* ring);

	void NecroVC_PcmRingBuf_WriteFrame(NecroVC_PcmRingBuf* buf, opus_int16 pcm[NecroVC_FrameSize]);

#ifdef __cplusplus
}
#endif
