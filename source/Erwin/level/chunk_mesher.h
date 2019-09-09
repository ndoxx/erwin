#pragma once

#include "mesh.h"

namespace erwin
{

class TerrainChunk;
class ChunkMesher
{
public:
    ChunkMesher();
    ~ChunkMesher();

    WMesh make_mesh(const TerrainChunk& chunk);

private:

};

} // namespace erwin
