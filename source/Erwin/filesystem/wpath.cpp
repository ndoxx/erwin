#include "filesystem/wpath.h"
#include "debug/logger.h"

#include <regex>

namespace erwin
{

fs::path WPath::s_root_dir_;
fs::path WPath::s_user_dir_;
fs::path WPath::s_resource_dir_;
fs::path WPath::s_config_dir_;
fs::path WPath::s_app_resource_dir_;
fs::path WPath::s_sys_config_dir_;
fs::path WPath::s_sys_resource_dir_;

const std::map<hash_t, const fs::path&> WPath::s_protos = {
    {"usr"_h, WPath::s_user_dir_},
    {"res"_h, WPath::s_resource_dir_},
    {"cfg"_h, WPath::s_config_dir_},
    {"appres"_h, WPath::s_app_resource_dir_},
    {"sysres"_h, WPath::s_sys_resource_dir_},
    {"syscfg"_h, WPath::s_sys_config_dir_},
};

static const std::regex r_univ("^(.+?)://(.+)");

std::string WPath::make_universal(const std::string& proto, const fs::path& path)
{
    hash_t hproto = H_(proto.c_str());
    auto findit = s_protos.find(hproto);
    W_ASSERT(findit != s_protos.end(), "Unknown protocol.");
    return proto + "://" + fs::relative(path, findit->second).string();
}

WPath::WPath(const std::string& proto, const fs::path& path) : WPath(make_universal(proto, path)) {}

WPath::WPath(const std::string& path) : universal_(path), resource_id_(H_(universal_.c_str()))
{
    std::smatch match;
    if(std::regex_search(universal_, match, r_univ))
    {
        auto protocol = match[1].str();
        auto relative = match[2].str();
        protocol_ = H_(protocol.c_str());

        switch(protocol_)
        {
        case "usr"_h: {
            W_ASSERT(!s_user_dir_.empty(), "User directory is not initialized.");
            absolute_ = s_user_dir_ / relative;
            break;
        }
        case "res"_h: {
            W_ASSERT(!s_resource_dir_.empty(), "Resource base directory is not initialized.");
            absolute_ = s_resource_dir_ / relative;
            break;
        }
        case "cfg"_h: {
            W_ASSERT(!s_config_dir_.empty(), "Client config directory is not initialized.");
            absolute_ = s_config_dir_ / relative;
            break;
        }
        case "appres"_h: {
            W_ASSERT(!s_app_resource_dir_.empty(), "Application resource directory is not initialized.");
            absolute_ = s_app_resource_dir_ / relative;
            break;
        }
        case "sysres"_h: {
            W_ASSERT(!s_sys_resource_dir_.empty(), "System resource directory is not initialized.");
            absolute_ = s_sys_resource_dir_ / relative;
            break;
        }
        case "syscfg"_h: {
            W_ASSERT(!s_sys_config_dir_.empty(), "System config directory is not initialized.");
            absolute_ = s_sys_config_dir_ / relative;
            break;
        }
        default: {
            DLOGE("ios") << "Unrecognized protocol: " << protocol << std::endl;
            DLOGI << "Defaulting to root directory." << std::endl;
            absolute_ = s_root_dir_ / relative;
        }
        }
    }
    else
        absolute_ = universal_;

    extension_id_ = H_(absolute_.extension().c_str());
}

WPath::WPath(const fs::path& path)
    : universal_(path.string()), absolute_(path), resource_id_(H_(universal_.c_str())),
      extension_id_(H_(absolute_.extension().c_str()))
{}

fs::path WPath::relative() const
{
    switch(protocol_)
    {
    case "usr"_h:
        return fs::relative(absolute_, s_user_dir_);
    case "res"_h:
        return fs::relative(absolute_, s_resource_dir_);
    case "cfg"_h:
        return fs::relative(absolute_, s_config_dir_);
    case "appres"_h:
        return fs::relative(absolute_, s_app_resource_dir_);
    case "sysres"_h:
        return fs::relative(absolute_, s_sys_resource_dir_);
    case "syscfg"_h:
        return fs::relative(absolute_, s_sys_config_dir_);
    default:
        return fs::relative(absolute_, s_root_dir_);
    }
}

fs::path WPath::operator/(const std::string& rhs)
{
    W_ASSERT(is_directory(), "Cannot concatenate paths, LHS is not a directory.");
    return absolute_ / rhs;
}

std::ostream& operator<<(std::ostream& stream, const WPath& rhs)
{
    stream << WCC('p') << "\"" << rhs.universal_ << "\"" << WCC(0);
    return stream;
}

WPath operator"" _wp(const char* path, size_t) { return WPath(std::string(path)); }

} // namespace erwin