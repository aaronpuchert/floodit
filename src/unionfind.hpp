#ifndef UNIONFIND_HPP
#define UNIONFIND_HPP

#include <numeric>
#include <vector>

/**
 * Very simple Union-find data structure.
 */
class UnionFind
{
public:
	UnionFind(unsigned numElements) : parent(numElements, 0)
	{
		std::iota(parent.begin(), parent.end(), 0);
	}

	unsigned find(unsigned element) const
	{
		while (parent[element] != element)
			element = parent[element];
		return element;
	}

	void merge(unsigned a, unsigned b)
	{
		a = find(a);
		b = find(b);
		if (a < b)
			parent[b] = a;
		else if (b < a)
			parent[a] = b;
	}

private:
	std::vector<unsigned> parent;
};

#endif
