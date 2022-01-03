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
#include <stdexcept>
#include <utility>
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

State::State(const Graph &graph, MoveTrie &trie)
	: filled(graph.getNumNodes(), false)
	, moves(trie.append(MoveTrie::initial(),
		graph[graph.getRootIndex()].color))
{
	// Check that the graph is reduced. We are going to assume that later.
	for (unsigned index = 0; index < graph.getNumNodes(); ++index) {
		const Graph::Node &node = graph[index];
		for (unsigned neighbor : node.neighbors)
			assert(node.color != graph[neighbor].color);
	}

	filled[graph.getRootIndex()] = true;
	valuation = computeValuation(graph);
}

bool State::move(const Graph &graph, MoveTrie &trie, color_t next)
{
	assert(next != moves.back());

	color_t last = moves.back();
	moves = trie.append(moves, next);

	if (next > last) {
		// Does the move change anything?
		bool expansion = false;
		for (unsigned node = 0; node < filled.size(); ++node) {
			if (graph[node].color == next && !filled[node]) {
				for(unsigned neighbor : graph[node].neighbors) {
					if (filled[neighbor]) {
						filled[node] = true;
						expansion = true;
					}
				}
			}
		}

		if (!expansion)
			return false;
	}
	else {
		// Does the move change anything that couldn't have happened before?
		bool additionalExpansion = false;
		for (unsigned node = 0; node < filled.size(); ++node) {
			if (graph[node].color == next && !filled[node]) {
				// Was any of the neighbors filled before the last move?
				bool prev = false;
				for (unsigned neighbor : graph[node].neighbors) {
					if (filled[neighbor]) {
						filled[node] = true;
						if (graph[neighbor].color != last)
							prev = true;
					}
				}
				if (filled[node] && !prev)
					additionalExpansion = true;
			}
		}

		if (!additionalExpansion)
			return false;
	}

	valuation = computeValuation(graph);
	return true;
}

unsigned State::computeValuation(const Graph &graph) const
{
	// Obtain a lower bound for the number of moves left. This is done by
	// induction: If a move fills all remaining nodes of some color, it must be
	// optimal, so we can just apply this move. Otherwise, we use a
	// "color-blind" move as it combines the effects of all possible moves. This
	// procedure will reduce the given state until it reaches the filled state.

	// Bitfield to mark visited nodes (to avoid visiting a node more than once).
	std::vector<bool> visited = filled;

	// Current (to be expanded) and next layer of nodes.
	std::vector<unsigned> current, next;
	current.reserve(filled.size());
	next.reserve(filled.size());

	// The remaining number of nodes for each color.
	std::vector<unsigned> colorCounts = graph.getColorCounts();

	// We start with all the nodes that are already filled.
	for (unsigned index = 0; index < filled.size(); ++index) {
		if (filled[index]) {
			current.push_back(index);
			--colorCounts[graph[index].color];
		}
	}

	// Number of colors that can be eliminated in the next move.
	unsigned numExposedColors = 0;
	unsigned minMovesLeft = 0;

	// This will serve as a backup copy of colorCounts in the loop.
	std::vector<unsigned> colorCountsOld(colorCounts.size());

	// Proceed layer by layer, expanding the current layer to obtain the next
	// layer. The vector colorCounts keeps track of the colors of nodes that
	// haven't been visited yet. If an entry of colorCounts reaches zero, the
	// corresponding move from the current state will fill the remaining nodes
	// of this color.
	while (!current.empty()) {
		if (numExposedColors > 0) {
			// We can eliminate colors.
			// We also combine all these elimination moves.
			minMovesLeft += numExposedColors;
			numExposedColors = 0;
			// Backup copy of colorCounts.
			std::copy(
				colorCounts.begin(), colorCounts.end(), colorCountsOld.begin());
			for (unsigned node : current) {
				// If the color is to be eliminated, expand the node.
				if (colorCountsOld[graph[node].color] == 0) {
					// Expand node.
					for (unsigned neighbor : graph[node].neighbors) {
						if (!visited[neighbor]) {
							next.push_back(neighbor);
							visited[neighbor] = true;
							if (--colorCounts[graph[neighbor].color] == 0)
								++numExposedColors;
						}
					}
				}
				else
					next.push_back(node);
			}
		}
		else {
			// Nothing found, do the color-blind pseudo-move.
			++minMovesLeft;
			// Expand current layer of nodes.
			for (unsigned node : current) {
				// Expand node.
				for (unsigned neighbor : graph[node].neighbors) {
					if (!visited[neighbor]) {
						next.push_back(neighbor);
						visited[neighbor] = true;
						if (--colorCounts[graph[neighbor].color] == 0)
							++numExposedColors;
					}
				}
			}
		}

		// Move the next layer into the current.
		std::swap(current, next);
		next.clear();
	}

	return moves.size() + minMovesLeft;
}

std::vector<color_t> State::materializeMoves() const
{
	std::vector<color_t> result(moves.size());
	moves.materialize(result.data());
	return result;
}

bool State::done() const
{
	return std::all_of(filled.begin(), filled.end(), [](bool x) { return x; });
}

namespace {

struct StateCompare
{
	bool operator()(const State &a, const State &b) const
	{
		if (a.getValuation() != b.getValuation())
			return a.getValuation() > b.getValuation();
		return a.getNumMoves() < b.getNumMoves();
	}
};

} // anonymous namespace

std::vector<color_t> computeBestSequence(const Graph &graph)
{
	std::vector<State> queue;
	Trie<color_t> trie;

	queue.emplace_back(graph, trie);

	while (!queue.empty()) {
		std::pop_heap(queue.begin(), queue.end(), StateCompare{});
		State state = std::move(queue.back());
		queue.pop_back();

		if (state.done())
			return state.materializeMoves();

		// Try all colors but the last one used.
		color_t numColors = graph.getColorCounts().size();
		for (color_t next = 0; next < numColors; ++next) {
			if (next == state.getLastColor())
				continue;

			State nextState = state;
			if (!nextState.move(graph, trie, next))
				continue;

			queue.push_back(std::move(nextState));
			std::push_heap(queue.begin(), queue.end(), StateCompare{});
		}
	}

	// If we didn't find any way to flood fill the entire graph, then it's
	// probably not connected.
	throw std::runtime_error("Graph seems to be not connected");
}
