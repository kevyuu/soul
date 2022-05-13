#pragma once

#include <concepts>

namespace soul
{

	template <typename T>
	class IntrusiveList
	{
	public:

		void insert(T* val)
		{
			if (head_ != nullptr) head_->prev = val;
			val->next = head_;
			val->prev = nullptr;
			head_ = val;
		}

		void remove(T* val)
		{
			T* prev = val->prev;
			T* next = val->next;

			if (prev != nullptr) prev->next = next;
			else head_ = next;
			if (next != nullptr) next->prev = prev;
			val->prev = nullptr;
			val->next = nullptr;
		}

		void clear()
		{
			head_ = nullptr;
		}

		class Iterator
		{
		public:
			friend class IntrusiveList<T>;

			explicit Iterator(T* node)
				: node_(node)
			{}

			explicit operator bool() const
			{
				return node_ != nullptr;
			}

			bool operator==(const Iterator& other) const
			{
				return node_ = other.node_;
			}

			bool operator!=(const Iterator& other) const
			{
				return node_ != other.node_;
			}

			T& operator*()
			{
				return *node_;
			}

			const T& operator*() const
			{
				return *node_;
			}

			const T* operator->() const
			{
				return node_;
			}

			Iterator& operator++()
			{
				node_ = node_->next;
				return *this;
			}
		private:
			T* node_;
		};

		Iterator begin() const
		{
			return Iterator(head_);
		}

		Iterator end() const
		{
			return Iterator(nullptr);
		}

		T* get_head()
		{
			return head_;
		}

	private:
		T* head_ = nullptr;
	};
}
