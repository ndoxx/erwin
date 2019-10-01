#ifndef COLORS_H
#define COLORS_H

#include "glm/glm.hpp"

namespace erwin
{
namespace color
{

extern glm::vec3 rgb2hsl(const glm::vec3& rgb_color);
extern glm::vec3 hsl2rgb(const glm::vec3& hsl_color);
extern glm::ivec3 rgbfloat2rgbuint(const glm::vec3& rgb_color);

extern glm::vec3 random_color(unsigned long long seed,
                               float saturation=1.f,
                               float lightness=0.5f);

inline glm::ivec3 random_color_uint(unsigned long long seed,
                                       float saturation=1.f,
                                       float lightness=0.5f)
{
    return rgbfloat2rgbuint(random_color(seed, saturation, lightness));
}

inline glm::vec3 rgb2hsl(float r, float g, float b)
{
    return rgb2hsl(glm::vec3(r,g,b));
}

inline glm::vec3 hsl2rgb(float h, float s, float l)
{
    return hsl2rgb(glm::vec3(h,s,l));
}

} // namespace color
} // namsepace wcore

#endif // COLORS_H
