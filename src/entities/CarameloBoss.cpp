#include "entities/CarameloBoss.h"

#include <algorithm>
#include <cmath>

namespace
{
Vector2 directionTo(const Vector2 from, const Vector2 to)
{
    const float x = to.x - from.x;
    const float y = to.y - from.y;
    const float magnitude = std::sqrt(x * x + y * y);
    return magnitude > 0.001F ? Vector2{x / magnitude, y / magnitude} : Vector2{1.0F, 0.0F};
}
} // namespace

void CarameloBoss::reset(const Vector2 position, const int roomId)
{
    position_ = position;
    roomId_ = roomId;
    hp_ = maxHp_;
    state_ = BossState::Idle;
    attack_ = BossAttack::Bite;
    stateTimer_ = 0.8F;
    attackDuration_ = 0.2F;
    attackCounter_ = 0;
    comboCharges_ = 0;
    phaseTwo_ = false;
    damageApplied_ = false;
    engaged_ = false;
}

void CarameloBoss::update(const Vector2 playerPosition, const Dungeon& dungeon, const float deltaTime)
{
    if (!engaged_ || isDead())
    {
        return;
    }

    if (!phaseTwo_ && hp_ <= maxHp_ / 2)
    {
        phaseTwo_ = true;
        state_ = BossState::PhaseTransition;
        stateTimer_ = 1.25F;
        damageApplied_ = true;
        return;
    }

    stateTimer_ = std::max(0.0F, stateTimer_ - deltaTime);
    switch (state_)
    {
    case BossState::Idle:
        if (stateTimer_ <= 0.0F)
        {
            state_ = BossState::ChooseAttack;
            stateTimer_ = 0.08F;
        }
        break;

    case BossState::ChooseAttack:
        if (stateTimer_ <= 0.0F)
        {
            chooseAttack(playerPosition);
        }
        break;

    case BossState::Telegraph:
        if (stateTimer_ <= 0.0F)
        {
            state_ = BossState::Attack;
            damageApplied_ = false;
            switch (attack_)
            {
            case BossAttack::Bite: attackDuration_ = 0.22F; break;
            case BossAttack::Paw: attackDuration_ = 0.30F; break;
            case BossAttack::Charge: attackDuration_ = phaseTwo_ ? 0.52F : 0.46F; break;
            case BossAttack::Bark: attackDuration_ = phaseTwo_ ? 0.62F : 0.55F; break;
            }
            stateTimer_ = attackDuration_;
        }
        break;

    case BossState::Attack:
        if (attack_ == BossAttack::Charge)
        {
            const float speed = phaseTwo_ ? 330.0F : 275.0F;
            moveWithCollision(Vector2{attackDirection_.x * speed * deltaTime, attackDirection_.y * speed * deltaTime}, dungeon);
        }

        if (stateTimer_ <= 0.0F)
        {
            if (attack_ == BossAttack::Charge && comboCharges_ > 0)
            {
                --comboCharges_;
                attackDirection_ = directionTo(position_, playerPosition);
                state_ = BossState::Telegraph;
                stateTimer_ = 0.30F;
            }
            else
            {
                state_ = BossState::Recover;
                stateTimer_ = phaseTwo_ ? 0.48F : 0.82F;
            }
        }
        break;

    case BossState::Recover:
        if (stateTimer_ <= 0.0F)
        {
            state_ = BossState::Idle;
            stateTimer_ = phaseTwo_ ? 0.12F : 0.24F;
        }
        break;

    case BossState::PhaseTransition:
        if (stateTimer_ <= 0.0F)
        {
            state_ = BossState::Idle;
            stateTimer_ = 0.20F;
        }
        break;

    case BossState::Dead:
        break;
    }
}

bool CarameloBoss::takeDamage(const int damage)
{
    if (isDead() || state_ == BossState::PhaseTransition)
    {
        return false;
    }

    hp_ = std::max(0, hp_ - damage);
    if (hp_ <= 0)
    {
        state_ = BossState::Dead;
        stateTimer_ = 0.0F;
        return true;
    }
    return false;
}

