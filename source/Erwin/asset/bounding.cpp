#include "asset/bounding.h"
#include "glm/gtx/string_cast.hpp"

namespace erwin
{

Ray::Ray(const glm::vec3& origin, const glm::vec3& end):
origin(origin),
end(end),
direction(glm::normalize(end-origin))
{
	
}

// TODO: test another version which leverages glm::unProject()
Ray::Ray(const glm::vec2& screen_coords, const glm::mat4& VP_inverse)
{
    // Convert to NDC coordinates
    glm::vec4 coords_near(2.f*screen_coords-1.f, 1.f, 1.f);
    glm::vec4 coords_far(coords_near);
    coords_near[2] = -1.0f; // far plane

    // Unproject
    glm::vec4 coords_near_w(VP_inverse*coords_near);
    coords_near_w /= coords_near_w.w;
    glm::vec4 coords_far_w(VP_inverse*coords_far);
    coords_far_w /= coords_far_w.w;

    // Initialize
    origin    = coords_near_w;
    end       = coords_far_w;
    direction = glm::normalize(end-origin);
}


/*
// WCore:
bool inverse_affine(const mat4& m, mat4& Inverse)
{
    mat3 submat(m.submatrix(3,3));
    mat3 submatInv;
    vec3 trans0(m[12],m[13],m[14]);
    if(!inverse(submat,submatInv)) return false;
    vec3 trans = -1.0*(submatInv*trans0);

    Inverse[0] = submatInv[0]; Inverse[4] = submatInv[3]; Inverse[8] = submatInv[6];   Inverse[12] = trans[0];
    Inverse[1] = submatInv[1]; Inverse[5] = submatInv[4]; Inverse[9] = submatInv[7];   Inverse[13] = trans[1];
    Inverse[2] = submatInv[2]; Inverse[6] = submatInv[5]; Inverse[10] = submatInv[8];  Inverse[14] = trans[2];
    Inverse[3] = 0.0f;         Inverse[7] = 0.0f;         Inverse[11] = 0.0f;          Inverse[15] = 1.0f;

    return true;
}
*/

Ray Ray::to_model_space(const glm::mat4& model_matrix) const
{
    glm::mat4 model_inv = glm::inverse(model_matrix); // TODO: use a faster affine inversion
    glm::vec3 origin_m  = model_inv*glm::vec4(origin, 1.f);
    glm::vec3 end_m     = model_inv*glm::vec4(end, 1.f);

    return Ray(origin_m, end_m);
}

static constexpr float k_epsilon = 0.0001f;

// Scott Owen's algo for ray/AABB intersection
// adapted from pseudo-code in: https://www.siggraph.org//education/materials/HyperGraph/raytrace/rtinter3.htm
bool Ray::collides_extent(const Extent& extent, CollisionData& data)
{
    float Tnear = -std::numeric_limits<float>::max();
    float Tfar  = std::numeric_limits<float>::max();

    // For each X/Y/Z slab
    for(size_t ii=0; ii<3; ++ii)
    {
        float xxl = extent[2*ii];
        float xxh = extent[2*ii+1];
        float xxo = origin[int(ii)];
        float xxd = direction[int(ii)];

        // If ray parallel to planes
        if(std::abs(xxd)<k_epsilon)
        {
            // If ray origin not between slab, no intersection for this slab
            if(xxo < xxl || xxo > xxh)
                return false;
        }
        else
        {
            // Compute the intersection distance of the planes
            float T1 = (xxl - xxo) / xxd;
            float T2 = (xxh - xxo) / xxd;

            if(T1 > T2)
                std::swap(T1, T2); // Make sure T1 is the intersection with the near plane
            if(T1 > Tnear)
                Tnear = T1; // Tnear will converge to the largest T1
            if(T2 < Tfar)
                Tfar = T2;  // Tfar will converge to the smallest T2
        }
    }

    // Box miss / box behind ray
    if(Tfar<Tnear || Tfar<0)
        return false;

    data.near = Tnear;
    data.far  = Tfar;
    return true;
}


#ifdef W_DEBUG
std::ostream& operator <<(std::ostream& stream, const Extent& dims)
{
	stream << "Ext{" 
		   << "x: " << dims[0] << "->" << dims[1] << ", "
		   << "y: " << dims[2] << "->" << dims[3] << ", "
		   << "z: " << dims[4] << "->" << dims[5] << "}";
	return stream;
}

std::ostream& operator <<(std::ostream& stream, const Ray& ray)
{
	stream << "Ray{(" 
		   << ray.origin.x << "," << ray.origin.y << "," << ray.origin.z << ")->("
		   << ray.end.x    << "," << ray.end.y    << "," << ray.end.z << ")}";
	return stream;
}
#endif

} // namespace erwin