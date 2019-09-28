#pragma once

// Used by client applications

#ifdef W_ENTRY_POINT
	#include "core/entry_point.h"
#endif

#include "core/application.h"
#include "core/wtypes.h"
#include "core/layer.h"
#include "core/file_system.h"
#include "input/camera_2d_controller.h"
#include "event/event.h"
#include "event/window_events.h"
#include "debug/logger.h"

#include "render/renderer_2d.h"

#include "imgui.h"
#include "imgui/imgui_utils.h"
