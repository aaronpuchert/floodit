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
#include <queue>
#include <stdexcept>
#include "unionfind.hpp"

Graph::Graph(unsigned numNodes)
	: nodes(numNodes), rootIndex(0), colorCounts(1, numNodes) {}

void Graph::setColor(unsigned index, color_t color)
{
	--colorCounts[nodes[index].color];
	nodes[index].color = color;

	if (color >= colorCounts.size())
		colorCounts.resize(color+1);
	++colorCounts[color];
}

void Graph::addEdge(unsigned a, unsigned b)
{
	nodes[a].neighbors.push_back(b);
	nodes[b].neighbors.push_back(a);
}

void Graph::reduce()
{
	// If a node has the same color as a neighbor, merge them.
	UnionFind partitions(nodes.size());
	for (unsigned i = 0; i < nodes.size(); ++i)
		for (unsigned neighbor : nodes[i].neighbors)
			if (nodes[i].color == nodes[neighbor].color)
				partitions.merge(i, neighbor);

	// Create a map for renumbering the nodes. Update color counts.
	std::vector<unsigned> reduced(nodes.size(), 0);
	for (unsigned i = 1; i < nodes.size(); ++i) {
		assert(partitions.find(i) <= i);
		reduced[i] = reduced[i-1] + (partitions.find(i) == i);
		if (partitions.find(i) != i)
			--colorCounts[nodes[i].color];
	}

	// Update root index.
	rootIndex = reduced[partitions.find(rootIndex)];

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

	// Check that we (still) have all colors.
	unsigned numColors =
		std::count_if(colorCounts.begin(), colorCounts.end(),
			[](unsigned i) { return i > 0; });
	if (numColors != colorCounts.size())
		throw std::runtime_error("We have no nodes for some colors");
}

State::State(const Graph &graph)
	: graph(&graph), filled(graph.getNumNodes(), false),
	  moves(1, graph.getNode(graph.getRootIndex()).color)
{
	// Check that the graph is reduced. We are going to assume that later.
	for (unsigned index = 0; index < graph.getNumNodes(); ++index) {
		const Graph::Node &node = graph.getNode(index);
		for (unsigned neighbor : node.neighbors)
			assert(node.color != graph.getNode(neighbor).color);
	}

	filled[graph.getRootIndex()] = true;
	valuation = computeValuation();
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
			if (graph->getNode(node).color == next && !filled[node])
				for(unsigned neighbor : graph->getNode(node).neighbors)
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
			if (graph->getNode(node).color == next && !filled[node])
			{
				// Was any of the neighbors filled before the last move?
				bool prev = false;
				for(unsigned neighbor : graph->getNode(node).neighbors)
					if (filled[neighbor])
					{
						filled[node] = true;
						if (graph->getNode(neighbor).color != last)
							prev = true;
					}
				if (filled[node] && !prev)
					additionalExpansion = true;
			}

		if (!additionalExpansion)
			return false;
	}

	valuation = computeValuation();
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

	// We observe the following: for every distance d of which we have nodes,
	// the sum of the distance plus the number of colors of nodes of distance
	// larger than d is a lower bound for the number of moves needed. That is
	// because the first d moves can at most remove nodes of distance less than
	// or equal to d, and the remaining colors have to be cleared by separate
	// moves. The maximum of this number over all d for which we have nodes is
	// obviously still a lower bound, hence admissible. It is also consistent.

	// The remaining number of nodes for each color.
	std::vector<unsigned> colorCounts = graph->getColorCounts();
	// The remaining number of colors.
	unsigned numColors = colorCounts.size();
	unsigned max = 0;
	for (unsigned distance = 0; !current.empty(); ++distance)
	{
		// Expand current layer of nodes.
		for (unsigned node : current)
		{
			if (--colorCounts[graph->getNode(node).color] == 0)
				--numColors;
			for (unsigned neighbor : graph->getNode(node).neighbors)
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
		max = std::max(max, distance + numColors);
	}

	return moves.size() + max;
}

bool State::done() const
{
	return std::all_of(filled.begin(), filled.end(), [](bool x) { return x; });
}

struct StateCompare
{
	bool operator()(const State &a, const State &b) const
	{
		if (a.getValuation() != b.getValuation())
			return a.getValuation() > b.getValuation();
		return a.getMoves().size() < b.getMoves().size();
	}
};

std::vector<color_t> computeBestSequence(const Graph &graph)
{
	std::priority_queue<State, std::vector<State>, StateCompare> queue;
	queue.push(State(graph));

	while (!queue.empty()) {
		State state = queue.top();
		queue.pop();
		if (state.done())
			return state.getMoves();

		// Try all colors but the last one used.
		color_t numColors = graph.getColorCounts().size();
		for (color_t next = 0; next < numColors; ++next) {
			if (next == state.getLastColor())
				continue;

			State nextState = state;
			if (nextState.move(next))
				queue.push(std::move(nextState));
		}
	}

	// If we didn't find any way to flood fill the entire graph, then it's
	// probably not connected.
	throw std::runtime_error("Graph seems to be not connected");
}
