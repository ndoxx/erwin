#include "shader_packer.h"
#include "filesystem/spv_file.h"
#include "core/core.h"
#include "debug/logger.h"

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>

using namespace erwin;

namespace fudge
{
namespace shd
{

static spv::ShaderType type_from_hstring(hash_t htype)
{
	switch(htype)
	{
		case "vertex"_h: 	return spv::ShaderType::Vertex;
		case "vert"_h: 	    return spv::ShaderType::Vertex;
		case "geometry"_h: 	return spv::ShaderType::Geometry;
		case "geom"_h: 	    return spv::ShaderType::Geometry;
		case "fragment"_h: 	return spv::ShaderType::Fragment;
		case "frag"_h: 	    return spv::ShaderType::Fragment;
		default: 			return spv::ShaderType::None;
	}
}

static std::string extension_from_type(spv::ShaderType type)
{
	switch(type)
	{
		case spv::ShaderType::Vertex:   return ".vert";
		case spv::ShaderType::Geometry: return ".geom";
		case spv::ShaderType::Fragment: return ".frag";
		default:                        return ".bad";
	}
}

static std::string spv_file_from_type(spv::ShaderType type)
{
	switch(type)
	{
		case spv::ShaderType::Vertex:   return "vert.spv";
		case spv::ShaderType::Geometry: return "geom.spv";
		case spv::ShaderType::Fragment: return "frag.spv";
		default:                        return "bad.spv";
	}
}

static void parse_includes(std::string& source, const fs::path& source_dir)
{
    // Find all #include directives, extract file location
    static const std::string include_token = "#include";
    size_t pos = source.find(include_token, 0);

    std::vector<std::string> files;
    std::vector<std::pair<uint32_t,uint32_t>> inc_pos_len;
    while(pos != std::string::npos)
    {
        size_t eol = source.find_first_of("\r\n", pos);
        size_t begin = pos + include_token.size() + 1;
        size_t next_line_pos = source.find_first_not_of("\r\n", eol);

        files.push_back(source.substr(begin, eol - begin));
        inc_pos_len.push_back(std::make_pair(pos,next_line_pos-pos-1));

        pos = source.find(include_token, next_line_pos);
    }

    // Remove include directives from source and replace by actual included source
    int char_offset = 0;
    for(int ii=0; ii<files.size(); ++ii)
    {
        DLOG("fudge", 1) << "including: " << WCC('p') << files[ii] << WCC(0) << std::endl;
        uint32_t pos = inc_pos_len[ii].first + char_offset;
        uint32_t len = inc_pos_len[ii].second;
        source.erase(pos, len);

		std::ifstream ifs(source_dir / files[ii]);
		std::string inc_source((std::istreambuf_iterator<char>(ifs)),
                                std::istreambuf_iterator<char>());
        source.insert(pos, inc_source);

        // Kepp track of the number of characters added and removed
        char_offset += inc_source.size()-len;
    }
}

static std::vector<std::pair<spv::ShaderType, std::string>> parse(const std::string& full_source, const fs::path& source_dir)
{
	std::vector<std::pair<spv::ShaderType, std::string>> sources;

	static const std::string type_token = "#type";
	size_t pos = full_source.find(type_token, 0);
	while(pos != std::string::npos)
	{
		size_t eol = full_source.find_first_of("\r\n", pos);
		W_ASSERT(eol != std::string::npos, "Syntax error!");

		size_t begin = pos + type_token.size() + 1;
		std::string type = full_source.substr(begin, eol - begin);
		hash_t htype = H_(type.c_str());
		spv::ShaderType shader_type = type_from_hstring(htype);
		W_ASSERT(shader_type!=spv::ShaderType::None, "Invalid shader type specified!");

		size_t next_line_pos = full_source.find_first_not_of("\r\n", eol);
		pos = full_source.find(type_token, next_line_pos);
		sources.push_back(std::make_pair(shader_type,
				                         full_source.substr(next_line_pos, pos - (next_line_pos == std::string::npos ? full_source.size() - 1 : next_line_pos))));
	}

	for(auto&& [type, source]: sources)
	{
        parse_includes(source, source_dir);
	}

	return sources;
}

void make_shader_spirv(const fs::path& source_path, const fs::path& output_dir)
{
	fs::path source_dir = source_path.parent_path();
	fs::path tmp_dir = output_dir / "tmp";
	std::string shader_name = source_path.stem().string();

    std::ifstream ifs(source_path);
    // Read stream to buffer and parse full source
    auto sources = parse(std::string((std::istreambuf_iterator<char>(ifs)),
                                      std::istreambuf_iterator<char>()),
    					 source_dir);
    ifs.close();

    // Export temporary source files for each shader
    std::vector<fs::path> spvs;
    bool success = true;
    for(auto&& [type, source]: sources)
    {
    	fs::path shader_file = tmp_dir / (shader_name + extension_from_type(type));
    	fs::path out_spv     = tmp_dir / spv_file_from_type(type);
    	spvs.push_back(out_spv);

    	std::ofstream ofs(shader_file);
		ofs << source;
		ofs.close();

    	// Compile shader file
		std::stringstream cmd;
		cmd << "glslangValidator -G -e main -o " << out_spv.string() << " " << shader_file.string();
		int error = system(cmd.str().c_str());
		success &= (error==0);

    	// Erase temporary source file
    	fs::remove(shader_file);
    }

    if(success)
    {
    	fs::path out_path = output_dir / (source_path.stem().string() + ".spv");
    	DLOG("fudge",1) << "Successfully compiled shader program. Now, packing." << std::endl;
    	DLOGI << WCC('p') << out_path << WCC(0) << std::endl;
    	spv::SPVDescriptor desc { out_path };
    	for(auto&& spv: spvs)
    	{
    		desc.shaders.emplace_back();
    		spv::SPVShaderDescriptor& shd_desc = desc.shaders.back();

    		std::ifstream ifs(spv, std::ios::binary|std::ios::ate);
    		std::ifstream::pos_type len = ifs.tellg();
			shd_desc.type = type_from_hstring(H_(spv.stem().string().c_str()));
			shd_desc.data.resize(len);
			ifs.seekg(0, std::ios::beg);
    		ifs.read(&shd_desc.data[0], len);
		    ifs.close();
    	}
    	spv::write_spv(desc);
    }

	// Erase temporary spv files
	for(auto&& spv: spvs)
		fs::remove(spv);
}

} // namespace shd
} // namespace fudge