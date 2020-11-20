#pragma once
#include <filesystem>
#include <ostream>
#include <map>
#include "core/core.h"

namespace fs = std::filesystem;

namespace erwin
{

class WPath
{
public:
	WPath() = default;
    WPath(const std::string& proto, const fs::path& path);
    explicit WPath(const std::string& path);
    explicit WPath(const fs::path& path);

    /**
     * @brief      Set the root directory.
     *
     * @param[in]  path  The path
     */
    static inline void set_root_directory(const fs::path& path)
    {
        K_ASSERT(fs::exists(path), "Directory does not exist.");
        K_ASSERT(fs::is_directory(path), "Not a directory.");
        s_root_dir_ = path;
    }

    /**
     * @brief      Set the user directory.
     *
     * @param[in]  path  The path
     */
    static inline void set_user_directory(const fs::path& path)
    {
        K_ASSERT(fs::exists(path), "Directory does not exist.");
        K_ASSERT(fs::is_directory(path), "Not a directory.");
        s_user_dir_ = path;
    }

    /**
     * @brief      Set the game resources directory.
     *
     * @param[in]  path  The path
     */
    static inline void set_resource_directory(const fs::path& path)
    {
        K_ASSERT(fs::exists(path), "Directory does not exist.");
        K_ASSERT(fs::is_directory(path), "Not a directory.");
        s_resource_dir_ = path;
    }

    /**
     * @brief      Set the application configuration directory.
     *
     * @param[in]  path  The path
     */
    static inline void set_application_config_directory(const fs::path& path)
    {
        K_ASSERT(fs::exists(path), "Directory does not exist.");
        K_ASSERT(fs::is_directory(path), "Not a directory.");
        s_config_dir_ = path;
    }

    /**
     * @brief      Set the application resource directory.
     *
     * @param[in]  path  The path
     */
    static inline void set_application_resource_directory(const fs::path& path)
    {
        K_ASSERT(fs::exists(path), "Directory does not exist.");
        K_ASSERT(fs::is_directory(path), "Not a directory.");
        s_app_resource_dir_ = path;
    }

    /**
     * @brief      Set the system resource directory.
     *
     * @param[in]  path  The path
     */
    static inline void set_system_resource_directory(const fs::path& path)
    {
        K_ASSERT(fs::exists(path), "Directory does not exist.");
        K_ASSERT(fs::is_directory(path), "Not a directory.");
        s_sys_resource_dir_ = path;
    }

    /**
     * @brief      Set the system configuration directory.
     *
     * @param[in]  path  The path
     */
    static inline void set_system_config_directory(const fs::path& path)
    {
        K_ASSERT(fs::exists(path), "Directory does not exist.");
        K_ASSERT(fs::is_directory(path), "Not a directory.");
        s_sys_config_dir_ = path;
    }

    /**
     * @brief      Get the this relative to the base directory.
     *
     * @return     The relative path. Returned path is given relative to the root if
     *             no protocol was specified in the universal string.
     */
    fs::path relative() const;

    /**
     * @brief      Get the absolute path.
     *
     * @return     The absolute path.
     */
    inline const fs::path& absolute() const { return absolute_; }

    /**
     * @brief      Get the universal path as a string.
     *
     * @return     The universal path.
     */
    inline const std::string& universal() const { return universal_; }

    /**
     * @brief      Get the absolute path as a C string.
     *
     * @return     C string containing the absolute path.
     */
    inline const fs::path::value_type* c_str() const { return absolute_.c_str(); }

    /**
     * @brief      Get the absolute path as a string.
     *
     * @return     String containing the absolute path.
     */
    inline std::string string() const { return absolute_.string(); }

    /**
     * @brief      Get the file extension as a string.
     *
     * @return     The extension string (starting with a dot).
     */
    inline std::string extension() const { return absolute_.extension().string(); }


    /**
     * @brief      Get the stem as a string (file name without extension)
     *
     * @return     The stem
     */
    inline std::string stem() const { return absolute_.stem().string(); }

    /**
     * @brief      Get a hash of the universal path. Can be used to identify
     *             resources uniquely.
     *
     * @return     String hash of the universal path.
     */
    inline hash_t resource_id() const { return resource_id_; }

    /**
     * @brief      Get a hash of the file extension string.
     *
     * @return     The extension string hash.
     */
    inline hash_t extension_hash() const { return extension_id_; }

    /**
     * @brief      Determine if file extension matches a specified one.
     *
     * @param[in]  other  String hash of the extension to check against.
     *
     * @return     True if matches, False otherwise.
     */
    inline bool check_extension(hash_t other) const { return extension_id_ == other; }

    /**
     * @brief      Determine if file or directory exists.
     *
     * @return     True if exists, false otherwise.
     */
    inline bool exists() const { return fs::exists(absolute_); }

    /**
     * @brief      Determine if path is empty.
     *
     * @return     True if empty, false otherwise.
     */
    inline bool empty() const { return absolute_.empty(); }

    /**
     * @brief      Determines if path points to a file.
     *
     * @return     True if path points to a file, False otherwise.
     */
    inline bool is_file() const { return fs::is_regular_file(absolute_); }

    /**
     * @brief      Determines if path represents a directory.
     *
     * @return     True if path represents a directory, False otherwise.
     */
    inline bool is_directory() const { return fs::is_directory(absolute_); }

    /**
     * @brief      Concatenate this path with another one specified as a string.
     *             This path must point to a directory or an assertion will be triggered.
     *
     * @param[in]  rhs   Path to append to this one.
     *
     * @return     Result of the concatenation.
     */
    fs::path operator /(const std::string& rhs);

    friend std::ostream& operator <<(std::ostream& stream, const WPath& rhs);

private:
    static std::string make_universal(const std::string& proto, const fs::path& path);

private:
	static fs::path s_root_dir_;
    static fs::path s_user_dir_;
    static fs::path s_resource_dir_;
    static fs::path s_config_dir_;
    static fs::path s_app_resource_dir_;
    static fs::path s_sys_config_dir_;
    static fs::path s_sys_resource_dir_;
    static const std::map<hash_t, const fs::path&> s_protos;

    std::string universal_;
    fs::path absolute_;
    hash_t resource_id_  = 0;
    hash_t extension_id_ = 0;
    hash_t protocol_ = 0;
};


/**
 * @brief      Create a WPath object from a universal path specified as a string.
 *
 * @param[in]  path       The path specified as a string.
 * @param[in]  <unnamed>  String size.
 *
 * @return     A WPath object constructed from the input string.
 */
WPath operator "" _wp(const char* path, size_t);


} // namespace erwin