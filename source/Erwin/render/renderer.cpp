#include "render/renderer.h"

#include <map>
#include <memory>

#include "core/clock.hpp"
#include "core/config.h"
#include "debug/logger.h"
#include "math/color.h"
#include "memory/arena.h"
#include "memory/handle_pool.h"
#include "render/backend.h"
#include "render/query_timer.h"
#include "filesystem/filesystem.h"

namespace erwin
{

// This macro allows us to perform the same action for all handle structs.
// When creating a new renderer handle type, simply add a line here, and change allocation size.
#define FOR_ALL_HANDLES                                                                                                \
    DO_ACTION(IndexBufferHandle)                                                                                       \
    DO_ACTION(VertexBufferLayoutHandle)                                                                                \
    DO_ACTION(VertexBufferHandle)                                                                                      \
    DO_ACTION(VertexArrayHandle)                                                                                       \
    DO_ACTION(UniformBufferHandle)                                                                                     \
    DO_ACTION(ShaderStorageBufferHandle)                                                                               \
    DO_ACTION(TextureHandle)                                                                                           \
    DO_ACTION(CubemapHandle)                                                                                           \
    DO_ACTION(ShaderHandle)                                                                                            \
    DO_ACTION(FramebufferHandle)

/*
          _____                _              ____
         |  __ \              | |            / __ \
         | |__) |___ _ __   __| | ___ _ __  | |  | |_   _  ___ _   _  ___
         |  _  // _ \ '_ \ / _` |/ _ \ '__| | |  | | | | |/ _ \ | | |/ _ \
         | | \ \  __/ | | | (_| |  __/ |    | |__| | |_| |  __/ |_| |  __/
         |_|  \_\___|_| |_|\__,_|\___|_|     \___\_\\__,_|\___|\__,_|\___|
*/

template <std::size_t SIZE> struct CommandBuffer
{
    typedef std::pair<uint64_t, void*> Entry;

    CommandBuffer() = default;
    CommandBuffer(memory::HeapArea& area, std::size_t size, const char* debug_name) { init(area, size, debug_name); }

    inline void init(memory::HeapArea& area, std::size_t size, const char* debug_name)
    {
        count = 0;
        storage.init(area, size, debug_name);
    }

    inline void reset()
    {
        storage.reset();
        count = 0;
    }

    inline bool is_full() const { return count >= SIZE; }

    std::size_t count;
    memory::LinearBuffer<> storage;
    Entry entries[SIZE];
};

using RenderCommandBuffer = CommandBuffer<k_max_render_commands>;
using DrawCommandBuffer = CommandBuffer<k_max_draw_calls>;

class RenderQueue
{
public:
    friend class Renderer;
    friend class DrawCommandWriter;

    RenderQueue() = default;
    explicit RenderQueue(memory::HeapArea& area);
    ~RenderQueue();

    void init(memory::HeapArea& area);

    // * These functions change the queue state persistently
    // Set clear color for this queue
    inline void set_clear_color(const glm::vec4& clear_color) { clear_color_ = clear_color; }
    // Check if the queue is full
    inline bool is_full() const { return command_buffer_.is_full(); }
    // Sort queue by sorting key
    void sort();
    // Dispatch all commands
    void flush();
    // Clear queue
    void reset();

private:
    uint8_t current_view_id_;
    glm::vec4 clear_color_;
    DrawCommandBuffer command_buffer_;
};

RenderQueue::RenderQueue(memory::HeapArea& area) { init(area); }

RenderQueue::~RenderQueue() {}

void RenderQueue::init(memory::HeapArea& area)
{
    clear_color_ = {0.f, 0.f, 0.f, 0.f};
    command_buffer_.init(area, cfg::get<size_t>("erwin.memory.renderer.queue_buffer"_h, 512_kB), "RenderQueue");
    current_view_id_ = 0;
}

void RenderQueue::sort()
{
    W_PROFILE_RENDER_FUNCTION()

    // Keys stored separately from commands to avoid touching data too
    // much during sort calls
    std::sort(std::begin(command_buffer_.entries), std::begin(command_buffer_.entries) + command_buffer_.count,
              [&](const DrawCommandBuffer::Entry& item1, const DrawCommandBuffer::Entry& item2) {
                  return item1.first < item2.first;
              });
}

void RenderQueue::reset()
{
    command_buffer_.reset();
    current_view_id_ = 0;
}

#if W_RC_PROFILE_DRAW_CALLS
// Allows to track draw calls from submission to dispatch during a single frame
struct FrameDrawCallData
{
    struct DrawCallSummary
    {
        uint64_t render_state;
        DrawCall::DrawCallType type;
        uint16_t shader_handle;
        uint32_t submission_index;
    };

    inline void on_submit(const DrawCall& dc, uint64_t key)
    {
        draw_calls.insert(
            std::make_pair(key, DrawCallSummary{dc.data.state_flags, dc.type, dc.data.shader.index, submitted++}));
    }

    inline void on_dispatch(uint64_t)
    {
        // Nothing to do for now, map ordering is the same as queue ordering once sorted
    }

    inline void reset()
    {
        draw_calls.clear();
        tracking = false;
        submitted = 0;
    }

    void export_json();

