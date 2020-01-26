#pragma once

#include <limits>
#include <cstring>
#include <array>
#include "glm/glm.hpp"
#ifdef W_DEBUG
	#include <ostream>
#endif

namespace erwin
{

// TMP: For now, only contains generic structures dealing with meshes, no real mesh data structure


struct Extent
{
	Extent()
	{
		value[0] = std::numeric_limits<float>::max();
		value[1] = -std::numeric_limits<float>::max();
		value[2] = std::numeric_limits<float>::max();
		value[3] = -std::numeric_limits<float>::max();
		value[4] = std::numeric_limits<float>::max();
		value[5] = -std::numeric_limits<float>::max();
	}

	Extent(const Extent& other)
	{
		std::memcpy(value, other.value, 6*sizeof(float));
	}

	inline void operator =(const Extent& other)
	{
		std::memcpy(value, other.value, 6*sizeof(float));
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
	friend std::ostream& operator <<(std::ostream& stream, const Extent& extent);
#endif

	float value[6];
};

namespace bound
{

static inline void to_model_space_vertices(const Extent& extent, std::array<glm::vec3, 8>& vertices)
{
	vertices[0] = { extent[1], extent[2], extent[5] };
	vertices[1] = { extent[1], extent[3], extent[5] };
	vertices[2] = { extent[0], extent[3], extent[5] };
	vertices[3] = { extent[0], extent[2], extent[5] };
	vertices[4] = { extent[1], extent[2], extent[4] };
	vertices[5] = { extent[1], extent[3], extent[4] };
	vertices[6] = { extent[0], extent[3], extent[4] };
	vertices[7] = { extent[0], extent[2], extent[4] };
}

static inline std::pair<glm::vec3, glm::vec3> to_vectors(const Extent& extent)
{
	return
	{
		// mid-point
		{ (extent[1]+extent[0])*0.5f,
		  (extent[3]+extent[2])*0.5f,
		  (extent[5]+extent[4])*0.5f },
		// half
		{ (extent[1]-extent[0])*0.5f,
		  (extent[3]-extent[2])*0.5f,
		  (extent[5]-extent[4])*0.5f }
	};
}


} // namespace bound

} // namespace erwin