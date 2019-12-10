#include <cstdint>
#include <vector>
#include <set>
#include "core/core.h"
#include "core/wtypes.h"
#include "glm/glm.hpp"

namespace puyo
{

/*
	TODO:
	[ ] Register each puyo as an entity in entity system
		[ ] Each entity has a renderable component with all information to render it on screen
	[ ] Before each evaluation phase, all free puyos are settled down by gravity
		[ ] A falling puyo stops when it collides with a settled puyo or the bottom of the dungeon
		[ ] Then, the board state is determined by iterating through the puyo list
*/

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

// TMP: move to entity renderable component
// Static geometry is handled separately
struct Renderable
{
	erwin::hash_t atlas;
	erwin::hash_t tile;
	glm::vec2 scale;
	glm::vec4 position;
	glm::vec4 tint;
	bool has_tint = false;
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
	inline const std::vector<Renderable>& get_renderables() const { return renderables_; }

	void update_connectivity();
	void update_renderables(); // TMP

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

	std::vector<Renderable> renderables_;
};

} // namespace puyo