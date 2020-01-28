#include "core/core.h"
#include <cstdio>

namespace detail
{

void assert_redirect()
{
	bool loop = true;
	while(loop)
	{
		printf("What should we do about that?\n  * 0, <enter>: BREAK\n  * 1:          CONTINUE ANYWAY\n  * 2:          EXIT =>[]\n> ");
		switch(getchar())
		{
			case '\n': W_DEBUG_BREAK();
			case '0': W_DEBUG_BREAK();
			case '1': loop = false; break;
			case '2': exit(-1);
		}
	}
}

} // namespace detail
