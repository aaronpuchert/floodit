#include <gtest/gtest.h>
#include <vector>
#include "trie.hpp"

TEST(TrieTest, SimpleSequence)
{
	constexpr unsigned char size = 64;

	Trie<unsigned char> trie;
	auto element = trie.initial();
	for (unsigned char i = 0; i < size; ++i) {
		element = trie.append(element, i);
		EXPECT_EQ(i, element.back());
	}

	ASSERT_EQ(size, element.size());
	unsigned char result[size];
	element.materialize(result);
	for (unsigned char i = 0; i < size; ++i)
		EXPECT_EQ(i, result[i]);
}

TEST(TrieTest, SequenceWithBranches)
{
	constexpr unsigned char size = 64, offset = 100;

	Trie<unsigned char> trie;
	auto element = trie.initial();
	std::vector<decltype(element)> branches(size, trie.initial());
	for (unsigned char i = 0; i < size; ++i) {
		branches[i] = trie.append(element, i + offset);
		element = trie.append(element, i);
	}

	ASSERT_EQ(size, element.size());
	unsigned char result[size+1];
	element.materialize(result);
	for (unsigned char i = 0; i < size; ++i)
		EXPECT_EQ(i, result[i]);

	for (unsigned char branch = 0; branch < size; ++branch) {
		ASSERT_EQ(branch+1, branches[branch].size());
		branches[branch].materialize(result);
		for (unsigned char i = 0; i < branch; ++i)
			EXPECT_EQ(i, result[i]);
		EXPECT_EQ(branch + offset, result[branch]);
	}
}

TEST(TrieTest, BinaryTree)
{
	constexpr unsigned depth = 12;

	Trie<bool> trie;

	std::vector<Trie<bool>::Sequence> nodes{Trie<bool>::initial()};
	for (unsigned d = 0; d < depth; ++d) {
		std::vector<Trie<bool>::Sequence> next;
		next.reserve(2 * nodes.size());
		for (auto node : nodes) {
			next.push_back(trie.append(node, false));
			next.push_back(trie.append(node, true));
		}
		nodes.swap(next);
	}

	ASSERT_EQ(1 << depth, nodes.size());
	bool result[depth];
	for (unsigned i = 0; i < (1 << depth); ++i) {
		ASSERT_EQ(depth, nodes[i].size());
		nodes[i].materialize(result);
		for (unsigned bit = 0; bit < depth; ++bit)
			EXPECT_EQ((i >> bit) & 1, result[(depth-1) - bit]);
	}
}
