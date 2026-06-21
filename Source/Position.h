#pragma once

#include <math.h>
#include <stdbool.h>

typedef struct
{
	float x;
	float y;
} NecroVC_Position;

static const NecroVC_Position NecroVC_Position_Null = {NAN, NAN};

inline bool NecroVC_Position_IsNull(NecroVC_Position position)
{
	return position.x == NAN || position.y == NAN;
}
