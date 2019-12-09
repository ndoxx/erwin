#include <iostream>
#include <iomanip>
#include <vector>
#include <functional>
#include <random>
#include <type_traits>
#include <cstdlib>
#include <cstring>
#include <bitset>

// using namespace erwin;

#include "../Applications/PoliPuyo/source/dungeon.h"
using namespace puyo;

void show_dungeon(std::ostream& stream, const Dungeon& dungeon)
{
	for(int yy=dungeon.get_height()-1; yy>=0; --yy)
	{
		for(int xx=0; xx<dungeon.get_width(); ++xx)
		{
			const Cell& cell = dungeon.get_cell(xx,yy);
			stream << "[" << (int)cell.type << "," << (int)cell.connectivity << "]";
		}
		stream << std::endl;
	}
}

int main(int argc, char** argv)
{
	std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<uint8_t> dist5(1,5);

	uint32_t w = 7, h = 16;
	Dungeon dungeon(w,h);
	for(int xx=0; xx<w; ++xx)
	{
		for(int yy=0; yy<h; ++yy)
		{
			dungeon.get_cell(xx,yy).type = dist5(rng);
		}
	}

	dungeon.update_connectivity();

	show_dungeon(std::cout, dungeon);

	return 0;
}
