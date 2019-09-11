#include <set>
#include <cstring>
#include <regex>
#include <sstream>

#include "platform/ogl_shader.h"
#include "core/core.h"
#include "core/wtypes.h"
#include "core/string_utils.h"
#include "debug/logger.h"

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

static void shader_error_report(GLuint ShaderID, std::set<int>& errlines)
{
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

OGLShader::OGLShader(const std::string& name, std::istream& source_stream):
Shader(name)
{
    if(!source_stream)
    {
    	DLOGE("shader") << "Shader source stream is bad!" << std::endl;
    }

    // Read stream to buffer and parse full source
    auto sources = parse(std::string((std::istreambuf_iterator<char>(source_stream)),
                                      std::istreambuf_iterator<char>()));
    build(sources);
    setup_uniform_registry();
}

OGLShader::OGLShader(const std::string& name, const std::string& source_string):
Shader(name)
{
	auto sources = parse(source_string);
	build(sources);
	setup_uniform_registry();
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

bool OGLShader::build(const std::vector<std::pair<ShaderType, std::string>>& sources)
{
	DLOGN("shader") << "Building OpenGL Shader program: \"" << name_ << "\" " << std::endl;

	std::vector<GLuint> shader_ids;

	// * Compile each shader
	int n_previous_lines = 0;
	for(auto&& [type, source]: sources)
	{
		DLOGI << "Compiling " << to_string(type) << "." << std::endl;
		
		// Compile shader from source
    	GLuint shader_id = glCreateShader(to_gl_shader_type(type));
    	const char* char_src = source.c_str();
		glShaderSource(shader_id, 1, &char_src, nullptr);
		glCompileShader(shader_id);

		// * Check compilation status
	    GLint is_compiled = 0;
	    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &is_compiled);

	    if(is_compiled == GL_FALSE)
	    {
	        DLOGE("shader") << "Shader \"" << name_ << "\" will not compile" << std::endl;

	        std::set<int> errlines;
	        shader_error_report(shader_id, errlines);

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

	        // We don't need the shader anymore.
	        glDeleteShader(shader_id);
	    	return false;
	    }

	    // Save shader id for later linking
	    shader_ids.push_back(shader_id);
	    n_previous_lines += std::count(source.begin(), source.end(), '\n');
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

void OGLShader::setup_uniform_registry()
{
#ifdef __DEBUG__
	// * Program active report: show detected active attributes
    GLint active_attribs;
    glGetProgramiv(rd_handle_, GL_ACTIVE_ATTRIBUTES, &active_attribs);
    DLOGN("shader") << "Detected " << active_attribs << " active attributes:" << std::endl;

    for(GLint ii=0; ii<active_attribs; ++ii)
    {
        char name[33];
        GLsizei length;
        GLint   size;
        GLenum  type;

        glGetActiveAttrib(rd_handle_, ii, 32, &length, &size, &type, name);
        GLint loc = glGetAttribLocation(rd_handle_, name);

    	DLOGI << ogl_attribute_type_to_string(type) << " " << WCC('u') << name << WCC(0) << " loc= " << loc << std::endl;
    }
#endif

    // Get number of active uniforms
    GLint num_active_uniforms;
    glGetProgramiv(rd_handle_, GL_ACTIVE_UNIFORMS, &num_active_uniforms);

    if(num_active_uniforms)
    {
    	DLOGN("shader") << "Detected " << num_active_uniforms << " active uniforms:" << std::endl;
    }
    // For each uniform register name in map
    for(unsigned int ii=0; ii<num_active_uniforms; ++ii)
    {
        GLchar name[33];
        GLsizei length=0;

        glGetActiveUniformName(rd_handle_, ii, 32, &length, name);
        GLint location = glGetUniformLocation(rd_handle_, name);
        uniform_locations_.insert(std::make_pair(H_(name), location));

        DLOGI << WCC('u') << name << WCC(0) << " [" << location << "] " << std::endl;
    }
}

} // namespace erwin