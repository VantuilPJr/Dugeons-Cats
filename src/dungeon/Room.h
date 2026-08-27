#pragma once

#include <vector>

enum class RoomType
{
    Start,
    Combat,
    Treasure,
    Scratcher,
    Normal,
    Boss,
};

struct GridPoint
{
    int x = 0;
    int y = 0;

    bool operator==(const GridPoint& other) const { return x == other.x && y == other.y; }
};

struct Room
{
    int id = -1;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    RoomType type = RoomType::Normal;
    bool visited = false;
    std::vector<int> neighbors;

    [[nodiscard]] GridPoint center() const { return GridPoint{x + width / 2, y + height / 2}; }

    [[nodiscard]] bool contains(const int tileX, const int tileY) const
    {
        return tileX >= x && tileX < x + width && tileY >= y && tileY < y + height;
    }
};

inline const char* roomTypeName(const RoomType type)
{
    switch (type)
    {
    case RoomType::Start: return "ENTRADA";
    case RoomType::Combat: return "COMBATE";
    case RoomType::Treasure: return "TESOURO";
    case RoomType::Scratcher: return "ARRANHADOR";
    case RoomType::Normal: return "RUA";
    case RoomType::Boss: return "REI DAS RUAS";
    }
    return "SALA";
}

