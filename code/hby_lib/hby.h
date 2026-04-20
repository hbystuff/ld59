#pragma once

#ifdef _MSC_VER
#pragma warning(error:4013)
#pragma warning(error:4024)
#pragma warning(error:4020)
#pragma warning(error:4700)
#pragma warning(error:4133)
#pragma warning(error:4028)
#pragma warning(error:4029)
#pragma warning(error:4047)
#endif

#include "basic.h"
#include "app.h"
#include "gfx.h"
#include "vec.h"
#include "data.h"
#include "sprite.h"
#include "util.h"
#include "profiler.h"
// TODO: Get rid of loader for all the specific things.
//       loader module should only manage archive, timestamp, and save game and stuff.
#include "loader.h"
#include "audio.h"
#include "image_data.h"
#include "stb/stb_image.h"