    std::multimap<uint64_t, DrawCallSummary> draw_calls;
    fs::path json_path;
    bool tracking = false;
    uint32_t submitted = 0;
};

void FrameDrawCallData::export_json()
{
    DLOGN("render") << "Exporting frame draw call profile:" << std::endl;
    DLOGI << WCC('p') << json_path << std::endl;

    auto p_ofs = wfs::get_ostream(json_path, wfs::ascii);
    auto& ofs = *p_ofs;

    ofs << "{"
        << "\"draw_calls\":[" << std::endl;

    // We assume map ordering is the same as queue ordering, elements are thus
    // presented in dispatch order
    uint32_t count = 0;
    for(auto&& [key, summary] : draw_calls)
    {
        RenderState render_state;
        render_state.decode(summary.render_state);
        ofs << "{"
            << "\"key\":" << key << ","
            << "\"sub\":" << summary.submission_index << ","
            << "\"typ\":" << int(summary.type)
            << ","
            // << "\"shd\":\"" << s_storage.shaders[summary.shader_handle]->get_name() << "\","
            << "\"sta\":\"" << render_state.to_string() << "\"" << ((count < submitted - 1) ? "}," : "}") << std::endl;

        ++count;
    }

    ofs << "]}" << std::endl;

    reset();

    DLOG("render", 1) << "done" << std::endl;
}
#endif

/*
           _____ _
          / ____| |
         | (___ | |_ ___  _ __ __ _  __ _  ___
          \___ \| __/ _ \| '__/ _` |/ _` |/ _ \
          ____) | || (_) | | | (_| | (_| |  __/
         |_____/ \__\___/|_|  \__,_|\__, |\___|
                                     __/ |
                                    |___/
*/

static int FRONT = 0;
static int BACK = 1;

static struct RendererStorage
{
    inline void init(memory::HeapArea* area)
    {
        renderer_memory_ = area;
        pre_buffer_.init(*renderer_memory_, cfg::get<size_t>("erwin.memory.renderer.pre_buffer"_h, 512_kB), "CB-Pre");
        post_buffer_.init(*renderer_memory_, cfg::get<size_t>("erwin.memory.renderer.post_buffer"_h, 512_kB),
                          "CB-Post");
        auxiliary_arena_.init(*renderer_memory_, cfg::get<size_t>("erwin.memory.renderer.auxiliary_arena"_h, 2_MB),
                              "Auxiliary");
        handle_arena_.init(*renderer_memory_, k_render_handle_alloc_size, "RenderHandles");
        queue_.init(*renderer_memory_);

// Init handle pools
#define DO_ACTION(HANDLE_NAME) HANDLE_NAME::init_pool(handle_arena_);
        FOR_ALL_HANDLES
#undef DO_ACTION

        // Init render queue
        queue_.set_clear_color(glm::vec4(0.f, 0.f, 0.f, 0.f));
    }

    inline void release()
    {
// Destroy handle pools
#define DO_ACTION(HANDLE_NAME) HANDLE_NAME::destroy_pool(handle_arena_);
        FOR_ALL_HANDLES
#undef DO_ACTION
    }

    bool initialized_ = false;
    bool profiling_enabled_ = false;

    WScope<QueryTimer> query_timer;
    Renderer::Statistics stats[2]; // Double buffered
#if W_RC_PROFILE_DRAW_CALLS
    FrameDrawCallData draw_call_data;
#endif

    memory::HeapArea* renderer_memory_ = nullptr;
    RenderCommandBuffer pre_buffer_;
    RenderCommandBuffer post_buffer_;
    Renderer::AuxArena auxiliary_arena_;
    LinearArena handle_arena_;
    RenderQueue queue_;
} s_storage;

void RenderQueue::flush()
{
    W_PROFILE_RENDER_FUNCTION()
    // Set clear color
    gfx::backend->set_clear_color(clear_color_.r, clear_color_.g, clear_color_.b, clear_color_.a);

    // Execute all draw commands in command buffer
    for(size_t ii = 0; ii < command_buffer_.count; ++ii)
    {
        auto&& [key, cmd] = command_buffer_.entries[ii];

        // k_skip is the max key value possible, so if we arrive here, simply break out of the loop
        if(key == SortKey::k_skip)
            break;

#if W_RC_PROFILE_DRAW_CALLS
        if(s_storage.draw_call_data.tracking)
            s_storage.draw_call_data.on_dispatch(key);
#endif
        command_buffer_.storage.seek(cmd);

        uint16_t type;
        command_buffer_.storage.read(&type);

        // If type is a draw call, unroll and dispatch dependencies first
        if(type == uint16_t(DrawCommand::Draw))
        {
            uint8_t dependency_count;
            static void* deps[k_max_draw_call_dependencies];
            command_buffer_.storage.read(&dependency_count);
            for(uint8_t jj = 0; jj < dependency_count; ++jj)
                command_buffer_.storage.read(&deps[jj]);

            void* ret = command_buffer_.storage.head();
            for(uint8_t jj = 0; jj < dependency_count; ++jj)
            {
                command_buffer_.storage.seek(deps[jj]);
                uint16_t dep_type;
                command_buffer_.storage.read(&dep_type);
                gfx::backend->dispatch_draw(dep_type, command_buffer_.storage);
            }
            command_buffer_.storage.seek(ret);
        }

        gfx::backend->dispatch_draw(type, command_buffer_.storage);
    }
}

// Helper class for command buffer access
class RenderCommandWriter
{
public:
    explicit RenderCommandWriter(RenderCommand type)
        : type_(type), cmdbuf_(get_command_buffer(type_)), head_(cmdbuf_.storage.head())
    {
        cmdbuf_.storage.write(&type_);
    }

