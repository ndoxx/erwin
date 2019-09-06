#include <iostream>
#include <iomanip>
#include <cstring>

#include "chunk.h"

namespace erwin
{

TerrainChunk::TerrainChunk(uint32_t width, uint32_t height, Cell* data):
width_(width),
height_(height)
{
    size_t count = width_*height_;
    cells_ = new Cell[count];

    if(data)
        memcpy(cells_, data, count*sizeof(Cell));
    else
        for(size_t ii=0; ii<count; ++ii)
            cells_[ii] = 0;
}


TerrainChunk::TerrainChunk(std::istream& stream)
{
    read_stream(stream);
}

TerrainChunk::~TerrainChunk()
{
    delete[] cells_;
}

void TerrainChunk::read_stream(std::istream& stream)
{
    stream.read(reinterpret_cast<char*>(&width_), sizeof(width_));
    stream.read(reinterpret_cast<char*>(&height_), sizeof(height_));

    size_t count = width_*height_;
    cells_ = new Cell[count];

    stream.read(reinterpret_cast<char*>(cells_), count*sizeof(Cell));
}

void TerrainChunk::write_stream(std::ostream& stream) const
{
    stream.write(reinterpret_cast<const char*>(&width_), sizeof(width_));
    stream.write(reinterpret_cast<const char*>(&height_), sizeof(height_));
    stream.write(reinterpret_cast<const char*>(cells_), width_*height_*sizeof(Cell));
}

void TerrainChunk::dbg_display()
{
    for(uint32_t ii=0; ii<width_; ++ii)
    {
        for(uint32_t jj=0; jj<height_; ++jj)
        {
            std::cout << std::setfill('0') << std::setw(4) << std::hex << cells_[index(ii,jj)] << " ";
        }
        std::cout << std::endl;
    }
}


} // namespace erwin
