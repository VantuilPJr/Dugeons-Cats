#include "dungeon/DungeonGenerator.h"
#include "dungeon/DungeonGraph.h"
#include "items/Inventory.h"
#include "structures/LinkedList.h"
#include "structures/Queue.h"
#include "structures/Stack.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
int failures = 0;

void expect(const bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}
} // namespace

int main()
{
    Stack<int> stack;
    stack.push(10);
    stack.push(20);
    expect(stack.size() == 2, "stack size");
    expect(stack.pop() == 20 && stack.pop() == 10, "stack is LIFO");

    Queue<int> queue;
    queue.push(10);
    queue.push(20);
    expect(queue.size() == 2, "queue size");
    expect(queue.pop() == 10 && queue.pop() == 20, "queue is FIFO");

    LinkedList<int> list;
    list.add(4);
    list.add(7);
    list.add(9);
    expect(list.find([](const int value) { return value == 7; }) != nullptr, "linked list find");
    expect(list.removeFirstIf([](const int value) { return value == 7; }), "linked list remove");
    expect(list.size() == 2, "linked list size after remove");

    Inventory inventory;
    inventory.add(ItemType::Sachet);
    inventory.add(ItemType::Catnip);
    inventory.add(ItemType::Sachet);
    expect(inventory.count(ItemType::Sachet) == 2, "inventory count");
    expect(inventory.consume(ItemType::Sachet), "inventory consume");
    expect(inventory.count(ItemType::Sachet) == 1, "inventory count after consume");

    DungeonGraph graph;
    for (int index = 0; index < 5; ++index)
    {
        graph.addVertex();
    }
    graph.addConnection(0, 1);
    graph.addConnection(1, 2);
    graph.addConnection(0, 3);
    graph.addConnection(3, 4);
    graph.addConnection(4, 2);
    expect(graph.isConnected(), "graph connectivity through DFS stack");
    const auto path = graph.breadthFirstShortestPath(0, 2);
    expect(path.size() == 3 && path.front() == 0 && path.back() == 2, "BFS shortest room path");

    DungeonGenerator generator;
    for (const unsigned int seed : {1U, 7U, 42U, 2026U})
    {
        Dungeon dungeon = generator.generate(seed);
        expect(dungeon.rooms().size() >= 6, "generated room count");
        expect(dungeon.graph().isConnected(), "generated graph connected");
        const auto bossPath = dungeon.graph().breadthFirstShortestPath(dungeon.startRoomId(), dungeon.bossRoomId());
        expect(bossPath.size() >= 2, "entrance reaches boss through BFS");
        const GridPoint start = dungeon.roomCenter(dungeon.startRoomId());
        const GridPoint boss = dungeon.roomCenter(dungeon.bossRoomId());
        expect(dungeon.isWalkableTile(start.x, start.y), "start room center is floor");
        expect(dungeon.isWalkableTile(boss.x, boss.y), "boss room center is floor");
    }

    if (failures == 0)
    {
        std::cout << "All core tests passed.\n";
        return EXIT_SUCCESS;
    }

    std::cerr << failures << " test(s) failed.\n";
    return EXIT_FAILURE;
}

