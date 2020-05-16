#include "project/project.h"
#include "core/config.h"
#include "filesystem/filesystem.h"
#include "filesystem/xml_file.h"
#include "utils/string.h"
#include "debug/logger.h"

using namespace erwin;

namespace editor
{
namespace project
{

static ProjectSettings s_current_project;

static const fs::path user_settings_path = "config/settings.xml";
static const fs::path default_settings_path = "source/Applications/Editor/config/default_settings.xml";

bool load_global_settings()
{
	DLOGN("editor") << "Loading settings." << std::endl;

	fs::path user_filepath    = filesystem::get_user_dir() / user_settings_path;
	fs::path default_filepath = filesystem::get_root_dir() / default_settings_path;
	if(!filesystem::ensure_user_config(user_filepath, default_filepath))
		return false;

	// Read file and parse
	return cfg::load(user_filepath);
}

bool save_global_settings()
{
	fs::path filepath = filesystem::get_user_dir() / user_settings_path;
	DLOGN("config") << "Saving settings:" << std::endl;
	DLOGI << WCC('p') << filepath << std::endl;

	return cfg::save(filepath);
}

void set_current_project_file(const fs::path& filepath)
{
	cfg::set("settings.project.last_project"_h, filepath);
}





static bool parse_project(void* /*node*/)
{

    return true;
}

bool load_project(const fs::path& filepath)
{
	DLOGN("editor") << "Loading project." << std::endl;

	// Read file and parse
	xml::XMLFile project_f(filepath);
	if(!project_f.read())
		return false;

	bool success = parse_project(project_f.root);

	if(success)
	{
		s_current_project.project_file = filepath;
	}

	return success;
}

bool save_project()
{

	return true;
}

const ProjectSettings& get_project_settings()
{
	return s_current_project;
}


} // namespace project
} // namespace editor