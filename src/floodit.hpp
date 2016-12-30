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
	explicit Graph(std::vector<Node> &&nodelist);
	void reduce();
	unsigned getNumNodes() const { return nodes.size(); }
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
	explicit State(const Graph &graph);

	/**
	 * Do a move.
	 *
	 * @param next Color for move.
	 * @return True, if the move makes sense.
	 */
	bool move(color_t next);
	int computeValuation() const;
	const std::vector<color_t> getMoves() const { return moves; }
	color_t getLastColor() const { return moves.back(); }
	bool done() const;

private:
	const Graph &graph;

	std::vector<bool> filled;
	std::vector<color_t> moves;
};

/**
 * A^* algorithm to compute the best sequence.
 */
std::vector<color_t> computeBestSequence(const Graph &graph, color_t numColors);
