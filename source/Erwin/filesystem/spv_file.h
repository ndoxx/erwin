#pragma once

/*
	Spir-V binary shader program file
*/

#include <vector>
#include <filesystem>

#include "render/texture_common.h"

namespace fs = std::filesystem;

namespace erwin
{
namespace spv
{

enum class ShaderType: uint8_t
{
	None = 0,
	Vertex,
	Geometry,
	Fragment,
};

struct SPVShaderDescriptor
{
	ShaderType type = ShaderType::None;
	std::string entry_point = "main";
	std::vector<char> data;
};

struct SPVDescriptor
{
	fs::path filepath;
	std::vector<SPVShaderDescriptor> shaders;
};

extern void read_spv(SPVDescriptor& desc);
extern void write_spv(const SPVDescriptor& desc);

} // namespace spv
} // namespace erwin
