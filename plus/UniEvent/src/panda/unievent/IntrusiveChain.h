#pragma once
#include <cassert>
#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <vector>

#include "Debug.h"

namespace panda { namespace unievent {

template <typename T> struct Cloneable {
    virtual ~Cloneable() = default;

protected:
    virtual T clone() const = 0;
};

template <typename T> struct IntrusiveChainNode : Cloneable<T> {
    template <typename S> friend class IntrusiveChain;
    template <typename S> friend class IntrusiveChainIterator;

    T next() const { return next_; }
    T prev() const { return prev_; }

    T next_;
    T prev_;
};

/// Iterator is not cyclic, so to work with the standard bidirectional algorithms it uses head and tail pointers alongside with the current one.
/// Typical cyclic implementation uses an extra sentinel Node object, which we will definitely do not want to allocate.
template <typename T> struct IntrusiveChainIterator {
    template <typename S> friend struct IntrusiveChain;
    template <typename S> friend struct IntrusiveChainIterator;

    using difference_type   = ptrdiff_t;
    using value_type        = T;
    using pointer           = T*;
    using reference         = T&;
    using iterator_category = std::bidirectional_iterator_tag;

    IntrusiveChainIterator (const T& tail, const T& current = nullptr) : tail_(tail), current_(current) {}
    
    template <typename S>
    IntrusiveChainIterator (const IntrusiveChainIterator<S>& o) : tail_(o.tail_), current_(o.current_) {}

    T&   operator*  () { return current_; }
    T*   operator-> () { return &current_; }
    bool operator== (const IntrusiveChainIterator& other) const { return current_ == other.current_; }
    bool operator!= (const IntrusiveChainIterator& other) const { return !(*this == other); }

    IntrusiveChainIterator& operator++ () {
        current_ = current_->next_;
        return *this;
    }

    IntrusiveChainIterator operator++ (int) {
        IntrusiveChainIterator pos(*this);
        current_ = current_->next_;
        return pos;
    }

    IntrusiveChainIterator& operator-- () {
        if (current_) current_ = current_->prev_;
        else          current_ = tail_;
        return *this;
    }

    IntrusiveChainIterator operator-- (int) {
        IntrusiveChainIterator pos(*this);
        return --pos;
    }

    IntrusiveChainIterator& operator= (const IntrusiveChainIterator& oth) {
        this->tail_ = oth.tail_;
        this->current_ = oth.current_;
        return *this;
    }

private:
    T tail_;
    T current_; // current=nullptr indicates end()
};

template <typename T> struct IntrusiveChain : Cloneable<IntrusiveChain<T>> {
    using value_type             = T;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using reference              = T&;
    using const_reference        = const T&;
    using pointer                = T*;
    using const_pointer          = const T*;
    using iterator               = IntrusiveChainIterator<T>;
    using const_iterator         = IntrusiveChainIterator<T>;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    ~IntrusiveChain() {
        clear();
    }

    IntrusiveChain() : head_(), tail_() {}

    IntrusiveChain(std::initializer_list<T> il) : IntrusiveChain() {
        for (const T& node : il) {
            push_back(node);
        }
    }

    IntrusiveChain(const IntrusiveChain&) = delete;
    IntrusiveChain& operator=(const IntrusiveChain&) = delete;

    IntrusiveChain(IntrusiveChain&& other) noexcept {
        std::swap(head_, other.head_);
        std::swap(tail_, other.tail_);
    }

    IntrusiveChain& operator=(IntrusiveChain&& other) noexcept {
        if (this != &other)  
        {  
            std::swap(head_, other.head_);
            std::swap(tail_, other.tail_);
        } 
        return *this;
    }

    IntrusiveChain clone() const {
        IntrusiveChain result;
        std::for_each(begin(), end(), [&](T& node){ result.push_back(node->clone()); });
        return result;
    }

    void push_back(const T& node) {
        if (empty()) {
            head_ = tail_ = node;
        } else {
            tail_->next_ = node;
            node->prev_  = tail_;
            node->next_  = nullptr;
            tail_        = node;
        }
    }

    void push_front(const T& node) {
        if (empty()) {
            head_ = tail_ = node;
        } else {
            head_->prev_ = node;
            node->prev_  = nullptr;
            node->next_  = head_;
            head_        = node;
        }
    }

    /// @return false if empty or the last one
    bool pop_back() {
        if (head_ == tail_) {
            head_ = tail_ = nullptr;
            return false;
        } else {
            tail_        = tail_->prev_;
            tail_->next_ = tail_->next_->prev_ = nullptr;
            return true;
        }
    }

    /// @return false if empty or the last one
    bool pop_front() {
        if (head_ == tail_) {
            head_ = tail_ = nullptr;
            return false;
        } else {
            head_        = head_->next_;
            head_->prev_ = head_->prev_->next_ = nullptr;
            return true;
        }
    }

    const_iterator insert(const_iterator pos, const T& node) {
        if (pos.current_) {
            if (pos.current_->prev_) {
                node->prev_                = pos.current_->prev_;
                node->next_                = pos.current_;
                pos.current_->prev_->next_ = node;
                pos.current_->prev_        = node;
            } else {
                if (empty()) {
                    tail_       = node;
                    node->next_ = nullptr;
                } else {
                    node->next_         = pos.current_;
                    pos.current_->prev_ = node;
                }

                head_       = node;
                node->prev_ = nullptr;
            }
        } else {
            // it means that current points to end()
            push_back(node);
        }
        
        return const_iterator(tail_, node);
    }

    const_iterator erase(const_iterator pos) {
        if (pos.current_) {
            if (pos.current_->prev_) {
                pos.current_->prev_->next_ = pos.current_->next_;
            } else {
                head_ = pos.current_->next_;
            }
            
            T current = pos.current_->next_;
            if (pos.current_->next_) {
                pos.current_->next_->prev_ = pos.current_->prev_;
            } else {
                tail_ = pos.current_->prev_;
            }

            pos.current_->prev_ = pos.current_->next_ = nullptr;

            return const_iterator(tail_, current);
        } else {
            // it means that current points to end()
            return end();
        }
    }

    void clear() {
        while (pop_back());
    }

    reference front() { return head_; }
    reference back() { return tail_; }

    const_reference front() const { return head_; }
    const_reference back() const { return tail_; }

    iterator begin() { return iterator(tail_, head_); }
    iterator end() { return iterator(tail_); }

    const_iterator begin() const { return const_iterator(tail_, head_); }
    const_iterator end() const { return const_iterator(tail_); }
    
    const_iterator cbegin() const { return const_iterator(tail_, head_); }
    const_iterator cend() const { return const_iterator(tail_); }

    bool empty() const { return !head_; }

    size_type size() const { return std::distance(begin(), end()); }

    void dump_refs(std::ostream& out) const {
        for (const T& node : *this) {
            out << "node:";
            if (node) {
                out << "\"" << *node << "\""
                    << " prev: " << (node->prev_ ? node->prev_->refcnt() : 0) 
                    << " next: " << (node->next_ ? node->next_->refcnt() : 0);
            } else {
                out << "null";
            }
            out << std::endl;
        }
    }

private:
    T head_;
    T tail_;
};

template <typename T> std::ostream& operator<<(std::ostream& out, const IntrusiveChain<T>& chain) {
    for (auto node : chain) {
        out << "node: ";
        if (node) {
            out << *node;
        } else {
            out << "null";
        }
        out << std::endl;
    }
    return out;
}

}} // namespace panda::event
