#pragma once

#include <iostream>
#include <cstdint>

namespace erwin
{

typedef uint16_t Cell;

enum CellWall: uint8_t
{
    CELL_WALL_NONE = 0,
    CELL_FLOOR     = 1,
    CELL_CEILING   = 2,
    CELL_WALL_NE   = 4,
    CELL_WALL_NW   = 8,
    CELL_WALL_SE   = 16,
    CELL_WALL_SW   = 32
};

enum CellFlag: uint8_t
{
    CELL_FLAG_NONE  = 0, // Walkable
    CELL_FLAG_BLOCK = 1
};

// [ flags ][   wall bits   ][material id]
// [_|_|_|_][SW|SE|NW|NE|C|F][_|_|_|_|_|_]
//       12                6            0

#define WALL_OFFSET 6
#define FLAG_OFFSET 12

static inline Cell make_cell(uint8_t material_id,
                             CellWall wall_bits,
                             CellFlag flag_bits)
{
    return Cell(uint16_t(material_id)
               |uint16_t(wall_bits)<<WALL_OFFSET
               |uint16_t(flag_bits)<<FLAG_OFFSET);
}

/*
    Contains static level geometry for one level chunk.
    Access: x loop: innermost

    TODO:
        - handle multiple layers (y axis)
        - more possible faces (à la marching cubes)
*/

class TerrainChunk
{
public:
    TerrainChunk(uint32_t width, uint32_t height, Cell* data=nullptr);
    TerrainChunk(std::istream& stream);
    ~TerrainChunk();

    inline uint32_t get_width() const  { return width_; }
    inline uint32_t get_height() const { return height_; }

    inline void set_cell(uint32_t xx, uint32_t zz, Cell value)       { cells_[index(xx,zz)] = value; }
    inline Cell get_cell(uint32_t xx, uint32_t zz) const             { return cells_[index(xx,zz)]; }
    inline uint8_t get_material_id(uint32_t xx, uint32_t zz) const   { return uint8_t(get_cell(xx,zz)&0xFF); }
    inline CellWall get_walls(uint32_t xx, uint32_t zz) const        { return CellWall((get_cell(xx,zz)>>WALL_OFFSET)&0xFF); }
    inline CellFlag get_flags(uint32_t xx, uint32_t zz) const        { return CellFlag((get_cell(xx,zz)>>FLAG_OFFSET)&0xFF); }
    inline bool has_wall(uint32_t xx, uint32_t zz, CellWall w) const { return bool(get_walls(xx,zz)&w); }

    void read_stream(std::istream& stream);
    void write_stream(std::ostream& stream) const;

    void dbg_display();

private:
    inline size_t index(uint32_t xx, uint32_t zz) const { return zz*width_+xx; }

private:
    uint32_t width_;
    uint32_t height_;
    Cell* cells_;
};

} // namespace erwin