    template <typename T> inline void write(T* source) { cmdbuf_.storage.write(source); }
    inline void write_str(const std::string& str) { cmdbuf_.storage.write_str(str); }

    inline void submit()
    {
        W_ASSERT_FMT(!cmdbuf_.is_full(), "Command buffer %d is full!", int(type_));
        uint64_t key = uint64_t(cmdbuf_.count);
        cmdbuf_.entries[cmdbuf_.count++] = {key, head_};
    }

private:
    enum class Phase
    {
        Pre,
        Post
    };

    inline RenderCommandBuffer& get_command_buffer(Phase phase)
    {
        switch(phase)
        {
        case Phase::Pre:
            return s_storage.pre_buffer_;
        case Phase::Post:
            return s_storage.post_buffer_;
        }
    }

    inline RenderCommandBuffer& get_command_buffer(RenderCommand command)
    {
        Phase phase = (command < RenderCommand::Post) ? Phase::Pre : Phase::Post;
        return get_command_buffer(phase);
    }

private:
    RenderCommand type_;
    RenderCommandBuffer& cmdbuf_;
    void* head_;
};

// Helper class for draw command buffer access
class DrawCommandWriter
{
public:
    explicit DrawCommandWriter(DrawCommand type)
        : type_(type), cmdbuf_(s_storage.queue_.command_buffer_), head_(cmdbuf_.storage.head())
    {
        cmdbuf_.storage.write(&type_);
    }

    template <typename T> inline void write(T* source) { cmdbuf_.storage.write(source); }
    inline void write_str(const std::string& str) { cmdbuf_.storage.write_str(str); }

