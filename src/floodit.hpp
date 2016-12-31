#include <vector>

typedef char color_t;

/**
 * Colored undirected graph.
 */
class Graph
{
public:
	struct Node
	{
		std::vector<unsigned> neighbors;	///< Sorted list of neighbor nodes.
		color_t color;                      ///< Color of the node.
	};

public:
	/**
	 * Construct graph from a vector of nodes.
	 * @param nodelist Vector of @ref Node%s.
	 */
	explicit Graph(std::vector<Node> &&nodelist);

	/**
	 * Reduce the graph.
	 *
	 * We can safely merge adjacent nodes of the same color, because they will
	 * always be filled together.
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

private:
	std::vector<Node> nodes;
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
std::vector<color_t> computeBestSequence(const Graph &graph, color_t numColors);
