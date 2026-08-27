#pragma once

#include <vector>

class DungeonGraph
{
public:
    int addVertex();
    void addConnection(int first, int second);

    [[nodiscard]] const std::vector<int>& neighbors(int vertex) const;
    [[nodiscard]] std::vector<int> depthFirstTraversal(int start) const;
    [[nodiscard]] std::vector<int> breadthFirstShortestPath(int start, int goal) const;
    [[nodiscard]] std::vector<int> breadthFirstDistances(int start) const;
    [[nodiscard]] bool isConnected() const;
    [[nodiscard]] int vertexCount() const;

private:
    [[nodiscard]] bool isValidVertex(int vertex) const;

    std::vector<std::vector<int>> adjacency_;
};

