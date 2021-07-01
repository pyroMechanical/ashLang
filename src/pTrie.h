#pragma once

#include <memory>
#include <atomic>


namespace ash
{

#define BITS_PER_NODE 6
#define MAX_TRIE_DEPTH (sizeof(size_t) + BITS_PER_NODE - 1) / BITS_PER_NODE
#define HASH_MASK (1 << BITS_PER_NODE) - 1
#define ROOT_SIZE sizeof(size_t) % BITS_PER_NODE

	static size_t sparse_to_compact(const size_t bitmap, const size_t sparse)
	{
		auto bitpos = (1 << sparse) - 1;

		return __popcnt64(bitpos & bitmap);
	}

	enum class node_type {root_node, branch_node, leaf_node};

	class node
	{
	public:
		std::atomic<size_t> count = 1;
		node_type type;
		~node() = default;
	public:
		void add_ref()
		{
			std::atomic_fetch_add_explicit(&count, size_t{ 1 }, std::memory_order_relaxed);
		}

		void release()
		{
			if (std::atomic_fetch_sub_explicit(&count, size_t{ 1 }, std::memory_order_release) == 1)
			{
				this->~node();
			}
		}
	};

}