#ifndef FLOODIT_HPP
#define FLOODIT_HPP

#include "trie.hpp"

#include <vector>

typedef unsigned char color_t;

/**
 * Colored undirected graph.
 */
class Graph
{
public:
	struct Node
	{
		std::vector<unsigned> neighbors;    ///< Sorted list of neighbor nodes.
		color_t color;                      ///< Color of the node.
	};

public:
	/**
	 * Construct completely unconnected, all nodes having color 0.
	 *
	 * @param numNodes Number of nodes for the graph.
	 */
	explicit Graph(unsigned numNodes);

	/// Get index of root node.
	unsigned getRootIndex() const { return rootIndex; }
	/// Set @p index of root node.
	void setRootIndex(unsigned index) { rootIndex = index; }

	/**
	 * Set color of node @p index to @p color.
	 */
	void setColor(unsigned index, color_t color);

	/**
	 * Add an edge between nodes @p a and @p b.
	 */
	void addEdge(unsigned a, unsigned b);

	/**
	 * Reduce the graph.
	 *
	 * We can safely merge adjacent nodes of the same color, because they will
	 * always be filled together.
	 *
	 * @note The old root node will be part of the new root node.
	 */
	void reduce();

	/**
	 * Get number of nodes in the graph.
	 * @return Number of nodes.
	 */
	unsigned getNumNodes() const { return nodes.size(); }

	/**
	 * Get node by index.
	 * @param i Index of node.
	 * @return Node at index @p i.
	 */
	const Node& operator[](unsigned i) const { return nodes[i]; }

	/**
	 * Get a vector that contains the number of nodes for each color.
	 * @return Color statistic.
	 */
	const std::vector<unsigned>& getColorCounts() const { return colorCounts; }

private:
	std::vector<Node> nodes;
	unsigned rootIndex;

	std::vector<unsigned> colorCounts;
};

/**
 * State class.
 */
class State
{
public:
	using MoveTrie = Trie<color_t>;

	/**
	 * Create initial state based on a graph.
	 * @param graph Graph to be based on.
	 * @param trie Data structure to store moves.
	 */
	State(const Graph &graph, MoveTrie &trie);

	/**
	 * Do a move.
	 * @param graph Graph to be based on.
	 * @param trie Data structure to store moves.
	 * @param next Color for move.
	 * @return True, if the move makes sense.
	 */
	bool move(const Graph &graph, MoveTrie &trie, color_t next);

	/**
	 * Get valuation of the state.
	 * @return Lower bound on the total number of moves required.
	 */
	unsigned getValuation() const { return valuation; }

	/**
	 * Get the number of moves that lead to the state.
	 * @return Number of moves, including the initial color of node 0.
	 */
	unsigned getNumMoves() const { return moves.size(); }

	/**
	 * Get the moves that lead to the state.
	 * @return Vector of moves, including the initial color of node 0.
	 */
	std::vector<color_t> materializeMoves() const;

	/**
	 * Get the color of the last move.
	 * @return Color of the last move.
	 */
	color_t getLastColor() const { return moves.back(); }

	/**
	 * Are we done?
	 * @return True, if all nodes are filled.
	 */
	bool done() const;

private:
	unsigned computeValuation(const Graph &graph) const;

private:
	std::vector<bool> filled;
	MoveTrie::Sequence moves;
	unsigned valuation;
};

/**
 * A^* algorithm to compute the best sequence.
 */
std::vector<color_t> computeBestSequence(const Graph &graph);

#endif
