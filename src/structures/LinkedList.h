#pragma once

#include <cstddef>
#include <utility>

template <typename T>
class LinkedList
{
public:
    LinkedList() = default;
    ~LinkedList() { clear(); }

    LinkedList(const LinkedList&) = delete;
    LinkedList& operator=(const LinkedList&) = delete;

    void add(const T& value)
    {
        Node* node = new Node{value, nullptr};
        append(node);
    }

    void add(T&& value)
    {
        Node* node = new Node{std::move(value), nullptr};
        append(node);
    }

    template <typename Predicate>
    bool removeFirstIf(Predicate predicate)
    {
        Node* previous = nullptr;
        Node* current = head_;
        while (current != nullptr)
        {
            if (predicate(current->value))
            {
                if (previous != nullptr)
                {
                    previous->next = current->next;
                }
                else
                {
                    head_ = current->next;
                }
                if (current == tail_)
                {
                    tail_ = previous;
                }
                delete current;
                --size_;
                return true;
            }
            previous = current;
            current = current->next;
        }
        return false;
    }

    template <typename Predicate>
    [[nodiscard]] const T* find(Predicate predicate) const
    {
        for (Node* current = head_; current != nullptr; current = current->next)
        {
            if (predicate(current->value))
            {
                return &current->value;
            }
        }
        return nullptr;
    }

    template <typename Function>
    void forEach(Function function) const
    {
        for (Node* current = head_; current != nullptr; current = current->next)
        {
            function(current->value);
        }
    }

    [[nodiscard]] std::size_t size() const { return size_; }
    [[nodiscard]] bool empty() const { return head_ == nullptr; }

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

