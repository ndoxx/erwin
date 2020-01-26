#pragma once

#include <limits>
#include "glm/glm.hpp"
#ifdef W_DEBUG
	#include <ostream>
#endif

namespace erwin
{

// TMP: For now, only contains generic structures dealing with meshes, no real mesh data structure


struct Dimensions
{
	Dimensions()
	{
		value[0] = std::numeric_limits<float>::max();
		value[1] = -std::numeric_limits<float>::max();
		value[2] = std::numeric_limits<float>::max();
		value[3] = -std::numeric_limits<float>::max();
		value[4] = std::numeric_limits<float>::max();
		value[5] = -std::numeric_limits<float>::max();
	}

	inline void update(const glm::vec3& position)
	{
		for(int dd=0; dd<3; ++dd)
		{
			value[2*dd]   = fmin(value[2*dd],   position[dd]); // x/y/z-min
			value[2*dd+1] = fmax(value[2*dd+1], position[dd]); // x/y/z-max
		}
	}

	inline float& operator[](int index)      { return value[index]; }
	inline float operator[](int index) const { return value[index]; }
	inline float& xmin() { return value[0]; }
	inline float& xmaw() { return value[1]; }
	inline float& ymin() { return value[2]; }
	inline float& ymax() { return value[3]; }
	inline float& zmin() { return value[4]; }
	inline float& zmax() { return value[5]; }

#ifdef W_DEBUG
	friend std::ostream& operator <<(std::ostream& stream, const Dimensions& dimensions);
#endif

	float value[6];
};

} // namespace erwin