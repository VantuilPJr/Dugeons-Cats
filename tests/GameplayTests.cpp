#include "dungeon/DungeonGenerator.h"
#include "entities/CarameloBoss.h"
#include "entities/Enemy.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
int failures = 0;

void expect(const bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

Vector2 worldCenter(const Dungeon& dungeon, const int roomId)
{
    const GridPoint center = dungeon.roomCenter(roomId);
    return Vector2{static_cast<float>(center.x * Dungeon::kTileSize + 8), static_cast<float>(center.y * Dungeon::kTileSize + 8)};
}
} // namespace

int main()
{
    Dungeon dungeon = DungeonGenerator{}.generate(20260827U);
    const int roomId = dungeon.bossRoomId();
    const Vector2 center = worldCenter(dungeon, roomId);

    Enemy rat(EnemyType::Rat, center, roomId);
    expect(!rat.isDead(), "rat starts alive");
    expect(rat.takeDamage(999, Vector2{1.0F, 0.0F}, dungeon), "lethal enemy damage reports death");
    expect(rat.isDead(), "rat reaches dead state");

    CarameloBoss boss;
    boss.reset(center, roomId);
    boss.engage();
    for (int step = 0; step < 30; ++step)
    {
        boss.update(Vector2{center.x + 40.0F, center.y}, dungeon, 0.05F);
    }
    expect(boss.engaged() && !boss.isDead(), "boss state machine runs while engaged");

    boss.takeDamage(boss.maxHp() / 2);
    boss.update(Vector2{center.x + 40.0F, center.y}, dungeon, 0.05F);
    expect(boss.phaseTwo(), "boss changes to phase two at half HP");
    expect(boss.state() == BossState::PhaseTransition, "boss exposes phase transition state");

    for (int step = 0; step < 30; ++step)
    {
        boss.update(Vector2{center.x + 40.0F, center.y}, dungeon, 0.05F);
    }
    expect(boss.state() != BossState::PhaseTransition, "phase transition finishes");
    expect(boss.takeDamage(9999), "lethal boss damage reports victory");
    expect(boss.isDead(), "boss reaches dead state");

    if (failures == 0)
    {
        std::cout << "All gameplay tests passed.\n";
        return EXIT_SUCCESS;
    }

    std::cerr << failures << " gameplay test(s) failed.\n";
    return EXIT_FAILURE;
}

