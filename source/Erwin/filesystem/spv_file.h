#pragma once

/*
	Spir-V binary shader module
		* Simple parsing routines for .spv files
*/

#include <vector>
#include <filesystem>

#include "core/wtypes.h"

namespace fs = std::filesystem;

namespace erwin
{
namespace spv
{

enum class ExecutionModel: uint32_t
{
    Vertex = 0,
    TessellationControl = 1,
    TessellationEvaluation = 2,
    Geometry = 3,
    Fragment = 4,
    GLCompute = 5,
};

struct ShaderStageDescriptor
{
	ExecutionModel execution_model;
	std::string entry_point;
};

extern std::vector<ShaderStageDescriptor> parse_stages(const fs::path& path);

} // namespace spv
} // namespace erwin
