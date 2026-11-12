#include "../log.h"
#include "../game.h"
#include "../chart_set.h"
#include "../hooks/chart_load_hook.h"
#include "chart_loader.h"

namespace iidxtra::chart_loader
{
    auto constexpr remap_chart_index(const int index) -> int
    {
        switch (index)
        {
            case 0: return 3;  // SP BEGINNER
            case 1: return 1;  // SP NORMAL
            case 2: return 0;  // SP HYPER
            case 3: return 2;  // SP ANOTHER
            case 4: return 4;  // SP LEGGENDARIA
            case 5: return 9;  // DP BEGINNER [unused]
            case 6: return 7;  // DP NORMAL
            case 7: return 6;  // DP HYPER
            case 8: return 8;  // DP ANOTHER
            case 9: return 10; // DP LEGGENDARIA
            default:
            	return 1; // Unknown: fall back to SP NORMAL
        }
    }

	auto load_custom_chart(void* output, const int index) -> bool
	{
		// Map the in-game chart index to the .1 index. (e.g. 3 -> 2 for ANOTHER)
		auto real_index = remap_chart_index(index);

		// DP-to-SP sets always mirror the difficulty the player picked, even
		// when the game was handed the double play index so that the sound
		// bank would be filled from the DP chart.
		if (chart_set::is_dp_to_sp(chart_set::active) && bm2dx::state)
		{
			auto const difficulty = (chart_load_hook::next_player_id == 0
				? bm2dx::state->p1_difficulty
				: bm2dx::state->p2_difficulty);
			real_index = remap_chart_index(difficulty);
		}

		// Pull the mutated chart from the database; the mutated hash doubles
		// as the chart id.
		auto const pulled = chart_set::pull_chart(
			bm2dx::state->active_music->id, real_index,
			static_cast<std::uint8_t*>(output), bm2dx::CHART_BUFFER_BYTES);

		if (!pulled.has_value())
			return false;

		// Save the chart ID for later use in stage_result_hook.
		(chart_load_hook::next_player_id == 0 ?
			chart_load_hook::last_chart_id_p1:
			chart_load_hook::last_chart_id_p2) = pulled->hash;

		// A DP-to-SP chart only carries single play notes. The counts the game
		// read from the double play chart would otherwise linger in the buffer.
		if (chart_set::is_dp_to_sp(chart_set::active))
		{
			auto* const buffer = static_cast<bm2dx::chart_buffer_t*>(output);
			buffer->p1_note_count = static_cast<std::int32_t>(pulled->notes);
			buffer->p2_note_count = 0;
		}

		auto const original_notes = chart_set::stock.music[bm2dx::state->active_music->id].charts[real_index].notes;
		auto const replacement_notes = pulled->notes;

		// If the note count changed, display the difference à la 2dxplus.
        if (replacement_notes == original_notes)
        	return true;

	    log::print("[{}] P{}: {} + {} -> {} notes", chart_set::active, chart_load_hook::next_player_id + 1,
                   original_notes, replacement_notes - original_notes, replacement_notes);

		return true;
	}
}
