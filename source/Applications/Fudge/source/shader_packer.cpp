#include "shader_packer.h"
#include "filesystem/filesystem.h"
#include "filesystem/spv_file.h"
#include "core/core.h"
#include <kibble/string/string.h>
#include "render/shader_lang.h"
#include <kibble/logger/logger.h>

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>

using namespace erwin;

namespace fudge
{
namespace spv
{

static std::string extension_from_type(erwin::slang::ExecutionModel type)
{
	switch(type)
	{
        case erwin::slang::ExecutionModel::Vertex:                 return ".vert";
        case erwin::slang::ExecutionModel::TessellationControl:    return ".tesc";
		case erwin::slang::ExecutionModel::TessellationEvaluation: return ".tese";
		case erwin::slang::ExecutionModel::Geometry:               return ".geom";
        case erwin::slang::ExecutionModel::Fragment:               return ".frag";
		case erwin::slang::ExecutionModel::Compute:                return ".comp";
	}
}

static std::string spv_file_from_type(erwin::slang::ExecutionModel type)
{
	switch(type)
	{
		case erwin::slang::ExecutionModel::Vertex:                 return "vert.spv";
        case erwin::slang::ExecutionModel::TessellationControl:    return "tesc.spv";
        case erwin::slang::ExecutionModel::TessellationEvaluation: return "tese.spv";
		case erwin::slang::ExecutionModel::Geometry:               return "geom.spv";
		case erwin::slang::ExecutionModel::Fragment:               return "frag.spv";
        case erwin::slang::ExecutionModel::Compute:                return "comp.spv";
	}
}

extern bool check_toolchain()
{
    if(system("glslangValidator -v > /dev/null 2>&1"))
    {
        KLOGW("fudge") << "glslangValidator not found, skipping shader compilation." << std::endl;
        return false;
    }
    if(system("spirv-link --version > /dev/null 2>&1"))
    {
        KLOGW("fudge") << "spirv-link not found, skipping shader compilation." << std::endl;
        return false;
    }
    if(system("spirv-opt --version > /dev/null 2>&1"))
    {
        KLOGW("fudge") << "spirv-opt not found, skipping shader compilation." << std::endl;
        return false;
    }
    
    return true;
}

// TODO: Stop using system calls, compile shaders programatically.
void make_shader_spirv(const fs::path& source_path, const fs::path& output_dir)
{
	fs::path source_dir = source_path.parent_path();
	fs::path tmp_dir = output_dir / "tmp";
    fs::path out_path = output_dir / (source_path.stem().string() + ".spv");
	std::string shader_name = source_path.stem().string();

    std::vector<std::pair<erwin::slang::ExecutionModel, std::string>> sources;
    erwin::slang::pre_process_GLSL(source_path, sources);

    // Export temporary source files for each shader
    std::vector<fs::path> spvs;
    std::string spvs_str;
    bool success = true;
    for(auto&& [type, source]: sources)
    {
    	fs::path shader_file = tmp_dir / (shader_name + extension_from_type(type));
    	fs::path out_spv     = tmp_dir / spv_file_from_type(type);
    	spvs.push_back(out_spv);
    	spvs_str += out_spv.string() + " ";

    	std::ofstream ofs(shader_file);
		ofs << source;
		ofs.close();

    	// Compile shader file
		std::stringstream cmd;
		cmd << "glslangValidator -G -e main -o " << out_spv.string() << " " << shader_file.string();
        // cmd << " > /dev/null";
		int error = system(cmd.str().c_str());
		success &= (error==0);

    	// Erase temporary source file
    	fs::remove(shader_file);
    }

    if(success)
    {
    	KLOG("fudge",1) << "Successfully compiled shaders. Now, linking." << std::endl;
    	KLOGI << kb::KS_PATH_ << out_path.filename() << kb::KC_ << std::endl;

		std::stringstream cmd;
		cmd << "spirv-link " << spvs_str << "-o " << out_path.string();
		system(cmd.str().c_str());

        // Optimize output spv
        cmd.str("");
        cmd << "spirv-opt ";
        cmd << "--inline-entry-points-exhaustive ";
        cmd << "--convert-local-access-chains ";
        cmd << "--eliminate-local-single-block ";
        cmd << "--eliminate-local-single-store ";
        cmd << "--eliminate-insert-extract ";
        cmd << "--eliminate-dead-code-aggressive ";
        cmd << "--eliminate-dead-branches ";
        cmd << "--merge-blocks ";
        cmd << "--eliminate-local-single-block ";
        cmd << "--eliminate-local-single-store ";
        cmd << "--eliminate-local-multi-store ";
        cmd << "--eliminate-insert-extract ";
        cmd << "--eliminate-dead-code-aggressive ";
        cmd << "-o " << out_path.string() << " " << out_path.string();

        KLOG("fudge",1) << "Optimizing." << std::endl;
        system(cmd.str().c_str());

        // Check output file
        auto stages = erwin::spv::parse_stages(out_path);
        for(auto&& stage: stages)
        {
            KLOG("fudge",1) << "Entry point for " << extension_from_type(stage.execution_model)
                            << ": " << kb::KS_GOOD_ << stage.entry_point << kb::KC_ << std::endl;
        }
    }
}

} // namespace spv
} // namespace fudge