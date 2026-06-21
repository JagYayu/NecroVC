#pragma once

#define NECROVC_CLAMP(min, val, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))
