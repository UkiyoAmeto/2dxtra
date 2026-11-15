#include <algorithm>
#include <array>
#include <cmath>
#include <cwchar>

#include "lr2_judge.h"
#include "../chart_set.h"
#include "../diag.h"

namespace iidxtra::lr2_judge
{
    namespace
    {
        // The LR2 judgement windows, in milliseconds:
        // pgreat / great / good / bad. Anything wider than "bad" is a POOR.
        struct windows_t
        {
            float pgreat;
            float great;
            float good;
            float bad;
        };

        auto constexpr window_poor = 1000.0f;
        auto constexpr window_bad = 200.0f;

        // The game keeps its judgement windows in milliseconds: a normal
        // SP play reports pgreat 16.4 / great 33.1 / good 116.4 / bad 249.8,
        // which are the usual IIDX windows, so the LR2 table goes in as is.
        auto to_game_units(const float milliseconds) -> float
            { return milliseconds; }

        auto windows_for(const level selected) -> windows_t
        {
            switch (selected)
            {
                case level::easy:      return { 21.0f, 60.0f, 120.0f, window_bad };
                case level::normal:    return { 18.0f, 40.0f, 100.0f, window_bad };
                case level::hard:      return { 15.0f, 30.0f,  60.0f, window_bad };
                case level::very_hard: return {  8.0f, 24.0f,  40.0f, window_bad };
                default:               return {  0.0f,  0.0f,   0.0f,      0.0f };
            }
        }

        // Rewrites one window table. The early slots are the negative
        // boundaries (widest first) and the late slots the positive ones
        // (tightest first), which is how the game walks them when it
        // classifies a hit.
        auto apply_windows(bm2dx::timing_t& timing, const windows_t& windows) -> void
        {
            auto fields = std::array<float*, 9> {{
                &timing.early_poor, &timing.early_bad, &timing.early_good,
                &timing.early_great, &timing.early_pgreat,
                &timing.late_pgreat, &timing.late_great, &timing.late_good,
                &timing.late_bad,
            }};

            auto order = std::array<int, 9> {{ 0, 1, 2, 3, 4, 5, 6, 7, 8 }};
            std::sort(order.begin(), order.end(), [&fields](const int a, const int b) {
                return *fields[a] < *fields[b];
            });

            auto const early = std::array<float, 5> {{
                -to_game_units(window_poor),
                -to_game_units(windows.bad),
                -to_game_units(windows.good),
                -to_game_units(windows.great),
                -to_game_units(windows.pgreat),
            }};

            auto const late = std::array<float, 4> {{
                to_game_units(windows.pgreat),
                to_game_units(windows.great),
                to_game_units(windows.good),
                to_game_units(windows.bad),
            }};

            // Sorted order keeps the widest boundary first, which is the
            // order the game walks when it classifies a hit. The five early
            // slots always take the negative table and the four late slots
            // the positive one, so the assignment never depends on whether
            // a stale value happens to be a tiny positive number.
            for (auto i = std::size_t { 0 }; i < order.size(); ++i)
            {
                auto const slot = order[i];

                if (i < early.size())
                    *fields[slot] = early[i];
                else
                    *fields[slot] = late[i - early.size()];
            }
        }
    }

    auto level_from_name(const std::string_view name) -> level
    {
        auto constexpr marker = std::string_view { "LR2-Judge : " };
        auto const position = name.find(marker);

        if (position == std::string_view::npos)
            return level::none;

        // Only the plain and the Kichiku variants exist.
        if (position != 0 && name.substr(0, position) != "Kichiku with ")
            return level::none;

        auto const suffix = name.substr(position + marker.size());

        if (suffix == "Easy")    return level::easy;
        if (suffix == "Normal")  return level::normal;
        if (suffix == "Hard")    return level::hard;
        if (suffix == "V.Hard")  return level::very_hard;

        return level::none;
    }

    auto active_level() -> level
    {
        return level_from_name(chart_set::active);
    }

    auto keeps_game_judge() -> bool
    {
        auto const* const music = bm2dx::state ? bm2dx::state->active_music : nullptr;

        if (music == nullptr)
            return false;

        // GAMBOL on ANOTHER keeps the game's judgement.
        if (std::wcsncmp(music->title, L"GAMBOL", 6) != 0)
            return false;

        auto const difficulty = (bm2dx::state->p1_active
            ? bm2dx::state->p1_difficulty: bm2dx::state->p2_difficulty);

        return difficulty == 3;
    }

    auto apply(bm2dx::timing_data_t* data, const int player) -> void
    {
        if (data == nullptr || player < 0 || player > 1)
            return;

        auto const selected = active_level();

        if (selected == level::none || keeps_game_judge())
            return;

        auto const windows = windows_for(selected);

        // Keep the windows the game installed so the first play can report
        // which scale it uses (frames vs milliseconds).
        auto const before = data->timing[player].keys;

        apply_windows(data->timing[player].keys, windows);
        apply_windows(data->timing[player].scratch, windows);

        auto const after = data->timing[player].keys;

        auto static reported = false;

        if (!reported)
        {
            reported = true;

            auto const dump = [&] (char const* label, bm2dx::timing_t const& w)
            {
                diag::write("[lr2] %s %d: %.3f %.3f %.3f %.3f %.3f | %.3f %.3f %.3f %.3f",
                    label, static_cast<int>(selected),
                    w.early_poor, w.early_bad, w.early_good, w.early_great, w.early_pgreat,
                    w.late_pgreat, w.late_great, w.late_good, w.late_bad);
            };

            dump("game", before);
            dump("lr2 ", after);
        }
    }
}
