#pragma once

#include <cstdint>
#include "imgui.h"

namespace ImGui
{

enum class DialogState: uint8_t
{
	NONE = 0,
	OK,
	CANCEL,
	YES,
	NO,
};

extern void OpenModal(const char* title);
extern DialogState CheckOkCancel(const char* title, const char* text);
extern DialogState CheckYesNo(const char* title, const char* text);

} // namespace ImGui