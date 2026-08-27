#include "entities/Enemy.h"

#include <algorithm>
#include <cmath>

namespace
{
Vector2 directionTo(const Vector2 from, const Vector2 to)
{
    const Vector2 delta{to.x - from.x, to.y - from.y};
    const float magnitude = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    return magnitude > 0.001F ? Vector2{delta.x / magnitude, delta.y / magnitude} : Vector2{};
}
} // namespace

Enemy::Enemy(const EnemyType type, const Vector2 position, const int roomId)
    : type_(type), position_(position), roomId_(roomId)
{
    configureStats();
}

void Enemy::update(const Vector2 playerPosition, const Dungeon& dungeon, const float deltaTime)
{
    if (isDead())
    {
        return;
    }

    attackCooldown_ = std::max(0.0F, attackCooldown_ - deltaTime);
    stateTimer_ = std::max(0.0F, stateTimer_ - deltaTime);
    const float distance = distanceTo(playerPosition);
    const Vector2 towardPlayer = directionTo(position_, playerPosition);

    switch (state_)
    {
    case EnemyState::Idle:
        if (distance <= detectionRange_)
        {
            state_ = EnemyState::Chase;
        }
        break;

    case EnemyState::Chase:
        facing_ = towardPlayer;
        if (type_ == EnemyType::Bird)
        {
            if (distance > 88.0F)
            {
                moveWithCollision(Vector2{towardPlayer.x * speed_ * deltaTime, towardPlayer.y * speed_ * deltaTime}, dungeon);
            }
            else if (distance < 55.0F)
            {
                moveWithCollision(Vector2{-towardPlayer.x * speed_ * deltaTime, -towardPlayer.y * speed_ * deltaTime}, dungeon);
            }
            if (attackCooldown_ <= 0.0F && distance < 145.0F)
            {
                beginTelegraph(playerPosition);
            }
        }
        else
        {
            if (distance > attackRange_)
            {
                moveWithCollision(Vector2{towardPlayer.x * speed_ * deltaTime, towardPlayer.y * speed_ * deltaTime}, dungeon);
            }
            if (attackCooldown_ <= 0.0F && distance <= attackRange_ + 4.0F)
            {
                beginTelegraph(playerPosition);
            }
        }
        break;

    case EnemyState::Telegraph:
        facing_ = attackDirection_;
        if (stateTimer_ <= 0.0F)
        {
            state_ = EnemyState::Attack;
            stateTimer_ = type_ == EnemyType::StrayDog ? 0.34F : 0.18F;
            damageApplied_ = false;
        }
        break;

    case EnemyState::Attack:
        if (type_ == EnemyType::Bird || type_ == EnemyType::StrayDog)
        {
            const float attackSpeed = type_ == EnemyType::Bird ? 205.0F : 230.0F;
            moveWithCollision(Vector2{attackDirection_.x * attackSpeed * deltaTime, attackDirection_.y * attackSpeed * deltaTime}, dungeon);
        }
        if (stateTimer_ <= 0.0F)
        {
            state_ = EnemyState::Recover;
            stateTimer_ = type_ == EnemyType::StrayDog ? 0.75F : 0.42F;
            attackCooldown_ = type_ == EnemyType::Bird ? 1.05F : (type_ == EnemyType::StrayDog ? 1.35F : 0.72F);
        }
        break;

    case EnemyState::Recover:
    case EnemyState::Hurt:
        if (stateTimer_ <= 0.0F)
        {
            state_ = EnemyState::Chase;
        }
        break;

    case EnemyState::Dead:
        break;
    }
}

bool Enemy::takeDamage(const int damage, const Vector2 knockbackDirection, const Dungeon& dungeon)
{
    if (isDead())
    {
        return false;
    }

    hp_ = std::max(0, hp_ - damage);
    moveWithCollision(Vector2{knockbackDirection.x * 7.0F, knockbackDirection.y * 7.0F}, dungeon);
    if (hp_ <= 0)
    {
        state_ = EnemyState::Dead;
        stateTimer_ = 0.0F;
        return true;
    }

    state_ = EnemyState::Hurt;
    stateTimer_ = 0.14F;
    return false;
}

Rectangle Enemy::collider() const
{
    return Rectangle{position_.x - size_ * 0.5F, position_.y - size_ * 0.5F, size_, size_};
}

Rectangle Enemy::attackCollider() const
{
    const float reach = type_ == EnemyType::StrayDog ? 18.0F : 12.0F;
    const float attackSize = type_ == EnemyType::StrayDog ? 30.0F : 22.0F;
    const Vector2 center{position_.x + attackDirection_.x * reach, position_.y + attackDirection_.y * reach};
    return Rectangle{center.x - attackSize * 0.5F, center.y - attackSize * 0.5F, attackSize, attackSize};
}

void Enemy::configureStats()
{
    switch (type_)
    {
    case EnemyType::Rat:
        hp_ = maxHp_ = 22;
        damage_ = 7;
        size_ = 14.0F;
        speed_ = 48.0F;
        attackRange_ = 20.0F;
        break;
    case EnemyType::Bird:
        hp_ = maxHp_ = 18;
        damage_ = 9;
        size_ = 15.0F;
        speed_ = 72.0F;
        attackRange_ = 78.0F;
        break;
    case EnemyType::StrayDog:
        hp_ = maxHp_ = 58;
        damage_ = 16;
        size_ = 24.0F;
        speed_ = 58.0F;
        detectionRange_ = 205.0F;
        attackRange_ = 29.0F;
        break;
    }
}

void Enemy::moveWithCollision(const Vector2 delta, const Dungeon& dungeon)
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

void Enemy::beginTelegraph(const Vector2 playerPosition)
{
    state_ = EnemyState::Telegraph;
    attackDirection_ = directionTo(position_, playerPosition);
    stateTimer_ = type_ == EnemyType::StrayDog ? 0.58F : (type_ == EnemyType::Bird ? 0.38F : 0.28F);
}

float Enemy::distanceTo(const Vector2 target) const
{
    const float x = target.x - position_.x;
    const float y = target.y - position_.y;
    return std::sqrt(x * x + y * y);
}

const char* enemyTypeName(const EnemyType type)
{
    switch (type)
    {
    case EnemyType::Rat: return "Rato";
    case EnemyType::Bird: return "Passaro";
    case EnemyType::StrayDog: return "Vira-lata";
    }
    return "Inimigo";
}

const char* enemyStateName(const EnemyState state)
{
    switch (state)
    {
    case EnemyState::Idle: return "IDLE";
    case EnemyState::Chase: return "CHASE";
    case EnemyState::Telegraph: return "TELEGRAPH";
    case EnemyState::Attack: return "ATTACK";
    case EnemyState::Recover: return "RECOVER";
    case EnemyState::Hurt: return "HURT";
    case EnemyState::Dead: return "DEAD";
    }
    return "?";
}

