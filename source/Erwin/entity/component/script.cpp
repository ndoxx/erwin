#include "entity/component/script.h"

#include <fstream>
#include <sstream>

namespace erwin
{

ComponentScript::ComponentScript(const std::string& universal_path):
file_path(universal_path)
{
	detect_entry_point();
}

ComponentScript::ComponentScript(const WPath& path):
file_path(path)
{
	detect_entry_point();
}

void ComponentScript::detect_entry_point()
{
    std::ifstream ifs(file_path.absolute());
    std::string line, pragma, kw, arg;
    std::getline(ifs, line);
    std::stringstream ss(line);
    ss >> pragma >> kw >> arg;
    if(!pragma.compare("#pragma"))
        if(!kw.compare("entry_point"))
            entry_point = arg;
}


} // namespace erwin