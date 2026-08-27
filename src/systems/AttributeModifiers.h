#pragma once

#include <algorithm>

struct CombatAttributes
{
    int attack = 0;
    float speed = 0.0F;
    float attackInterval = 1.0F;
    float criticalChance = 0.0F;
};

inline CombatAttributes calculateRunAttributes(const CombatAttributes& base, const int permanentAttackBonus, const bool felineRage)
{
    CombatAttributes result = base;
    result.attack += std::max(0, permanentAttackBonus);
    if (felineRage)
    {
        result.attack = static_cast<int>(result.attack * 1.55F);
        result.speed *= 1.28F;
        result.attackInterval /= 1.45F;
        result.criticalChance += 0.18F;
    }
    return result;
}

