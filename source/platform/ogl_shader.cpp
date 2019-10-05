#include <set>
#include <cstring>
#include <regex>
#include <sstream>

#include "platform/ogl_shader.h"
#include "platform/ogl_buffer.h"
#include "core/core.h"
#include "core/wtypes.h"
#include "core/string_utils.h"
#include "core/intern_string.h"
#include "filesystem/filesystem.h"
#include "debug/logger.h"
#include "render/texture.h"

#include "glad/glad.h"

namespace erwin
{

static ShaderType type_from_hstring(hash_t htype)
{
	switch(htype)
	{
		case "vertex"_h: 	return ShaderType::Vertex;
		case "geometry"_h: 	return ShaderType::Geometry;
		case "fragment"_h: 	return ShaderType::Fragment;
		default: 			return ShaderType::None;
	}
}

static GLenum to_gl_shader_type(ShaderType type)
{
	switch(type)
	{
		case ShaderType::Vertex:   return GL_VERTEX_SHADER;
		case ShaderType::Geometry: return GL_GEOMETRY_SHADER;
		case ShaderType::Fragment: return GL_FRAGMENT_SHADER;
		case ShaderType::None:     return GL_NONE;
	}
}

static std::string to_string(ShaderType type)
{
	switch(type)
	{
		case ShaderType::Vertex:   return "Vertex shader";
		case ShaderType::Geometry: return "Geometry shader";
		case ShaderType::Fragment: return "Fragment shader";
		case ShaderType::None:     return "[UNKNOWN TYPE] shader";
	}
}

static std::string ogl_attribute_type_to_string(GLenum type)
{
	switch(type)
	{
		case GL_FLOAT: 				return "GL_FLOAT";
		case GL_FLOAT_VEC2: 		return "GL_FLOAT_VEC2";
		case GL_FLOAT_VEC3: 		return "GL_FLOAT_VEC3";
		case GL_FLOAT_VEC4: 		return "GL_FLOAT_VEC4";
		case GL_FLOAT_MAT2: 		return "GL_FLOAT_MAT2";
		case GL_FLOAT_MAT3: 		return "GL_FLOAT_MAT3";
		case GL_FLOAT_MAT4: 		return "GL_FLOAT_MAT4";
		case GL_FLOAT_MAT2x3: 		return "GL_FLOAT_MAT2x3";
		case GL_FLOAT_MAT2x4: 		return "GL_FLOAT_MAT2x4";
		case GL_FLOAT_MAT3x2: 		return "GL_FLOAT_MAT3x2";
		case GL_FLOAT_MAT3x4: 		return "GL_FLOAT_MAT3x4";
		case GL_FLOAT_MAT4x2: 		return "GL_FLOAT_MAT4x2";
		case GL_FLOAT_MAT4x3: 		return "GL_FLOAT_MAT4x3";
		case GL_INT: 				return "GL_INT";
		case GL_INT_VEC2: 			return "GL_INT_VEC2";
		case GL_INT_VEC3: 			return "GL_INT_VEC3";
		case GL_INT_VEC4: 			return "GL_INT_VEC4";
		case GL_UNSIGNED_INT: 		return "GL_UNSIGNED_INT";
		case GL_UNSIGNED_INT_VEC2: 	return "GL_UNSIGNED_INT_VEC2";
		case GL_UNSIGNED_INT_VEC3: 	return "GL_UNSIGNED_INT_VEC3";
		case GL_UNSIGNED_INT_VEC4: 	return "GL_UNSIGNED_INT_VEC4";
		case GL_DOUBLE: 			return "GL_DOUBLE";
		case GL_DOUBLE_VEC2: 		return "GL_DOUBLE_VEC2";
		case GL_DOUBLE_VEC3: 		return "GL_DOUBLE_VEC3";
		case GL_DOUBLE_VEC4: 		return "GL_DOUBLE_VEC4";
		case GL_DOUBLE_MAT2: 		return "GL_DOUBLE_MAT2";
		case GL_DOUBLE_MAT3: 		return "GL_DOUBLE_MAT3";
		case GL_DOUBLE_MAT4: 		return "GL_DOUBLE_MAT4";
		case GL_DOUBLE_MAT2x3: 		return "GL_DOUBLE_MAT2x3";
		case GL_DOUBLE_MAT2x4: 		return "GL_DOUBLE_MAT2x4";
		case GL_DOUBLE_MAT3x2: 		return "GL_DOUBLE_MAT3x2";
		case GL_DOUBLE_MAT3x4: 		return "GL_DOUBLE_MAT3x4";
		case GL_DOUBLE_MAT4x2: 		return "GL_DOUBLE_MAT4x2";
		case GL_DOUBLE_MAT4x3: 		return "GL_DOUBLE_MAT4x3";
		default:					return "[[UNKNOWN TYPE]]";
	}
}

static std::string ogl_uniform_type_to_string(GLenum type)
{
    switch(type)
    {
        case GL_FLOAT:          return "GL_FLOAT";
        case GL_FLOAT_VEC2:     return "GL_FLOAT_VEC2";
        case GL_FLOAT_VEC3:     return "GL_FLOAT_VEC3";
        case GL_FLOAT_VEC4:     return "GL_FLOAT_VEC4";
        case GL_INT:            return "GL_INT";
        case GL_INT_VEC2:       return "GL_INT_VEC2";
        case GL_INT_VEC3:       return "GL_INT_VEC3";
        case GL_INT_VEC4:       return "GL_INT_VEC4";
        case GL_BOOL:           return "GL_BOOL";
        case GL_BOOL_VEC2:      return "GL_BOOL_VEC2";
        case GL_BOOL_VEC3:      return "GL_BOOL_VEC3";
        case GL_BOOL_VEC4:      return "GL_BOOL_VEC4";
        case GL_FLOAT_MAT2:     return "GL_FLOAT_MAT2";
        case GL_FLOAT_MAT3:     return "GL_FLOAT_MAT3";
        case GL_FLOAT_MAT4:     return "GL_FLOAT_MAT4";
        case GL_SAMPLER_2D:     return "GL_SAMPLER_2D";
        case GL_SAMPLER_CUBE:   return "GL_SAMPLER_CUBE";
        default:                return "[[UNKNOWN TYPE]]";
    }
}

static std::string ogl_interface_to_string(GLenum iface)
{
    switch(iface)
    {
        case GL_PROGRAM_INPUT:        return "Attribute";
        case GL_UNIFORM:              return "Uniform";
        case GL_UNIFORM_BLOCK:        return "Uniform Block";
        case GL_SHADER_STORAGE_BLOCK: return "Storage Block";
        case GL_BUFFER_VARIABLE:      return "Buffer Variable";
        default:                      return "[[UNKNOWN TYPE]]";
    }
}

static void shader_error_report(GLuint ShaderID, int n_previous_lines, const std::string& source)
{
    std::set<int> errlines;

    char* log = nullptr;
    GLsizei logsize = 0;

    glGetShaderiv(ShaderID, GL_INFO_LOG_LENGTH, &logsize);

    log = (char*) malloc(logsize + 1);
    W_ASSERT(log, "Cannot allocate memory for Shader Error Report!");

    memset(log, '\0', logsize + 1);
    glGetShaderInfoLog(ShaderID, logsize, &logsize, log);
    DLOGR("shader") << log << std::endl;

    // * Find error line numbers
    std::string logstr(log);
    static std::regex rx_errline("\\d+\\((\\d+)\\)\\s:\\s");
    std::regex_iterator<std::string::iterator> it(logstr.begin(), logstr.end(), rx_errline);
    std::regex_iterator<std::string::iterator> end;

    while(it != end)
    {
        errlines.insert(std::stoi((*it)[1]));
        ++it;
    }

    free(log);

    // * Show problematic lines
    std::istringstream source_iss(source);

    std::string line;
    int nline = 1;
    while(std::getline(source_iss, line))
    {
        if(errlines.find(nline++)!=errlines.end())
        {
            int actual_line = nline + n_previous_lines;
            trim(line);
            DLOGR("shader") << "\033[1;38;2;255;200;10m> \033[1;38;2;255;90;90m"
                            << actual_line << "\033[1;38;2;255;200;10m : " << line << std::endl;
        }
    }
}

static void program_error_report(GLuint ProgramID)
{
    char* log = nullptr;
    GLsizei logsize = 0;

    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &logsize);

