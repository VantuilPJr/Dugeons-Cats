#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>

template <typename T>
class Stack
{
public:
    Stack() = default;
    ~Stack() { clear(); }

    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;

    void push(const T& value) { head_ = new Node{value, head_}; ++size_; }
    void push(T&& value) { head_ = new Node{std::move(value), head_}; ++size_; }

    T pop()
    {
        if (empty())
        {
            throw std::out_of_range("Cannot pop an empty stack");
        }

        Node* removed = head_;
        T value = std::move(removed->value);
        head_ = removed->next;
        delete removed;
        --size_;
        return value;
    }

    [[nodiscard]] const T& top() const
    {
        if (empty())
        {
            throw std::out_of_range("Cannot read an empty stack");
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
        size_ = 0;
    }

private:
    struct Node
    {
        T value;
        Node* next;
    };

    Node* head_ = nullptr;
    std::size_t size_ = 0;
};

