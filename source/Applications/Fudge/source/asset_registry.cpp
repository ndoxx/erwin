#include "asset_registry.h"
#include "core/core.h"
#include "debug/logger.h"

#include <unordered_map>
#include <chrono>
#include <fstream>

using namespace erwin;

namespace fudge
{
namespace far
{

// Fudge Asset Registry file
struct FARHeader
{
    uint32_t magic;         // Magic number to check file format validity
    uint16_t version_major; // Version major number
    uint16_t version_minor; // Version minor number
    uint64_t num_entries;   // Number of entries
};
/*
struct FAREntry
{
	uint64_t asset_name; // Hashed asset name
	uint64_t timestamp;  // Combined hashed timestamps of all component unpacked assets
};*/

#define FAR_MAGIC 0x52414657 // ASCII(WFAR)
#define FAR_VERSION_MAJOR 1
#define FAR_VERSION_MINOR 0

static std::unordered_map<uint64_t, uint64_t> s_registry; // <name hash, timestamp hash>

template <class T>
inline void hash_combine(uint64_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

void load(const fs::path& registry_file_path)
{
	if(!fs::exists(registry_file_path))
	{
		DLOG("fudge",1) << "No asset registry file at the moment." << std::endl;
		DLOGI << "A registry file will be created." << std::endl;
		return;
	}

	DLOG("fudge",1) << "Loading asset registry:" << std::endl;
	DLOGI << WCC('p') << registry_file_path << std::endl;

    std::ifstream ifs(registry_file_path, std::ios::binary);
	
	FARHeader header;
    ifs.read(reinterpret_cast<char*>(&header), sizeof(FARHeader));

    // Sanity check
    W_ASSERT(header.magic == FAR_MAGIC, "Invalid FAR file: magic number mismatch.");
    W_ASSERT(header.version_major == FAR_VERSION_MAJOR, "Invalid FAR file: version (major) mismatch.");
    W_ASSERT(header.version_minor == FAR_VERSION_MINOR, "Invalid FAR file: version (minor) mismatch.");

    for(size_t ii=0; ii<header.num_entries; ++ii)
    {
        uint64_t key;
        uint64_t value;
    	ifs.read(reinterpret_cast<char*>(&key), sizeof(uint64_t));
    	ifs.read(reinterpret_cast<char*>(&value), sizeof(uint64_t));
        s_registry.insert(std::make_pair(key,value));
    }
}

void save(const fs::path& registry_file_path)
{
	DLOG("fudge",1) << "Saving asset registry:" << std::endl;
	DLOGI << WCC('p') << registry_file_path << std::endl;

	FARHeader header;
	header.magic = FAR_MAGIC;
	header.version_major = FAR_VERSION_MAJOR;
	header.version_minor = FAR_VERSION_MINOR;
	header.num_entries = s_registry.size();

    std::ofstream ofs(registry_file_path, std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(&header), sizeof(FARHeader));

    for(auto&& [key, value]: s_registry)
    {
    	ofs.write(reinterpret_cast<const char*>(&key), sizeof(uint64_t));
    	ofs.write(reinterpret_cast<const char*>(&value), sizeof(uint64_t));
    }
}

bool need_create(const fs::directory_entry& entry)
{
	// TODO:
	// Also check if output file already exists

	// Compute entry name hash
	uint64_t hname = std::hash<std::string>{}(entry.path().string());

	// Compute timestamp hash from entry
	uint64_t ts_hash = FAR_MAGIC;
	if(entry.is_directory())
	{
		// Iterate over all files in this directory and combine timestamp hashes
    	for(auto& sub_entry: fs::directory_iterator(entry.path()))
    	{
    		auto sub_path = sub_entry.path();
			auto ftime = fs::last_write_time(sub_path);
			std::time_t cftime = decltype(ftime)::clock::to_time_t(ftime);
			hash_combine(ts_hash, cftime);
    	}
	}
	else if(entry.is_regular_file())
	{
		// Compute timestamp hash
		auto ftime = fs::last_write_time(entry.path());
		std::time_t cftime = decltype(ftime)::clock::to_time_t(ftime);
		hash_combine(ts_hash, cftime);
	}

	// Check for an existing entry at that name
	// If no entry, file needs to be created
	auto it = s_registry.find(hname);
	if(it == s_registry.end())
	{
		s_registry.insert(std::make_pair(hname, ts_hash));
		return true;
	}

	// An entry exists, create file only if timestamp hash has changed
	if(it->second != ts_hash)
	{
		it->second = ts_hash;
		return true;
	}

	return false;
}

} // namespace far
} // namespace fudge