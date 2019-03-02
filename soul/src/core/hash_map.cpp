#include "hash_map.h"
#include "debug.h"

#include <cstdlib>

namespace Soul {

	void HashMap::init(uint32 capacity) {
		this->capacity = capacity;
		keys = (uint32*) malloc(sizeof(capacity * sizeof(uint32)));
		values = (uint32*)malloc(sizeof(capacity * sizeof(uint32)));
		memset(keys, 0, capacity * sizeof(uint32));
	}

	uint32 HashMap::get(uint32 key) {
		uint32 start_idx = key % capacity;
		uint32 end_idx = start_idx == 0 ? capacity - 1 : start_idx - 1;
		for (int i = start_idx; i < capacity; i++, i %= capacity) {
			if (keys[i] == key) return values[i];
			if (values[i] == HashMap::VALUE_EMPTY) return values[i];
		}
		return 0;
	}

	void HashMap::add(uint32 key, uint32 value) {
		SOUL_ASSERT(0, value != HashMap::VALUE_EMPTY, "Value for hash map must not be %s. It is allocated for empty value", HashMap::VALUE_EMPTY);
		uint32 start_idx = key % capacity;
		uint32 end_idx = start_idx == 0 ? capacity - 1 : start_idx - 1;
		for (int i = start_idx; i != end_idx; i++, i %= capacity) {
			SOUL_ASSERT(0, keys[i] != key, "Key = %s is already added to the hash map.", key);
			if (values[i] == HashMap::VALUE_EMPTY) {
				keys[i] = key;
				values[i] = value;
				return;
			}
		}
		SOUL_ASSERT(0, false, "Hash map capacity is not enough");
	}

	void HashMap::cleanup() {
		capacity = 0;
		free(keys);
		free(values);
	}

}