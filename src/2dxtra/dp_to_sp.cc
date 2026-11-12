#include "dp_to_sp.h"

// NOTE: the converted charts are cached in 2dxtra.sqlite, keyed by the hash of
// the chart they were built from. Changing the merge below does not invalidate
// that cache, so a new merge only shows up after the affected chart rows are
// cleared (`DELETE FROM charts WHERE chart_set IN (5, 6);`).

#include <algorithm>
#include <array>
#include <cstdint>
#include <deque>
#include <limits>
#include <vector>

#include "game.h"
#include "urafumen.h"

namespace dp_to_sp
{
    namespace
    {
        using event = bm2dx::chart_event_t;
        using enum bm2dx::chart_event_type;

        constexpr std::size_t COLUMN_COUNT = 8;
        constexpr std::int8_t SCRATCH_COLUMN = 7;
        constexpr std::int8_t SCRATCH_ALIAS = 107;
        constexpr std::int8_t BGM_CHANNEL = 0;
        constexpr std::int16_t NO_SAMPLE = -1;
        constexpr std::int32_t NO_OFFSET =
            (std::numeric_limits<std::int32_t>::min)() / 4;

        // Two notes sharing a lane closer than this cannot be pressed as
        // separate inputs, so one of them has to move to another lane.
        constexpr std::int32_t MIN_SAME_LANE_GAP = 24;

        // Coming back to the same lane inside this window reads as a jack, so
        // the merge moves the note elsewhere whenever it can.
        constexpr std::int32_t JACK_WINDOW = 140;
        constexpr std::int32_t JACK_WEIGHT = 4;

        // Leaving the note on its own lane is preferred, and the scratch lane
        // is only used for notes that were already there.
        constexpr std::int32_t MOVE_COST = 40;
        constexpr std::int32_t SCRATCH_COST = 60;

        // Moving a scratch note off the turntable is the last resort; it only
        // happens when both sides scratch at the same instant.
        constexpr std::int32_t SCRATCH_EXIT_COST = 400;

        struct span
        {
            std::int32_t start;
            std::int32_t end;
        };

        struct candidate
        {
            std::int32_t offset;
            std::int32_t length;
            std::int8_t source_column;
            std::int16_t sample;
            bool placed = false;
            std::int8_t column = 0;
        };

        auto column_of(const std::int8_t parameter) -> std::int8_t
        {
            if (parameter == SCRATCH_ALIAS)
                return SCRATCH_COLUMN;

            if (parameter < 0 || parameter >= static_cast<std::int8_t>(COLUMN_COUNT))
                return SCRATCH_COLUMN;

            return parameter;
        }

        // A positive length marks a charge note; the note occupies its lane
        // until the tail is released, so the whole span is reserved.
        auto length_of(const event& record) -> std::int32_t
            { return record.value > 0 ? record.value : 0; }

        auto conflicts(const span& a, const span& b) -> bool
        {
            // Expand both spans by the gap so two taps that would land inside
            // the same judgment window count as conflicting as well.
            return a.start <= b.end + MIN_SAME_LANE_GAP &&
                   b.start <= a.end + MIN_SAME_LANE_GAP;
        }

