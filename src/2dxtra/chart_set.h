#pragma once

#include "game.h"
#include "database.h"
#include <optional>
#include <unordered_map>

namespace iidxtra::chart_set
{
	struct chart_t
	{
		std::string id;
		std::uint32_t notes;
		bm2dx::notes_radar_t radar;

		// the difficulty (analyze gate) the chart is stored under; differs from
		// the loader index for sets that mirror one difficulty onto another
		// (DP-to-SP)
		std::uint8_t difficulty = 0;
	};

	struct music_t
	{
		std::unordered_map<std::uint8_t, chart_t> charts;
	};

	struct chart_set_t
	{
		int id = -1;
		std::unordered_map<std::uint32_t, music_t> music;
		std::size_t count;
	};

	// whether we can currently switch chart sets
	// enabled in music select, disabled during gameplay
	extern bool switch_enabled;

	// minimal representation of music_data.bin
	extern chart_set_t stock;

	// chart sets loaded from the database
	extern std::unordered_map<std::string, chart_set_t> custom;

	// currently active chart set
	extern std::string active;

	// Set the database to read chart sets from.
	auto init(database::db*) -> void;

	// The database handle (nullptr until init()).
	auto cache() -> database::db*;

	// (Re)load all chart sets from the database into `custom`.
	auto load_sets() -> void;

	// Whether any chart in the database carries this id (mutated hash).
	auto exists(const std::string& id) -> bool;

	// Whether this set stores double charts re-targeted at the single play
	// difficulty slots (DP-to-SP family).
	auto is_dp_to_sp(const std::string& name) -> bool;

	// Whether the active set carries a chart for this music in the given slot.
	auto has_chart(std::uint32_t music_id, std::uint8_t loader_index) -> bool;

	// Pull the active set's mutated chart for a music entry into `dst`.
	// `loader_index` is the .1 chart index used by the game.
	auto pull_chart(int music_id, int loader_index, std::uint8_t* dst,
	                std::size_t capacity) -> std::optional<database::pulled_chart>;

	auto revert() -> void;
	auto set_active(const std::string& name) -> void;
}
