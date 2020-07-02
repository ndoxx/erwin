#include "filesystem/file_path.h"

namespace erwin
{

FilePath::FilePath(const std::string& full_path):
full_path_(full_path),
base_dir_(full_path_.parent_path()),
file_path_(full_path_.filename()),
resource_id_(H_(file_path_.c_str())),
extension_id_(H_(file_path_.extension().c_str()))
{}

FilePath::FilePath(const fs::path& full_path):
full_path_(full_path),
base_dir_(full_path_.parent_path()),
file_path_(full_path_.filename()),
resource_id_(H_(file_path_.c_str())),
extension_id_(H_(file_path_.extension().c_str()))
{}

FilePath::FilePath(const fs::path& base_dir, const fs::path& file_path):
full_path_(base_dir / file_path),
base_dir_(base_dir),
file_path_(file_path),
resource_id_(H_(file_path_.c_str())),
extension_id_(H_(file_path_.extension().c_str()))
{}

std::ostream& operator <<(std::ostream& stream, const FilePath& rhs)
{
	stream << rhs.full_path();
	return stream;
}


} // namespace erwin