#include "dungeon.h"
#include <iostream>

namespace puyo
{

Dungeon::Dungeon(uint32_t width, uint32_t height):
width_(width),
height_(height)
{
	cells_ = new Cell[width*height];
	visited_ = new bool[width*height];
	for(int ii=0; ii<width*height; ++ii)
	{
		cells_[ii] = {0,0};
		visited_[ii] = false;
	}
}

Dungeon::~Dungeon()
{
	delete[] cells_;
	delete[] visited_;
}

void Dungeon::update_connectivity()
{
	for(int xx=0; xx<width_; ++xx)
	{
		for(int yy=0; yy<height_; ++yy)
		{
			// For each non void cell
			Cell& cell = get_cell(xx,yy);
			if(cell.type != CellType::NONE)
			{
				uint8_t connectivity = 0;
				// Check neighbors of same color
				connectivity += (xx-1 >= 0)      ? 1*uint8_t(get_cell(xx-1,yy).type == cell.type) : 0; // Left
				connectivity += (xx+1 < width_)  ? 2*uint8_t(get_cell(xx+1,yy).type == cell.type) : 0; // Right
				connectivity += (yy-1 >= 0)      ? 4*uint8_t(get_cell(xx,yy-1).type == cell.type) : 0; // Down
				connectivity += (yy+1 < height_) ? 8*uint8_t(get_cell(xx,yy+1).type == cell.type) : 0; // Up
				cell.connectivity = connectivity;
			}
		}
	}
}

void Dungeon::flood_fill(int x, int y, uint8_t type, Group& group)
{
	const Cell& cell = get_cell(x,y);
	if(visited_[INDEX(x,y,width_)] || type != cell.type)
		return;

	visited_[INDEX(x,y,width_)] = true;
	group.insert({x,y});

	if(x-1 >= 0 && get_cell(x-1,y).type == type)
		flood_fill(x-1,y,type,group);

	if(x+1 < width_ && get_cell(x+1,y).type == type)
		flood_fill(x+1,y,type,group);

	if(y-1 >= 0 && get_cell(x,y-1).type == type)
		flood_fill(x,y-1,type,group);

	if(y+1 < height_ && get_cell(x,y+1).type == type)
		flood_fill(x,y+1,type,group);
}

void Dungeon::find_groups_4(std::set<Group>& groups)
{
	// * Clear visited table
	for(int ii=0; ii<width_*height_; ++ii)
		visited_[ii] = false;

	// * Find groups recursively, add to group set if more than 4 cells in a group
	for(int xx=0; xx<width_; ++xx)
	{
		for(int yy=0; yy<height_; ++yy)
		{
			const Cell& cell = get_cell(xx,yy);
			if(cell.type != CellType::NONE && !visited_[INDEX(xx,yy,width_)])
			{
				Group group = { { xx,yy } };
				flood_fill(xx,yy,cell.type,group);
				if(group.size() >= 4)
					groups.insert(group);
			}
		}
	}
}


} // namespace puyo