Rectangle CarameloBoss::collider() const
{
    return Rectangle{position_.x - 23.0F, position_.y - 17.0F, 46.0F, 34.0F};
}

Rectangle CarameloBoss::attackCollider() const
{
    if (attack_ == BossAttack::Charge)
    {
        return collider();
    }

    const float reach = attack_ == BossAttack::Bite ? 28.0F : 22.0F;
    const float size = attack_ == BossAttack::Bite ? 40.0F : 58.0F;
    const Vector2 center{position_.x + attackDirection_.x * reach, position_.y + attackDirection_.y * reach};
    return Rectangle{center.x - size * 0.5F, center.y - size * 0.5F, size, size};
}

bool CarameloBoss::attackHits(const Rectangle target) const
{
    if (!canDealDamage())
    {
        return false;
    }

    if (attack_ != BossAttack::Bark)
    {
        return CheckCollisionRecs(attackCollider(), target);
    }

    const Vector2 targetCenter{target.x + target.width * 0.5F, target.y + target.height * 0.5F};
    const float x = targetCenter.x - position_.x;
    const float y = targetCenter.y - position_.y;
    const float distance = std::sqrt(x * x + y * y);
    const float radius = barkRadius();
    return distance <= radius && distance >= std::max(0.0F, radius - 24.0F);
}

int CarameloBoss::damage() const
{
    const int phaseBonus = phaseTwo_ ? 5 : 0;
    switch (attack_)
    {
    case BossAttack::Bite: return 18 + phaseBonus;
    case BossAttack::Paw: return 20 + phaseBonus;
    case BossAttack::Charge: return 23 + phaseBonus;
    case BossAttack::Bark: return 14 + phaseBonus;
    }
    return 18;
}

float CarameloBoss::barkRadius() const
{
    if (attack_ != BossAttack::Bark || state_ != BossState::Attack)
    {
        return 0.0F;
    }
    const float progress = 1.0F - stateTimer_ / attackDuration_;
    return progress * (phaseTwo_ ? 142.0F : 112.0F);
}

void CarameloBoss::chooseAttack(const Vector2 playerPosition)
{
    attack_ = static_cast<BossAttack>(attackCounter_ % 4);
    ++attackCounter_;
    attackDirection_ = directionTo(position_, playerPosition);
    comboCharges_ = (phaseTwo_ && attack_ == BossAttack::Charge) ? 1 : 0;
    state_ = BossState::Telegraph;

    float telegraph = 0.0F;
    switch (attack_)
    {
    case BossAttack::Bite: telegraph = 0.55F; break;
    case BossAttack::Paw: telegraph = 0.72F; break;
    case BossAttack::Charge: telegraph = 0.82F; break;
    case BossAttack::Bark: telegraph = 0.92F; break;
    }
    stateTimer_ = telegraph * (phaseTwo_ ? 0.72F : 1.0F);
}

void CarameloBoss::moveWithCollision(const Vector2 delta, const Dungeon& dungeon)
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

const char* bossStateName(const BossState state)
{
    switch (state)
    {
    case BossState::Idle: return "IDLE";
    case BossState::ChooseAttack: return "CHOOSE";
    case BossState::Telegraph: return "TELEGRAPH";
    case BossState::Attack: return "ATTACK";
    case BossState::Recover: return "RECOVER";
    case BossState::PhaseTransition: return "PHASE 2";
    case BossState::Dead: return "DEAD";
    }
    return "?";
}

const char* bossAttackName(const BossAttack attack)
{
    switch (attack)
    {
    case BossAttack::Bite: return "MORDIDA";
    case BossAttack::Paw: return "PATADA";
    case BossAttack::Charge: return "INVESTIDA";
    case BossAttack::Bark: return "LATIDO";
    }
    return "ATAQUE";
}

