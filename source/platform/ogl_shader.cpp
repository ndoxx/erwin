#include <set>
#include <cstring>
#include <regex>
#include <sstream>

#include "platform/ogl_shader.h"
#include "platform/ogl_buffer.h"
#include "core/core.h"
#include "utils/string.h"
#include "core/intern_string.h"
#include "filesystem/filesystem.h"
#include "filesystem/spv_file.h"
#include "debug/logger.h"

#include "glad/glad.h"

namespace erwin
{

static GLenum to_gl_shader_type(slang::ExecutionModel model)
{
    switch(model)
    {
        case slang::ExecutionModel::Vertex:                 return GL_VERTEX_SHADER;
        case slang::ExecutionModel::TessellationControl:    return GL_TESS_CONTROL_SHADER;
        case slang::ExecutionModel::TessellationEvaluation: return GL_TESS_EVALUATION_SHADER;
        case slang::ExecutionModel::Geometry:               return GL_GEOMETRY_SHADER;
        case slang::ExecutionModel::Fragment:               return GL_FRAGMENT_SHADER;
        case slang::ExecutionModel::Compute:                return GL_COMPUTE_SHADER;
    }
}

[[maybe_unused]] static std::string ogl_attribute_type_to_string(GLenum type)
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

static ShaderDataType ogl_attribute_type_to_shader_data_type(GLenum type)
{
    // Float = 0, Vec2, Vec3, Vec4, Mat3, Mat4, Int, IVec2, IVec3, IVec4, Bool
    switch(type)
    {
        case GL_FLOAT:              return ShaderDataType::Float;
        case GL_FLOAT_VEC2:         return ShaderDataType::Vec2;
        case GL_FLOAT_VEC3:         return ShaderDataType::Vec3;
        case GL_FLOAT_VEC4:         return ShaderDataType::Vec4;
        case GL_FLOAT_MAT3:         return ShaderDataType::Mat3;
        case GL_FLOAT_MAT4:         return ShaderDataType::Mat4;
        case GL_INT:                return ShaderDataType::Int;
        case GL_INT_VEC2:           return ShaderDataType::IVec2;
        case GL_INT_VEC3:           return ShaderDataType::IVec3;
        case GL_INT_VEC4:           return ShaderDataType::IVec4;
        default:                    return ShaderDataType::Float;
    }
}

[[maybe_unused]] static std::string ogl_uniform_type_to_string(GLenum type)
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

[[maybe_unused]] static std::string ogl_interface_to_string(GLenum iface)
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

static inline hash_t slot_to_sampler2D_name(uint32_t slot)
{
    switch(slot)
    {
        case 0: return "SAMPLER_2D_0"_h;
        case 1: return "SAMPLER_2D_1"_h;
        case 2: return "SAMPLER_2D_2"_h;
        case 3: return "SAMPLER_2D_3"_h;
        case 4: return "SAMPLER_2D_4"_h;
        case 5: return "SAMPLER_2D_5"_h;
        case 6: return "SAMPLER_2D_6"_h;
        case 7: return "SAMPLER_2D_7"_h;
        default: return 0;
    }
}

static inline hash_t slot_to_sampler_cube_name(uint32_t slot)
{
    switch(slot)
    {
        case 0: return "SAMPLER_CUBE_0"_h;
        case 1: return "SAMPLER_CUBE_1"_h;
        case 2: return "SAMPLER_CUBE_2"_h;
        case 3: return "SAMPLER_CUBE_3"_h;
        case 4: return "SAMPLER_CUBE_4"_h;
        case 5: return "SAMPLER_CUBE_5"_h;
        case 6: return "SAMPLER_CUBE_6"_h;
        case 7: return "SAMPLER_CUBE_7"_h;
        default: return 0;
    }
}

static std::string get_shader_error_report(GLuint ShaderID)
{
    char* log = nullptr;
    GLsizei logsize = 0;

    glGetShaderiv(ShaderID, GL_INFO_LOG_LENGTH, &logsize);

    log = static_cast<char*>(malloc(size_t(logsize) + 1));
    W_ASSERT(log, "Cannot allocate memory for Shader Error Report!");

    memset(log, '\0', size_t(logsize) + 1);
    glGetShaderInfoLog(ShaderID, logsize, &logsize, log);

    std::string ret(log);
    free(log);
    return ret;
}

static const std::regex rx_errline("\\d+\\((\\d+)\\)\\s:\\s");
static void shader_error_report(GLuint ShaderID, int line_offset, const std::string& source)
{
    std::set<int> errlines;

    std::string logstr = get_shader_error_report(ShaderID);

    DLOGR("shader") << logstr << std::endl;

    // * Find error line numbers
    std::regex_iterator<std::string::iterator> it(logstr.begin(), logstr.end(), rx_errline);
    std::regex_iterator<std::string::iterator> end;

    while(it != end)
    {
        errlines.insert(std::stoi((*it)[1]));
        ++it;
    }

    // * Show problematic lines
    std::istringstream source_iss(source);

    std::string line;
    int nline = 1;
    while(std::getline(source_iss, line))
    {
        if(errlines.find(nline++)!=errlines.end())
        {
            int actual_line = nline + line_offset;
            su::trim(line);
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

    log = static_cast<char*>(malloc(size_t(logsize) + 1));
    W_ASSERT(log, "Cannot allocate memory for Program Error Report!");

    memset(log, '\0', size_t(logsize) + 1);
    glGetProgramInfoLog(ProgramID, logsize, &logsize, log);
    DLOGR("shader") << log << std::endl;

    free(log);
}

bool OGLShader::init(const std::string& name, const fs::path& filepath)
{
    std::string extension(filepath.extension().string());
    if(!extension.compare(".glsl"))
        return init_glsl(name, filepath);
    else if(!extension.compare(".spv"))
        return init_spirv(name, filepath);
    return false;
}

// Initialize shader from packed GLSL source
bool OGLShader::init_glsl(const std::string& name, const fs::path& glsl_file)
{
    W_PROFILE_FUNCTION()

    name_ = name;
    filepath_ = glsl_file;

    std::vector<std::pair<slang::ExecutionModel, std::string>> sources;
    slang::pre_process_GLSL(glsl_file, sources);
    bool success = build(sources);
    if(success)
        introspect();
    else
    {
        // Load default shader
        DLOGW("shader") << "Loading default red shader as a fallback." << std::endl;
        sources.clear();
        slang::pre_process_GLSL(filesystem::get_system_asset_dir() / "shaders/red_shader.glsl", sources);
        introspect();
    }
    return success;
}
// Initialize shader from SPIR-V file
bool OGLShader::init_spirv(const std::string& name, const fs::path& spv_file)
{
    W_PROFILE_FUNCTION()

    name_ = name;
    filepath_ = spv_file;

    bool success = build_spirv(spv_file);
    if(success)
        introspect();
    return success;
}

void OGLShader::bind() const
{
    glUseProgram(rd_handle_);

    // Bind resources
    for(auto&& [key, buffer]: bound_buffers_)
        glBindBufferBase(buffer.target, buffer.binding_point, buffer.render_handle);
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

uint32_t OGLShader::get_texture_count() const
{
    return uint32_t(texture_slots_.size());
}

void OGLShader::attach_texture_2D(const OGLTexture2D& texture, uint32_t slot) const
{
    texture.bind(slot);
    send_uniform<int>(slot_to_sampler2D_name(slot), int(slot));
}

void OGLShader::attach_cubemap(const OGLCubemap& cubemap, uint32_t slot) const
{
    cubemap.bind(slot);
    send_uniform<int>(slot_to_sampler_cube_name(slot), int(slot));
}

void OGLShader::attach_shader_storage(const OGLShaderStorageBuffer& buffer)
{
    hash_t hname = H_(buffer.get_name().c_str());
    auto it = block_bindings_.find(hname);
    if(it == block_bindings_.end())
    {
        DLOGW("shader") << "Unknown binding name: " << buffer.get_name() << std::endl;
        return;
    }
    GLint binding_point = GLint(it->second);
    uint32_t render_handle = buffer.get_handle();
    bound_buffers_.insert(std::make_pair(buffer.get_unique_id(), ResourceBinding{ binding_point, render_handle, GL_SHADER_STORAGE_BUFFER }));
}

void OGLShader::attach_uniform_buffer(const OGLUniformBuffer& buffer)
{
    hash_t hname = H_(buffer.get_name().c_str());
    auto it = block_bindings_.find(hname);
    if(it == block_bindings_.end())
    {
        DLOGW("shader") << "Unknown binding name: " << buffer.get_name() << std::endl;
        return;
    }
    GLint binding_point = GLint(it->second);
    uint32_t render_handle = buffer.get_handle();
    bound_buffers_.insert(std::make_pair(buffer.get_unique_id(), ResourceBinding{ binding_point, render_handle, GL_UNIFORM_BUFFER }));
}

void OGLShader::bind_shader_storage(const OGLShaderStorageBuffer& buffer, uint32_t size, uint32_t base_offset) const
{
    hash_t hname = H_(buffer.get_name().c_str());
    GLint binding_point = GLint(block_bindings_.at(hname));
    if(size)
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding_point, buffer.get_handle(), base_offset, size);
    else
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_point, buffer.get_handle());
}

void OGLShader::bind_uniform_buffer(const OGLUniformBuffer& buffer, uint32_t size, uint32_t offset) const
{
    hash_t hname = H_(buffer.get_name().c_str());
    GLint binding_point = GLint(block_bindings_.at(hname));
    if(size)
        glBindBufferRange(GL_UNIFORM_BUFFER, binding_point, buffer.get_handle(), offset, size);
    else
        glBindBufferBase(GL_UNIFORM_BUFFER, binding_point, buffer.get_handle());
}

const BufferLayout& OGLShader::get_attribute_layout() const
{
    return attribute_layout_;
}

bool OGLShader::build(const std::vector<std::pair<slang::ExecutionModel, std::string>>& sources)
{
    W_PROFILE_FUNCTION()

	DLOGN("shader") << "Building OpenGL Shader program: \"" << name_ << "\" " << std::endl;

	std::vector<uint32_t> shader_ids;

	// * Compile each shader
	for(auto&& [type, source]: sources)
	{
		DLOGI << "Compiling " << to_string(type) << "." << std::endl;
		
		// Compile shader from source
    	GLuint shader_id = glCreateShader(to_gl_shader_type(type));
    	const char* char_src = source.c_str();
		glShaderSource(shader_id, 1, &char_src, nullptr);
		glCompileShader(shader_id);

		// Check compilation status
	    GLint is_compiled = 0;
	    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &is_compiled);

	    if(is_compiled == GL_FALSE)
	    {
	        DLOGE("shader") << "Shader \"" << name_ << "\" will not compile" << std::endl;
            shader_error_report(shader_id, 0, source);

	        // We don't need the shader anymore.
	        glDeleteShader(shader_id);
	    	return false;
	    }

	    // Save shader id for later linking
	    shader_ids.push_back(shader_id);
	}

    // Link program
    if(link(shader_ids))
    {
        DLOGI << "Program \"" << name_ << "\" is ready." << std::endl;
        return true;
    }

    return false;
}

bool OGLShader::build_spirv(const fs::path& filepath)
{
    W_PROFILE_FUNCTION()

    DLOGN("shader") << "Building SPIR-V OpenGL Shader program: \"" << name_ << "\" " << std::endl;
    std::vector<uint32_t> shader_ids;

    auto stages = spv::parse_stages(filepath);

    std::vector<uint8_t> spirv;
    filesystem::get_file_as_vector(filepath, spirv);

    for(auto&& stage: stages)
    {
        slang::ExecutionModel type = slang::ExecutionModel(stage.execution_model);
        
        DLOG("shader",1) << "Specializing " << to_string(type) << "." << std::endl;
        DLOGI << "Entry point: " << stage.entry_point << std::endl;

        GLuint shader_id = glCreateShader(to_gl_shader_type(type));
        glShaderBinary(1, &shader_id, GL_SHADER_BINARY_FORMAT_SPIR_V, spirv.data(), spirv.size());
        glSpecializeShader(shader_id, static_cast<const GLchar*>(stage.entry_point.c_str()), 0, nullptr, nullptr);

        // Check compilation status
        GLint is_compiled = 0;
        glGetShaderiv(shader_id, GL_COMPILE_STATUS, &is_compiled);

        if(is_compiled == GL_FALSE)
        {
            DLOGE("shader") << "Shader \"" << name_ << "\" will not compile" << std::endl;
            DLOGR("shader") << get_shader_error_report(shader_id) << std::endl;

            // We don't need the shader anymore.
            glDeleteShader(shader_id);
            return false;
        }

        // Save shader id for later linking
        shader_ids.push_back(shader_id);
    }

    // Link program
    if(link(shader_ids))
    {
        DLOGI << "Program \"" << name_ << "\" is ready." << std::endl;
        return true;
    }

    return false;
}

bool OGLShader::link(const std::vector<GLuint>& shader_ids)
{
    W_PROFILE_FUNCTION()

    // * Link program
    DLOGI << "Linking program." << std::endl;
    rd_handle_ = glCreateProgram();
    for(auto&& shader_id: shader_ids)
        glAttachShader(rd_handle_, shader_id);

    glLinkProgram(rd_handle_);

    // * Check linking status
    GLint is_linked = 0;
    glGetProgramiv(rd_handle_, GL_LINK_STATUS, static_cast<int*>(&is_linked));

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

    return true;
}

struct BlockElement
{
    int32_t offset;
    int32_t type;
    std::string name;
};

void OGLShader::introspect()
{
    W_PROFILE_FUNCTION()
    
    // Interfaces to query
    static const std::vector<GLenum> interfaces
    {
        GL_PROGRAM_INPUT, GL_UNIFORM, GL_BUFFER_VARIABLE, 
        GL_UNIFORM_BLOCK, GL_SHADER_STORAGE_BLOCK
    };
    // Properties to get for each interface
    static const std::map<GLenum, std::vector<GLenum>> properties_map
    {
        {GL_PROGRAM_INPUT,        {GL_NAME_LENGTH, GL_TYPE, GL_LOCATION}},
        {GL_UNIFORM,              {GL_NAME_LENGTH, GL_BLOCK_INDEX, GL_TYPE, GL_LOCATION}},
        {GL_UNIFORM_BLOCK,        {GL_NAME_LENGTH, GL_BUFFER_BINDING, GL_NUM_ACTIVE_VARIABLES}},
        {GL_SHADER_STORAGE_BLOCK, {GL_NAME_LENGTH, GL_BUFFER_BINDING}},
        {GL_BUFFER_VARIABLE,      {GL_NAME_LENGTH, GL_BLOCK_INDEX, GL_TYPE}},
    };
    // Block-uniform properties
    static const std::vector<GLenum> unif_props {GL_NAME_LENGTH, GL_TYPE, GL_OFFSET};
    static const std::vector<GLenum> active_unif_prop {GL_ACTIVE_VARIABLES};

    std::vector<GLint> prop_values; // Will receive queried properties
    std::string resource_name;
    [[maybe_unused]] std::string uniform_name;

    // For attribute layout detection
    std::vector<BufferLayoutElement> attribute_layout;
    uint32_t attribute_count = 0;

    for(size_t ii=0; ii<interfaces.size(); ++ii)
    {
        // Get active objects count
        GLint num_active;
        GLenum iface = interfaces[ii];
        glGetProgramInterfaceiv(rd_handle_, iface, GL_ACTIVE_RESOURCES, &num_active);

        if(num_active)
        {
            DLOG("shader",1) << "[" << WCC(102,153,0) << ogl_interface_to_string(iface) << WCC(0) 
                             << "] active: " << num_active << std::endl;
        }
        else
            continue;

        // Get properties
        const auto& properties = properties_map.at(iface);
        prop_values.resize(properties.size());
        
        if(iface == GL_PROGRAM_INPUT)
            attribute_layout.resize(size_t(num_active));

        for(int jj=0; jj<num_active; ++jj)
        {
            glGetProgramResourceiv(rd_handle_, iface, jj, properties.size(),
                                   &properties[0], prop_values.size(), nullptr, &prop_values[0]);

            resource_name.resize(size_t(prop_values[0])); // The length of the name
            glGetProgramResourceName(rd_handle_, iface, jj, resource_name.size(), nullptr, &resource_name[0]);
            hash_t hname = H_(resource_name.c_str());

            // Register resource
            if(iface == GL_PROGRAM_INPUT)
            {
                // PROPS = 0: GL_NAME_LENGTH, 1: GL_TYPE, 2: GL_LOCATION
                DLOGI << "[Loc: " << prop_values[2] << "] " << ogl_attribute_type_to_string(GLenum(prop_values[1])) 
                      << " " << WCC('u') << resource_name << WCC(0) << std::endl;
                // For attribute layout detection
                if(prop_values[2] != -1)
                {
                    attribute_layout[size_t(prop_values[2])] = BufferLayoutElement(H_(resource_name.c_str()), ogl_attribute_type_to_shader_data_type(GLenum(prop_values[1])));
                    ++attribute_count;
                }
            }
            else if(iface == GL_UNIFORM)
            {
                // PROPS = 0: GL_NAME_LENGTH, 1: GL_BLOCK_INDEX, 2: GL_TYPE, 3: GL_LOCATION
                // We only want non-block uniforms
                if(prop_values[1]==-1)
                {
                    uniform_locations_.insert(std::make_pair(hname, prop_values[3])); // Save location
                    if(prop_values[2] == GL_SAMPLER_2D || prop_values[2] == GL_SAMPLER_CUBE)
                        texture_slots_.insert(std::make_pair(hname, current_slot_++));
                    DLOGI << "[Loc: " << prop_values[3] << "] " << ogl_uniform_type_to_string(GLenum(prop_values[2])) 
                          << " " << WCC('u') << resource_name << WCC(0) << std::endl;
                }

            }
            else if(iface == GL_UNIFORM_BLOCK)
            {
                // PROPS = 0: GL_NAME_LENGTH, 1: GL_BUFFER_BINDING, 2: GL_NUM_ACTIVE_VARIABLES
                block_bindings_.insert(std::make_pair(hname, prop_values[1]));
                DLOGI << "[Binding: " << prop_values[1] << "] " << WCC('n') << resource_name << std::endl;
#ifdef W_DEBUG
                int num_active_uniforms = prop_values[2];
                if(num_active_uniforms==0) continue;

                // Get all active uniform indices for this block
                std::vector<GLint> block_unif_indices(static_cast<size_t>(num_active_uniforms));
                glGetProgramResourceiv(rd_handle_, GL_UNIFORM_BLOCK, jj, 1, &active_unif_prop[0], 
                                       num_active_uniforms, nullptr, &block_unif_indices[0]);

                // Iterate over all uniforms in this block
                std::vector<BlockElement> block_elements;
                for(size_t kk=0; kk<size_t(num_active_uniforms); ++kk)
                {
                    // UNIF PROPS = 0: GL_NAME_LENGTH, 1: GL_TYPE, 2: GL_LOCATION
                    std::vector<GLint> unif_prop_values(unif_props.size());
                    glGetProgramResourceiv(rd_handle_, GL_UNIFORM, block_unif_indices[kk], unif_props.size(),
                                           &unif_props[0], unif_prop_values.size(), nullptr, &unif_prop_values[0]);
                    uniform_name.resize(size_t(unif_prop_values[0]));
                    glGetProgramResourceName(rd_handle_, GL_UNIFORM, block_unif_indices[kk], uniform_name.size(), 
                                             nullptr, &uniform_name[0]);
                    // Save uniform porperties
                    block_elements.push_back(BlockElement{unif_prop_values[2], unif_prop_values[1], uniform_name});
                }
                // Sort uniforms by offset and display
                std::sort(block_elements.begin(), block_elements.end(), [](const BlockElement& a, const BlockElement& b) -> bool
                { 
                    return a.offset < b.offset; 
                });
                for(auto&& elt: block_elements)
                {
                    DLOGI << "[Offset: " << elt.offset << "] " << ogl_uniform_type_to_string(GLenum(elt.type)) << " " 
                          << WCC('u') << elt.name << WCC(0) << std::endl;
                }
#endif
            }
            else if(iface == GL_SHADER_STORAGE_BLOCK)
            {
                // PROPS = 0: GL_NAME_LENGTH, 1: GL_BUFFER_BINDING
                block_bindings_.insert(std::make_pair(hname, prop_values[1]));
                DLOGI << "[Binding: " << prop_values[1] << "] " << WCC('u') << resource_name << std::endl;
            }
            /*else if(iface == GL_BUFFER_VARIABLE)
            {
                // PROPS = 0: GL_NAME_LENGTH, 1: GL_BLOCK_INDEX, 2: GL_TYPE
                DLOGI << "[" << prop_values[1] << "] " << ogl_uniform_type_to_string(prop_values[2]) << " " 
                      << WCC('u') << resource_name << WCC(0) << std::endl;
            }*/
        }
    }

    attribute_layout_.init(attribute_layout.data(), attribute_count);

    DLOG("shader",1) << "--------" << std::endl;
}

#ifdef W_DEBUG
static inline void warn_unknown_uniform(const std::string& shader_name, hash_t u_name)
{
	static std::set<hash_t> marked; // So that we don't warn twice for the same uniform
    hash_t id = H_(shader_name.c_str()) ^ u_name;

	if(marked.find(id) == marked.end())
    {
		DLOGW("shader") << "Unknown uniform submitted to \"" << shader_name << "\": \"" << istr::resolve(u_name) << "\"" << std::endl;
		marked.insert(id);
	}
}
#endif

template <>
bool OGLShader::send_uniform<bool>(hash_t name, const bool& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
#ifdef W_DEBUG
        warn_unknown_uniform(name_, name);
#endif
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
#ifdef W_DEBUG
        warn_unknown_uniform(name_, name);
#endif
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
#ifdef W_DEBUG
        warn_unknown_uniform(name_, name);
#endif
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
#ifdef W_DEBUG
        warn_unknown_uniform(name_, name);
#endif
        return false;
    }

