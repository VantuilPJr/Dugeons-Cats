#pragma once

#include "items/Item.h"
#include "structures/LinkedList.h"

#include <cstddef>

class Inventory
{
public:
    void add(const ItemType type) { items_.add(Item{type}); }

    bool consume(const ItemType type)
    {
        return items_.removeFirstIf([type](const Item& item) { return item.type == type; });
    }

    [[nodiscard]] std::size_t count(const ItemType type) const
    {
        std::size_t result = 0;
        items_.forEach([&result, type](const Item& item) {
            if (item.type == type)
            {
                ++result;
            }
        });
        return result;
    }

    [[nodiscard]] std::size_t size() const { return items_.size(); }
    void clear() { items_.clear(); }

private:
    LinkedList<Item> items_;
};

