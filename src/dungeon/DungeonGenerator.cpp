#include "dungeon/DungeonGenerator.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

Dungeon DungeonGenerator::generate(const std::uint32_t seed) const
{
    Dungeon dungeon;
    dungeon.setSeed(seed);
    std::mt19937 random(seed);
    std::uniform_int_distribution<int> roomWidth(8, 13);
    std::uniform_int_distribution<int> roomHeight(6, 10);
    std::uniform_int_distribution<int> roomX(2, Dungeon::kWidth - 15);
    std::uniform_int_distribution<int> roomY(2, Dungeon::kHeight - 12);

    auto& rooms = dungeon.rooms();
    constexpr int targetRoomCount = 12;
    for (int attempt = 0; attempt < 500 && static_cast<int>(rooms.size()) < targetRoomCount; ++attempt)
    {
        Room candidate;
        candidate.id = static_cast<int>(rooms.size());
        candidate.width = roomWidth(random);
        candidate.height = roomHeight(random);
        candidate.x = std::min(roomX(random), Dungeon::kWidth - candidate.width - 2);
        candidate.y = std::min(roomY(random), Dungeon::kHeight - candidate.height - 2);

        if (!overlapsExisting(candidate, rooms))
        {
            rooms.push_back(candidate);
        }
    }

    if (rooms.size() < 6)
    {
        rooms.clear();
        const GridPoint positions[]{{3, 3}, {20, 3}, {38, 3}, {3, 23}, {20, 23}, {39, 23}};
        for (int index = 0; index < 6; ++index)
        {
            Room fallback;
            fallback.id = index;
            fallback.x = positions[index].x;
            fallback.y = positions[index].y;
            fallback.width = 10;
            fallback.height = 8;
            rooms.push_back(fallback);
        }
    }

    for (std::size_t index = 0; index < rooms.size(); ++index)
    {
        dungeon.graph().addVertex();
    }

    for (int roomId = 1; roomId < static_cast<int>(rooms.size()); ++roomId)
    {
        int nearest = 0;
        int nearestDistance = std::numeric_limits<int>::max();
        const GridPoint center = rooms[static_cast<std::size_t>(roomId)].center();
        for (int candidate = 0; candidate < roomId; ++candidate)
        {
            const GridPoint other = rooms[static_cast<std::size_t>(candidate)].center();
            const int distance = std::abs(center.x - other.x) + std::abs(center.y - other.y);
            if (distance < nearestDistance)
            {
                nearestDistance = distance;
                nearest = candidate;
            }
        }
        dungeon.graph().addConnection(roomId, nearest);
    }

    if (rooms.size() > 4)
    {
        std::uniform_int_distribution<int> roomIndex(0, static_cast<int>(rooms.size()) - 1);
        for (int extra = 0; extra < 3; ++extra)
        {
            const int first = roomIndex(random);
            const int second = roomIndex(random);
            if (first != second)
            {
                dungeon.graph().addConnection(first, second);
            }
        }
    }

    const auto distances = dungeon.graph().breadthFirstDistances(0);
    int bossRoom = 0;
    for (int index = 1; index < static_cast<int>(distances.size()); ++index)
    {
        if (distances[index] > distances[bossRoom])
        {
            bossRoom = index;
        }
    }
    dungeon.setStartRoomId(0);
    dungeon.setBossRoomId(bossRoom);

    for (Room& room : rooms)
    {
        if (room.id == 0)
        {
            room.type = RoomType::Start;
        }
        else if (room.id == bossRoom)
        {
            room.type = RoomType::Boss;
        }
        else
        {
            switch (room.id % 5)
            {
            case 1: room.type = RoomType::Combat; break;
            case 2: room.type = RoomType::Treasure; break;
            case 3: room.type = RoomType::Scratcher; break;
            case 4: room.type = RoomType::Combat; break;
            default: room.type = RoomType::Normal; break;
            }
        }
        carveRoom(dungeon, room);
    }

    const auto dfsOrder = dungeon.graph().depthFirstTraversal(0);
    for (const int roomId : dfsOrder)
    {
        rooms[static_cast<std::size_t>(roomId)].visited = true;
    }

    for (Room& room : rooms)
    {
        room.neighbors = dungeon.graph().neighbors(room.id);
        for (const int neighbor : room.neighbors)
        {
            if (room.id < neighbor)
            {
                carveCorridor(dungeon, room.center(), rooms[static_cast<std::size_t>(neighbor)].center(), (random() & 1U) == 0U);
            }
        }
    }

    return dungeon;
}

bool DungeonGenerator::overlapsExisting(const Room& candidate, const std::vector<Room>& rooms)
{
    constexpr int margin = 2;
    for (const Room& room : rooms)
    {
        if (candidate.x - margin < room.x + room.width
            && candidate.x + candidate.width + margin > room.x
            && candidate.y - margin < room.y + room.height
            && candidate.y + candidate.height + margin > room.y)
        {
            return true;
        }
    }
    return false;
}

void DungeonGenerator::carveRoom(Dungeon& dungeon, const Room& room)
{
    for (int y = room.y; y < room.y + room.height; ++y)
    {
        for (int x = room.x; x < room.x + room.width; ++x)
        {
            const bool border = x == room.x || y == room.y || x == room.x + room.width - 1 || y == room.y + room.height - 1;
            dungeon.setTile(x, y, border ? TileType::Wall : TileType::Floor);
        }
    }
}

void DungeonGenerator::carveCorridor(Dungeon& dungeon, GridPoint first, GridPoint second, const bool horizontalFirst)
{
    auto carveHorizontal = [&dungeon](int from, const int to, const int y) {
        const int direction = from <= to ? 1 : -1;
        while (from != to + direction)
        {
            carveCorridorFloor(dungeon, from, y);
            from += direction;
        }
    };
    auto carveVertical = [&dungeon](int from, const int to, const int x) {
        const int direction = from <= to ? 1 : -1;
        while (from != to + direction)
        {
            carveCorridorFloor(dungeon, x, from);
            from += direction;
        }
    };

    if (horizontalFirst)
    {
        carveHorizontal(first.x, second.x, first.y);
        carveVertical(first.y, second.y, second.x);
    }
    else
    {
        carveVertical(first.y, second.y, first.x);
        carveHorizontal(first.x, second.x, second.y);
    }
}

void DungeonGenerator::carveCorridorFloor(Dungeon& dungeon, const int x, const int y)
{
    for (int offset = -1; offset <= 1; ++offset)
    {
        for (int side = -2; side <= 2; ++side)
        {
            if (dungeon.tileAt(x + side, y + offset) == TileType::Empty)
            {
                dungeon.setTile(x + side, y + offset, TileType::Wall);
            }
            if (dungeon.tileAt(x + offset, y + side) == TileType::Empty)
            {
                dungeon.setTile(x + offset, y + side, TileType::Wall);
            }
        }
    }

    for (int offset = -1; offset <= 1; ++offset)
    {
        dungeon.setTile(x + offset, y, TileType::Floor);
        dungeon.setTile(x, y + offset, TileType::Floor);
    }
}
