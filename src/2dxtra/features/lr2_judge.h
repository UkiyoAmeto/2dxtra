#pragma once

#include <string>
#include <string_view>

#include "../game.h"

namespace iidxtra::lr2_judge
{
    // The LR2 judge windows the user can pick from the loader.
    enum class level
    {
        none,
        easy,
        normal,
        hard,
        very_hard,
    };

    // Returns the level encoded in a chart set name, or none.
    auto level_from_name(std::string_view name) -> level;

    // Returns the level of the chart set that is currently loaded.
    auto active_level() -> level;

    // True when the loaded chart keeps the game's own judgement
    // (GAMBOL on ANOTHER is the one exception the user asked for).
    auto keeps_game_judge() -> bool;

    // Replaces the judge windows the game just installed with the LR2 ones.
    auto apply(bm2dx::timing_data_t* data, int player) -> void;
}
