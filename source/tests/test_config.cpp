#include "../core/config.h"
#include "../debug/logger.h"

#include "../core/hetero.hpp"

using namespace erwin;

struct partial_visitor: visitor_base<int, bool, std::string>
{
    template<class T>
    void operator()(const std::string& key, T& value) 
    {
    	DLOG("core",1) << key << ": " << value << std::endl;
    }
};

struct float_visitor: visitor_base<float>
{
    template<class T>
    void operator()(T& value) 
    {
    	DLOG("core",1) << value << std::endl;
    }
};

int main()
{
    WLOGGER.attach_all("ConsoleSink", std::make_unique<dbg::ConsoleSink>());
    WLOGGER.spawn();
    DLOG("core",1) << "CONFIG TEST" << std::endl;

    //Config cfg;

    {
	    HeteroMap hm;
	    hm.set<int>("frame_rate", 60);
	    hm.set<float>("speed", 1.864f);
	    hm.set<bool>("dead", true);
	    hm.set<int>("max_time", 25);
	    hm.set<float>("density", 1.22345f);
	    hm.set<float>("angle_rad", 2.677955f);
	    hm.set<std::string>("path", "path/to/some/file");

	    hm.visit(partial_visitor{});
	    hm.visit(float_visitor{});

    	DLOG("core",1) << "get() max_time: " << hm.get<int>("max_time") << std::endl;
    	DLOG("core",1) << "get() path: " << hm.get<std::string>("path") << std::endl;
    	//DLOG("core",1) << "get ghost: " << hm.get<int>("ghost") << std::endl;
    	//DLOG("core",1) << "get ghost: " << hm.get<double>("ghost") << std::endl;
    }

	return 0;
}