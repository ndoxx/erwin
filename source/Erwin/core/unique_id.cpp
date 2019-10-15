#include "core/unique_id.h"

namespace erwin
{
namespace id
{

static W_ID s_cur_id = 0;

W_ID unique_id()
{
	return ++s_cur_id; // 0 is reserved
}

} // namespace id
} // namespace erwin