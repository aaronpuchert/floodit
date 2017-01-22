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
	 * @note This will of course renumber the nodes, but node 0 will be part of
	 * the new node 0.
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
	const Node& getNode(unsigned i) const { return nodes[i]; }

	/**
	 * Get a vector that contains the number of nodes for each color.
	 * @return Color statistic.
	 */
	const std::vector<unsigned>& getColorCounts() const { return colorCounts; }

private:
	std::vector<Node> nodes;
	std::vector<unsigned> colorCounts;
};

/**
 * State class.
 */
class State
{
public:
	/**
	 * Create initial state based on a graph.
	 * @param graph Graph to be based on.
	 */
	explicit State(const Graph &graph);

	/**
	 * Do a move.
	 * @param next Color for move.
	 * @return True, if the move makes sense.
	 */
	bool move(color_t next);

	/**
	 * Get valuation of the state.
	 * @return Lower bound on the total number of moves required.
	 */
	int getValuation() const { return valuation; }

	/**
	 * Get the moves that lead to the state.
	 * @return Vector of moves, including the initial color of node 0.
	 */
	const std::vector<color_t>& getMoves() const { return moves; }

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
	int computeValuation() const;

private:
	const Graph *graph;

	std::vector<bool> filled;
	std::vector<color_t> moves;
	int valuation;
};

/**
 * A^* algorithm to compute the best sequence.
 */
std::vector<color_t> computeBestSequence(const Graph &graph);
