#include "filesystem/obj_file.h"
#include "debug/logger.h"
#include "glm/glm.hpp"

#include <string_view>
#include <cstdio>


namespace erwin
{
namespace obj
{

#define TEXCOORD 0x01
#define NORMAL 0x02

MeshData read(std::istream& stream)
{
	MeshData data;

    bool process_uv = true;
    bool process_normals = true;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> uvs;
    std::vector<glm::vec3> normals;

    std::string line;
    while(std::getline(stream, line))
    {
        std::string_view line_view{line.c_str(), line.size()};

        // Vertex
        if(line_view[0] == 'v' && line_view[1] == ' ')
        {
            glm::vec3 pos;
            float w = 1.f;
            if(sscanf(line.c_str(), "v %f %f %f %f", &pos[0], &pos[1], &pos[2], &w) == 4)
            {
                pos /= w;
                positions.push_back(pos);
            }
            else if(sscanf(line.c_str(), "v %f %f %f", &pos[0], &pos[1], &pos[2]) == 3)
                positions.push_back(pos);
        }
        // UV
        else if(line_view[1] == 't')
        {
            glm::vec3 uv(0.f);
            if(sscanf(line.c_str(), "vt %f %f %f", &uv[0], &uv[1], &uv[2]) == 3)
                uvs.push_back(uv);
            else if(sscanf(line.c_str(), "vt %f %f", &uv[0], &uv[1]) == 2)
                uvs.push_back(uv);
        }
        // Normal
        else if(line_view[1] == 'n')
        {
            glm::vec3 normal;
            if(sscanf(line.c_str(), "vn %f %f %f", &normal[0], &normal[1], &normal[2]) == 3)
                normals.push_back(glm::normalize(normal));
        }
        // Face
        else if(line_view[0] == 'f')
        {
            std::array<int, 9> idxs;
            bool tri_ok = false;
            bool has_uv = false;
            bool has_nm = false;

            // f v1 v2 v3
            if(sscanf(line.c_str(), "f %d %d %d", &idxs[0], &idxs[1], &idxs[2]) == 3)
            {
                tri_ok = true;
            }
            // f v1// v2// v3//
            else if(sscanf(line.c_str(), "f %d// %d// %d//", &idxs[0], &idxs[1], &idxs[2]) == 3)
            {
                tri_ok = true;
            }
            // f v1//vn1 v2//vn2 v3//vn3
            else if(sscanf(line.c_str(), "f %d//%d %d//%d %d//%d", &idxs[0], &idxs[3], &idxs[1], &idxs[4], &idxs[2],
                           &idxs[5]) == 6)
            {
                tri_ok = true;
                has_nm = true;
            }
            // f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
            else if(sscanf(line.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d", &idxs[0], &idxs[6], &idxs[3], &idxs[1],
                           &idxs[7], &idxs[4], &idxs[2], &idxs[8], &idxs[5]) == 9)
            {
                tri_ok = true;
                has_uv = true;
                has_nm = true;
            }
            else
            {
                DLOGE("nuclear") << "Unrecognized sequence during face extraction: " << std::endl;
                DLOGI << line << std::endl;
            }

            if(tri_ok)
            {
                // Remap indices, Wavefront Obj format starts counting at 1
                std::transform(idxs.begin(), idxs.end(), idxs.begin(), [](int idx) -> int { return idx - 1; });

                Triangle tri;

                tri.indices = glm::i32vec3(idxs[0], idxs[1], idxs[2]);
                tri.vertices[0] = positions[size_t(idxs[0])];
                tri.vertices[1] = positions[size_t(idxs[1])];
                tri.vertices[2] = positions[size_t(idxs[2])];

                tri.attributes = 0;

                if(process_uv && has_uv)
                {
                    tri.uvs[0] = uvs[size_t(idxs[6])];
                    tri.uvs[1] = uvs[size_t(idxs[7])];
                    tri.uvs[2] = uvs[size_t(idxs[8])];

                    tri.attributes |= TEXCOORD;
                }

                if(process_normals && has_nm)
                {
                    tri.normals[0] = normals[size_t(idxs[3])];
                    tri.normals[1] = normals[size_t(idxs[4])];
                    tri.normals[2] = normals[size_t(idxs[5])];

                    tri.attributes |= NORMAL;
                }

                // tri.material = material;
                data.triangles.push_back(tri);
            }
        }
    }

    return data;
}


} // namespace obj
} // namespace erwin