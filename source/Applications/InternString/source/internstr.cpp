#include <iostream>
#include <fstream>
#include <filesystem>
#include <regex>
#include <string>

#ifdef __linux__
    #include <unistd.h>
    #include <climits>
#elif _WIN32

#endif

#include "core/wtypes.h"

using namespace erwin;
namespace fs = std::filesystem;

static fs::path self_path_;
static fs::path root_path_;
static fs::path src_path_;
static fs::path conf_path_;

// Non greedy regex that matches the H_("any_str") macro
static std::regex hash_str_tag("H_\\(\"([a-zA-Z0-9_\\.]+?)\"\\)");
// Non greedy regex that matches the "any_str"_h string literal
// BUG: seems to be greedy anyway -> will generate a faulty file
static std::regex hash_str_literal_tag("\"([a-zA-Z0-9_\\.]+?)\"_h");
// Associates hashes to original strings
static std::map<hash_t, std::string> intern_strings_;

// Get path to executable
static fs::path get_selfpath()
{
#ifdef __linux__
    char buff[PATH_MAX];
    std::size_t len = ::readlink("/proc/self/exe", buff, sizeof(buff)-1);
    if (len != -1)
    {
        buff[len] = '\0';
        return fs::path(buff);
    }
    else
    {
        std::cerr << "Cannot read self path using readlink." << std::endl;
        return fs::path();
    }
#elif _WIN32

    std::cerr << "get_selfpath() not yet implemented." << std::endl;
    return fs::path();

#endif
}

static void register_intern_string(const std::string& intern)
{
    hash_t hash_intern = H_(intern.c_str()); // Hashed string

    auto it = intern_strings_.find(hash_intern);
    if(it == intern_strings_.end())
    {
        std::cout << "-> " << hash_intern << " -> " + intern << std::endl;
        intern_strings_.insert(std::make_pair(hash_intern, intern));
    }
    else if(it->second.compare(intern)) // Detect hash collision
    {
        std::cerr << "Hash collision detected:" << std::endl;
        std::cout << "-> " << it->second << " -> " << it->first << std::endl;
        std::cout << "-> " << intern     << " -> " << hash_intern << std::endl;
        do
        {
            std::cout << '\n' << "Press ENTER to continue...";
        } while (std::cin.get() != '\n');
    }
}

// Parse a single file for hash macros
static void parse_entry(const fs::directory_entry& entry)
{
    if(entry.is_directory())
        return;

    std::string extension = entry.path().extension().string();
    bool valid = (!extension.compare(".cpp")) ||
                 (!extension.compare(".h"))   ||
                 (!extension.compare(".hpp"));
    if(!valid)
        return;

    // * Copy file to string
    std::cout << "  * " << entry.path().string() << std::endl;
    std::ifstream ifs(entry.path());
    if(!ifs.is_open())
    {
        std::cerr << "Unable to open file. Skipping." << std::endl;
        return;
    }
    std::string source_str((std::istreambuf_iterator<char>(ifs)),
                            std::istreambuf_iterator<char>());


    // * Match string hash macros and update table
    {
        std::regex_iterator<std::string::iterator> it(source_str.begin(), source_str.end(), hash_str_tag);
        std::regex_iterator<std::string::iterator> end;

        while(it != end)
        {
            std::string intern((*it)[1]); // The intern string
            register_intern_string(intern);
            ++it;
        }
    }
    {
        std::regex_iterator<std::string::iterator> it(source_str.begin(), source_str.end(), hash_str_literal_tag);
        std::regex_iterator<std::string::iterator> end;

        while(it != end)
        {
            std::string intern((*it)[1]); // The intern string
            register_intern_string(intern);
            ++it;
        }
    }
}

int main()
{
    std::cout << "Intern string utilitary launched." << std::endl;

    // * Locate executable path, root directory, config directory and source directories
    std::cout << "Locating sources." << std::endl;
    self_path_ = get_selfpath();
    root_path_ = self_path_.parent_path().parent_path();

    std::cout << "-> " << "Self path: " << self_path_.string() << std::endl;
    std::cout << "-> " << "Root path: " << root_path_.string() << std::endl;

    src_path_ = root_path_ / "source";
    conf_path_ = root_path_ / "config";

    if(!fs::exists(src_path_))
    {
        std::cerr << "Unable to locate source path." << std::endl;
        return -1;
    }
    if(!fs::exists(conf_path_))
    {
        std::cerr << "Unable to locate config path." << std::endl;
        return -1;
    }

    std::cout << "-> " << "Source path: "  + src_path_.string() << std::endl;

    std::cout << "Parsing source code for hash string occurrences." << std::endl;
    auto erwin_path = src_path_ / "Erwin";
    auto platform_path = src_path_ / "platform";
    auto app_path = src_path_ / "Applications";
    for(const auto& entry: fs::recursive_directory_iterator(erwin_path))
        parse_entry(entry);
    for(const auto& entry: fs::recursive_directory_iterator(platform_path))
        parse_entry(entry);
    for(const auto& entry: fs::recursive_directory_iterator(app_path))
        parse_entry(entry);
    
    // * Write intern string table to text file
    fs::path txt_path = conf_path_ / "intern_strings.txt";
    std::cout << "Exporting intern string table to text file." << std::endl;
    std::cout << "-> " << txt_path.string() << std::endl;
    std::ofstream out_txt(txt_path);
    if(out_txt.is_open())
    {
        for(auto&& [key,value]: intern_strings_)
        {
            out_txt << key << " " << value << std::endl;
        }
        out_txt.close();
    }
    else
    {
        std::cerr << "Unable to open output text file." << std::endl;
    }
    std::cout << "Done."<< std::endl;

    return 0;
}
