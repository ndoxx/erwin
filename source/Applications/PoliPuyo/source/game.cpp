#include "game.h"
#include <random>
#include <iostream>

namespace puyo
{

Game* Game::instance_ = nullptr;

void Game::init()
{
	std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<uint8_t> dist5(0,5);
	uint32_t w = 7, h = 16;

	instance_ = new Game();
	instance_->dungeon_ = new Dungeon(w,h);

	// Random dungeon
	for(int xx=0; xx<w; ++xx)
	{
		for(int yy=0; yy<h; ++yy)
		{
			instance_->dungeon_->get_cell(xx,yy).type = dist5(rng);
		}
	}
	instance_->dungeon_->update_connectivity();
	instance_->dungeon_->update_renderables();

	std::set<Dungeon::Group> groups;
	instance_->dungeon_->find_groups_4(groups);

	for(auto&& group: groups)
	{
		for(auto&& coords: group)
			std::cout << "[" << coords.first << "," << coords.second << "]";
		std::cout << std::endl;
	}
}

void Game::shutdown()
{
	delete instance_->dungeon_;
	delete instance_;
}



} // namespace puyo