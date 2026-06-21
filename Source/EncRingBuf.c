#include "EncRingBuf.h"

#include <string.h>

bool NecroVC_EncRingBuf_IsEmpty(const NecroVC_EncRingBuf* encRingBuf)
{
	return encRingBuf->readIndex == encRingBuf->writeIndex;
}

bool NecroVC_EncRingBuf_IsFull(const NecroVC_EncRingBuf* encRingBuf)
{
	int next = (encRingBuf->writeIndex + 1) % NecroVC_CaptureRingSize;
	return next == encRingBuf->readIndex;
}

NecroVC_EncRingBufFrame NecroVC_EncRingBuf_ReadFrame(NecroVC_EncRingBuf* encRingBuf)
{
	int i = encRingBuf->readIndex;
	NecroVC_EncRingBufFrame frame = {
	    (const unsigned char*)encRingBuf->datas[i],
	    encRingBuf->lengths[i],
	};
	encRingBuf->readIndex = (i + 1) % NecroVC_CaptureRingSize;
	return frame;
}

void NecroVC_EncRingBuf_WriteFrame(NecroVC_EncRingBuf* encRingBuf, NecroVC_EncRingBufFrame frame)
{
	int index = encRingBuf->writeIndex;
	memcpy(encRingBuf->datas[index], frame.data, frame.length);
	encRingBuf->lengths[index] = frame.length;
	encRingBuf->writeIndex = (index + 1) % NecroVC_CaptureRingSize;
}
