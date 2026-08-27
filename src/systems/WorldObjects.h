#pragma once

#include "items/Item.h"

#include <raylib.h>

struct WorldItem
{
    ItemType type = ItemType::Sachet;
    Vector2 position{};
    bool active = true;
};

struct TrashCan
{
    Vector2 position{};
    bool opened = false;
};

struct Scratcher
{
    Vector2 position{};
    bool used = false;
};

struct Particle
{
    Vector2 position{};
    Vector2 velocity{};
    Color color = WHITE;
    float life = 0.0F;
    float maxLife = 0.0F;
    float size = 1.0F;
};

