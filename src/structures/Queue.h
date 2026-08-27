#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>

template <typename T>
class Queue
{
public:
    Queue() = default;
    ~Queue() { clear(); }

    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    void push(const T& value)
    {
        Node* node = new Node{value, nullptr};
        append(node);
    }

    void push(T&& value)
    {
        Node* node = new Node{std::move(value), nullptr};
        append(node);
    }

    T pop()
    {
        if (empty())
        {
            throw std::out_of_range("Cannot pop an empty queue");
        }

        Node* removed = head_;
        T value = std::move(removed->value);
        head_ = removed->next;
        delete removed;
        --size_;
        if (head_ == nullptr)
        {
            tail_ = nullptr;
        }
        return value;
    }

    [[nodiscard]] const T& front() const
    {
        if (empty())
        {
            throw std::out_of_range("Cannot read an empty queue");
        }
        return head_->value;
    }

    [[nodiscard]] bool empty() const { return head_ == nullptr; }
    [[nodiscard]] std::size_t size() const { return size_; }

    void clear()
    {
        while (head_ != nullptr)
        {
            Node* removed = head_;
            head_ = head_->next;
            delete removed;
        }
        tail_ = nullptr;
        size_ = 0;
    }

private:
    struct Node
    {
        T value;
        Node* next;
    };

    void append(Node* node)
    {
        if (tail_ != nullptr)
        {
            tail_->next = node;
        }
        else
        {
            head_ = node;
        }
        tail_ = node;
        ++size_;
    }

    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    std::size_t size_ = 0;
};

