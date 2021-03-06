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

struct Extent
{
	Extent(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
	{
		value[0] = xmin;
		value[1] = xmax;
		value[2] = ymin;
		value[3] = ymax;
		value[4] = zmin;
		value[5] = zmax;
	}

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

	inline Extent& operator =(const Extent& other)
	{
		std::memcpy(value, other.value, 6*sizeof(float));
		return *this;
	}

	inline void update(const glm::vec3& position)
	{
		for(int dd=0; dd<3; ++dd)
		{
			value[2*dd]   = std::min(value[2*dd],   position[dd]); // x/y/z-min
			value[2*dd+1] = std::max(value[2*dd+1], position[dd]); // x/y/z-max
		}
	}

	inline float& operator[](size_t index)      { return value[index]; }
	inline float operator[](size_t index) const { return value[index]; }
	inline float& xmin() { return value[0]; }
	inline float& xmax() { return value[1]; }
	inline float& ymin() { return value[2]; }
	inline float& ymax() { return value[3]; }
	inline float& zmin() { return value[4]; }
	inline float& zmax() { return value[5]; }
	inline float xmin() const { return value[0]; }
	inline float xmax() const { return value[1]; }
	inline float ymin() const { return value[2]; }
	inline float ymax() const { return value[3]; }
	inline float zmin() const { return value[4]; }
	inline float zmax() const { return value[5]; }

#ifdef W_DEBUG
	friend std::ostream& operator <<(std::ostream& stream, const Extent& extent);
#endif

	float value[6];
};

struct Ray
{
	struct CollisionData
	{
	    float near = 0.0f;
	    float far  = 0.0f;
	};

    Ray(const glm::vec3& origin, const glm::vec3& end);
    Ray(const glm::vec2& screen_coords, const glm::mat4& VP_inverse);

    Ray to_model_space(const glm::mat4& model_matrix) const;
	bool collides_extent(const Extent& extent, CollisionData& data);

	inline bool collides_OBB(const glm::mat4& model_matrix, const Extent& extent, float scale, CollisionData& data)
	{
	    // Transform ray to model space
	    bool ret = to_model_space(model_matrix).collides_extent(extent, data);
	    // Rescale hit data
	    if(ret)
	    {
	        data.near *= scale;
	        data.far  *= scale;
	    }
	    return ret;
	}

#ifdef W_DEBUG
	friend std::ostream& operator <<(std::ostream& stream, const Ray& ray);
#endif

    glm::vec3 origin;
    glm::vec3 end;
    glm::vec3 direction;
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
