#include "project/project.h"
#include "core/application.h"
#include "core/config.h"
#include "filesystem/xml_file.h"
#include <kibble/logger/logger.h>
#include <kibble/string/string.h>

using namespace erwin;

namespace editor
{
namespace project
{

static ProjectSettings s_current_project;

bool load_project(const std::string& filepath)
{
    KLOGN("editor") << "Loading project." << std::endl;

    // Detect and set assets directory
    fs::path res_dir = WFS().regular_path(filepath).parent_path() / "assets";
    K_ASSERT(fs::exists(res_dir), "Cannot find 'assets' folder near project file.");
    WFS().alias_directory(res_dir, "res");

    // Read file and parse
    xml::XMLFile project_f(filepath);
    if(!project_f.read())
        return false;

    s_current_project.registry.deserialize(project_f, "project");
    s_current_project.project_file = filepath;
    s_current_project.root_folder = std::string(WFS().regular_path(filepath).parent_path());

    KLOG("editor", 0) << "Name: " << kb::KS_NAME_
                      << s_current_project.registry.get("project.project_name"_h, std::string()) << std::endl;
    KLOG("editor", 0) << "Import paths:" << std::endl;
    KLOGI << "Atlas:    " << kb::KS_PATH_ << s_current_project.registry.get("project.content.import.atlas"_h)
          << std::endl;
    KLOGI << "HDR:      " << kb::KS_PATH_ << s_current_project.registry.get("project.content.import.hdr"_h)
          << std::endl;
    KLOGI << "Material: " << kb::KS_PATH_ << s_current_project.registry.get("project.content.import.material"_h)
          << std::endl;
    KLOGI << "Font:     " << kb::KS_PATH_ << s_current_project.registry.get("project.content.import.font"_h)
          << std::endl;
    KLOGI << "Mesh:     " << kb::KS_PATH_ << s_current_project.registry.get("project.content.import.mesh"_h)
          << std::endl;
    KLOGI << "Script:   " << kb::KS_PATH_ << s_current_project.registry.get("project.content.import.script"_h)
          << std::endl;
    KLOGI << "Scene:    " << kb::KS_PATH_ << s_current_project.registry.get("project.content.import.scene"_h)
          << std::endl;
    KLOG("editor", 0) << "Export paths:" << std::endl;
    KLOGI << "Atlas:    " << kb::KS_PATH_ << s_current_project.registry.get("project.content.export.atlas"_h)
          << std::endl;
    KLOGI << "HDR:      " << kb::KS_PATH_ << s_current_project.registry.get("project.content.export.hdr"_h)
          << std::endl;
    KLOGI << "Material: " << kb::KS_PATH_ << s_current_project.registry.get("project.content.export.material"_h)
          << std::endl;
    KLOGI << "Font:     " << kb::KS_PATH_ << s_current_project.registry.get("project.content.export.font"_h)
          << std::endl;
    KLOGI << "Mesh:     " << kb::KS_PATH_ << s_current_project.registry.get("project.content.export.mesh"_h)
          << std::endl;
    KLOGI << "Script:   " << kb::KS_PATH_ << s_current_project.registry.get("project.content.export.script"_h)
          << std::endl;
    KLOGI << "Scene:    " << kb::KS_PATH_ << s_current_project.registry.get("project.content.export.scene"_h)
          << std::endl;

    // Save as last project for future auto load
    cfg::set("settings.project.last_project"_h, filepath);

    s_current_project.loaded = true;
    return true;
}

bool save_project()
{
    KLOGN("editor") << "Saving project." << std::endl;

    // Read file and parse
    xml::XMLFile project_f(s_current_project.project_file);
    if(!project_f.read())
        return false;

    s_current_project.registry.serialize(project_f, "project");

    return true;
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
    std::string dirpath;
    switch(dir_key)
    {
    case DK::ATLAS:
        dirpath = s_current_project.registry.get("project.content.export.atlas"_h);
        break;
    case DK::HDR:
        dirpath = s_current_project.registry.get("project.content.export.hdr"_h);
        break;
    case DK::MATERIAL:
        dirpath = s_current_project.registry.get("project.content.export.material"_h);
        break;
    case DK::FONT:
        dirpath = s_current_project.registry.get("project.content.export.font"_h);
        break;
    case DK::MESH:
        dirpath = s_current_project.registry.get("project.content.export.mesh"_h);
        break;
    case DK::SCRIPT:
        dirpath = s_current_project.registry.get("project.content.export.script"_h);
        break;
    case DK::SCENE:
        dirpath = s_current_project.registry.get("project.content.export.scene"_h);
        break;

    case DK::WORK_ATLAS:
        dirpath = s_current_project.registry.get("project.content.import.atlas"_h);
        break;
    case DK::WORK_HDR:
        dirpath = s_current_project.registry.get("project.content.import.hdr"_h);
        break;
    case DK::WORK_MATERIAL:
        dirpath = s_current_project.registry.get("project.content.import.material"_h);
        break;
    case DK::WORK_FONT:
        dirpath = s_current_project.registry.get("project.content.import.font"_h);
        break;
    case DK::WORK_MESH:
        dirpath = s_current_project.registry.get("project.content.import.mesh"_h);
        break;
    case DK::WORK_SCRIPT:
        dirpath = s_current_project.registry.get("project.content.import.script"_h);
        break;
    case DK::WORK_SCENE:
        dirpath = s_current_project.registry.get("project.content.import.scene"_h);
        break;
    }

    if(dirpath.empty())
        return s_current_project.root_folder;

    return dirpath;
}

} // namespace project
} // namespace editor