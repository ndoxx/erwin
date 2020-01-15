#pragma once

#include "core/core.h"
#include "core/wtypes.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace erwin
{
namespace slang
{

// All possible stages of a GPU program
enum class ExecutionModel: uint32_t
{
    Vertex = 0,
    TessellationControl = 1,
    TessellationEvaluation = 2,
    Geometry = 3,
    Fragment = 4,
    Compute = 5,
};

// Helper function to select the appropriate execution model based on a hashed string.
// Typically; #type [str] directives are used in my .glsl files to mark the beginning 
// of a given execution model
[[maybe_unused]] static ExecutionModel hstring_to_execution_model(hash_t htype)
{
	switch(htype)
	{
        case "vertex"_h:                 return ExecutionModel::Vertex;
        case "vert"_h:                   return ExecutionModel::Vertex;
        case "tesselation_control"_h:    return ExecutionModel::TessellationControl;
        case "tesc"_h:                   return ExecutionModel::TessellationControl;
        case "tesselation_evaluation"_h: return ExecutionModel::TessellationEvaluation;
        case "tese"_h:                   return ExecutionModel::TessellationEvaluation;
        case "geometry"_h:               return ExecutionModel::Geometry;
        case "geom"_h:                   return ExecutionModel::Geometry;
        case "fragment"_h:               return ExecutionModel::Fragment;
        case "frag"_h:                   return ExecutionModel::Fragment;
        case "compute"_h:                return ExecutionModel::Compute;
        case "comp"_h:                   return ExecutionModel::Compute;
	}
    W_ASSERT(false, "Unknown shader type.");
    return ExecutionModel::Vertex;
}

// Helper debug function to display an execution model as a string
[[maybe_unused]] static std::string to_string(ExecutionModel type)
{
	switch(type)
	{
		case ExecutionModel::Vertex:                 return "Vertex shader";
        case ExecutionModel::TessellationControl:    return "Tesselation Control shader";
        case ExecutionModel::TessellationEvaluation: return "Tesselation Evaluation shader";
		case ExecutionModel::Geometry:               return "Geometry shader";
        case ExecutionModel::Fragment:               return "Fragment shader";
		case ExecutionModel::Compute:                return "Compute shader";
	}
}

// Register a path to an include directory
extern void register_include_directory(const fs::path& dir_path);
// Drop all include directories
extern void clear_include_directories();
// Split up a GLSL source into multiple source strings, one for each execution model,
// also handle includes
extern void pre_process_GLSL(const fs::path& filepath, std::vector<std::pair<ExecutionModel, std::string>>& sources);


} // namespace slang
} // namespace erwin