#include "shader_packer.h"
#include "filesystem/filesystem.h"
#include "filesystem/spv_file.h"
#include "core/core.h"
#include "core/string_utils.h"
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

static erwin::spv::ShaderType type_from_hstring(hash_t htype)
{
	switch(htype)
	{
		case "vertex"_h: 	return erwin::spv::ShaderType::Vertex;
		case "vert"_h: 	    return erwin::spv::ShaderType::Vertex;
		case "geometry"_h: 	return erwin::spv::ShaderType::Geometry;
		case "geom"_h: 	    return erwin::spv::ShaderType::Geometry;
		case "fragment"_h: 	return erwin::spv::ShaderType::Fragment;
		case "frag"_h: 	    return erwin::spv::ShaderType::Fragment;
		default: 			return erwin::spv::ShaderType::None;
	}
}

static std::string extension_from_type(erwin::spv::ShaderType type)
{
	switch(type)
	{
		case erwin::spv::ShaderType::Vertex:   return ".vert";
		case erwin::spv::ShaderType::Geometry: return ".geom";
		case erwin::spv::ShaderType::Fragment: return ".frag";
		default:                               return "";
	}
}

static std::string spv_file_from_type(erwin::spv::ShaderType type)
{
	switch(type)
	{
		case erwin::spv::ShaderType::Vertex:   return "vert.spv";
		case erwin::spv::ShaderType::Geometry: return "geom.spv";
		case erwin::spv::ShaderType::Fragment: return "frag.spv";
		default:                               return "";
	}
}

void test()
{

}

static void handle_includes(std::string& source, const fs::path& source_dir)
{
    // std::regex e_inc("\\s*#\\s*include\\s+(?:<[^>]*>|\"[^\"]*\")\\s*");
    std::regex e_inc("\\s*#\\s*include\\s+([<\"][^>\"]*[>\"])\\s*");
    source = rx::regex_replace(source, e_inc, [&](const std::smatch& m)
    {
        std::string result = m[1].str();
        std::string filename = result.substr(1, result.size()-2);
        DLOG("fudge", 1) << "including: " << WCC('p') << filename << WCC(0) << std::endl;
        return "\n" + filesystem::get_file_as_string(source_dir / filename) + "\n";
    });
}

static std::vector<std::pair<erwin::spv::ShaderType, std::string>> preprocess(const std::string& full_source, const fs::path& source_dir)
{
	std::vector<std::pair<erwin::spv::ShaderType, std::string>> sources;

	static const std::string type_token = "#type";
	size_t pos = full_source.find(type_token, 0);
	while(pos != std::string::npos)
	{
		size_t eol = full_source.find_first_of("\r\n", pos);
		W_ASSERT(eol != std::string::npos, "Syntax error!");

		size_t begin = pos + type_token.size() + 1;
		std::string type = full_source.substr(begin, eol - begin);
		hash_t htype = H_(type.c_str());
		erwin::spv::ShaderType shader_type = type_from_hstring(htype);
		W_ASSERT(shader_type!=erwin::spv::ShaderType::None, "Invalid shader type specified!");

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

    return true;
}

// TODO: Stop using system calls, compile shaders programatically.
void make_shader_spirv(const fs::path& source_path, const fs::path& output_dir)
{
	fs::path source_dir = source_path.parent_path();
	fs::path tmp_dir = output_dir / "tmp";
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
    	fs::path out_path = output_dir / (source_path.stem().string() + ".spv");
    	DLOG("fudge",1) << "Successfully compiled shaders. Now, linking." << std::endl;
    	DLOGI << WCC('p') << out_path.filename() << WCC(0) << std::endl;

		std::stringstream cmd;
		cmd << "spirv-link " << spvs_str << "-o " << out_path.string();
		system(cmd.str().c_str());

    	/*erwin::spv::SPVDescriptor desc { out_path };
    	for(auto&& spv: spvs)
    	{
    		desc.shaders.emplace_back();
    		erwin::spv::SPVShaderDescriptor& shd_desc = desc.shaders.back();

    		std::ifstream ifs(spv, std::ios::binary|std::ios::ate);
    		std::ifstream::pos_type len = ifs.tellg();
			shd_desc.type = type_from_hstring(H_(spv.stem().string().c_str()));
			shd_desc.data.resize(len);
			ifs.seekg(0, std::ios::beg);
    		ifs.read(&shd_desc.data[0], len);
		    ifs.close();
    	}
    	erwin::spv::write_spv(desc);*/
    }
}

} // namespace spv
} // namespace fudge