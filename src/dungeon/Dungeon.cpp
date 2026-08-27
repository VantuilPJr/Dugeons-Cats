#include "dungeon/Dungeon.h"

#include <algorithm>

TileType Dungeon::tileAt(const int x, const int y) const
{
    if (x < 0 || y < 0 || x >= kWidth || y >= kHeight)
    {
        return TileType::Wall;
    }
    return tiles_[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
}

void Dungeon::setTile(const int x, const int y, const TileType type)
{
    if (x >= 0 && y >= 0 && x < kWidth && y < kHeight)
    {
        tiles_[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] = type;
    }
}

bool Dungeon::isWalkableTile(const int x, const int y) const
{
    return tileAt(x, y) == TileType::Floor;
}

bool Dungeon::collidesWorld(const float x, const float y, const float width, const float height) const
{
    constexpr float inset = 0.5F;
    const int left = static_cast<int>((x + inset) / kTileSize);
    const int right = static_cast<int>((x + width - inset) / kTileSize);
    const int top = static_cast<int>((y + inset) / kTileSize);
    const int bottom = static_cast<int>((y + height - inset) / kTileSize);

    return !isWalkableTile(left, top) || !isWalkableTile(right, top)
        || !isWalkableTile(left, bottom) || !isWalkableTile(right, bottom);
}

int Dungeon::roomAtWorld(const float x, const float y) const
{
    const int tileX = static_cast<int>(x / kTileSize);
    const int tileY = static_cast<int>(y / kTileSize);
    for (const Room& room : rooms_)
    {
        if (room.contains(tileX, tileY))
        {
            return room.id;
        }
    }
    return -1;
}

GridPoint Dungeon::roomCenter(const int roomId) const
{
    if (roomId < 0 || roomId >= static_cast<int>(rooms_.size()))
    {
        return {};
    }
    return rooms_[static_cast<std::size_t>(roomId)].center();
}

