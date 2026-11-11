#include <algorithm>
#include <cstring>
#include "../game.h"
#include "../judgment.h"
#include "play_stats.h"

namespace iidxtra::play_stats
{
	namespace
	{
		constexpr int kPlayerCount = 2;
		constexpr int kGradeCount = 10;

		int judged[kPlayerCount][kGradeCount];
		std::int32_t observed_score[kPlayerCount];
	}

	auto reset() -> void
	{
		std::memset(judged, 0, sizeof(judged));
		std::memset(observed_score, 0, sizeof(observed_score));
	}

	auto on_judge(const int player, const int grade) -> void
	{
		if (player < 0 || player >= kPlayerCount)
			return;

		if (grade >= 0 && grade < kGradeCount)
			++judged[player][grade];

		sample_score(player);
	}

	auto sample_score(const int player) -> void
	{
		if (player < 0 || player >= kPlayerCount || bm2dx::play_state == nullptr)
			return;

		// The game keeps the live EX score in the play state; remember the
		// highest value seen so a late reset cannot hide the final score.
		auto const score = bm2dx::play_state->players[player].ex_score;

		if (score > observed_score[player])
			observed_score[player] = score;
	}

	auto judged_count(const int player) -> int
	{
		if (player < 0 || player >= kPlayerCount)
			return 0;

		auto total = 0;

		for (auto grade = 0; grade < kGradeCount; ++grade)
			total += judged[player][grade];

		return total;
	}

	auto has_judgments(const int player) -> bool
		{ return judged_count(player) > 0; }

	auto poor_count(const int player) -> int
	{
		if (player < 0 || player >= kPlayerCount)
			return 0;

		return judged[player][bm2dx::judge_grade::early_poor]
			+ judged[player][bm2dx::judge_grade::late_poor];
	}

	auto last_score(const int player) -> int
	{
		if (player < 0 || player >= kPlayerCount)
			return 0;

		auto score = observed_score[player];

		// Prefer the live value when the play state is still around.
		if (bm2dx::play_state != nullptr)
			score = std::max(score, bm2dx::play_state->players[player].ex_score);

		return score;
	}
}
