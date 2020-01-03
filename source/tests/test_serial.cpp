#include <filesystem>
#include <fstream>
#include <ostream>

#include "cereal/types/unordered_map.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/archives/binary.hpp"
#include "cereal/archives/xml.hpp"
#include "cereal/archives/json.hpp"

#include "debug/logger.h"
#include "debug/logger_sink.h"
#include "debug/logger_thread.h"

using namespace erwin;
namespace fs = std::filesystem;

class vec3_stub
{
public:
	vec3_stub(): x_(0.f), y_(0.f), z_(0.f) {}
	vec3_stub(float x,float y, float z): x_(x), y_(y), z_(z) {}

    friend std::ostream& operator<< (std::ostream& stream, const vec3_stub& value);

	template <class Archive>
	void serialize(Archive & ar)
	{
		ar(CEREAL_NVP(x_), 
		   CEREAL_NVP(y_),
		   CEREAL_NVP(z_));
	}

private:
	float x_;
	float y_;
	float z_;
};

std::ostream& operator<< (std::ostream& stream, const vec3_stub& value)
{
    stream << "(" << value.x_ << "," << value.y_ << "," << value.z_ << ")";
    return stream;
}

class quat_stub
{
public:
	quat_stub(): x_(0.f), y_(0.f), z_(0.f), w_(1.f) {}
	quat_stub(float x,float y, float z, float w): x_(x), y_(y), z_(z), w_(w) {}
    
    friend std::ostream& operator<< (std::ostream& stream, const quat_stub& value);

	template <class Archive>
	void serialize(Archive & ar)
	{
		ar(CEREAL_NVP(x_), 
		   CEREAL_NVP(y_), 
		   CEREAL_NVP(z_), 
		   CEREAL_NVP(w_));
	}

private:
	float x_;
	float y_;
	float z_;
	float w_;
};

std::ostream& operator<< (std::ostream& stream, const quat_stub& value)
{
    stream << "[" << value.x_ << "," << value.y_ << "," << value.z_ << "," << value.w_ << "]";
    return stream;
}

class cam_stub
{
public:
	cam_stub() = default;

	cam_stub(const vec3_stub& pos, const quat_stub& ori):
	position_(pos),
	orientation_(ori)
	{

	}

	template <class Archive>
	void save(Archive & ar) const
	{
    	DLOG("core",1) << "saving cam: " << position_ << " " << orientation_ << std::endl;
		ar(CEREAL_NVP(position_), 
		   CEREAL_NVP(orientation_));
	}

	template <class Archive>
	void load(Archive & ar)
	{
		ar(position_, orientation_);
    	DLOG("core",1) << "loading cam: " << position_ << " " << orientation_ << std::endl;
	}

private:
	vec3_stub position_;
	quat_stub orientation_;
};

int main()
{
    WLOGGER.attach_all("ConsoleSink", std::make_unique<dbg::ConsoleSink>());
    WLOGGER.spawn();
    DLOG("core",1) << "CEREAL TEST" << std::endl;

    // cam file exists, load it
    if(fs::exists("out.d/cam.cer"))
    {
		std::ifstream isb("out.d/cam.cer", std::ios::binary);
    	cereal::BinaryInputArchive iarchive(isb);
    	cam_stub cam;
    	iarchive(cam);
    }
    else
    {
		cam_stub cam(vec3_stub(1.5f,-2.8f,3.2f), quat_stub(1.2f,1.0f,0.2f,1.0f));

		std::ofstream osb("out.d/cam.cer", std::ios::binary);
		cereal::BinaryOutputArchive archive_b(osb);
		archive_b(cereal::make_nvp("cam0", cam));
		osb.close();

		std::ofstream osx("out.d/cam.xml");
		cereal::XMLOutputArchive archive_x(osx);
		archive_x(cereal::make_nvp("cam0", cam));
		osx.close();

		std::ofstream osj("out.d/cam.json");
		cereal::JSONOutputArchive archive_j(osj);
		archive_j(cereal::make_nvp("cam0", cam));
		osj.close();
	}

	return 0;
}