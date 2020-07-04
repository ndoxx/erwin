#include "project/project.h"
#include "core/config.h"
#include "debug/logger.h"
#include "filesystem/filesystem.h"
#include "filesystem/xml_file.h"
#include "utils/string.h"

using namespace erwin;

namespace editor
{
namespace project
{

static ProjectSettings s_current_project;

bool load_project(const FilePath& filepath)
{
    DLOGN("editor") << "Loading project." << std::endl;

    // Read file and parse
    xml::XMLFile project_f(filepath);
    if(!project_f.read())
        return false;

    s_current_project.registry.deserialize(project_f, "project");
    s_current_project.project_file = filepath.full_path();
    s_current_project.root_folder = filepath.base_path();

    DLOG("editor", 0) << "Name: " << WCC('n') << s_current_project.registry.get("project.project_name"_h, std::string())
                      << std::endl;
    DLOG("editor", 0) << "Import paths:" << std::endl;
    DLOGI << "Atlas:    " << WCC('p') << s_current_project.registry.get("project.content.import.atlas"_h) << std::endl;
    DLOGI << "HDR:      " << WCC('p') << s_current_project.registry.get("project.content.import.hdr"_h) << std::endl;
    DLOGI << "Material: " << WCC('p') << s_current_project.registry.get("project.content.import.material"_h)
          << std::endl;
    DLOGI << "Font:     " << WCC('p') << s_current_project.registry.get("project.content.import.font"_h) << std::endl;
    DLOGI << "Mesh:     " << WCC('p') << s_current_project.registry.get("project.content.import.mesh"_h) << std::endl;
    DLOGI << "Scene:    " << WCC('p') << s_current_project.registry.get("project.content.import.scene"_h) << std::endl;
    DLOG("editor", 0) << "Export paths:" << std::endl;
    DLOGI << "Atlas:    " << WCC('p') << s_current_project.registry.get("project.content.export.atlas"_h) << std::endl;
    DLOGI << "HDR:      " << WCC('p') << s_current_project.registry.get("project.content.export.hdr"_h) << std::endl;
    DLOGI << "Material: " << WCC('p') << s_current_project.registry.get("project.content.export.material"_h)
          << std::endl;
    DLOGI << "Font:     " << WCC('p') << s_current_project.registry.get("project.content.export.font"_h) << std::endl;
    DLOGI << "Mesh:     " << WCC('p') << s_current_project.registry.get("project.content.export.mesh"_h) << std::endl;
    DLOGI << "Scene:    " << WCC('p') << s_current_project.registry.get("project.content.export.scene"_h) << std::endl;

    // Save as last project for future auto load
    cfg::set("settings.project.last_project"_h, filepath);

    s_current_project.loaded = true;
    return true;
}

bool save_project()
{
    DLOGN("editor") << "Saving project." << std::endl;

    // Read file and parse
    xml::XMLFile project_f(FilePath(s_current_project.project_file));
    if(!project_f.read())
        return false;

    s_current_project.registry.serialize(project_f, "project");

    return true;
}

bool is_loaded()
{
    return s_current_project.loaded;
}

void close_project()
{
    DLOGN("editor") << "Closing project." << std::endl;

    s_current_project.registry.clear();
    s_current_project.project_file = "";
    s_current_project.root_folder = "";
    s_current_project.loaded = false;
}

const ProjectSettings& get_project_settings() { return s_current_project; }

FilePath asset_dir(DK dir_key)
{
    FilePath dirpath;
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
    case DK::WORK_SCENE:
        dirpath = s_current_project.registry.get("project.content.import.scene"_h);
        break;
    }

    if(dirpath.empty())
        return FilePath(s_current_project.root_folder);

    return dirpath;
}

FilePath asset_path(DK dir_key, const fs::path& file_path)
{
    auto dirpath = asset_dir(dir_key);
    return FilePath(dirpath.base_path(), dirpath.file_path() / file_path);
}

} // namespace project
} // namespace editor