    inline uint32_t submit(uint64_t key)
    {
        W_ASSERT(!cmdbuf_.is_full(), "Render queue is full!");
        cmdbuf_.entries[cmdbuf_.count] = {key, head_};
        return uint32_t(cmdbuf_.count++);
    }

private:
    DrawCommand type_;
    DrawCommandBuffer& cmdbuf_;
    void* head_;
};

void Renderer::init(memory::HeapArea& area)
{
    W_PROFILE_RENDER_FUNCTION()

    if(s_storage.initialized_)
    {
        DLOGW("render") << "[Renderer] Already initialized, skipping." << std::endl;
        return;
    }

    DLOGN("render") << "[Renderer] Allocating renderer storage." << std::endl;

    // Create and initialize storage object
    s_storage.init(&area);

    // TODO: Use config
    gfx::set_backend(GfxAPI::OpenGL);

    s_storage.query_timer = QueryTimer::create();

    DLOGI << "done" << std::endl;

    DLOGN("render") << "[Renderer] Creating main render targets." << std::endl;
    // Create main render targets
    {
        FramebufferLayout layout{
            // RGB: Albedo, A: Scaled emissivity
            {"albedo"_h, ImageFormat::RGBA16F, MIN_NEAREST | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE},
            // RG: Compressed normal, BA: ?
            {"normal"_h, ImageFormat::RGBA16_SNORM, MIN_NEAREST | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE},
            // R: Metallic, G: AO, B: Roughness, A: ?
            {"mar"_h, ImageFormat::RGBA8, MIN_NEAREST | MAG_LINEAR, TextureWrap::CLAMP_TO_EDGE},
        };
        FramebufferPool::create_framebuffer("GBuffer"_h, make_scope<FbRatioConstraint>(),
                                            FB_DEPTH_ATTACHMENT | FB_STENCIL_ATTACHMENT, layout);
    }
    {
        FramebufferLayout layout{
            // RGBA: HDR color
            {"albedo"_h, ImageFormat::RGBA16F, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE},
            // RGB: Glow color, A: Glow intensity
            {"glow"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_LINEAR, TextureWrap::CLAMP_TO_EDGE},
        };
        FramebufferPool::create_framebuffer("LBuffer"_h, make_scope<FbRatioConstraint>(),
                                            FB_DEPTH_ATTACHMENT | FB_STENCIL_ATTACHMENT, layout);
    }

    s_storage.initialized_ = true;

    // Renderer configuration
    gfx::backend->set_seamless_cubemaps_enabled(cfg::get<bool>("erwin.renderer.enable_cubemap_seamless"_h, false));

    DLOGI << "done" << std::endl;
}

void Renderer::shutdown()
{
    W_PROFILE_RENDER_FUNCTION()

    if(!s_storage.initialized_)
    {
        DLOGW("render") << "[Renderer] Not initialized, skipping shutdown." << std::endl;
        return;
    }

    flush();
    DLOGN("render") << "[Renderer] Releasing renderer storage." << std::endl;

    gfx::backend->release();

    s_storage.release();
    s_storage.initialized_ = false;

    DLOGI << "done" << std::endl;
}

uint8_t Renderer::next_layer_id()
{
    W_ASSERT(s_storage.queue_.current_view_id_ < 255, "View id overflow.");
    return s_storage.queue_.current_view_id_++;
}

Renderer::AuxArena& Renderer::get_arena() { return s_storage.auxiliary_arena_; }

#ifdef W_DEBUG
void Renderer::set_profiling_enabled(bool value) { s_storage.profiling_enabled_ = value; }

const Renderer::Statistics& Renderer::get_stats() { return s_storage.stats[BACK]; }
#endif

void Renderer::track_draw_calls(const fs::path& json_path)
{
#if W_RC_PROFILE_DRAW_CALLS
    s_storage.draw_call_data.json_path = json_path;
    s_storage.draw_call_data.tracking = true;
#endif
}

FramebufferHandle Renderer::default_render_target() { return gfx::backend->default_render_target(); }

TextureHandle Renderer::get_framebuffer_texture(FramebufferHandle handle, uint32_t index)
{
    return gfx::backend->get_framebuffer_texture(handle, index);
}

CubemapHandle Renderer::get_framebuffer_cubemap(FramebufferHandle handle)
{
    return gfx::backend->get_framebuffer_cubemap(handle);
}

hash_t Renderer::get_framebuffer_texture_name(FramebufferHandle handle, uint32_t index)
{
    return gfx::backend->get_framebuffer_texture_name(handle, index);
}

uint32_t Renderer::get_framebuffer_texture_count(FramebufferHandle handle)
{
    return gfx::backend->get_framebuffer_texture_count(handle);
}

void* Renderer::get_native_texture_handle(TextureHandle handle)
{
    return gfx::backend->get_native_texture_handle(handle);
}

VertexBufferLayoutHandle Renderer::create_vertex_buffer_layout(const std::vector<BufferLayoutElement>& elements)
{
    return gfx::backend->create_vertex_buffer_layout(elements);
}

const BufferLayout& Renderer::get_vertex_buffer_layout(VertexBufferLayoutHandle handle)
{
    return gfx::backend->get_vertex_buffer_layout(handle);
}

/*
bool Renderer::is_compatible(VertexBufferLayoutHandle layout, ShaderHandle shader)
{
    W_ASSERT(layout.is_valid(), "Invalid VertexBufferLayoutHandle!");
    W_ASSERT(shader.is_valid(), "Invalid ShaderHandle!");

    if(!s_storage.shader_compat[shader.index].ready)
        return false;

    return s_storage.vertex_buffer_layouts[layout.index]->compare(s_storage.shader_compat[shader.index].layout);
}
*/

static void flush_command_buffer(RenderCommandBuffer& cmdbuf)
{
    W_PROFILE_RENDER_FUNCTION()

    // Dispatch render commands in specified command buffer
    for(size_t ii = 0; ii < cmdbuf.count; ++ii)
    {
        auto&& [key, cmd] = cmdbuf.entries[ii];
        cmdbuf.storage.seek(cmd);
        uint16_t type;
        cmdbuf.storage.read(&type);
        gfx::backend->dispatch_command(type, cmdbuf.storage);
    }
    cmdbuf.reset();
}

static void sort_commands()
{
    W_PROFILE_RENDER_FUNCTION()
    // Keys stored separately from commands to avoid touching data too
    // much during sort calls
    std::sort(std::begin(s_storage.pre_buffer_.entries),
              std::begin(s_storage.pre_buffer_.entries) + s_storage.pre_buffer_.count,
              [&](const RenderCommandBuffer::Entry& item1, const RenderCommandBuffer::Entry& item2) {
                  return item1.first < item2.first;
              });
    std::sort(std::begin(s_storage.post_buffer_.entries),
              std::begin(s_storage.post_buffer_.entries) + s_storage.post_buffer_.count,
              [&](const RenderCommandBuffer::Entry& item1, const RenderCommandBuffer::Entry& item2) {
                  return item1.first < item2.first;
              });
}

void Renderer::flush()
{
    W_PROFILE_RENDER_FUNCTION()
    static nanoClock flush_clock;
    if(s_storage.profiling_enabled_)
    {
        s_storage.query_timer->start();
        flush_clock.restart();
    }

    // Sort command buffers
    sort_commands();
    // Dispatch pre buffer commands
    flush_command_buffer(s_storage.pre_buffer_);
    // Sort, flush and reset queue
    s_storage.queue_.sort();
    s_storage.queue_.flush();
    s_storage.queue_.reset();
    // Dispatch post buffer commands
    flush_command_buffer(s_storage.post_buffer_);
    // Reset auxiliary memory arena for next frame
    s_storage.auxiliary_arena_.reset();
    // BUGFIX: Avoids a nasty bug where multiple framebuffers will have garbage size
    // if nothing gets drawn to the default framebuffer when using ImGui::Docking / OpenGL
    // This solves the problem, but I'm still not completely sure why.
    gfx::backend->bind_default_framebuffer();

#if W_RC_PROFILE_DRAW_CALLS
    if(s_storage.draw_call_data.tracking)
        s_storage.draw_call_data.export_json();
#endif

    if(s_storage.profiling_enabled_)
    {
        auto GPU_render_duration = s_storage.query_timer->stop();
        auto CPU_flush_duration = flush_clock.get_elapsed_time();
        s_storage.stats[FRONT].GPU_render_time =
            float(std::chrono::duration_cast<std::chrono::microseconds>(GPU_render_duration).count());
        s_storage.stats[FRONT].CPU_flush_time =
            float(std::chrono::duration_cast<std::chrono::microseconds>(CPU_flush_duration).count());
    }

    std::swap(FRONT, BACK);
    s_storage.stats[FRONT].draw_call_count = 0;
}

/*
           _____                                          _
          / ____|                                        | |
         | |     ___  _ __ ___  _ __ ___   __ _ _ __   __| |___
         | |    / _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` / __|
         | |___| (_) | | | | | | | | | | | (_| | | | | (_| \__ \
          \_____\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_|___/
*/

IndexBufferHandle Renderer::create_index_buffer(const uint32_t* index_data, uint32_t count, DrawPrimitive primitive,
                                                UsagePattern mode)
{
    IndexBufferHandle handle = IndexBufferHandle::acquire();
    W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

    // Allocate auxiliary data
    uint32_t* auxiliary = nullptr;
    if(index_data)
    {
        auxiliary = W_NEW_ARRAY_DYNAMIC(uint32_t, count, s_storage.auxiliary_arena_);
        memcpy(auxiliary, index_data, count * sizeof(uint32_t));
    }
    else
        W_ASSERT(mode != UsagePattern::Static, "Index data can't be null in static mode.");

    // Write data
    RenderCommandWriter cw(RenderCommand::CreateIndexBuffer);
    cw.write(&handle);
    cw.write(&count);
    cw.write(&primitive);
    cw.write(&mode);
    cw.write(&auxiliary);
    cw.submit();

    return handle;
}

VertexBufferHandle Renderer::create_vertex_buffer(VertexBufferLayoutHandle layout, const float* vertex_data,
                                                  uint32_t count, UsagePattern mode)
{
    W_ASSERT(layout.is_valid(), "Invalid VertexBufferLayoutHandle!");

    VertexBufferHandle handle = VertexBufferHandle::acquire();
    W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

    // Allocate auxiliary data
    float* auxiliary = nullptr;
    if(vertex_data)
    {
        auxiliary = W_NEW_ARRAY_DYNAMIC(float, count, s_storage.auxiliary_arena_);
        memcpy(auxiliary, vertex_data, count * sizeof(float));
    }
    else
        W_ASSERT(mode != UsagePattern::Static, "Vertex data can't be null in static mode.");

    // Write data
    RenderCommandWriter cw(RenderCommand::CreateVertexBuffer);
    cw.write(&handle);
    cw.write(&layout);
    cw.write(&count);
    cw.write(&mode);
    cw.write(&auxiliary);
    cw.submit();

    return handle;
}

VertexArrayHandle Renderer::create_vertex_array(VertexBufferHandle vb, IndexBufferHandle ib)
{
    W_ASSERT(vb.is_valid(), "Invalid VertexBufferHandle!");

    VertexArrayHandle handle = VertexArrayHandle::acquire();
    W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

    // Write data
    RenderCommandWriter cw(RenderCommand::CreateVertexArray);
    cw.write(&handle);
    cw.write(&ib);
    cw.write(&vb);
    cw.submit();

    return handle;
}

VertexArrayHandle Renderer::create_vertex_array(const std::vector<VertexBufferHandle>& vbs, IndexBufferHandle ib)
{
    VertexArrayHandle handle = VertexArrayHandle::acquire();
    W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

    uint8_t VBO_count = uint8_t(vbs.size());

    RenderCommandWriter cw(RenderCommand::CreateVertexArrayMultipleVBO);
    cw.write(&handle);
    cw.write(&ib);
    cw.write(&VBO_count);

    for(auto vb : vbs)
    {
        W_ASSERT_FMT(vb.is_valid(), "Invalid VertexBufferHandle: %hu.", vb.index);
        cw.write(&vb);
    }
    cw.submit();

    return handle;
}

UniformBufferHandle Renderer::create_uniform_buffer(const std::string& name, const void* data, uint32_t size,
                                                    UsagePattern mode)
{
    UniformBufferHandle handle = UniformBufferHandle::acquire();
    W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

    // Allocate auxiliary data
    uint8_t* auxiliary = nullptr;
    if(data)
    {
        auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, s_storage.auxiliary_arena_);
        memcpy(auxiliary, data, size);
    }
    else
        W_ASSERT(mode != UsagePattern::Static, "UBO data can't be null in static mode.");

