#include "entities/Player.h"

#include "input/GameAction.h"
#include "input/InputManager.h"
#include "systems/AttributeModifiers.h"

#include <algorithm>
#include <cmath>

namespace
{
float length(const Vector2 value)
{
    return std::sqrt(value.x * value.x + value.y * value.y);
}

Vector2 normalized(const Vector2 value)
{
    const float magnitude = length(value);
    return magnitude > 0.001F ? Vector2{value.x / magnitude, value.y / magnitude} : Vector2{};
}
} // namespace

void Player::reset(const Vector2 position)
{
    position_ = position;
    direction_ = Vector2{1.0F, 0.0F};
    dodgeDirection_ = direction_;
    hp_ = maxHp_;
    permanentAttackBonus_ = 0;
    attackCooldown_ = 0.0F;
    attackTimer_ = 0.0F;
    invulnerabilityTimer_ = 0.0F;
    dodgeTimer_ = 0.0F;
    dodgeCooldown_ = 0.0F;
    rageTimer_ = 0.0F;
    rageCooldown_ = 0.0F;
    attackStarted_ = false;
    inventory_.clear();
    inventory_.add(ItemType::Sachet);
}

void Player::update(const InputManager& input, const Dungeon& dungeon, const float deltaTime)
{
    attackStarted_ = false;
    attackCooldown_ = std::max(0.0F, attackCooldown_ - deltaTime);
    attackTimer_ = std::max(0.0F, attackTimer_ - deltaTime);
    invulnerabilityTimer_ = std::max(0.0F, invulnerabilityTimer_ - deltaTime);
    dodgeTimer_ = std::max(0.0F, dodgeTimer_ - deltaTime);
    dodgeCooldown_ = std::max(0.0F, dodgeCooldown_ - deltaTime);
    rageTimer_ = std::max(0.0F, rageTimer_ - deltaTime);
    rageCooldown_ = std::max(0.0F, rageCooldown_ - deltaTime);

    const Vector2 movement = input.movement();
    if (length(movement) > 0.05F)
    {
        direction_ = normalized(movement);
    }

    if (input.isActionPressed(GameAction::Dodge) && dodgeCooldown_ <= 0.0F)
    {
        dodgeDirection_ = length(movement) > 0.05F ? normalized(movement) : direction_;
        dodgeTimer_ = 0.20F;
        dodgeCooldown_ = 0.85F;
    }

    if (input.isActionPressed(GameAction::Attack) && attackCooldown_ <= 0.0F && dodgeTimer_ <= 0.0F)
    {
        attackCooldown_ = attackInterval();
        attackTimer_ = 0.12F;
        attackStarted_ = true;
    }

    const Vector2 velocity = dodgeTimer_ > 0.0F
        ? Vector2{dodgeDirection_.x * 245.0F, dodgeDirection_.y * 245.0F}
        : Vector2{movement.x * speed(), movement.y * speed()};
    moveWithCollision(Vector2{velocity.x * deltaTime, velocity.y * deltaTime}, dungeon);
}

Rectangle Player::collider() const
{
    return Rectangle{position_.x - size_ * 0.5F, position_.y - size_ * 0.5F, size_, size_};
}

Rectangle Player::attackHitbox() const
{
    constexpr float hitboxSize = 28.0F;
    constexpr float reach = 17.0F;
    const Vector2 center{position_.x + direction_.x * reach, position_.y + direction_.y * reach};
    return Rectangle{center.x - hitboxSize * 0.5F, center.y - hitboxSize * 0.5F, hitboxSize, hitboxSize};
}

int Player::attack() const
{
    const CombatAttributes base{baseAttack_, baseSpeed_, baseAttackInterval_, baseCriticalChance_};
    return calculateRunAttributes(base, permanentAttackBonus_, isRaging()).attack;
}

float Player::speed() const
{
    const CombatAttributes base{baseAttack_, baseSpeed_, baseAttackInterval_, baseCriticalChance_};
    return calculateRunAttributes(base, permanentAttackBonus_, isRaging()).speed;
}

float Player::criticalChance() const
{
    const CombatAttributes base{baseAttack_, baseSpeed_, baseAttackInterval_, baseCriticalChance_};
    return calculateRunAttributes(base, permanentAttackBonus_, isRaging()).criticalChance;
}

int Player::rollAttackDamage(std::mt19937& random, bool& critical) const
{
    std::uniform_real_distribution<float> chance(0.0F, 1.0F);
    critical = chance(random) < criticalChance();
    return critical ? attack() * 2 : attack();
}

bool Player::takeDamage(const int damage)
{
    if (isInvulnerable() || isDead())
    {
        return false;
    }
    hp_ = std::max(0, hp_ - damage);
    invulnerabilityTimer_ = 0.72F;
    return true;
}

int Player::heal(const int amount)
{
    const int previous = hp_;
    hp_ = std::min(maxHp_, hp_ + amount);
    return hp_ - previous;
}

void Player::addPermanentAttack(const int amount)
{
    permanentAttackBonus_ += std::max(0, amount);
}

bool Player::useSachet()
{
    if (hp_ >= maxHp_ || !inventory_.consume(ItemType::Sachet))
    {
        return false;
    }
    heal(32);
    return true;
}

bool Player::useCatnip()
{
    if (rageCooldown_ > 0.0F || !inventory_.consume(ItemType::Catnip))
    {
        return false;
    }
    rageTimer_ = 8.0F;
    rageCooldown_ = 12.0F;
    return true;
}

void Player::moveWithCollision(const Vector2 delta, const Dungeon& dungeon)
{
    position_.x += delta.x;
    if (dungeon.collidesWorld(collider().x, collider().y, collider().width, collider().height))
    {
        position_.x -= delta.x;
    }

    position_.y += delta.y;
    if (dungeon.collidesWorld(collider().x, collider().y, collider().width, collider().height))
    {
        position_.y -= delta.y;
    }
}

float Player::attackInterval() const
{
    const CombatAttributes base{baseAttack_, baseSpeed_, baseAttackInterval_, baseCriticalChance_};
    return calculateRunAttributes(base, permanentAttackBonus_, isRaging()).attackInterval;
}
