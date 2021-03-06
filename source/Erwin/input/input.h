#pragma once

#include "core/core.h"
#include "event/window_events.h"
#include "input/action.h"
#include "input/keys.h"

#ifdef W_USE_EASTL
#include "EASTL/vector.h"
#else
#include <vector>
#endif

namespace erwin
{

// Abstract class for input polling
class W_API Input
{
protected:
    Input() = default;
    virtual ~Input() = default;

public:
    friend class Application;

    NON_COPYABLE(Input);
    NON_MOVABLE(Input);

    // --- Device interaction / polling --
    static inline bool is_key_pressed(keymap::WKEY keycode) { return INSTANCE_->is_key_pressed_impl(keycode); }
    static inline bool is_mouse_button_pressed(keymap::WMOUSE button)
    {
        return INSTANCE_->is_mouse_button_pressed_impl(button);
    }
    static inline std::pair<float, float> get_mouse_position() { return INSTANCE_->get_mouse_position_impl(); }
    static inline void set_mouse_position(float x, float y) { INSTANCE_->set_mouse_position_impl(x, y); }
    static inline void center_mouse_position() { INSTANCE_->center_mouse_position_impl(); }
    static inline void show_cursor(bool value) { INSTANCE_->show_cursor_impl(value); }

    // --- Action API ---

    struct ActionDescriptor
    {
        keymap::WKEY key;
        bool pressed;
        bool repeat;
        uint8_t mods;
        std::string name;
        std::string description;
    };

    static bool load_config();
    static bool save_config();

    static inline void modify_action(uint32_t action, keymap::WKEY key, uint8_t mods = keymap::WKEYMOD::NONE)
    {
        actions[action].key = key;
        actions[action].mods = mods;

        // If key is itself a "modifier" key, remove it from modifiers
        keymap::WKEYMOD k2m = keymap::key_to_keymod(key);
        if(k2m)
            actions[action].mods &= ~k2m;
    }

    static inline size_t get_action_count() { return actions.size(); }

    static inline const ActionDescriptor& get_action(uint32_t action) { return actions[action]; }

    static inline bool is_action_key_pressed(uint32_t action) { return is_key_pressed(actions[action].key); }

    static inline bool match_action(uint32_t action, const KeyboardEvent& e)
    {
        const auto& a = actions[action];
        return (a.key == e.key) && (a.mods == e.mods) && (a.pressed == e.pressed) && (a.repeat == e.repeat);
    }

protected:
    virtual bool is_key_pressed_impl(keymap::WKEY keycode) const = 0;
    virtual bool is_mouse_button_pressed_impl(keymap::WMOUSE button) const = 0;
    virtual std::pair<float, float> get_mouse_position_impl() const = 0;
    virtual void set_mouse_position_impl(float x, float y) const = 0;
    virtual void center_mouse_position_impl() const = 0;
    virtual void show_cursor_impl(bool value) const = 0;

private:
    static void init();
    static void shutdown()
    {
        delete INSTANCE_;
        INSTANCE_ = nullptr;
    }
    static bool parse_keybindings(void* node);

private:
    static Input* INSTANCE_;
    static std::vector<ActionDescriptor> actions;
};

} // namespace erwin