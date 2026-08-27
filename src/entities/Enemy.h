#pragma once

#include "dungeon/Dungeon.h"

#include <raylib.h>

enum class EnemyType
{
    Rat,
    Bird,
    StrayDog,
};

enum class EnemyState
{
    Idle,
    Chase,
    Telegraph,
    Attack,
    Recover,
    Hurt,
    Dead,
};

class Enemy
{
public:
    Enemy() = default;
    Enemy(EnemyType type, Vector2 position, int roomId);

    void update(Vector2 playerPosition, const Dungeon& dungeon, float deltaTime);
    bool takeDamage(int damage, Vector2 knockbackDirection, const Dungeon& dungeon);

    [[nodiscard]] Rectangle collider() const;
    [[nodiscard]] Rectangle attackCollider() const;
    [[nodiscard]] bool canDealDamage() const { return state_ == EnemyState::Attack && !damageApplied_; }
    void markDamageApplied() { damageApplied_ = true; }

    [[nodiscard]] EnemyType type() const { return type_; }
    [[nodiscard]] EnemyState state() const { return state_; }
    [[nodiscard]] Vector2 position() const { return position_; }
    [[nodiscard]] Vector2 facing() const { return facing_; }
    [[nodiscard]] int hp() const { return hp_; }
    [[nodiscard]] int maxHp() const { return maxHp_; }
    [[nodiscard]] int damage() const { return damage_; }
    [[nodiscard]] int roomId() const { return roomId_; }
    [[nodiscard]] float stateTimer() const { return stateTimer_; }
    [[nodiscard]] bool isDead() const { return state_ == EnemyState::Dead; }
    [[nodiscard]] bool dropProcessed() const { return dropProcessed_; }
    void markDropProcessed() { dropProcessed_ = true; }

private:
    void configureStats();
    void moveWithCollision(Vector2 delta, const Dungeon& dungeon);
    void beginTelegraph(Vector2 playerPosition);
    [[nodiscard]] float distanceTo(Vector2 target) const;

    EnemyType type_ = EnemyType::Rat;
    EnemyState state_ = EnemyState::Idle;
    Vector2 position_{};
    Vector2 facing_{1.0F, 0.0F};
    Vector2 attackDirection_{1.0F, 0.0F};
    int roomId_ = -1;
    int hp_ = 1;
    int maxHp_ = 1;
    int damage_ = 1;
    float size_ = 14.0F;
    float speed_ = 45.0F;
    float detectionRange_ = 170.0F;
    float attackRange_ = 22.0F;
    float attackCooldown_ = 0.0F;
    float stateTimer_ = 0.0F;
    bool damageApplied_ = false;
    bool dropProcessed_ = false;
};

const char* enemyTypeName(EnemyType type);
const char* enemyStateName(EnemyState state);

