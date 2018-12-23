#pragma once

#include <cstdlib>

namespace Soul {
	template <typename T>
	class Array
	{
	public:

		Array() {
			capacity = 0;
			count = 0;
		}

		void init(int capacity) {
			this->capacity = capacity;
			buffer = (T*)malloc(sizeof(T) * capacity);
			count = 0;
		}

		void cleanup() {
			free(buffer);
		}

		void pushBack(const T& item) {

			if (count == capacity) {
				T* oldBuffer = buffer;
				buffer = (T*)malloc(sizeof(T) * 2 * capacity);
				memcpy(buffer, oldBuffer, sizeof(T) * capacity);
				free(oldBuffer);
				capacity *= 2;
			}

			buffer[count] = item;
			count++;
		}

		T& operator[](int idx) {
			return buffer[idx];
		}

		T& get(int idx) const {
			return buffer[idx];
		}

		int getSize() const {
			return count;
		}

		T* buffer;

	private:
		int count;
		int capacity;
	};

}

