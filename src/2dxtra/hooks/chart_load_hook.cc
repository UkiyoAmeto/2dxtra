#include <meta.h>
#include <MinHook.h>
#include "../diag.h"
#include "../log.h"
#include "../game.h"
#include "../chart_set.h"
#include "../features/keysound_switch.h"
#include "../features/regular_speed.h"
#include "../features/chart_speed.h"
#include "../features/chart_loader.h"
#include "../features/play_stats.h"
#include "../features/scratch_swap.h"
#include "../features/cn_transformer.h"
#include "chart_load_hook.h"
#include "score_invalidator_hook.h"

namespace iidxtra::chart_load_hook
{
	// Set in the outer function to tell the inner function which player the
	// chart is loading for. Used to set the relevant last_chart_id value.
	std::uint8_t next_player_id;

    // Unique string IDs for the current chart, split across both players.
	std::string last_chart_id_p1, last_chart_id_p2;

    // Note counts of the previous chart. Used by the C API.
    std::uint32_t last_chart_note_count_p1, last_chart_note_count_p2;

	// Set when a DP-to-SP set loads a double play chart; the sound package the
	// game picks for the single play difficulty is then the wrong one.
	int dp_source_gate = -1;

    // original functions
	void* original_chart_loader_fn = nullptr;
	void* original_outer_chart_loader_fn = nullptr;

	// hook functions

	// The in-game single play chart index that holds the double play chart a
	// DP-to-SP set was built from.
	auto constexpr dp_index_for_sp(const int index) -> int
	{
		switch (index)
		{
			case 0: return 6; // SP BEGINNER -> DP NORMAL
			case 1: return 6; // SP NORMAL
			case 2: return 7; // SP HYPER
			case 3: return 8; // SP ANOTHER
			case 4: return 9; // SP LEGGENDARIA
			default: return -1;
		}
	}

	// The .1 index a DP-to-SP set stores the chart under for a single play
	// chart index (SP BEGINNER, NORMAL, HYPER, ANOTHER, LEGGENDARIA).
	auto constexpr sp_slot_for(const int index) -> int
	{
		switch (index)
		{
			case 0: return 3;
			case 1: return 1;
			case 2: return 0;
			case 3: return 2;
			case 4: return 4;
			default: return -1;
		}
	}

	auto chart_loader_hook_fn(void* output, const char* filename, int chart_index) -> bool
	{
		// Call the original function early to set some stuff up for us.
		auto result = reinterpret_cast<bool (*) (void*, const char*, int)>(original_chart_loader_fn)(output, filename, chart_index);

		// Custom chart loader
		if (!chart_set::active.empty())
			chart_loader::load_custom_chart(output, chart_index);

		// Work on a copy so a mutator can shorten the stream without having to
		// preserve whatever the game already parsed. The buffer is kept around
		// between loads; it is 96 KiB and every chart load needs exactly it.
		auto static events = std::vector<bm2dx::chart_event_t> {};
		events.resize(bm2dx::CHART_EVENT_CAPACITY);
		CopyMemory(events.data(), output, bm2dx::CHART_BUFFER_BYTES);

		{
			// Mutators
            keysound_switch::mutate(next_player_id, events);
			regular_speed::mutate(next_player_id, events);
			scratch_swap::mutate(next_player_id, events);
			cn_transformer::mutate(next_player_id, events);
			chart_speed::mutate(next_player_id, events);
		}

		// A mutator that dropped events has to leave the tail zeroed rather
		// than short, since the game reads the whole fixed-size buffer back.
		events.resize(bm2dx::CHART_EVENT_CAPACITY);
		CopyMemory(output, events.data(), bm2dx::CHART_BUFFER_BYTES);

		return result;
	}

