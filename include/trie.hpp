#ifndef TRIE_HPP
#define TRIE_HPP

#include <cassert>
#include <deque>

/**
 * Data structure for efficient storage of move histories.
 */
template<typename T>
class Trie
{
private:
	/**
	 * Block containing elements of the trie.
	 *
	 * Sequences can only be appended, meaning that full blocks can be
	 * considered immutable.
	 */
	class Block
	{
	public:
		Block() : pred(nullptr), length(0) {}
		explicit Block(const Block *predecessor)
			: pred(predecessor), length(predecessor->length) {}

		unsigned size() const { return length; }

		/// @return True, if data block is full.
		bool add(const T &t) {
			unsigned index = length % ELEMENTS_PER_BLOCK;
			data[index] = t;
			++length;
			return index == ELEMENTS_PER_BLOCK-1;
		}

		const T& back() const
		{
			assert(length != 0);
			unsigned last = (length-1) % ELEMENTS_PER_BLOCK;
			if (last != ELEMENTS_PER_BLOCK-1)
				return data[last];
			else
				return pred->data[ELEMENTS_PER_BLOCK-1];
		}

		void materialize(T *buffer) const
		{
			unsigned offset = length - ((length-1) % ELEMENTS_PER_BLOCK + 1);
			for (unsigned index = 0; offset + index < length; ++index)
				buffer[offset + index] = data[index];
			if (pred)
				pred->materialize(buffer);
		}

	private:
		const Block *pred;      // Predecessor block; nullptr if none.
		unsigned short length;  // Length of sequence including predecessors.

		// We want to use 2*sizeof(void*) per block.
		static constexpr unsigned ELEMENTS_PER_BLOCK =
			(sizeof(void*) - sizeof length) / sizeof(T);
		T data[ELEMENTS_PER_BLOCK];
	};

public:
	// Wrap blocks so that only we can append elements.
	class Sequence
	{
		friend Trie<T>;
		Sequence(Block block) : block(block) {}

	public:
		const T& back() const { return block.back(); }
		unsigned size() const { return block.size(); }
		void materialize(T *buffer) const { return block.materialize(buffer); }

	private:
		Block block;
	};

	static Sequence initial() { return Block{}; }

	Sequence append(Sequence sequence, const T &element)
	{
		if (sequence.block.add(element)) {
			blocks.push_back(sequence.block);
			return Block{&blocks.back()};
		}
		else
			return sequence;
	}

private:
	// Append-only queue of data blocks.
	static_assert(sizeof(Block) == 2*sizeof(void*), "Elements are too big");
	std::deque<Block> blocks;
};

#endif
