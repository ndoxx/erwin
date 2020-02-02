#pragma once

// Used by client applications

#ifdef W_ENTRY_POINT
	#include "core/entry_point.h"
#endif

#include "core/application.h"
#include "core/core.h"
#include "core/layer.h"
#include "core/config.h"
#include "filesystem/filesystem.h"
#include "input/camera_2d_controller.h"
#include "input/camera_3d_controller.h"
#include "event/event_bus.h"
#include "event/window_events.h"
#include "debug/logger.h"

#include "render/renderer.h"
#include "render/renderer_2d.h"
#include "render/renderer_3d.h"
#include "render/renderer_pp.h"
#include "render/common_geometry.h"
#include "asset/asset_manager.h"
#include "asset/material.h"

#include "entity/entity_manager.h"
#include "entity/component.h"
#include "entity/component_system.h"

#include "imgui.h"
#include "imgui/imgui_utils.h"