    // Write data
    RenderCommandWriter cw(RenderCommand::CreateUniformBuffer);
    cw.write(&handle);
    cw.write(&size);
    cw.write(&mode);
    cw.write_str(name);
    cw.write(&auxiliary);
    cw.submit();

    return handle;
}

ShaderStorageBufferHandle Renderer::create_shader_storage_buffer(const std::string& name, const void* data, uint32_t size,
                                                                 UsagePattern mode)
{
    ShaderStorageBufferHandle handle = ShaderStorageBufferHandle::acquire();
    W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

    // Allocate auxiliary data
    uint8_t* auxiliary = nullptr;
    if(data)
    {
        auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, s_storage.auxiliary_arena_);
        memcpy(auxiliary, data, size);
    }
    else
        W_ASSERT(mode != UsagePattern::Static, "SSBO data can't be null in static mode.");

    // Write data
    RenderCommandWriter cw(RenderCommand::CreateShaderStorageBuffer);
    cw.write(&handle);
    cw.write(&size);
    cw.write(&mode);
    cw.write_str(name);
    cw.write(&auxiliary);
    cw.submit();

    return handle;
}

ShaderHandle Renderer::create_shader(const fs::path& filepath, const std::string& name)
{
    ShaderHandle handle = ShaderHandle::acquire();
    W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

    RenderCommandWriter cw(RenderCommand::CreateShader);
    cw.write(&handle);
    cw.write_str(filepath.string());
    cw.write_str(name);
    cw.submit();

    return handle;
}

