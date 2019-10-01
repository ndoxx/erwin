#include "optimal_packing.h"

using namespace rectpack2D;

namespace fudge
{

rect_wh pack(std::vector<rect_xywh>& rectangles, uint32_t max_side)
{
    // * Configure Rectpack2D
    constexpr bool allow_flip = false;
    const auto discard_step = 1;
    const auto runtime_flipping_mode = flipping_option::ENABLED;
    using spaces_type = rectpack2D::empty_spaces<allow_flip, default_empty_spaces>;
    using rect_type = output_rect_t<spaces_type>;

    auto report_successful = [](rect_type&)
    {
        return callback_result::CONTINUE_PACKING;
    };
    auto report_unsuccessful = [](rect_type&)
    {
        return callback_result::ABORT_PACKING;
    };

    return find_best_packing<spaces_type>
    (
        rectangles,
        make_finder_input
        (
            max_side,
            discard_step,
            report_successful,
            report_unsuccessful,
            runtime_flipping_mode
        )
    );
}

} // namespace fudge
