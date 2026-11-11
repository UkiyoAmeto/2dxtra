#pragma once

#include "game.h"
#include <cstdint>
#include <string>
#include <unordered_map>

namespace database
{
	struct db;
}

namespace iidxtra::score_set
{
	struct score_t
	{
		std::int32_t clear[2];
		std::int32_t miss[2];
		std::int32_t score[2];
	};

	struct rival_score_t
	{
		std::int32_t clear;
		std::int32_t miss;
		std::int32_t score;
	};

	extern std::unordered_map<std::string, score_t> custom;
	extern std::unordered_map<std::string, rival_score_t> custom_rivals[6];

	extern bm2dx::game_score_t* stock_p1;
	extern bm2dx::game_score_t* stock_p2;
	extern bm2dx::game_score_t* stock_rivals_p1;
	extern bm2dx::game_score_t* stock_rivals_p2;

	auto init(database::db* db) -> bool;
	auto clear_stock() -> void;
	auto uninit() -> void;
	auto backup() -> void;
	auto reload_all() -> void;

	// Score archiving: every score set on a special chart is written to the
	// chart database so it survives restarts and chart set switching.
	auto reload_from_database() -> void;
	auto persist(const std::string& chart_id, int player) -> void;
	auto persist_rival(const std::string& chart_id, int rival) -> void;
}
