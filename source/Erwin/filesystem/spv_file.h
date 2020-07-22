#pragma once

/*
	Spir-V binary shader module
		* Simple parsing routines for .spv files
*/

#include <vector>

#include "core/core.h"
#include "render/shader_lang.h"
#include "filesystem/wpath.h"

namespace erwin
{
namespace spv
{

struct ShaderStageDescriptor
{
	slang::ExecutionModel execution_model;
	std::string entry_point;
};

extern std::vector<ShaderStageDescriptor> parse_stages(const WPath& path);

} // namespace spv
} // namespace erwin
