/**
 * The input matrix is first translated into a colored graph with root. The
 * graph is then reduced: adjacent nodes of the same color can be merged. Then
 * we run some kind of A^* on the graph: we have an easy lower bound on the
 * number of floodings still needed.
 *
 * For A^* we have to maintain a priority queue of states. It is therefore key
 * to compactly store the state while allowing quick computation of the
 * valuation.
 */
#include "floodit.hpp"

#include <algorithm>
#include <cassert>
#include "pq.hpp"
#include "unionfind.hpp"

Graph::Graph(std::vector<Node> &&nodelist) : nodes(std::move(nodelist))
{
	// Sort the adjacency lists.
	for (Node &node : nodes)
		std::sort(node.neighbors.begin(), node.neighbors.end());
}

void Graph::reduce()
{
	// If a node has the same color as a neighbor, merge them.
	UnionFind partitions(nodes.size());
	for (unsigned i = 0; i < nodes.size(); ++i)
		for (unsigned neighbor : nodes[i].neighbors)
			if (nodes[i].color == nodes[neighbor].color)
				partitions.merge(i, neighbor);

	// Create a map for renumbering the nodes.
	std::vector<unsigned> reduced(nodes.size(), 0);
	for (unsigned i = 1; i < nodes.size(); ++i) {
		assert(partitions.find(i) <= i);
		reduced[i] = reduced[i-1] + (partitions.find(i) == i);
	}

	// Now we merge the neighbor lists.
	for (unsigned i = 0; i < nodes.size(); ++i) {
		unsigned parentId = partitions.find(i);
		if (parentId != i) {
			Node &parent = nodes[parentId], &node = nodes[i];
			parent.neighbors.insert(parent.neighbors.end(),
				node.neighbors.begin(), node.neighbors.end());
		}
	}

	// We remove all other nodes.
	for (unsigned i = 0; i < nodes.size(); ++i)
		if (partitions.find(i) == i)
			nodes[reduced[i]] = nodes[i];

	nodes.resize(reduced.back() + 1);

	// Now we remap the neighbors and eliminate duplicates.
	for (unsigned i = 0; i < nodes.size(); ++i) {
		std::vector<unsigned> &neighbors = nodes[i].neighbors;
		std::transform(neighbors.begin(), neighbors.end(), neighbors.begin(),
			[&reduced, &partitions](unsigned n)
			{
				return reduced[partitions.find(n)];
			}
		);

		auto end = std::remove(neighbors.begin(), neighbors.end(), i);
		std::sort(neighbors.begin(), end);
		end = std::unique(neighbors.begin(), end);
		neighbors.erase(end, neighbors.end());
	}
}

State::State(const Graph &graph)
	: graph(graph), filled(graph.getNumNodes(), false),
	  moves(1, graph.getNode(0).color)
{
	filled[0] = true;
}

bool State::move(color_t next)
{
	assert(next != moves.back());

	color_t last = moves.back();
	moves.push_back(next);

	if (next > last)
	{
		// Does the move change anything?
		bool expansion = false;
		for (unsigned node = 0; node < filled.size(); ++node)
			if (graph.getNode(node).color == next && !filled[node])
				for(unsigned neighbor : graph.getNode(node).neighbors)
					if (filled[neighbor])
					{
						filled[node] = true;
						expansion = true;
					}

		if (!expansion)
			return false;
	}
	else
	{
		// Does the move change anything that couldn't have happened before?
		bool additionalExpansion = false;
		for (unsigned node = 0; node < filled.size(); ++node)
			if (graph.getNode(node).color == next && !filled[node])
			{
				// Was any of the neighbors filled before the last move?
				bool prev = false;
				for(unsigned neighbor : graph.getNode(node).neighbors)
					if (filled[neighbor])
					{
						filled[node] = true;
						if (graph.getNode(neighbor).color != last)
							prev = true;
					}
				if (filled[node] && !prev)
					additionalExpansion = true;
			}

		if (!additionalExpansion)
			return false;
	}

	return true;
}

int State::computeValuation() const
{
	// Bitfield to mark visited nodes (to avoid visiting a node more than once).
	std::vector<bool> visited = filled;

	// Current (distance = r) and next layer (distance = r+1) of nodes.
	std::vector<unsigned> current, next;
	current.reserve(filled.size());
	next.reserve(filled.size());

	// For distance = 0, we have all the nodes that are already filled.
	for (unsigned index = 0; index < filled.size(); ++index)
		if (filled[index])
			current.push_back(index);

	unsigned maxDist = 0;
	for (; !current.empty(); ++maxDist)
	{
		// Expand current layer of nodes.
		for (unsigned node : current)
		{
			for (unsigned neighbor : graph.getNode(node).neighbors)
			{
				// If we didn't visit the node yet, it has distance = r+1.
				if (!visited[neighbor])
				{
					next.push_back(neighbor);
					visited[neighbor] = true;
				}
			}
		}

		// Move the next layer into the current.
		std::swap(current, next);
		next.clear();
	}

	return moves.size() + maxDist;
}

bool State::done() const
{
	return std::all_of(filled.begin(), filled.end(), [](bool x) { return x; });
}

std::vector<color_t> computeBestSequence(const Graph &graph, color_t numColors)
{
	State initial(graph);
	special_priority_queue<State> queue(initial.computeValuation());
	queue.push(std::move(initial), initial.computeValuation());

	while (!queue.empty()) {
		const State &state = queue.top();
		if (state.done())
			return state.getMoves();

		// Try all colors but the last one used.
		for (color_t next = 0; next < numColors; ++next) {
			if (next == state.getLastColor())
				continue;

			State nextState = state;
			if (nextState.move(next))
				queue.push(std::move(nextState), nextState.computeValuation());
		}

		queue.pop();
	}

	// If we didn't find any way to flood fill the entire graph, then it's
	// probably not connected.
	throw std::runtime_error("Graph seems to be not connected");
}
