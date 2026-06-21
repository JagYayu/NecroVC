#pragma once

#include "NecroVC.h"

#include "opus_types.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

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

	bool NecroVC_EncRingBuf_IsEmpty(const NecroVC_EncRingBuf* ringBuffer);

	bool NecroVC_EncRingBuf_IsFull(const NecroVC_EncRingBuf* ringBuffer);

	NecroVC_EncRingBufFrame NecroVC_EncRingBuf_ReadFrame(NecroVC_EncRingBuf* ringBuffer);

	void NecroVC_EncRingBuf_WriteFrame(NecroVC_EncRingBuf* encRingBuf, NecroVC_EncRingBufFrame frame);

#ifdef __cplusplus
}
#endif
