#pragma once

#include "dungeon/Dungeon.h"
#include "items/Inventory.h"

#include <raylib.h>

#include <random>

class InputManager;

class Player
{
public:
    void reset(Vector2 position);
    void update(const InputManager& input, const Dungeon& dungeon, float deltaTime);

    [[nodiscard]] Rectangle collider() const;
    [[nodiscard]] Rectangle attackHitbox() const;
    [[nodiscard]] Vector2 position() const { return position_; }
    [[nodiscard]] Vector2 direction() const { return direction_; }
    [[nodiscard]] float size() const { return size_; }

    [[nodiscard]] bool attackStarted() const { return attackStarted_; }
    [[nodiscard]] bool attackActive() const { return attackTimer_ > 0.0F; }
    [[nodiscard]] bool isDodging() const { return dodgeTimer_ > 0.0F; }
    [[nodiscard]] bool isInvulnerable() const { return invulnerabilityTimer_ > 0.0F || isDodging(); }
    [[nodiscard]] bool isRaging() const { return rageTimer_ > 0.0F; }
    [[nodiscard]] bool isDead() const { return hp_ <= 0; }

    [[nodiscard]] int hp() const { return hp_; }
    [[nodiscard]] int maxHp() const { return maxHp_; }
    [[nodiscard]] int attack() const;
    [[nodiscard]] float speed() const;
    [[nodiscard]] float criticalChance() const;
    [[nodiscard]] float rageRemaining() const { return rageTimer_; }
    [[nodiscard]] float invulnerabilityRemaining() const { return invulnerabilityTimer_; }
    [[nodiscard]] int permanentAttackBonus() const { return permanentAttackBonus_; }

    [[nodiscard]] int rollAttackDamage(std::mt19937& random, bool& critical) const;
    bool takeDamage(int damage);
    int heal(int amount);
    void addPermanentAttack(int amount);

    bool useSachet();
    bool useCatnip();
    Inventory& inventory() { return inventory_; }
    [[nodiscard]] const Inventory& inventory() const { return inventory_; }

private:
    void moveWithCollision(Vector2 delta, const Dungeon& dungeon);
    [[nodiscard]] float attackInterval() const;

    Vector2 position_{};
    Vector2 direction_{1.0F, 0.0F};
    Vector2 dodgeDirection_{1.0F, 0.0F};
    float size_ = 18.0F;

    int hp_ = 100;
    int maxHp_ = 100;
    int baseAttack_ = 10;
    int permanentAttackBonus_ = 0;
    float baseSpeed_ = 105.0F;
    float baseAttackInterval_ = 0.38F;
    float baseCriticalChance_ = 0.10F;

    float attackCooldown_ = 0.0F;
    float attackTimer_ = 0.0F;
    float invulnerabilityTimer_ = 0.0F;
    float dodgeTimer_ = 0.0F;
    float dodgeCooldown_ = 0.0F;
    float rageTimer_ = 0.0F;
    float rageCooldown_ = 0.0F;
    bool attackStarted_ = false;

    Inventory inventory_;
};

