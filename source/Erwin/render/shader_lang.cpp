#include "render/shader_lang.h"
#include "core/application.h"
#include <algorithm>
#include <kibble/logger/logger.h>
#include <kibble/string/string.h>
#include <regex>
#include <vector>

namespace erwin
{
namespace slang
{

static struct
{
    std::vector<fs::path> include_dirs;
} s_storage;

void register_include_directory(const std::string& dir_path)
{
    if(WFS_.exists(dir_path))
    {
        KLOG("shader", 1) << "Shader include directory registered:" << std::endl;
        KLOGI << kb::KS_PATH_ << dir_path << std::endl;
        s_storage.include_dirs.push_back(WFS_.regular_path(dir_path));
    }
    else
    {
        KLOGE("shader") << "Shader include directory could not be found:" << std::endl;
        KLOGI << kb::KS_PATH_ << dir_path << std::endl;
    }
}

static fs::path find_include(const fs::path& base_dir, const std::string& filename)
{
    // First, check if file can be found in the direct vicinity
    if(fs::exists(base_dir / filename))
        return base_dir / filename;

    // Then, check all registered include directories
    return *std::find_if(s_storage.include_dirs.begin(), s_storage.include_dirs.end(),
                         [&filename](const fs::path& inc_path) { return fs::exists(inc_path / filename); }) /
           filename;
}

// From a source string, parse #include directives and return a new source string
// with #include directives replaced by the code they point to
static std::string handle_includes(const fs::path& base_dir, const std::string& source)
{
    // std::regex e_inc("\\s*#\\s*include\\s+(?:<[^>]*>|\"[^\"]*\")\\s*");
    std::regex e_inc("\\s*#\\s*include\\s+([<\"][^>\"]*[>\"])\\s*");
    return kb::su::rx::regex_replace(source, e_inc, [&](const std::smatch& m) {
        std::string result = m[1].str();
        std::string filename = result.substr(1, result.size() - 2);
        // KLOG("shader", 1) << "including: " << kb::KS_PATH_ << filename << kb::KC_ << std::endl;
        fs::path inc_path = find_include(base_dir, filename);
        K_ASSERT_FMT(inc_path.string().size() != 0, "Could not find include file: %s", filename.c_str());
        return "\n" + WFS_.get_file_as_string(inc_path) + "\n";
    });
}

void pre_process_GLSL(const std::string& filepath, std::vector<std::pair<ExecutionModel, std::string>>& sources)
{
    KLOG("shader", 1) << "Pre-processing source: " << std::endl;
    KLOGI << kb::KS_PATH_ << WFS_.regular_path(filepath).filename() << std::endl;

    std::string full_source(WFS_.get_file_as_string(filepath));
    fs::path base_directory = WFS_.regular_path(filepath).parent_path();

    // Look for #type directives to segment shader code
    static const std::string type_token = "#type";
    size_t pos = full_source.find(type_token, 0);
    while(pos != std::string::npos)
    {
        size_t eol = full_source.find_first_of("\r\n", pos);
        K_ASSERT(eol != std::string::npos, "Syntax error!");

        size_t begin = pos + type_token.size() + 1;
        std::string type = full_source.substr(begin, eol - begin);
        hash_t htype = H_(type.c_str());
        ExecutionModel em = hstring_to_execution_model(htype);

        size_t next_line_pos = full_source.find_first_not_of("\r\n", eol);
        pos = full_source.find(type_token, next_line_pos);

        std::string segment = full_source.substr(
            next_line_pos, pos - (next_line_pos == std::string::npos ? full_source.size() - 1 : next_line_pos));
        sources.push_back(std::make_pair(em, handle_includes(base_directory, segment)));
    }
}

} // namespace slang
} // namespace erwin