TextureHandle Renderer::create_texture_2D(const Texture2DDescriptor& desc)
{
    TextureHandle handle = TextureHandle::acquire();
    W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

    RenderCommandWriter cw(RenderCommand::CreateTexture2D);
    cw.write(&handle);
    cw.write(&desc);
    cw.submit();

    return handle;
}

CubemapHandle Renderer::create_cubemap(const CubemapDescriptor& desc)
{
    CubemapHandle handle = CubemapHandle::acquire();
    W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

    RenderCommandWriter cw(RenderCommand::CreateCubemap);
    cw.write(&handle);
    cw.write(&desc);
    cw.submit();

    return handle;
}

FramebufferHandle Renderer::create_framebuffer(uint32_t width, uint32_t height, uint8_t flags,
                                               const FramebufferLayout& layout)
{
    FramebufferHandle handle = FramebufferHandle::acquire();
    W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

    // Create handles for framebuffer textures
    FramebufferTextureVector texture_vector;
    bool has_cubemap = bool(flags & FBFlag::FB_CUBEMAP_ATTACHMENT);

    if(!has_cubemap)
    {
        bool has_depth = bool(flags & FBFlag::FB_DEPTH_ATTACHMENT);
        uint32_t tex_count = has_depth ? uint32_t(layout.get_count()) + 1
                                       : uint32_t(layout.get_count()); // Take the depth texture into account
        for(uint32_t ii = 0; ii < tex_count; ++ii)
        {
            TextureHandle tex_handle = TextureHandle::acquire();
            W_ASSERT(tex_handle.is_valid(), "No more free handle in handle pool.");
            texture_vector.handles.push_back(tex_handle);
            texture_vector.debug_names.push_back((has_depth && ii == tex_count - 1) ? "depth"_h
                                                                                    : layout[ii].target_name);
        }
    }
    else
    {
        CubemapHandle cm_handle = CubemapHandle::acquire();
        W_ASSERT(cm_handle.is_valid(), "No more free handle in handle pool.");
        W_ASSERT(layout.get_count() == 1, "Only one cubemap attachment per framebuffer allowed.");
        texture_vector.cubemap = cm_handle;
        texture_vector.debug_names.push_back(layout[0].target_name);
    }

    gfx::backend->add_framebuffer_texture_vector(handle, texture_vector);
    uint32_t count = uint32_t(layout.get_count());

    // Allocate auxiliary data
    FramebufferLayoutElement* auxiliary =
        W_NEW_ARRAY_DYNAMIC_ALIGN(FramebufferLayoutElement, count, s_storage.auxiliary_arena_, 8);
    memcpy(auxiliary, layout.data(), count * sizeof(FramebufferLayoutElement));

    RenderCommandWriter cw(RenderCommand::CreateFramebuffer);
    cw.write(&handle);
    cw.write(&width);
    cw.write(&height);
    cw.write(&flags);
    cw.write(&count);
    cw.write(&auxiliary);
    cw.submit();

    return handle;
}

void Renderer::update_index_buffer(IndexBufferHandle handle, const uint32_t* data, uint32_t count)
{
    W_ASSERT(handle.is_valid(), "Invalid IndexBufferHandle!");
    W_ASSERT(data, "No data!");

    uint32_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint32_t, count, s_storage.auxiliary_arena_);
    memcpy(auxiliary, data, count);

    RenderCommandWriter cw(RenderCommand::UpdateIndexBuffer);
    cw.write(&handle);
    cw.write(&count);
    cw.write(&auxiliary);
    cw.submit();
}

void Renderer::update_vertex_buffer(VertexBufferHandle handle, const void* data, uint32_t size)
{
    W_ASSERT(handle.is_valid(), "Invalid VertexBufferHandle!");
    W_ASSERT(data, "No data!");

    uint8_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, s_storage.auxiliary_arena_);
    memcpy(auxiliary, data, size);

    RenderCommandWriter cw(RenderCommand::UpdateVertexBuffer);
    cw.write(&handle);
    cw.write(&size);
    cw.write(&auxiliary);
    cw.submit();
}

