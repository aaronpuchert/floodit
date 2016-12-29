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
	friend bool operator==(const State &a, const State &b);

public:
	explicit State(const Graph &graph);
	State(const State &old, color_t next);
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
