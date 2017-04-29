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

std::vector<unsigned> State::nbFilled() const
{
	std::vector<unsigned> nf;
	nf.reserve(filled.size());
	bool visited[filled.size()];
	std::copy(filled.begin(), filled.end(), visited);

	for (unsigned node = 0; node < filled.size(); ++node) {
		if (filled[node]) {
			for (unsigned nb: graph->getNode(node).neighbors) {
				if (!visited[nb]){
					nf.push_back(nb);
					visited[nb] = true;
				}
			}
		}
	}
	return nf;
}

bool State::move(color_t next, std::vector<unsigned> nbFilled)
{
	assert(next != moves.back());

	color_t last = moves.back();
	moves.push_back(next);

	// Does the move change anything that couldn't have happened before?
	bool swappedMovesBetter = std::all_of(nbFilled.begin(), nbFilled.end(),
	[this, next, last](unsigned node) {
		if(graph->getNode(node).color != next) return true;
		std::vector<unsigned> nbs = graph->getNode(node).neighbors;
		return std::any_of(nbs.begin(), nbs.end(),
			[this, last](unsigned nb) { return filled[nb] &&
			graph->getNode(nb).color != last; } ); } ) &&
	(next < last || std::any_of(nbFilled.begin(), nbFilled.end(),
		[this, next, last](unsigned node) {
		if(graph->getNode(node).color != next) return false;
		std::vector<unsigned> nbs = graph->getNode(node).neighbors;
		return std::any_of(nbs.begin(), nbs.end(),
			[this, last](unsigned nb) { return !filled[nb] &&
			graph->getNode(nb).color == last; } ); } ));
	if (swappedMovesBetter)
		return false;

	for (unsigned node : nbFilled)
		filled[node] = (graph->getNode(node).color == next);


	valuation = computeValuation();
	return true;
}

int State::computeValuation() const
{
	// Bitfield to mark visited nodes (to avoid visiting a node more than once).
	bool visited[filled.size()];
	std::copy(filled.begin(), filled.end(), visited);

	// Current and next layer of nodes.
	unsigned array[2][filled.size()];
	std::pair<unsigned*, size_t> current = {array[0], 0}, next = {array[1], 0};

	// The number of nodes for each color that we haven't visited yet.
	const std::vector<unsigned>& cc = graph->getColorCounts();
	unsigned colorCounts[cc.size()];
	std::copy(cc.begin(), cc.end(), colorCounts);

	// Visit all filled nodes.
	for (unsigned index = 0; index < filled.size(); ++index) {
		if (filled[index]) {
			current.first[current.second++] = index;
			--colorCounts[graph->getNode(index).color];
		}
	}

	// Expand node, adapt colorCounts and return number of eliminated colors.
	auto expandNode = [this, &visited, &next, &colorCounts](unsigned node)
	{
		unsigned numEliminated = 0;
		for (unsigned neighbor : graph->getNode(node).neighbors) {
			if (!visited[neighbor]) {
				next.first[next.second++] = neighbor;
				visited[neighbor] = true;
				if (--colorCounts[graph->getNode(neighbor).color] == 0)
					++numEliminated;
			}
		}

		return numEliminated;
	};

	// We compute an admissible heuristic recursively: If there are no nodes
	// left, return 0. Furthermore, if a color can be eliminated in one move
	// from the current position, that move is an optimal move and we can
	// simply use it. Otherwise, all moves fill a subset of the neighbors of
	// the filled nodes. Thus, filling that layer gets us at least one step
	// closer to the end.

	// Number of colors that can be eliminated in the next move.
	unsigned numEliminated = 0;
	bool eliminated[cc.size()];
	unsigned distance = 0;

	while (current.second != 0) {
		if (numEliminated > 0) {
			// We can eliminate colors. Do just that.
			// We also combine all these elimination moves.
			distance += numEliminated;
			numEliminated = 0;

			// Detect the eliminated colors.
			for (unsigned i = 0; i < cc.size(); ++i)
				eliminated[i] = (colorCounts[i] == 0);
			for (unsigned i = 0; i < current.second; ++i) {
				unsigned node = current.first[i];
				if (eliminated[graph->getNode(node).color])
					numEliminated += expandNode(node);
				else
					next.first[next.second++] = node;
			}
		}
		else {
			++distance;

			// Nothing found, do the color-blind pseudo-move
			// Expand current layer of nodes.
			for (unsigned i = 0; i < current.second; ++i)
				numEliminated += expandNode(current.first[i]);
		}

		// Move the next layer into the current.
		std::swap(current, next);
		next.second = 0;
	}

	return moves.size() + distance;
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

		std::vector<unsigned> nbFilled = state.nbFilled();
		// Try all colors but the last one used.
		color_t numColors = graph.getColorCounts().size();
		for (color_t next = 0; next < numColors; ++next) {
			if (next == state.getLastColor())
				continue;

			State nextState = state;
			if (nextState.move(next, nbFilled))
				queue.push(std::move(nextState));
		}
	}

	// If we didn't find any way to flood fill the entire graph, then it's
	// probably not connected.
	throw std::runtime_error("Graph seems to be not connected");
}