void Renderer::update_uniform_buffer(UniformBufferHandle handle, const void* data, uint32_t size)
{
    W_ASSERT(handle.is_valid(), "Invalid UniformBufferHandle!");
    W_ASSERT(data, "No data!");

    uint8_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, s_storage.auxiliary_arena_);
    memcpy(auxiliary, data, size);

    RenderCommandWriter cw(RenderCommand::UpdateUniformBuffer);
    cw.write(&handle);
    cw.write(&size);
    cw.write(&auxiliary);
    cw.submit();
}

void Renderer::update_shader_storage_buffer(ShaderStorageBufferHandle handle, const void* data, uint32_t size)
{
    W_ASSERT(handle.is_valid(), "Invalid ShaderStorageBufferHandle!");
    W_ASSERT(data, "No data!");

    uint8_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, s_storage.auxiliary_arena_);
    memcpy(auxiliary, data, size);

    RenderCommandWriter cw(RenderCommand::UpdateShaderStorageBuffer);
    cw.write(&handle);
    cw.write(&size);
    cw.write(&auxiliary);
    cw.submit();
}

void Renderer::shader_attach_uniform_buffer(ShaderHandle shader, UniformBufferHandle ubo)
{
    W_ASSERT(shader.is_valid(), "Invalid ShaderHandle!");
    W_ASSERT(ubo.is_valid(), "Invalid UniformBufferHandle!");

    RenderCommandWriter cw(RenderCommand::ShaderAttachUniformBuffer);
    cw.write(&shader);
    cw.write(&ubo);
    cw.submit();
}

void Renderer::shader_attach_storage_buffer(ShaderHandle shader, ShaderStorageBufferHandle ssbo)
{
    W_ASSERT(shader.is_valid(), "Invalid ShaderHandle!");
    W_ASSERT(ssbo.is_valid(), "Invalid ShaderStorageBufferHandle!");

    RenderCommandWriter cw(RenderCommand::ShaderAttachStorageBuffer);
    cw.write(&shader);
    cw.write(&ssbo);
    cw.submit();
}

void Renderer::update_framebuffer(FramebufferHandle fb, uint32_t width, uint32_t height)
{
    W_ASSERT(fb.is_valid(), "Invalid FramebufferHandle!");

    RenderCommandWriter cw(RenderCommand::UpdateFramebuffer);
    cw.write(&fb);
    cw.write(&width);
    cw.write(&height);
    cw.submit();
}

void Renderer::clear_framebuffers()
{
    RenderCommandWriter cw(RenderCommand::ClearFramebuffers);
    cw.submit();
}

void Renderer::set_host_window_size(uint32_t width, uint32_t height)
{
    RenderCommandWriter cw(RenderCommand::SetHostWindowSize);
    cw.write(&width);
    cw.write(&height);
    cw.submit();
}

std::future<PixelData> Renderer::get_pixel_data(TextureHandle handle)
{
    W_ASSERT(handle.is_valid(), "Invalid TextureHandle.");

    // TODO: acquire from render device
    auto&& [token, fut] = gfx::backend->future_texture_data();
    RenderCommandWriter cw(RenderCommand::GetPixelData);
    cw.write(&handle);
    cw.write(&token);
    cw.submit();

    return std::move(fut);
}

void Renderer::generate_mipmaps(CubemapHandle cubemap)
{
    W_ASSERT(cubemap.is_valid(), "Invalid CubemapHandle!");

    RenderCommandWriter cw(RenderCommand::GenerateCubemapMipmaps);
    cw.write(&cubemap);
    cw.submit();
}

void Renderer::framebuffer_screenshot(FramebufferHandle fb, const fs::path& filepath)
{
    W_ASSERT(fb.is_valid(), "Invalid FramebufferHandle!");

    RenderCommandWriter cw(RenderCommand::FramebufferScreenshot);
    cw.write(&fb);
    cw.write_str(filepath.string());
    cw.submit();
}

void Renderer::destroy(IndexBufferHandle handle)
{
    W_ASSERT(handle.is_valid(), "Invalid IndexBufferHandle!");

    RenderCommandWriter cw(RenderCommand::DestroyIndexBuffer);
    cw.write(&handle);
    cw.submit();
}

void Renderer::destroy(VertexBufferLayoutHandle handle)
{
    W_ASSERT(handle.is_valid(), "Invalid VertexBufferLayoutHandle!");

    RenderCommandWriter cw(RenderCommand::DestroyVertexBufferLayout);
    cw.write(&handle);
    cw.submit();
}

void Renderer::destroy(VertexBufferHandle handle)
{
    W_ASSERT(handle.is_valid(), "Invalid VertexBufferHandle!");

    RenderCommandWriter cw(RenderCommand::DestroyVertexBuffer);
    cw.write(&handle);
    cw.submit();
}

void Renderer::destroy(VertexArrayHandle handle)
{
    W_ASSERT(handle.is_valid(), "Invalid VertexArrayHandle!");

    RenderCommandWriter cw(RenderCommand::DestroyVertexArray);
    cw.write(&handle);
    cw.submit();
}

void Renderer::destroy(UniformBufferHandle handle)
{
    W_ASSERT(handle.is_valid(), "Invalid UniformBufferHandle!");

    RenderCommandWriter cw(RenderCommand::DestroyUniformBuffer);
    cw.write(&handle);
    cw.submit();
}

