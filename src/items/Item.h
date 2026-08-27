#pragma once

enum class ItemType
{
    Sachet,
    Catnip,
    Coin,
};

struct Item
{
    ItemType type = ItemType::Sachet;
};

inline const char* itemName(const ItemType type)
{
    switch (type)
    {
    case ItemType::Sachet: return "Sache";
    case ItemType::Catnip: return "Catnip";
    case ItemType::Coin: return "Tampinha";
    }
    return "Item";
}

