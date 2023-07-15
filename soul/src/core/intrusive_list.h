#pragma once

#include "core/type.h"

namespace soul
{

  struct IntrusiveListNode {
    IntrusiveListNode* prev = nullptr;
    IntrusiveListNode* next = nullptr;
  };

  template <typename T>
  concept intrusive_node_type = std::derived_from<T, IntrusiveListNode>;

  template <intrusive_node_type T, b8 IsConst>
  class IntrusiveListIterator
  {
  public:
    using this_type = IntrusiveListIterator<T, IsConst>;
    using iterator = IntrusiveListIterator<T, false>;
    using value_type = T;
    using node_type = T;
    using pointer = std::conditional_t<IsConst, const T*, T*>;
    using reference = std::conditional_t<IsConst, const T&, T&>;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = ptrdiff_t;

    pointer node;

    IntrusiveListIterator();
    explicit IntrusiveListIterator(pointer node);
    IntrusiveListIterator(const iterator& x); // NOLINT(hicpp-explicit-conversions)
    auto operator=(const iterator& x) -> IntrusiveListIterator&;

    auto operator*() const -> reference;
    auto operator->() const -> pointer;

    auto operator++() -> IntrusiveListIterator&;
    auto operator--() -> IntrusiveListIterator&;

    auto operator++(int) -> IntrusiveListIterator;
    auto operator--(int) -> IntrusiveListIterator;
  };

  template <intrusive_node_type T, b8 IsConst>
  IntrusiveListIterator<T, IsConst>::IntrusiveListIterator() : node(nullptr)
  {
  }

  template <intrusive_node_type T, b8 IsConst>
  IntrusiveListIterator<T, IsConst>::IntrusiveListIterator(pointer node) : node(node)
  {
  }

  template <intrusive_node_type T, b8 IsConst>
  IntrusiveListIterator<T, IsConst>::IntrusiveListIterator(const iterator& x) : node(x.node)
  {
  }

  template <intrusive_node_type T, b8 IsConst>
  auto IntrusiveListIterator<T, IsConst>::operator=(const iterator& x)
    -> IntrusiveListIterator<T, IsConst>&
  {
    node = x.node;
    return *this;
  }

  template <intrusive_node_type T, b8 IsConst>
  auto IntrusiveListIterator<T, IsConst>::operator*() const -> reference
  {
    return *node;
  }

  template <intrusive_node_type T, b8 IsConst>
  auto IntrusiveListIterator<T, IsConst>::operator->() const -> pointer
  {
    return node;
  }

  template <intrusive_node_type T, b8 IsConst>
  auto IntrusiveListIterator<T, IsConst>::operator++() -> IntrusiveListIterator<T, IsConst>&
  {
    node = static_cast<pointer>(node->next);
    return *this;
  }

  template <intrusive_node_type T, b8 IsConst>
  auto IntrusiveListIterator<T, IsConst>::operator--() -> IntrusiveListIterator<T, IsConst>&
  {
    node = static_cast<pointer>(node->prev);
    return *this;
  }

  template <intrusive_node_type T, b8 IsConst>
  auto IntrusiveListIterator<T, IsConst>::operator++(int) -> IntrusiveListIterator<T, IsConst>
  {
    IntrusiveListIterator it(*this);
    node = static_cast<pointer>(node->next);
    return it;
  }

  template <intrusive_node_type T, b8 IsConst>
  auto IntrusiveListIterator<T, IsConst>::operator--(int) -> IntrusiveListIterator<T, IsConst>
  {
    IntrusiveListIterator it(*this);
    node = static_cast<pointer>(node->prev);
    return it;
  }

  template <intrusive_node_type T, b8 IsConst>
  auto operator==(
    const IntrusiveListIterator<T, IsConst>& a, const IntrusiveListIterator<T, IsConst>& b) -> b8
  {
    return a.node == b.node;
  }

  template <intrusive_node_type T, b8 IsConst, b8 IsConst2>
  auto operator==(
    const IntrusiveListIterator<T, IsConst>& a, const IntrusiveListIterator<T, IsConst2>& b) -> b8
  {
    return a.node == b.node;
  }

  template <intrusive_node_type T>
  class IntrusiveList
  {
  public:
    using this_type = IntrusiveList<T>;
    using node_type = T;
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = IntrusiveListIterator<T, false>;
    using const_iterator = IntrusiveListIterator<T, true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    IntrusiveList();
    IntrusiveList(const this_type& x) = delete;
    IntrusiveList(this_type&& x) = delete;

    auto operator=(const this_type& x) -> this_type& = delete;
    auto operator=(this_type&& x) -> IntrusiveList& = delete;

    ~IntrusiveList() = default;

    auto swap(this_type& rhs) noexcept -> void;
    friend auto swap(this_type& a, this_type& b) noexcept -> void { a.swap(b); }

    [[nodiscard]] auto begin() noexcept -> iterator;
    [[nodiscard]] auto begin() const noexcept -> const_iterator;
    [[nodiscard]] auto cbegin() const noexcept -> const_iterator;