	auto chart_loader_outer_hook_fn(void* output, int player, const char* filename, int chart_index) -> bool
	{
		// Immediately disable input until the player returns to music select.
		// It should already be disabled by the music select destructor hook, but let's be safe.
		chart_set::switch_enabled = false;

		// Reset invalid states to normal.
		score_invalidator_hook::reset(player);

		// A new chart is being loaded, so the previous play's judgments no
		// longer matter.
		play_stats::reset();

		if (player == 0)
		{
			last_chart_id_p1.clear();
			next_player_id = 0;
		}
		else if (player == 1)
		{
			last_chart_id_p2.clear();
			next_player_id = 1;
		}

		// A DP-to-SP set is built from the double play chart, and the wav ids in
		// that chart only mean the right thing when the sound bank was filled
		// from it too. Load the DP chart into the SP slot; the set overwrites
		// the events afterwards.
		auto const music_id = (bm2dx::state && bm2dx::state->active_music)
			? bm2dx::state->active_music->id : 0;

		auto effective_index = chart_index;

		dp_source_gate = -1;

		if (chart_set::is_dp_to_sp(chart_set::active))
		{
			auto const slot = sp_slot_for(chart_index);
			auto const dp_index = dp_index_for_sp(chart_index);

			if (slot >= 0 && dp_index >= 0 &&
				chart_set::has_chart(music_id, static_cast<std::uint8_t>(slot)))
			{
				effective_index = dp_index;
				dp_source_gate = dp_index;
			}
		}

		auto const audio_loads_before = chart_speed::audio_load_count();

		diag::write("[chart] enter music %u P%d index %d -> %d set '%s' (%s)",
			music_id, player + 1, chart_index, effective_index,
			chart_set::active.c_str(), filename ? filename : "");

		auto result = reinterpret_cast<bool (*) (void*, int, const char*, int)>(original_outer_chart_loader_fn)
            (output, player, filename, effective_index);

		// Fall back to the slot the game asked for if the swapped load failed.
		if (!result && effective_index != chart_index)
			result = reinterpret_cast<bool (*) (void*, int, const char*, int)>(original_outer_chart_loader_fn)
				(output, player, filename, chart_index);

		diag::write("[chart] music %u P%d style %d diff %d/%d set '%s' index %d -> %d (%llu audio loads, %s, %s)",
			music_id, player + 1,
			bm2dx::state ? bm2dx::state->play_style : -1,
			bm2dx::state ? bm2dx::state->p1_difficulty : -1,
			bm2dx::state ? bm2dx::state->p2_difficulty : -1,
			chart_set::active.c_str(), chart_index, effective_index,
			static_cast<unsigned long long>(chart_speed::audio_load_count() - audio_loads_before),
			result ? "ok" : "failed", filename ? filename : "");

		// The sound package variant characters are indexed by chart index, so
		// dump them once per play to be able to check the swap below.
		if (dp_source_gate >= 0 && bm2dx::state && bm2dx::state->active_music)
		{
			auto const* const variants = reinterpret_cast<char const*>(bm2dx::state->active_music) + 0x684;
			auto text = std::string {};

			for (int i = 0; i < 10; i++)
				text.push_back(variants[i] >= 0x20 && variants[i] <= 0x7E ? variants[i] : '.');

			diag::write("[chart] sound variants '%s' (source gate %d -> '%c')",
				text.c_str(), dp_source_gate, variants[dp_source_gate]);
		}

		// Real note counts should be available now.
        if (player == 0)
            last_chart_note_count_p1 = static_cast<bm2dx::chart_buffer_t*>(output)->p1_note_count;
        else if (player == 1)
            last_chart_note_count_p2 = static_cast<bm2dx::chart_buffer_t*>(output)->p2_note_count;

        // If score saving is disabled at compile-time, instantly invalidate the score.
        #if BLOCK_ALL_SCORE_SAVE == 1
            score_invalidator_hook::invalidate(0);
            score_invalidator_hook::invalidate(1);
        #endif

		return result;
	}

	auto was_last_score_custom() -> bool
		{ return (!last_chart_id_p1.empty() || !last_chart_id_p2.empty()); }

	auto install_hook() -> void
	{
		MH_CreateHook(bm2dx::addr->LOAD_CHART_FN_A, reinterpret_cast<LPVOID>(chart_loader_outer_hook_fn), &original_outer_chart_loader_fn);
		MH_CreateHook(bm2dx::addr->LOAD_CHART_FN_B, reinterpret_cast<LPVOID>(chart_loader_hook_fn), &original_chart_loader_fn);
	}
}