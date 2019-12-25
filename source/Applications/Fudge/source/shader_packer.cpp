#include "shader_packer.h"
#include "filesystem/filesystem.h"
#include "filesystem/spv_file.h"
#include "core/core.h"
#include "utils/string.h"
#include "debug/logger.h"

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

static erwin::spv::ExecutionModel type_from_hstring(hash_t htype)
{
	switch(htype)
	{
		case "vertex"_h: 	             return erwin::spv::ExecutionModel::Vertex;
		case "vert"_h: 	                 return erwin::spv::ExecutionModel::Vertex;
        case "tesselation_control"_h:    return erwin::spv::ExecutionModel::TessellationControl;
        case "tesc"_h:                   return erwin::spv::ExecutionModel::TessellationControl;
        case "tesselation_evaluation"_h: return erwin::spv::ExecutionModel::TessellationEvaluation;
        case "tese"_h:                   return erwin::spv::ExecutionModel::TessellationEvaluation;
		case "geometry"_h: 	             return erwin::spv::ExecutionModel::Geometry;
		case "geom"_h: 	                 return erwin::spv::ExecutionModel::Geometry;
		case "fragment"_h: 	             return erwin::spv::ExecutionModel::Fragment;
		case "frag"_h: 	                 return erwin::spv::ExecutionModel::Fragment;
        case "compute"_h:                return erwin::spv::ExecutionModel::GLCompute;
        case "comp"_h:                   return erwin::spv::ExecutionModel::GLCompute;
	}
    return erwin::spv::ExecutionModel(0);
}

static std::string extension_from_type(erwin::spv::ExecutionModel type)
{
	switch(type)
	{
        case erwin::spv::ExecutionModel::Vertex:                 return ".vert";
        case erwin::spv::ExecutionModel::TessellationControl:    return ".tesc";
		case erwin::spv::ExecutionModel::TessellationEvaluation: return ".tese";
		case erwin::spv::ExecutionModel::Geometry:               return ".geom";
        case erwin::spv::ExecutionModel::Fragment:               return ".frag";
		case erwin::spv::ExecutionModel::GLCompute:              return ".comp";
	}
}

static std::string spv_file_from_type(erwin::spv::ExecutionModel type)
{
	switch(type)
	{
		case erwin::spv::ExecutionModel::Vertex:                 return "vert.spv";
        case erwin::spv::ExecutionModel::TessellationControl:    return "tesc.spv";
        case erwin::spv::ExecutionModel::TessellationEvaluation: return "tese.spv";
		case erwin::spv::ExecutionModel::Geometry:               return "geom.spv";
		case erwin::spv::ExecutionModel::Fragment:               return "frag.spv";
        case erwin::spv::ExecutionModel::GLCompute:              return "comp.spv";
	}
}

void test()
{

}

static void handle_includes(std::string& source, const fs::path& source_dir)
{
    // std::regex e_inc("\\s*#\\s*include\\s+(?:<[^>]*>|\"[^\"]*\")\\s*");
    std::regex e_inc("\\s*#\\s*include\\s+([<\"][^>\"]*[>\"])\\s*");
    source = erwin::su::rx::regex_replace(source, e_inc, [&](const std::smatch& m)
    {
        std::string result = m[1].str();
        std::string filename = result.substr(1, result.size()-2);
        DLOG("fudge", 1) << "including: " << WCC('p') << filename << WCC(0) << std::endl;
        return "\n" + filesystem::get_file_as_string(source_dir / filename) + "\n";
    });
}

static std::vector<std::pair<erwin::spv::ExecutionModel, std::string>> preprocess(const std::string& full_source, const fs::path& source_dir)
{
	std::vector<std::pair<erwin::spv::ExecutionModel, std::string>> sources;

	static const std::string type_token = "#type";
	size_t pos = full_source.find(type_token, 0);
	while(pos != std::string::npos)
	{
		size_t eol = full_source.find_first_of("\r\n", pos);
		W_ASSERT(eol != std::string::npos, "Syntax error!");

		size_t begin = pos + type_token.size() + 1;
		std::string type = full_source.substr(begin, eol - begin);
		hash_t htype = H_(type.c_str());
		erwin::spv::ExecutionModel shader_type = type_from_hstring(htype);

		size_t next_line_pos = full_source.find_first_not_of("\r\n", eol);
		pos = full_source.find(type_token, next_line_pos);
		sources.push_back(std::make_pair(shader_type,
				                         full_source.substr(next_line_pos, pos - (next_line_pos == std::string::npos ? full_source.size() - 1 : next_line_pos))));
	}

	for(auto&& [type, source]: sources)
	{
        handle_includes(source, source_dir);
	}

	return sources;
}

extern bool check_toolchain()
{
    if(system("glslangValidator -v > /dev/null 2>&1"))
    {
        DLOGW("fudge") << "glslangValidator not found, skipping shader compilation." << std::endl;
        return false;
    }
    if(system("spirv-link --version > /dev/null 2>&1"))
    {
        DLOGW("fudge") << "spirv-link not found, skipping shader compilation." << std::endl;
        return false;
    }
    if(system("spirv-opt --version > /dev/null 2>&1"))
    {
        DLOGW("fudge") << "spirv-opt not found, skipping shader compilation." << std::endl;
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

    std::ifstream ifs(source_path);
    // Read stream to buffer and preprocess full source
    auto sources = preprocess(std::string((std::istreambuf_iterator<char>(ifs)),
                                           std::istreambuf_iterator<char>()),
    					      source_dir);
    ifs.close();

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
    	DLOG("fudge",1) << "Successfully compiled shaders. Now, linking." << std::endl;
    	DLOGI << WCC('p') << out_path.filename() << WCC(0) << std::endl;

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

        DLOG("fudge",1) << "Optimizing." << std::endl;
        system(cmd.str().c_str());

        // Check output file
        auto stages = erwin::spv::parse_stages(out_path);
        for(auto&& stage: stages)
        {
            DLOG("fudge",1) << "Entry point for " << extension_from_type(stage.execution_model)
                            << ": " << WCC('g') << stage.entry_point << WCC(0) << std::endl;
        }
    }
}

} // namespace spv
} // namespace fudge