    [[nodiscard]] auto end() noexcept -> iterator;
    [[nodiscard]] auto end() const noexcept -> const_iterator;
    [[nodiscard]] auto cend() const noexcept -> const_iterator;

    auto rbegin() noexcept -> reverse_iterator;
    [[nodiscard]] auto rbegin() const noexcept -> const_reverse_iterator;
    [[nodiscard]] auto crbegin() const noexcept -> const_reverse_iterator;

    auto rend() noexcept -> reverse_iterator;
    [[nodiscard]] auto rend() const noexcept -> const_reverse_iterator;
    [[nodiscard]] auto crend() const noexcept -> const_reverse_iterator;

    [[nodiscard]] auto size() const noexcept -> usize;
    [[nodiscard]] auto empty() const noexcept -> b8;

    [[nodiscard]] auto front() -> reference;
    [[nodiscard]] auto front() const -> const_reference;
    [[nodiscard]] auto back() -> reference;
    [[nodiscard]] auto back() const -> const_reference;

    auto push_front(value_type& x) -> void;
    auto push_back(value_type& x) -> void;
    auto pop_front() -> void;
    auto pop_back() -> void;

    auto contains(const value_type& x) const -> b8;

    [[nodiscard]] auto locate(value_type& x) -> iterator;
    [[nodiscard]] auto locate(const value_type& x) const -> const_iterator;

    auto insert(const_iterator pos, value_type& x) -> iterator;
    auto erase(const_iterator pos) -> iterator;
    auto erase(const_iterator pos, const_iterator last) -> iterator;

    static auto remove(value_type& value) -> void;

    auto clear() noexcept -> void;

    auto splice(const_iterator pos, value_type& value) -> void;
    auto splice(const_iterator pos, this_type& list) -> void;
    auto splice(const_iterator pos, this_type& list, const_iterator iterator) -> void;
    auto splice(const_iterator pos, this_type& list, const_iterator first, const_iterator last)
      -> void;

  private:
    IntrusiveListNode anchor_;
  };