    glUniform2fv(it->second, 1, static_cast<const GLfloat*>(&value[0]));
    return true;
}

template <>
bool OGLShader::send_uniform<glm::vec3>(hash_t name, const glm::vec3& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
#ifdef W_DEBUG
        warn_unknown_uniform(name_, name);
#endif
        return false;
    }

    glUniform3fv(it->second, 1, static_cast<const GLfloat*>(&value[0]));
    return true;
}

template <>
bool OGLShader::send_uniform<glm::vec4>(hash_t name, const glm::vec4& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
#ifdef W_DEBUG
        warn_unknown_uniform(name_, name);
#endif
        return false;
    }

    glUniform4fv(it->second, 1, static_cast<const GLfloat*>(&value[0]));
    return true;
}

template <>
bool OGLShader::send_uniform<glm::mat2>(hash_t name, const glm::mat2& value) const
{
    auto it = uniform_locations_.find(name);
    if(it == uniform_locations_.end())
    {
#ifdef W_DEBUG
        warn_unknown_uniform(name_, name);
#endif
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
#ifdef W_DEBUG
        warn_unknown_uniform(name_, name);
#endif
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
#ifdef W_DEBUG
        warn_unknown_uniform(name_, name);
#endif
        return false;
    }

    glUniformMatrix4fv(it->second, 1, GL_FALSE, &value[0][0]);
    return true;
}

} // namespace erwin