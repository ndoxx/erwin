#include "render/render_state.h"
#include <sstream>

namespace erwin
{

std::string RenderState::to_string()
{
	std::stringstream ss;

	// Blend state
	ss << "Bl:";
	switch(blend_state)
	{
		case BlendState::Opaque: ss << "O"; break;
		case BlendState::Alpha:  ss << "A"; break;
		case BlendState::Light:  ss << "L"; break;
	}

	// Rasterizer state
	ss << "|Cl:";
	if(rasterizer_state.clear_flags == ClearFlags::CLEAR_NONE)
		ss << "0";
	else
	{
		if(rasterizer_state.clear_flags & ClearFlags::CLEAR_COLOR_FLAG)   ss << "C";
		if(rasterizer_state.clear_flags & ClearFlags::CLEAR_DEPTH_FLAG)   ss << "D";
		if(rasterizer_state.clear_flags & ClearFlags::CLEAR_STENCIL_FLAG) ss << "S";
	}
	ss << "|Cu:";
	switch(rasterizer_state.cull_mode)
	{
		case CullMode::None:  ss << "0"; break;
    	case CullMode::Front: ss << "F"; break;
    	case CullMode::Back:  ss << "B"; break;
    }

    // Depth/Stencil state
	if(depth_stencil_state.depth_test_enabled)
	{
		ss << "|DF:";
		switch(depth_stencil_state.depth_func)
		{
			case DepthFunc::Less:   ss << "L";  break;
	    	case DepthFunc::LEqual: ss << "LE"; break;
		}
	}
	if(depth_stencil_state.stencil_test_enabled)
	{
		ss << "|SF:";
		switch(depth_stencil_state.stencil_func)
		{
		    case StencilFunc::Always:	ss << "A";  break;
		    case StencilFunc::NotEqual:	ss << "NE"; break;
		}
		ss << "|SO:";
		switch(depth_stencil_state.stencil_operator)
		{
		    case StencilOperator::LightVolume: ss << "LV";  break;
		}
	}

	// Render target
	ss << "|RT:" << render_target;

	return ss.str();
}


} // namespace erwin