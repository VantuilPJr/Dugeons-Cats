#pragma once

#include "dungeon/DungeonGraph.h"
#include "dungeon/Room.h"

#include <array>
#include <cstdint>
#include <vector>

enum class TileType : std::uint8_t
{
    Empty,
    Floor,
    Wall,
    Obstacle,
};

class Dungeon
{
public:
    // Distrito amplo: continua sendo uma dungeon de um piso, mas com escala
    // suficiente para funcionar como uma zona urbana aberta e conectada.
    static constexpr int kWidth = 112;
    static constexpr int kHeight = 72;
    static constexpr int kTileSize = 24;

    using TileRow = std::array<TileType, kWidth>;
    using TileMatrix = std::array<TileRow, kHeight>;

    [[nodiscard]] TileType tileAt(int x, int y) const;
    void setTile(int x, int y, TileType type);
    [[nodiscard]] bool isWalkableTile(int x, int y) const;
    [[nodiscard]] bool collidesWorld(float x, float y, float width, float height) const;
    [[nodiscard]] int roomAtWorld(float x, float y) const;
    [[nodiscard]] GridPoint roomCenter(int roomId) const;

    [[nodiscard]] const TileMatrix& tiles() const { return tiles_; }
    [[nodiscard]] const std::vector<Room>& rooms() const { return rooms_; }
    [[nodiscard]] std::vector<Room>& rooms() { return rooms_; }
    [[nodiscard]] const DungeonGraph& graph() const { return graph_; }
    [[nodiscard]] DungeonGraph& graph() { return graph_; }
    [[nodiscard]] int startRoomId() const { return startRoomId_; }
    [[nodiscard]] int bossRoomId() const { return bossRoomId_; }
    [[nodiscard]] std::uint32_t seed() const { return seed_; }

    void setStartRoomId(int id) { startRoomId_ = id; }
    void setBossRoomId(int id) { bossRoomId_ = id; }
    void setSeed(std::uint32_t seed) { seed_ = seed; }

private:
    TileMatrix tiles_{}; // MATRIZ academica que governa desenho e colisao.
    std::vector<Room> rooms_;
    DungeonGraph graph_;
    int startRoomId_ = 0;
    int bossRoomId_ = 0;
    std::uint32_t seed_ = 0;
};
