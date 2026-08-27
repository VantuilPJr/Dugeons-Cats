#include "dungeon/DungeonGraph.h"

#include "structures/Queue.h"
#include "structures/Stack.h"

#include <algorithm>
#include <stdexcept>

int DungeonGraph::addVertex()
{
    adjacency_.emplace_back();
    return static_cast<int>(adjacency_.size()) - 1;
}

void DungeonGraph::addConnection(const int first, const int second)
{
    if (!isValidVertex(first) || !isValidVertex(second) || first == second)
    {
        throw std::out_of_range("Invalid dungeon graph connection");
    }

    auto addUnique = [](std::vector<int>& list, const int value) {
        if (std::find(list.begin(), list.end(), value) == list.end())
        {
            list.push_back(value);
        }
    };
    addUnique(adjacency_[first], second);
    addUnique(adjacency_[second], first);
}

const std::vector<int>& DungeonGraph::neighbors(const int vertex) const
{
    if (!isValidVertex(vertex))
    {
        throw std::out_of_range("Invalid dungeon graph vertex");
    }
    return adjacency_[vertex];
}

std::vector<int> DungeonGraph::depthFirstTraversal(const int start) const
{
    if (!isValidVertex(start))
    {
        return {};
    }

    std::vector<int> order;
    std::vector<bool> visited(adjacency_.size(), false);
    Stack<int> pending; // DFS -> PILHA academica.
    pending.push(start);

    while (!pending.empty())
    {
        const int current = pending.pop();
        if (visited[current])
        {
            continue;
        }

        visited[current] = true;
        order.push_back(current);
        const auto& next = adjacency_[current];
        for (auto it = next.rbegin(); it != next.rend(); ++it)
        {
            if (!visited[*it])
            {
                pending.push(*it);
            }
        }
    }
    return order;
}

std::vector<int> DungeonGraph::breadthFirstShortestPath(const int start, const int goal) const
{
    if (!isValidVertex(start) || !isValidVertex(goal))
    {
        return {};
    }

    std::vector<int> parent(adjacency_.size(), -1);
    std::vector<bool> visited(adjacency_.size(), false);
    Queue<int> pending; // BFS -> FILA academica.
    pending.push(start);
    visited[start] = true;

    while (!pending.empty())
    {
        const int current = pending.pop();
        if (current == goal)
        {
            break;
        }

        for (const int next : adjacency_[current])
        {
            if (!visited[next])
            {
                visited[next] = true;
                parent[next] = current;
                pending.push(next);
            }
        }
    }

    if (!visited[goal])
    {
        return {};
    }

    std::vector<int> path;
    for (int current = goal; current != -1; current = parent[current])
    {
        path.push_back(current);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<int> DungeonGraph::breadthFirstDistances(const int start) const
{
    std::vector<int> distance(adjacency_.size(), -1);
    if (!isValidVertex(start))
    {
        return distance;
    }

    Queue<int> pending;
    pending.push(start);
    distance[start] = 0;
    while (!pending.empty())
    {
        const int current = pending.pop();
        for (const int next : adjacency_[current])
        {
            if (distance[next] < 0)
            {
                distance[next] = distance[current] + 1;
                pending.push(next);
            }
        }
    }
    return distance;
}

bool DungeonGraph::isConnected() const
{
    return adjacency_.empty() || depthFirstTraversal(0).size() == adjacency_.size();
}

int DungeonGraph::vertexCount() const
{
    return static_cast<int>(adjacency_.size());
}

bool DungeonGraph::isValidVertex(const int vertex) const
{
    return vertex >= 0 && vertex < static_cast<int>(adjacency_.size());
}

