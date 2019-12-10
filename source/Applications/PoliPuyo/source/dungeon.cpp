#include "dungeon.h"
#include <iostream>

namespace puyo
{

using namespace erwin;

static std::vector<hash_t> s_con_tiles =
{
	"connectivity_00"_h,
	"connectivity_01"_h,
	"connectivity_02"_h,
	"connectivity_03"_h,
	"connectivity_04"_h,
	"connectivity_05"_h,
	"connectivity_06"_h,
	"connectivity_07"_h,
	"connectivity_08"_h,
	"connectivity_09"_h,
	"connectivity_10"_h,
	"connectivity_11"_h,
	"connectivity_12"_h,
	"connectivity_13"_h,
	"connectivity_14"_h,
	"connectivity_15"_h,
};
static std::vector<hash_t> s_dun_tiles =
{
	"dungeon_c"_h,
	"dungeon_tl"_h,
	"dungeon_t"_h,
	"dungeon_tr"_h,
	"dungeon_r"_h,
	"dungeon_br"_h,
	"dungeon_b"_h,
	"dungeon_bl"_h,
	"dungeon_l"_h,
};
static std::vector<glm::vec4> s_tints =
{
	{0.f,0.f,0.f,0.f},
	{0.9275f, 0.1078f, 0.1373f, 1.f},
	{0.2412f, 0.8706f, 0.1961f, 1.f},
	{0.9647f, 0.7529f, 0.1275f, 1.f},
	{0.1255f, 0.2686f, 0.9667f, 1.f},
	{0.3765f, 0.0196f, 0.9333f, 1.f},
};

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

void Dungeon::update_renderables()
{
	renderables_.clear();

	float scale_x = 0.1f;
	float scale_y = 0.1f;
	for(int yy=height_-1; yy>=0; --yy)
	{
		float pos_y = -1.f + 2.f*yy*scale_y;
		for(int xx=0; xx<width_; ++xx)
		{
			float pos_x = -1.f + 2.f*xx*scale_x;
			const Cell& cell = get_cell(xx,yy);
			hash_t tile = s_con_tiles.at(cell.connectivity);
			const glm::vec4& tint = s_tints[cell.type];

			renderables_.push_back(Renderable{"connectivity"_h, tile, {scale_x,scale_y}, {pos_x,pos_y,0.f,1.f}, tint, true});
			renderables_.push_back(Renderable{"dungeon"_h, "dungeon_c"_h, {scale_x,scale_y}, {pos_x,pos_y,-0.1f,1.f}});
		}
	}

	// Dungeon walls
	float pos_xl = -1.f - 2.f*scale_x;
	float pos_xr = -1.f + 2.f*width_*scale_x;
	float pos_yb = -1.f - 2.f*scale_y;
	float pos_yt = -1.f + 2.f*height_*scale_y;
	for(int yy=0; yy<height_; ++yy)
	{
		float pos_y = -1.f + 2.f*yy*scale_y;
		renderables_.push_back(Renderable{"dungeon"_h, "dungeon_l"_h, {scale_x,scale_y}, {pos_xl,pos_y,-0.1f,1.f}});
		renderables_.push_back(Renderable{"dungeon"_h, "dungeon_r"_h, {scale_x,scale_y}, {pos_xr,pos_y,-0.1f,1.f}});
	}
	for(int xx=0; xx<width_; ++xx)
	{
		float pos_x = -1.f + 2.f*xx*scale_y;
		renderables_.push_back(Renderable{"dungeon"_h, "dungeon_b"_h, {scale_x,scale_y}, {pos_x,pos_yb,-0.1f,1.f}});
		renderables_.push_back(Renderable{"dungeon"_h, "dungeon_t"_h, {scale_x,scale_y}, {pos_x,pos_yt,-0.1f,1.f}});
	}
	renderables_.push_back(Renderable{"dungeon"_h, "dungeon_tl"_h, {scale_x,scale_y}, {pos_xl,pos_yt,-0.1f,1.f}});
	renderables_.push_back(Renderable{"dungeon"_h, "dungeon_tr"_h, {scale_x,scale_y}, {pos_xr,pos_yt,-0.1f,1.f}});
	renderables_.push_back(Renderable{"dungeon"_h, "dungeon_bl"_h, {scale_x,scale_y}, {pos_xl,pos_yb,-0.1f,1.f}});
	renderables_.push_back(Renderable{"dungeon"_h, "dungeon_br"_h, {scale_x,scale_y}, {pos_xr,pos_yb,-0.1f,1.f}});
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