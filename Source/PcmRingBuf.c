#include "PcmRingBuf.h"

#include <string.h>

bool NecroVC_PcmRingBuf_IsEmpty(const NecroVC_PcmRingBuf* ring)
{
	return ring->readIndex == ring->writeIndex;
}

bool NecroVC_PcmRingBuf_IsFull(const NecroVC_PcmRingBuf* ring)
{
	int next = (ring->writeIndex + 1) % NecroVC_PlaybackRingSize;
	return next == ring->readIndex;
}

void NecroVC_PcmRingBuf_WriteFrame(NecroVC_PcmRingBuf* buf, opus_int16 pcm[NecroVC_FrameSize])
{
	if (NecroVC_PcmRingBuf_IsFull(buf))
	{
		buf->readIndex = (buf->readIndex + 1) % NecroVC_PlaybackRingSize;
	}
	memcpy(buf->datas[buf->writeIndex], pcm, NecroVC_FrameSize * sizeof(opus_int16));
	buf->writeIndex = (buf->writeIndex + 1) % NecroVC_PlaybackRingSize;
}
