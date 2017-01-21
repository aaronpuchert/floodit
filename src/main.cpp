#include <algorithm>
#include <iostream>
#include <fstream>
#include <map>
#include "floodit.hpp"

class ColorArray
{
public:
	ColorArray(unsigned rows, unsigned columns);
	void setColor(unsigned row, unsigned column, std::string color);

	Graph createGraph();
	std::vector<std::string> getColors() const;

private:
	unsigned nodeIndex(unsigned row, unsigned column) const
		{ return row * columns + column; }

private:
	unsigned rows, columns;
	std::map<std::string, color_t> colorMap;
	std::vector<decltype(colorMap)::const_iterator> array;
};

ColorArray::ColorArray(unsigned rows, unsigned columns)
	: rows(rows), columns(columns), array(rows * columns) {}

void ColorArray::setColor(unsigned int row, unsigned int column,
                          std::string color)
{
	auto it = colorMap.insert(std::pair<std::string, color_t>{color, 0});
	array[nodeIndex(row, column)] = it.first;
}

Graph ColorArray::createGraph()
{
	// Assign numbers to colors.
	color_t color = 0;
	for (auto& pair : colorMap)
		pair.second = color++;

	std::vector<Graph::Node> nodes(rows*columns);

	for (unsigned i = 0; i < rows; ++i) {
		for (unsigned j = 0; j < columns; ++j) {
			std::vector<unsigned> neighbors;
			if (i > 0)          neighbors.push_back(nodeIndex(i-1, j));
			if (j > 0)          neighbors.push_back(nodeIndex(i, j-1));
			if (i < rows-1)     neighbors.push_back(nodeIndex(i+1, j));
			if (j < columns-1)  neighbors.push_back(nodeIndex(i, j+1));

			nodes[nodeIndex(i, j)] =
				Graph::Node{std::move(neighbors),
					array[nodeIndex(i, j)]->second};
		}
	}

	return Graph(std::move(nodes));
}

std::vector<std::string> ColorArray::getColors() const
{
	std::vector<std::string> colors(colorMap.size());
	std::transform(colorMap.begin(), colorMap.end(), colors.begin(),
		[](const std::pair<std::string, color_t> pair) { return pair.first; }
	);

	return colors;
}

ColorArray readData(std::istream& input)
{
	unsigned rows, columns;
	input >> rows >> columns;

	ColorArray array(rows, columns);
	std::string entry;
	for (unsigned row = 0; row < rows; ++row) {
		for (unsigned column = 0; column < columns; ++column)
		{
			input >> entry;
			array.setColor(row, column, entry);
		}
	}

	return array;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		std::cout << "Usage: " << argv[0] << " filename\n\n"
			"The file should have the number of rows and columns in the first "
			"line, then the colors of each cell. The colors are strings "
			"of non-whitespace characters.\n";
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

	std::vector<std::string> colors = array.getColors();
	std::cout << "A shortest sequence of " << result.size() - 1
	          << " moves is given by:\n\n    [" << colors[result[0]] << "] ";
	for (unsigned move = 1; move < result.size(); ++move)
		std::cout << colors[result[move]] << " ";
	std::cout << '\n';
}