    log = (char*) malloc(logsize + 1);
    W_ASSERT(log, "Cannot allocate memory for Program Error Report!");

    memset(log, '\0', logsize + 1);
    glGetProgramInfoLog(ProgramID, logsize, &logsize, log);
    DLOGR("shader") << log << std::endl;

    free(log);
}

OGLShader::OGLShader(const std::string& name, const fs::path& filepath):
Shader(name),
filepath_(filepath)
{
    std::ifstream ifs(filepath);

    // Read stream to buffer and parse full source
    auto sources = parse(std::string((std::istreambuf_iterator<char>(ifs)),
                                      std::istreambuf_iterator<char>()));
    build(sources);
    register_resources();
}

OGLShader::OGLShader(const std::string& name, const std::string& source_string):
Shader(name),
filepath_("")
{
	auto sources = parse(source_string);
	build(sources);
	register_resources();
}

OGLShader::~OGLShader()
{

}

void OGLShader::bind() const
{
    glUseProgram(rd_handle_);
}

void OGLShader::unbind() const
{
    glUseProgram(0);
}

uint32_t OGLShader::get_texture_slot(hash_t sampler) const
{
    W_ASSERT(texture_slots_.find(sampler)!=texture_slots_.end(), "Unknown sampler name!");
    return texture_slots_.at(sampler);
}

