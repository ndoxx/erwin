#include "project/project.h"
#include "core/application.h"
#include <kibble/logger/logger.h>
#include <kibble/string/string.h>

using namespace erwin;

namespace editor
{
namespace project
{

static ProjectSettings s_current_project;

void load_project(const std::string& filepath)
{
    KLOGN("editor") << "Loading project." << std::endl;

    // Detect and set assets directory
    fs::path res_dir = WFS_.regular_path(filepath).parent_path() / "assets";
    K_ASSERT(fs::exists(res_dir), "Cannot find 'assets' folder near project file.");
    WFS_.alias_directory(res_dir, "res");

    auto& reg = s_current_project.registry;
    reg.load_toml(filepath, "project");
    s_current_project.project_file = filepath;
    s_current_project.root_folder = std::string(WFS_.regular_path(filepath).parent_path());

    KLOG("editor", 0) << "Name: " << kb::KS_NAME_ << reg.get<std::string>("project.project_name"_h, "") << std::endl;
    KLOG("editor", 0) << "Import paths:" << std::endl;
    KLOGI << "Atlas:    " << kb::KS_PATH_ << reg.get<std::string>("project.content.import.atlas"_h, "") << std::endl;
    KLOGI << "HDR:      " << kb::KS_PATH_ << reg.get<std::string>("project.content.import.hdr"_h, "") << std::endl;
    KLOGI << "Material: " << kb::KS_PATH_ << reg.get<std::string>("project.content.import.material"_h, "") << std::endl;
    KLOGI << "Font:     " << kb::KS_PATH_ << reg.get<std::string>("project.content.import.font"_h, "") << std::endl;
    KLOGI << "Mesh:     " << kb::KS_PATH_ << reg.get<std::string>("project.content.import.mesh"_h, "") << std::endl;
    KLOGI << "Script:   " << kb::KS_PATH_ << reg.get<std::string>("project.content.import.script"_h, "") << std::endl;
    KLOGI << "Scene:    " << kb::KS_PATH_ << reg.get<std::string>("project.content.import.scene"_h, "") << std::endl;
    KLOG("editor", 0) << "Export paths:" << std::endl;
    KLOGI << "Atlas:    " << kb::KS_PATH_ << reg.get<std::string>("project.content.export.atlas"_h, "") << std::endl;
    KLOGI << "HDR:      " << kb::KS_PATH_ << reg.get<std::string>("project.content.export.hdr"_h, "") << std::endl;
    KLOGI << "Material: " << kb::KS_PATH_ << reg.get<std::string>("project.content.export.material"_h, "") << std::endl;
    KLOGI << "Font:     " << kb::KS_PATH_ << reg.get<std::string>("project.content.export.font"_h, "") << std::endl;
    KLOGI << "Mesh:     " << kb::KS_PATH_ << reg.get<std::string>("project.content.export.mesh"_h, "") << std::endl;
    KLOGI << "Script:   " << kb::KS_PATH_ << reg.get<std::string>("project.content.export.script"_h, "") << std::endl;
    KLOGI << "Scene:    " << kb::KS_PATH_ << reg.get<std::string>("project.content.export.scene"_h, "") << std::endl;

    // Save as last project for future auto load
    CFG_.set("settings.project.last_project"_h, filepath);

    s_current_project.loaded = true;
}

void save_project()
{
    KLOGN("editor") << "Saving project." << std::endl;
    s_current_project.registry.save_toml(s_current_project.project_file, "project");
}

bool is_loaded() { return s_current_project.loaded; }

void close_project()
{
    KLOGN("editor") << "Closing project." << std::endl;

    s_current_project.registry.clear();
    s_current_project.project_file = {};
    s_current_project.root_folder = {};
    s_current_project.loaded = false;
}

const ProjectSettings& get_project_settings() { return s_current_project; }

std::string asset_dir(DK dir_key)
{
    const auto& reg = s_current_project.registry;
    const auto& def = s_current_project.root_folder;
    switch(dir_key)
    {
    case DK::ATLAS:
        return reg.get<std::string>("project.content.export.atlas"_h, def);
    case DK::HDR:
        return reg.get<std::string>("project.content.export.hdr"_h, def);
    case DK::MATERIAL:
        return reg.get<std::string>("project.content.export.material"_h, def);
    case DK::FONT:
        return reg.get<std::string>("project.content.export.font"_h, def);
    case DK::MESH:
        return reg.get<std::string>("project.content.export.mesh"_h, def);
    case DK::SCRIPT:
        return reg.get<std::string>("project.content.export.script"_h, def);
    case DK::SCENE:
        return reg.get<std::string>("project.content.export.scene"_h, def);

    case DK::WORK_ATLAS:
        return reg.get<std::string>("project.content.import.atlas"_h, def);
    case DK::WORK_HDR:
        return reg.get<std::string>("project.content.import.hdr"_h, def);
    case DK::WORK_MATERIAL:
        return reg.get<std::string>("project.content.import.material"_h, def);
    case DK::WORK_FONT:
        return reg.get<std::string>("project.content.import.font"_h, def);
    case DK::WORK_MESH:
        return reg.get<std::string>("project.content.import.mesh"_h, def);
    case DK::WORK_SCRIPT:
        return reg.get<std::string>("project.content.import.script"_h, def);
    case DK::WORK_SCENE:
        return reg.get<std::string>("project.content.import.scene"_h, def);
    }

    return def;
}

} // namespace project
} // namespace editor