  template <intrusive_node_type T>
  IntrusiveList<T>::IntrusiveList() : anchor_({&anchor_, &anchor_})
  {
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::swap(this_type& rhs) noexcept -> void
  {
    IntrusiveListNode tmp(anchor_);
    anchor_ = rhs.anchor_;
    rhs.anchor_ = tmp;

    if (anchor_.next == &rhs.anchor_) {
      anchor_.next = &anchor_;
      anchor_.prev = &anchor_;
    } else {
      anchor_.next->prev = &anchor_;
      anchor_.prev->next = &anchor_;
    }

    if (rhs.anchor_.next == &anchor_) {
      rhs.anchor_.next = &rhs.anchor_;
      rhs.anchor_.prev = &rhs.anchor_;
    } else {
      rhs.anchor_.next->prev = &rhs.anchor_;
      rhs.anchor_.prev->next = &rhs.anchor_;
    }
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::begin() noexcept -> iterator
  {
    return iterator(static_cast<T*>(anchor_.next));
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::begin() const noexcept -> const_iterator
  {
    return const_iterator(static_cast<T*>(anchor_.next));
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::cbegin() const noexcept -> const_iterator
  {
    return const_iterator(static_cast<T*>(anchor_.next));
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::end() noexcept -> iterator
  {
    return iterator(static_cast<T*>(&anchor_));
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::end() const noexcept -> const_iterator
  {
    return const_iterator(static_cast<const T*>(&anchor_));
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::cend() const noexcept -> const_iterator
  {
    return const_iterator(static_cast<const T*>(&anchor_));
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::rbegin() noexcept -> reverse_iterator
  {
    return reverse_iterator(iterator(static_cast<T*>(&anchor_)));
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::rbegin() const noexcept -> const_reverse_iterator
  {
    return const_reverse_iterator(const_iterator(static_cast<const T*>(&anchor_)));
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::crbegin() const noexcept -> const_reverse_iterator
  {
    return const_reverse_iterator(const_iterator(static_cast<const T*>(&anchor_)));
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::rend() noexcept -> reverse_iterator
  {
    return reverse_iterator(iterator(static_cast<T*>(anchor_.next)));
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::rend() const noexcept -> const_reverse_iterator
  {
    return const_reverse_iterator(const_iterator(static_cast<T*>(anchor_.next)));
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::crend() const noexcept -> const_reverse_iterator
  {
    return const_reverse_iterator(const_iterator(static_cast<T*>(anchor_.next)));
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::size() const noexcept -> usize
  {
    usize size = 0;
    for (const IntrusiveListNode* node = anchor_.next; node != &anchor_; node = node->next) {
      size++;
    }
    return size;
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::empty() const noexcept -> b8
  {
    return anchor_.next == &anchor_;
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::front() -> reference
  {
    return *static_cast<T*>(anchor_.next);
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::front() const -> const_reference
  {
    return *static_cast<T*>(anchor_.next);
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::back() -> reference
  {
    return *static_cast<T*>(anchor_.prev);
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::back() const -> const_reference
  {
    return *static_cast<T*>(anchor_.prev);
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::push_front(value_type& x) -> void
  {
    x.next = anchor_.next;
    x.prev = &anchor_;

    anchor_.next->prev = &x;
    anchor_.next = &x;
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::push_back(value_type& x) -> void
  {
    x.prev = anchor_.prev;
    x.next = &anchor_;

    anchor_.prev->next = &x;
    anchor_.prev = &x;
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::pop_front() -> void
  {
    SOUL_ASSERT(0, !empty(), "List cannot be empty for pop front");
    erase(const_iterator(static_cast<const T*>(anchor_.next)));
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::pop_back() -> void
  {
    SOUL_ASSERT(0, !empty(), "List cannot be empty for pop back");
    erase(const_iterator(static_cast<const T*>(anchor_.prev)));
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::contains(const value_type& x) const -> b8
  {
    for (const IntrusiveListNode* node = anchor_.next; node != &anchor_; node = node->next) {
      if (node == &x) {
        return true;
      }
    }
    return false;
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::locate(value_type& x) -> iterator
  {
    for (IntrusiveListNode* node = anchor_.next; node != &anchor_; node = node->next) {
      if (node == &x) {
        return iterator(static_cast<T*>(node));
      }
    }
    return iterator(static_cast<T*>(&anchor_));
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::locate(const value_type& x) const -> const_iterator
  {
    for (const IntrusiveListNode* node = anchor_.next; node != &anchor_; node = node->next) {
      if (node == &x) {
        return const_iterator(static_cast<const T*>(node));
      }
    }
    return const_iterator(static_cast<const T*>(&anchor_));
  }

  // NOLINTBEGIN(cppcoreguidelines-pro-type-const-cast)
  template <intrusive_node_type T>
  auto IntrusiveList<T>::insert(const_iterator pos, value_type& x) -> iterator
  {
    IntrusiveListNode& next = *const_cast<node_type*>(pos.node);
    IntrusiveListNode& prev = *static_cast<node_type*>(next.prev);
    prev.next = &x;
    next.prev = &x;
    x.next = &next;
    x.prev = &prev;

    return iterator(&x);
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::erase(const_iterator pos) -> iterator
  {
    IntrusiveListNode& next = *static_cast<node_type*>(pos.node->next);
    IntrusiveListNode& prev = *static_cast<node_type*>(pos.node->prev);
    prev.next = &next;
    next.prev = &prev;

    return iterator(static_cast<node_type*>(&next));
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::erase(const_iterator pos, const_iterator last) -> iterator
  {
    IntrusiveListNode& prev = *static_cast<node_type*>(pos.node->prev);
    IntrusiveListNode& next = *const_cast<node_type*>(last.node);

    prev.next = &next;
    next.prev = &prev;

    return iterator(const_cast<node_type*>(last.node));
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::remove(value_type& value) -> void
  {
    IntrusiveListNode& prev = *value.prev;
    IntrusiveListNode& next = *value.next;

    prev.next = &next;
    next.prev = &prev;
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::clear() noexcept -> void
  {
    anchor_.next = &anchor_;
    anchor_.prev = &anchor_;
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::splice(const_iterator pos, value_type& value) -> void
  {
    if (pos.node != &value) {
      remove(value);
      insert(pos, value);
    }
  }

  template <intrusive_node_type T>
  // ReSharper disable once CppMemberFunctionMayBeStatic
  auto IntrusiveList<T>::splice(const_iterator pos, this_type& list) -> void
  {
    if (!list.empty()) {
      IntrusiveListNode& new_next = *const_cast<node_type*>(pos.node);
      IntrusiveListNode& new_prev = *new_next.prev;
      IntrusiveListNode& list_head = *list.anchor_.next;
      IntrusiveListNode& list_tail = *list.anchor_.prev;

      new_prev.next = &list_head;
      new_next.prev = &list_tail;

      list_head.prev = &new_prev;
      list_tail.next = &new_next;

      list.anchor_.next = &list.anchor_;
      list.anchor_.prev = &list.anchor_;
    }
  }

  template <intrusive_node_type T>
  auto IntrusiveList<T>::splice(const_iterator pos, this_type& list, const_iterator iterator)
    -> void
  {
    if (pos != iterator) {
      list.erase(iterator);
      insert(pos, *(const_cast<T*>(iterator.node)));
    }
  }

  template <intrusive_node_type T>
  // ReSharper disable once CppMemberFunctionMayBeStatic
  auto IntrusiveList<T>::splice(
    const_iterator pos, this_type& /*list*/, const_iterator first, const_iterator last) -> void
  {
    if (first != last) {
      IntrusiveListNode& first_node = *const_cast<node_type*>(first.node);
      IntrusiveListNode& last_node = *(last.node->prev);

      first_node.prev->next = last_node.next;
      last_node.next->prev = first_node.prev;

      IntrusiveListNode& new_next = *const_cast<node_type*>(pos.node);
      IntrusiveListNode& new_prev = *new_next.prev;

      first_node.prev = &new_prev;
      last_node.next = &new_next;
      new_next.prev = &last_node;
      new_prev.next = &first_node;
    }
  }
  // NOLINTEND(cppcoreguidelines-pro-type-const-cast)

} // namespace soul