void OGLShader::attach_texture(hash_t sampler, const Texture2D& texture) const
{
    uint32_t slot = get_texture_slot(sampler);
    texture.bind(slot);
    send_uniform<int>(sampler, slot);
}

void OGLShader::attach_shader_storage(const ShaderStorageBuffer& buffer) const
{
    hash_t hname = H_(buffer.get_name().c_str());
    GLint binding_point = block_bindings_.at(hname);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_point, static_cast<const OGLShaderStorageBuffer&>(buffer).get_handle());
    // GLuint block_index = glGetProgramResourceIndex(rd_handle_, GL_SHADER_STORAGE_BLOCK, buffer.get_name().c_str());
    // glShaderStorageBlockBinding(rd_handle_, block_index, binding_point);
}

void OGLShader::attach_uniform_buffer(const UniformBuffer& buffer) const
{
    hash_t hname = H_(buffer.get_name().c_str());
    GLint binding_point = block_bindings_.at(hname);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding_point, static_cast<const OGLUniformBuffer&>(buffer).get_handle());
    // GLuint block_index = glGetProgramResourceIndex(rd_handle_, GL_UNIFORM_BLOCK, buffer.get_name().c_str());
    // glUniformBlockBinding(rd_handle_, block_index, binding_point);
}

std::vector<std::pair<ShaderType, std::string>> OGLShader::parse(const std::string& full_source)
{
	std::vector<std::pair<ShaderType, std::string>> sources;

	static const std::string type_token = "#type";
	size_t pos = full_source.find(type_token, 0);
	while(pos != std::string::npos)
	{
		size_t eol = full_source.find_first_of("\r\n", pos);
		W_ASSERT(eol != std::string::npos, "Syntax error!");

		size_t begin = pos + type_token.size() + 1;
		std::string type = full_source.substr(begin, eol - begin);
		hash_t htype = H_(type.c_str());
		ShaderType shader_type = type_from_hstring(htype);
		W_ASSERT(shader_type!=ShaderType::None, "Invalid shader type specified!");

		size_t next_line_pos = full_source.find_first_not_of("\r\n", eol);
		pos = full_source.find(type_token, next_line_pos);
		sources.push_back(std::make_pair(shader_type,
				                         full_source.substr(next_line_pos, pos - (next_line_pos == std::string::npos ? full_source.size() - 1 : next_line_pos))));
	}

	return sources;
}

std::string OGLShader::parse_includes(const std::string& source)
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
    std::string ret(source);
    int char_offset = 0;
    for(int ii=0; ii<files.size(); ++ii)
    {
        DLOG("shader", 1) << "including: " << WCC('p') << files[ii] << std::endl;
        uint32_t pos = inc_pos_len[ii].first + char_offset;
        uint32_t len = inc_pos_len[ii].second;
        ret.erase(pos, len);

        if(!filepath_.empty())
        {
            std::string inc_source = filesystem::get_asset_string(filepath_.parent_path() / files[ii]);
            ret.insert(pos, inc_source);

            // Kepp track of the number of characters added and removed
            char_offset += inc_source.size()-len;
        }
        else
        {
            DLOGE("shader") << "Cannot include from string source shader!" << std::endl;
            char_offset -= len;
        }
    }

    return ret;
}

