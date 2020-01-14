#pragma once

/*
	Spir-V binary shader module
		* Simple parsing routines for .spv files
*/

#include <vector>
#include <filesystem>

#include "core/wtypes.h"
#include "render/shader_lang.h"

namespace fs = std::filesystem;

namespace erwin
{
namespace spv
{

struct ShaderStageDescriptor
{
	slang::ExecutionModel execution_model;
	std::string entry_point;
};

extern std::vector<ShaderStageDescriptor> parse_stages(const fs::path& path);

} // namespace spv
} // namespace erwin
