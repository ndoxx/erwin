#pragma once

#include <filesystem>
#include <ostream>
#include "core/hashstr.h"

namespace fs = std::filesystem;

namespace erwin
{

class FilePath
{
public:
    // TODO: Extend for paths inside a resource file

	FilePath() = default;
    explicit FilePath(const std::string& full_path);
    explicit FilePath(const fs::path& full_path);
    FilePath(const fs::path& base_dir, const fs::path& file_path);

    inline bool exists() const { return fs::exists(full_path_); }
    inline bool empty() const { return full_path_.empty(); }
    inline const fs::path& full_path() const { return full_path_; }
    inline const fs::path& file_path() const { return file_path_; }
    inline const fs::path& base_path() const { return base_dir_; }
    inline const fs::path::value_type* c_str() const { return full_path_.c_str(); }
    inline std::string string() const { return full_path_.string(); }
    inline std::string extension() const { return file_path_.extension().string(); }
    inline hash_t extension_hash() const { return extension_id_; }
    inline hash_t resource_id() const { return resource_id_; }
    inline bool check_extension(hash_t other) const { return extension_id_ == other; }

    friend std::ostream& operator <<(std::ostream& stream, const FilePath& rhs);

private:
    fs::path full_path_;
    fs::path base_dir_;
    fs::path file_path_;
    hash_t resource_id_ = 0;
    hash_t extension_id_ = 0;
};


} // namespace erwin