bool OGLShader::build(const std::vector<std::pair<ShaderType, std::string>>& sources)
{
	DLOGN("shader") << "Building OpenGL Shader program: \"" << name_ << "\" " << std::endl;

	std::vector<GLuint> shader_ids;

	// * Compile each shader
	int n_previous_lines = 0;
    int current = 0;
	for(auto&& [type, source]: sources)
	{
		DLOGI << "Compiling " << to_string(type) << "." << std::endl;
		
        // Check for includes
        std::string source_includes = parse_includes(source);

        // Keep track of lines for error report
        int n_orig_lines = std::count(source.begin(), source.end(), '\n');
        int n_total_lines = std::count(source_includes.begin(), source_includes.end(), '\n');
        int n_lines_included = n_total_lines-n_orig_lines;

		// Compile shader from source
    	GLuint shader_id = glCreateShader(to_gl_shader_type(type));
    	const char* char_src = source_includes.c_str();
		glShaderSource(shader_id, 1, &char_src, nullptr);
		glCompileShader(shader_id);

		// Check compilation status
	    GLint is_compiled = 0;
	    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &is_compiled);

	    if(is_compiled == GL_FALSE)
	    {
	        DLOGE("shader") << "Shader \"" << name_ << "\" will not compile" << std::endl;
            shader_error_report(shader_id, n_previous_lines-n_lines_included+current, source_includes);

	        // We don't need the shader anymore.
	        glDeleteShader(shader_id);
	    	return false;
	    }

	    // Save shader id for later linking
	    shader_ids.push_back(shader_id);

        n_previous_lines += n_total_lines;
        ++current;
	}

	// * Link program
	DLOGI << "Linking program." << std::endl;
	rd_handle_ = glCreateProgram();
	for(auto&& shader_id: shader_ids)
    	glAttachShader(rd_handle_, shader_id);

    glLinkProgram(rd_handle_);

    // * Check linking status
    GLint is_linked = 0;
    glGetProgramiv(rd_handle_, GL_LINK_STATUS, (int*) &is_linked);

    if(is_linked == GL_FALSE)
    {
        DLOGE("render") << "Unable to link shaders." << std::endl;
        program_error_report(rd_handle_);

        //We don't need the program anymore.
        glDeleteProgram(rd_handle_);
        //Don't leak shaders either.
		for(auto&& shader_id: shader_ids)
        	glDeleteShader(shader_id);

        return false;
    }

    // * Detach shaders
	for(auto&& shader_id: shader_ids)
    	glDetachShader(rd_handle_, shader_id);

	DLOGI << "Program \"" << name_ << "\" is ready." << std::endl;

	return true;
}

void OGLShader::register_resources()
{
    static std::vector<GLenum> interfaces { GL_PROGRAM_INPUT, GL_UNIFORM, GL_BUFFER_VARIABLE, GL_UNIFORM_BLOCK, GL_SHADER_STORAGE_BLOCK };
    static std::map<GLenum, std::vector<GLenum>> properties_map
    {
        {GL_PROGRAM_INPUT,        {GL_NAME_LENGTH, GL_TYPE, GL_ARRAY_SIZE}},
        {GL_UNIFORM,              {GL_NAME_LENGTH, GL_TYPE, GL_ARRAY_SIZE}},
        {GL_BUFFER_VARIABLE,      {GL_NAME_LENGTH, GL_TYPE, GL_BLOCK_INDEX, GL_ARRAY_SIZE}},
        {GL_UNIFORM_BLOCK,        {GL_NAME_LENGTH, GL_BUFFER_BINDING}},
        {GL_SHADER_STORAGE_BLOCK, {GL_NAME_LENGTH, GL_BUFFER_BINDING}},
    };

    for(int ii=0; ii<interfaces.size(); ++ii)
    {
        // Get active objects count
        GLint num_active;
        GLenum iface = interfaces[ii];
        glGetProgramInterfaceiv(rd_handle_, iface, GL_ACTIVE_RESOURCES, &num_active);

        if(num_active)
        {
            DLOG("shader",1) << "[" << ogl_interface_to_string(iface) << "] active: " << num_active << std::endl;
        }

        // Get properties
        const auto& properties = properties_map.at(iface);
        std::vector<GLint> prop_values(properties.size());
        std::vector<GLchar> name_data(256);
        
        for(int jj=0; jj<num_active; ++jj)
        {
            glGetProgramResourceiv(rd_handle_, iface, jj, properties.size(),
                                   &properties[0], prop_values.size(), nullptr, &prop_values[0]);

            name_data.resize(prop_values[0]); //The length of the name.
            glGetProgramResourceName(rd_handle_, iface, jj, name_data.size(), nullptr, &name_data[0]);
            std::string resource_name((char*)&name_data[0], name_data.size() - 1);
            hash_t hname = H_(resource_name.c_str());

            // Register resource
            if(iface == GL_PROGRAM_INPUT)
            {
                GLint loc = glGetAttribLocation(rd_handle_, resource_name.c_str());
                DLOGI << "[" << loc << "] " << ogl_attribute_type_to_string(prop_values[1]) << " " << WCC('u') << resource_name << WCC(0) << std::endl;
            }
            else if(iface == GL_UNIFORM)
            {
                GLint loc = glGetUniformLocation(rd_handle_, resource_name.c_str());
                uniform_locations_.insert(std::make_pair(hname, loc));

                if(prop_values[1] == GL_SAMPLER_2D || prop_values[1] == GL_SAMPLER_CUBE)
                    texture_slots_.insert(std::make_pair(hname, current_slot_++));

                DLOGI << "[" << loc << "] " << ogl_uniform_type_to_string(prop_values[1]) << " " << WCC('u') << resource_name << WCC(0) << std::endl;
            }
            else if(iface == GL_BUFFER_VARIABLE)
            {
                DLOGI << "[" << prop_values[2] << "] " << ogl_uniform_type_to_string(prop_values[1]) << " " << WCC('u') << resource_name << WCC(0) << std::endl;
            }
            else if(iface == GL_UNIFORM_BLOCK)
            {
                block_bindings_.insert(std::make_pair(hname, prop_values[1]));
                DLOGI << "[" << prop_values[1] << "] " << WCC('u') << resource_name << std::endl;
            }
            else if(iface == GL_SHADER_STORAGE_BLOCK)
            {
                block_bindings_.insert(std::make_pair(hname, prop_values[1]));
                DLOGI << "[" << prop_values[1] << "] " << WCC('u') << resource_name << std::endl;
            }
        }
    }
}

