#pragma once

#include <memory>
#include <atomic>


namespace ash
{

#define BITS_PER_NODE 6
#define MAX_TRIE_DEPTH (sizeof(uint64_t) + BITS_PER_ARRAY - 1) / BITS_PER_ARRAY
#define HASH_MASK (1 << BITS_PER_NODE) - 1

	namespace util
	{
		static size_t sparse_to_compact(const size_t bitmap, const size_t sparse)
		{
			auto bitpos = (1 << sparse) - 1;

			return __popcnt64(bitpos & bitmap);
		}

		class RefCounted
		{
		public:
			std::atomic<size_t> count = 1;
		};

	}

	using namespace util;

}