#pragma once

#include <cstdint>

namespace iidxtra::play_stats
{
	// Live counters for the play that is currently in progress.
	//
	// The game only writes a score table entry when a play is cleared, so
	// failed attempts leave nothing behind for us to copy. These counters are
	// filled in by the judgment hook instead, which runs for every judgment
	// including the notes that are missed.
	//
	// Counters are reset when the player block is reset (i.e. before a new
	// play starts) and read back when the stage result scene is entered.

	auto reset() -> void;

	// Called from the judgment apply hook, once for every applied judgment.
	auto on_judge(int player, int grade) -> void;

	// Remember the live EX score of a player; safe to call at any time.
	auto sample_score(int player) -> void;

	auto has_judgments(int player) -> bool;
	auto judged_count(int player) -> int;

	// Number of POOR judgments (early + late) seen so far.
	auto poor_count(int player) -> int;

	// Highest EX score that was observed while the play was running.
	auto last_score(int player) -> int;
}
