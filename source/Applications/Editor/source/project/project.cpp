#include "project/project.h"
#include "filesystem/filesystem.h"
#include "filesystem/xml_file.h"
#include "utils/string.h"
#include "debug/logger.h"

using namespace erwin;

namespace editor
{
namespace project
{

static bool parse_settings(void* /*node*/)
{
	// rapidxml::xml_node<>* xnode = static_cast<rapidxml::xml_node<>*>(node);

    return true;
}

bool load_global_settings()
{
	DLOGN("editor") << "Loading settings." << std::endl;

	fs::path user_filepath    = filesystem::get_user_dir() / "config/settings.xml";
	fs::path default_filepath = filesystem::get_root_dir() / "config/default_settings.xml";
	if(!filesystem::ensure_user_config(user_filepath, default_filepath))
		return false;

	// Read file and parse
	xml::XMLFile settings_f(user_filepath);
	if(!settings_f.read())
		return false;

	return parse_settings(settings_f.root);
}


} // namespace project
} // namespace editor