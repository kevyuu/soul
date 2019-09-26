#pragma once

#include <cstdlib>

template <typename T>
class Array
{
public:


	Array() {
		
	}

	void init(int capacity) {
		this->capacity = capacity;
		buffer = (T*) malloc(sizeof(T) * capacity);
		count = 0;
	}

	void shutdown() {
	    free(buffer);
	}

	void push_back(const T& item) {
		buffer[count] = item;
		count++;
	}

	T get(int idx) const {
		return buffer[idx];
	}

	int getSize() const {
		return count;
	}

	~Array() {

	}

	

private:
	T * buffer;
	int count;
	int capacity;
};