        auto merge(const std::vector<event>& source) -> std::vector<event>
        {
            auto extras = std::vector<event> {};
            auto notes = std::vector<candidate> {};

            // Last keysound seen on each column of each side, in the source
            // chart. Every note is mapped to the sound its own side would
            // have played.
            auto column_sample = std::array<std::array<std::int16_t, COLUMN_COUNT>, 2> {};

            for (auto& side: column_sample)
                side.fill(NO_SAMPLE);

            for (auto const& record: source)
            {
                if (record.type == END_OF_SONG)
                {
                    extras.push_back(record);
                    break;
                }

                if (record.type == SAMPLE_P1 || record.type == SAMPLE_P2)
                {
                    // The two sides keep separate keysound timelines. They
                    // are folded into one later, next to the notes that need
                    // them, so the timelines cannot overwrite each other.
                    auto const side = record.type == SAMPLE_P1 ? 0: 1;
                    column_sample[side][column_of(record.parameter)] = record.value;
                    continue;
                }

                if (record.type != NOTE_P1 && record.type != NOTE_P2)
                {
                    extras.push_back(record);
                    continue;
                }

                auto const side = record.type == NOTE_P1 ? 0: 1;
                auto const column = column_of(record.parameter);

                notes.push_back(candidate {
                    .offset = record.offset,
                    .length = length_of(record),
                    .source_column = column,
                    .sample = column_sample[side][column] });
            }

            // Reserve every lane a note already sits on (the first player's
            // side is placed first, so it keeps its lanes), then place the
            // second player's notes on whichever lane is cheapest.
            auto occupied = std::array<std::deque<span>, COLUMN_COUNT> {};
            auto last_start = std::array<std::int32_t, COLUMN_COUNT> {};

            last_start.fill(NO_OFFSET);

            auto placed_count = std::int32_t { 0 };

            for (auto& note: notes)
            {
                auto const start = note.offset;
                auto const end = note.offset + note.length;

                for (auto column = std::size_t { 0 }; column < COLUMN_COUNT; ++column)
                {
                    // Nothing that ended before the minimum gap can collide
                    // with this note any more.
                    while (!occupied[column].empty() &&
                           occupied[column].front().end + MIN_SAME_LANE_GAP < start)
                        occupied[column].pop_front();
                }

                auto best = -1;
                auto best_cost = (std::numeric_limits<std::int32_t>::max)();

                for (auto column = 0; column < static_cast<int>(COLUMN_COUNT); ++column)
                {
                    auto blocked = false;

                    for (auto const& other: occupied[column])
                    {
                        if (!conflicts({ start, end }, other))
                            continue;

                        blocked = true;
                        break;
                    }

                    if (blocked)
                        continue;

                    auto cost = 0;

                    if (note.source_column == SCRATCH_COLUMN)
                    {
                        // Scratch notes belong on the turntable. Only a hard
                        // collision (both sides scratching at the same instant)
                        // moves one of them off it, and then the fallback lane
                        // is picked by the usual jack penalty.
                        if (column != SCRATCH_COLUMN)
                        {
                            cost += SCRATCH_EXIT_COST;

                            auto const gap = start - last_start[column];

                            if (gap < JACK_WINDOW)
                                cost += static_cast<std::int32_t>(JACK_WINDOW - gap) * JACK_WEIGHT;
                        }
                    }
                    else
                    {
                        if (column != note.source_column)
                            cost += MOVE_COST;

                        if (column == SCRATCH_COLUMN)
                            cost += SCRATCH_COST;

                        auto const gap = start - last_start[column];

                        if (gap < JACK_WINDOW)
                            cost += static_cast<std::int32_t>(JACK_WINDOW - gap) * JACK_WEIGHT;
                    }

                    if (cost < best_cost)
                    {
                        best_cost = cost;
                        best = column;
                    }
                }

                if (best < 0)
                    continue;

                note.placed = true;
                note.column = static_cast<std::int8_t>(best);
                ++placed_count;

                occupied[best].push_back({ start, end });
                last_start[best] = start;
            }

            auto converted = std::vector<event> {};
            converted.reserve(notes.size() * 2 + extras.size());

            // The keysound of a column only has to be re-announced when it
            // changes, since the game keeps the last one around.
            auto last_sample = std::array<std::int16_t, COLUMN_COUNT> {};
            last_sample.fill(NO_SAMPLE);

            // The game only hands us a fixed size buffer, and a handful of the
            // densest charts would not fit with a keysound in front of every
            // note. Those tail notes are left silent instead of dropping the
            // whole chart.
            auto const limit =
                static_cast<std::size_t>(bm2dx::CHART_EVENT_CAPACITY);
            auto extras_pushed = std::size_t { 0 };

            auto extra = extras.begin();

            for (auto const& note: notes)
            {
                while (extra != extras.end() && extra->offset <= note.offset)
                {
                    converted.push_back(*extra++);
                    ++extras_pushed;
                }

                auto const extras_left = extras.size() - extras_pushed;
                auto const room = extras_left < limit ? limit - extras_left: 0;

                if (!note.placed)
                {
                    // No lane is free for this note. Keep its keysound alive
                    // by playing it from the background channel instead.
                    if (note.sample != NO_SAMPLE && converted.size() + 1 <= room)
                        converted.push_back(event { note.offset, BGM,
                            static_cast<std::int8_t>(BGM_CHANNEL), note.sample });

                    continue;
                }

                auto const column = static_cast<std::size_t>(note.column);
                auto const wants_sample =
                    note.sample != NO_SAMPLE && last_sample[column] != note.sample;

                if (converted.size() + (wants_sample ? 2u: 1u) > room)
                    continue;

                if (wants_sample)
                {
                    converted.push_back(event { note.offset, SAMPLE_P1,
                        note.column, note.sample });

                    last_sample[column] = note.sample;
                }

                converted.push_back(event { note.offset, NOTE_P1, note.column,
                    static_cast<std::int16_t>(note.length) });
            }

            while (extra != extras.end() && converted.size() < limit)
                converted.push_back(*extra++);

            // Samples dropped from the stream can leave two identical
            // background events next to each other.
            auto const end = std::unique(converted.begin(), converted.end(),
                [](const event& a, const event& b)
                {
                    return a.type == BGM && b.type == BGM &&
                        a.offset == b.offset && a.parameter == b.parameter &&
                        a.value == b.value;
                });

            converted.erase(end, converted.end());

            // The note counter fields describe the merged chart now.
            for (auto& record: converted)
            {
                if (record.type != NOTE_COUNT)
                    continue;

                record.value = static_cast<std::int16_t>(
                    record.parameter == 0 ? placed_count: 0);
            }

            return converted;
        }
    }

    auto convert_in_place(std::uint8_t* buffer, const std::size_t capacity)
        -> std::size_t
    {
        if (buffer == nullptr || capacity < urafumen::EVENT_SIZE)
            return 0;

        auto parsed = urafumen::parse_events_until_eos({ buffer, capacity });
        if (!parsed)
            return 0;

        auto const converted = merge(*parsed);
        auto const written = converted.size() * urafumen::EVENT_SIZE;

        if (written > capacity)
            return 0;

        auto* cursor = buffer;

        for (auto const& record: converted)
        {
            bm2dx::write_chart_event(cursor, record);
            cursor += urafumen::EVENT_SIZE;
        }

        return written;
    }
}
