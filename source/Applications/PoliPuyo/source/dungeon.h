#include <cstdint>
#include <vector>
#include <set>
#include "core/core.h"

namespace puyo
{

enum CellType: uint8_t
{
	NONE,
	RED,
	GREEN,
	YELLOW,
	BLUE,
	PURPLE,
};

struct Cell
{
	uint8_t type;
	uint8_t connectivity;
};

#define INDEX( X , Y , W ) ( Y * W ) + ( X )

class Dungeon
{
public:
	typedef std::set<std::pair<int,int>> Group;

	Dungeon(uint32_t width, uint32_t height);
	~Dungeon();

	inline uint32_t get_width() const  { return width_; }
	inline uint32_t get_height() const { return height_; }
	inline Cell& get_cell(int x, int y)
	{
		W_ASSERT(x<width_  && x>=0, "x index out of bounds.");
		W_ASSERT(y<height_ && y>=0, "y index out of bounds.");
		return cells_[INDEX(x,y,width_)];
	}
	inline const Cell& get_cell(int x, int y) const
	{
		W_ASSERT(x<width_  && x>=0, "x index out of bounds.");
		W_ASSERT(y<height_ && y>=0, "y index out of bounds.");
		return cells_[INDEX(x,y,width_)];
	}

	void update_connectivity();

	// Find groups of 4 or more of the same color
	void find_groups_4(std::set<Group>& groups);

private:
	// Recursively add neighbors of the same color to group
	void flood_fill(int x, int y, uint8_t type, Group& group);

private:
	uint32_t width_;
	uint32_t height_;
	Cell* cells_;
	bool* visited_;
};

} // namespace puyo