static inline void warn_unknown_uniform(const std::string& shader_name, hash_t u_name)
{
#ifdef __DEBUG__
	static std::set<hash_t> marked; // So that we don't warn twice for the same uniform
    hash_t id = H_(shader_name.c_str()) ^ u_name;

	if(marked.find(id) == marked.end())
    {
		DLOGW("shader") << "Unknown uniform submitted to \"" << shader_name << "\": \"" << istr::resolve(u_name) << "\"" << std::endl;
		marked.insert(id);
	}
#endif
}

template <>
bool OGLShader::send_uniform<bool>(hash_t name, const bool& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
        warn_unknown_uniform(name_, name);
        return false;
    }

    glUniform1i(it->second, value);
    return true;
}


template <>
bool OGLShader::send_uniform<float>(hash_t name, const float& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
        warn_unknown_uniform(name_, name);
        return false;
    }

    glUniform1f(it->second, value);
    return true;
}

template <>
bool OGLShader::send_uniform<int>(hash_t name, const int& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
        warn_unknown_uniform(name_, name);
        return false;
    }

    glUniform1i(it->second, value);
    return true;
}

template <>
bool OGLShader::send_uniform<glm::vec2>(hash_t name, const glm::vec2& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
        warn_unknown_uniform(name_, name);
        return false;
    }

    glUniform2fv(it->second, 1, (const GLfloat*)&value);
    return true;
}

template <>
bool OGLShader::send_uniform<glm::vec3>(hash_t name, const glm::vec3& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
        warn_unknown_uniform(name_, name);
        return false;
    }

    glUniform3fv(it->second, 1, (const GLfloat*)&value);
    return true;
}

template <>
bool OGLShader::send_uniform<glm::vec4>(hash_t name, const glm::vec4& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
        warn_unknown_uniform(name_, name);
        return false;
    }

    glUniform4fv(it->second, 1, (const GLfloat*)&value);
    return true;
}

template <>
bool OGLShader::send_uniform<glm::mat2>(hash_t name, const glm::mat2& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
        warn_unknown_uniform(name_, name);
        return false;
    }

    glUniformMatrix2fv(it->second, 1, GL_FALSE, &value[0][0]);
    return true;
}

template <>
bool OGLShader::send_uniform<glm::mat3>(hash_t name, const glm::mat3& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
        warn_unknown_uniform(name_, name);
        return false;
    }

    glUniformMatrix3fv(it->second, 1, GL_FALSE, &value[0][0]);
    return true;
}

template <>
bool OGLShader::send_uniform<glm::mat4>(hash_t name, const glm::mat4& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
        warn_unknown_uniform(name_, name);
        return false;
    }

    glUniformMatrix4fv(it->second, 1, GL_FALSE, &value[0][0]);
    return true;
}

} // namespace erwin