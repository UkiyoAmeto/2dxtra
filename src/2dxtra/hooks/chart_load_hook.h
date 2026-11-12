#pragma once

#include <string>

namespace iidxtra::chart_load_hook
{
	extern std::uint8_t next_player_id;
	extern std::string last_chart_id_p1, last_chart_id_p2;
    extern std::uint32_t last_chart_note_count_p1, last_chart_note_count_p2;

	// The double play chart index a DP-to-SP set loaded its chart from, or -1
	// when the sound package of the chart the game asked for is the right one.
	extern int dp_source_gate;

	auto install_hook() -> void;
	auto was_last_score_custom() -> bool;
}