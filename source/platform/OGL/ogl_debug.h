#pragma once

#include "render/renderer_config.h"
#include "core/core.h"

namespace erwin
{

#if W_RC_BREAK_ON_API_ERROR
	#define GL_BEGIN_DBG() glGetError();
	#define GL_ASSERT_ON_ERROR() if(auto err = glGetError(); err != GL_NO_ERROR) { W_ASSERT_FMT(false, "An API error occurred: %d", err); }
	#define GL_END_DBG() GL_ASSERT_ON_ERROR()
#else
	#define GL_BEGIN_DBG()
	#define GL_ASSERT_ON_ERROR()
	#define GL_END_DBG()
#endif

} // namespace erwin