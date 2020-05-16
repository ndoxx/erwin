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