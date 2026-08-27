#pragma once

#include "dungeon/Dungeon.h"

#include <cstdint>

class DungeonGenerator
{
public:
    [[nodiscard]] Dungeon generate(std::uint32_t seed) const;

private:
    static bool overlapsExisting(const Room& candidate, const std::vector<Room>& rooms);
    static void carveRoom(Dungeon& dungeon, const Room& room);
    static void carveCorridor(Dungeon& dungeon, GridPoint first, GridPoint second, bool horizontalFirst);
    static void carveCorridorFloor(Dungeon& dungeon, int x, int y);
};

