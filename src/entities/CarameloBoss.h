#pragma once

#include "dungeon/Dungeon.h"

#include <raylib.h>

enum class BossState
{
    Idle,
    ChooseAttack,
    Telegraph,
    Attack,
    Recover,
    PhaseTransition,
    Dead,
};

enum class BossAttack
{
    Bite,
    Paw,
    Charge,
    Bark,
};

class CarameloBoss
{
public:
    void reset(Vector2 position, int roomId);
    void update(Vector2 playerPosition, const Dungeon& dungeon, float deltaTime);
    bool takeDamage(int damage);

    [[nodiscard]] Rectangle collider() const;
    [[nodiscard]] Rectangle attackCollider() const;
    [[nodiscard]] bool attackHits(Rectangle target) const;
    [[nodiscard]] bool canDealDamage() const { return state_ == BossState::Attack && !damageApplied_; }
    void markDamageApplied() { damageApplied_ = true; }

    [[nodiscard]] Vector2 position() const { return position_; }
    [[nodiscard]] Vector2 attackDirection() const { return attackDirection_; }
    [[nodiscard]] int roomId() const { return roomId_; }
    [[nodiscard]] int hp() const { return hp_; }
    [[nodiscard]] int maxHp() const { return maxHp_; }
    [[nodiscard]] int damage() const;
    [[nodiscard]] bool phaseTwo() const { return phaseTwo_; }
    [[nodiscard]] bool isDead() const { return state_ == BossState::Dead; }
    [[nodiscard]] bool engaged() const { return engaged_; }
    void engage() { engaged_ = true; }
    [[nodiscard]] BossState state() const { return state_; }
    [[nodiscard]] BossAttack attack() const { return attack_; }
    [[nodiscard]] float stateTimer() const { return stateTimer_; }
    [[nodiscard]] float barkRadius() const;

private:
    void chooseAttack(Vector2 playerPosition);
    void moveWithCollision(Vector2 delta, const Dungeon& dungeon);

    Vector2 position_{};
    Vector2 attackDirection_{1.0F, 0.0F};
    int roomId_ = -1;
    int hp_ = 260;
    int maxHp_ = 260;
    BossState state_ = BossState::Idle;
    BossAttack attack_ = BossAttack::Bite;
    float stateTimer_ = 0.8F;
    float attackDuration_ = 0.2F;
    int attackCounter_ = 0;
    int comboCharges_ = 0;
    bool phaseTwo_ = false;
    bool damageApplied_ = false;
    bool engaged_ = false;
};

const char* bossStateName(BossState state);
const char* bossAttackName(BossAttack attack);