void Renderer::destroy(ShaderStorageBufferHandle handle)
{
    W_ASSERT(handle.is_valid(), "Invalid ShaderStorageBufferHandle!");

    RenderCommandWriter cw(RenderCommand::DestroyShaderStorageBuffer);
    cw.write(&handle);
    cw.submit();
}

void Renderer::destroy(ShaderHandle handle)
{
    W_ASSERT(handle.is_valid(), "Invalid ShaderHandle!");

    RenderCommandWriter cw(RenderCommand::DestroyShader);
    cw.write(&handle);
    cw.submit();
}

void Renderer::destroy(TextureHandle handle)
{
    W_ASSERT(handle.is_valid(), "Invalid TextureHandle!");

    RenderCommandWriter cw(RenderCommand::DestroyTexture2D);
    cw.write(&handle);
    cw.submit();
}

void Renderer::destroy(CubemapHandle handle)
{
    W_ASSERT(handle.is_valid(), "Invalid CubemapHandle!");

    RenderCommandWriter cw(RenderCommand::DestroyCubemap);
    cw.write(&handle);
    cw.submit();
}

void Renderer::destroy(FramebufferHandle handle, bool detach_textures)
{
    W_ASSERT(handle.is_valid(), "Invalid FramebufferHandle!");

    RenderCommandWriter cw(RenderCommand::DestroyFramebuffer);
    cw.write(&handle);
    cw.write(&detach_textures);
    cw.submit();
}

void Renderer::submit(uint64_t key, const DrawCall& dc)
{
#if W_RC_PROFILE_DRAW_CALLS
    if(s_storage.draw_call_data.tracking)
        s_storage.draw_call_data.on_submit(dc, key);
#endif

    DrawCommandWriter cw(DrawCommand::Draw);

    // Handle dependencies
    cw.write(&dc.dependency_count);
    for(uint8_t ii = 0; ii < dc.dependency_count; ++ii)
    {
        void* ptr = s_storage.queue_.command_buffer_.entries[dc.dependencies[ii]].second;
        cw.write(&ptr);
    }

    cw.write(&dc.type);
    cw.write(&dc.data);
    cw.write(&dc.texture_count);
    for(uint8_t ii = 0; ii < dc.texture_count; ++ii)
        cw.write(&dc.textures[ii]);
    cw.write(&dc.cubemap_count);
    for(uint8_t ii = 0; ii < dc.cubemap_count; ++ii)
        cw.write(&dc.cubemaps[ii]);
    if(dc.type == DrawCall::IndexedInstanced || dc.type == DrawCall::ArrayInstanced)
        cw.write(&dc.instance_count);

    if(s_storage.profiling_enabled_)
        ++s_storage.stats[FRONT].draw_call_count;

    cw.submit(key);
}

void Renderer::clear(uint64_t key, FramebufferHandle target, uint32_t flags, const glm::vec4& clear_color)
{
    W_ASSERT_FMT(target.is_valid(), "Invalid FramebufferHandle: %hu", target.index);

    uint32_t color = color::pack(clear_color);

    DrawCommandWriter cw(DrawCommand::Clear);
    cw.write(&target);
    cw.write(&flags);
    cw.write(&color);

    cw.submit(key);
}

void Renderer::blit_depth(uint64_t key, FramebufferHandle source, FramebufferHandle target)
{
    W_ASSERT_FMT(source.is_valid(), "Invalid FramebufferHandle: %hu", source.index);
    W_ASSERT_FMT(target.is_valid(), "Invalid FramebufferHandle: %hu", target.index);

    DrawCommandWriter cw(DrawCommand::BlitDepth);
    cw.write(&source);
    cw.write(&target);

    cw.submit(key);
}

uint32_t Renderer::update_shader_storage_buffer(ShaderStorageBufferHandle handle, const void* data, uint32_t size,
                                                DataOwnership copy)
{
    W_ASSERT_FMT(handle.is_valid(), "Invalid ShaderStorageBufferHandle: %hu", handle.index);
    W_ASSERT(data, "Data is null.");

    DrawCommandWriter cw(DrawCommand::UpdateShaderStorageBuffer);
    cw.write(&handle);
    cw.write(&size);

    if(data && bool(copy))
    {
        void* data_copy = W_NEW_ARRAY_DYNAMIC(uint8_t, size, s_storage.auxiliary_arena_);
        memcpy(data_copy, data, size);
        cw.write(&data_copy);
    }
    else
        cw.write(&data);

    return cw.submit(SortKey::k_skip);
}

uint32_t Renderer::update_uniform_buffer(UniformBufferHandle handle, const void* data, uint32_t size, DataOwnership copy)
{
    W_ASSERT_FMT(handle.is_valid(), "Invalid UniformBufferHandle: %hu", handle.index);
    W_ASSERT(data, "Data is null.");

    DrawCommandWriter cw(DrawCommand::UpdateUniformBuffer);
    cw.write(&handle);
    cw.write(&size);

    if(data && bool(copy))
    {
        void* data_copy = W_NEW_ARRAY_DYNAMIC(uint8_t, size, s_storage.auxiliary_arena_);
        memcpy(data_copy, data, size);
        cw.write(&data_copy);
    }
    else
        cw.write(&data);

    return cw.submit(SortKey::k_skip);
}

} // namespace erwin