#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>
#include "floodit.hpp"

class ColorArray
{
public:
	ColorArray(unsigned rows, unsigned columns, std::vector<color_t> &&colors);
	Graph createGraph() const;
	color_t getNumColors() const;

private:
	unsigned nodeIndex(unsigned row, unsigned column) const
		{ return row * columns + column; }

private:
	unsigned rows, columns;
	std::vector<color_t> array;
};

ColorArray::ColorArray(unsigned rows, unsigned columns,
                       std::vector<color_t> &&colors)
	: rows(rows), columns(columns), array(std::move(colors))
{
	assert(array.size() == rows * columns);
}

Graph ColorArray::createGraph() const
{
	std::vector<Graph::Node> nodes(rows*columns);

	for (unsigned i = 0; i < rows; ++i) {
		for (unsigned j = 0; j < columns; ++j) {
			std::vector<unsigned> neighbors;
			if (i > 0)          neighbors.push_back(nodeIndex(i-1, j));
			if (j > 0)          neighbors.push_back(nodeIndex(i, j-1));
			if (i < rows-1)     neighbors.push_back(nodeIndex(i+1, j));
			if (j < columns-1)  neighbors.push_back(nodeIndex(i, j+1));

			nodes[nodeIndex(i, j)] =
				Graph::Node{std::move(neighbors), array[nodeIndex(i, j)]};
		}
	}

	return Graph(std::move(nodes));
}

color_t ColorArray::getNumColors() const
{
	auto max = std::max_element(array.begin(), array.end());
	return *max + 1;
}

ColorArray readData(std::istream& input)
{
	unsigned rows, columns;
	input >> rows >> columns;

	std::vector<color_t> array(rows * columns, 0);
	int entry;
	for (unsigned i = 0; i < rows*columns; ++i)
	{
		input >> entry;
		array[i] = static_cast<color_t>(entry);
	}

	return ColorArray(rows, columns, std::move(array));
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		std::cout << "Usage: " << argv[0] << " filename\n\n"
			"The file should have the number of rows and columns in the first "
			"line, then the colors of each cell. The colors are consecutive "
			"integers starting with 0.\n";
		return 1;
	}

	std::ifstream file(argv[1]);
	if (file.fail()) {
		std::cerr << "Error: could not open file '" << argv[1] << "'.\n";
		return 1;
	}

	ColorArray array = readData(file);
	Graph graph = array.createGraph();
	graph.reduce();
	std::vector<color_t> result =
		computeBestSequence(graph);

	std::cout << "A shortest sequence of " << result.size() - 1
	          << " moves is given by:\n\n    ";
	for (color_t color : result)
		std::cout << static_cast<int>(color) << " ";
	std::cout << '\n';
}
