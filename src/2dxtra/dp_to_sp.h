#pragma once

#include <cstddef>
#include <cstdint>

namespace dp_to_sp
{
    // Merges the second player's side of a double play chart onto the first
    // player's lanes so that the chart can be played by a single player.
    //
    // Returns the number of bytes written, or 0 when the buffer could not be
    // converted (in which case the buffer is left untouched).
    auto convert_in_place(std::uint8_t* buffer, std::size_t capacity) -> std::size_t;
}
