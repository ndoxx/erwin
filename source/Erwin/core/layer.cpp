#include "layer.h"

namespace erwin
{


Layer::Layer(const std::string& debug_name):
debug_name_(debug_name),
enabled_(true)
{
	
}

Layer::~Layer()
{
	
}

void Layer::update()
{
	if(enabled_)
		on_update();
}


} // namespace erwin