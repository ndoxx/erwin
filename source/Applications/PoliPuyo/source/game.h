#include "dungeon.h"

namespace puyo
{

class Game
{
public:
	static void init();
	static void shutdown();

	static inline Game* instance() { return instance_; }
	inline Dungeon& get_dungeon() { return *dungeon_; }

private:
	static Game* instance_;

	Dungeon* dungeon_;
};


} // namespace puyo