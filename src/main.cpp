#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include <mutex>
#include <thread>

#include "floodit.hpp"

namespace {

class ColorArray
{
public:
	ColorArray(unsigned rows, unsigned columns,
	           unsigned originRow, unsigned originColumn);
	void setColor(unsigned row, unsigned column, std::string&& color);

	Graph createGraph();
	std::vector<std::string> getColors() const;

private:
	unsigned nodeIndex(unsigned row, unsigned column) const
		{ return row * columns + column; }

private:
	const unsigned rows, columns;
	std::map<std::string, color_t> colorMap;
	std::vector<decltype(colorMap)::const_iterator> array;
	unsigned originIndex;
};

ColorArray::ColorArray(unsigned rows, unsigned columns,
                       unsigned originRow, unsigned originColumn)
	: rows(rows), columns(columns), array(rows * columns),
	  originIndex(nodeIndex(originRow, originColumn)) {}

void ColorArray::setColor(unsigned int row, unsigned int column,
                          std::string&& color)
{
	auto it = colorMap.insert({std::move(color), 0});
	array[nodeIndex(row, column)] = it.first;
}

Graph ColorArray::createGraph()
{
	// Assign numbers to colors.
	color_t color = 0;
	for (auto &pair : colorMap)
		pair.second = color++;

	Graph graph(rows * columns);
	graph.setRootIndex(originIndex);
	for (unsigned i = 0; i < rows; ++i) {
		for (unsigned j = 0; j < columns; ++j) {
			if (i > 0)
				graph.addEdge(nodeIndex(i-1, j), nodeIndex(i, j));
			if (j > 0)
				graph.addEdge(nodeIndex(i, j-1), nodeIndex(i, j));

			graph.setColor(nodeIndex(i, j), array[nodeIndex(i, j)]->second);
		}
	}

	return graph;
}

std::vector<std::string> ColorArray::getColors() const
{
	std::vector<std::string> colors(colorMap.size());
	std::transform(colorMap.begin(), colorMap.end(), colors.begin(),
		[](const std::pair<std::string, color_t> pair) { return pair.first; }
	);

	return colors;
}

class PuzzleQueue
{
	struct QueueElement
	{
		QueueElement(Graph &&graph, std::vector<std::string> &&colors)
			: graph(std::move(graph)), colors(std::move(colors)) {}

		Graph graph;
		const std::vector<std::string> colors;
		std::vector<color_t> result;
		bool done = false;
	};

public:
	PuzzleQueue(std::istream &input, std::ostream &output, unsigned rows,
	            unsigned columns, unsigned originRow, unsigned originColumn)
		: input(input), output(output), rows(rows), columns(columns),
		  originRow(originRow), originColumn(originColumn) {}

	/**
	 * Read puzzles from input and solve them until input is exhausted.
	 *
	 * This function may be run by multiple threads at the same time.
	 * The results will be in the order of the input, regardless of which puzzle
	 * is finished first.
	 */
	void solve()
	{
		std::unique_lock<std::mutex> lock(mutex);

		while (QueueElement *puzzle = readPuzzle()) {
			lock.unlock();

			// Reduce graph and solve puzzle. Note that only the ‘done’ flag is
			// considered shared, so we don't need the lock here.
			puzzle->graph.reduce();
			puzzle->result = computeBestSequence(puzzle->graph);

			lock.lock();
			puzzle->done = true;
			flushResults();
		}
	}

private:
	/**
	 * Read and enqueue a puzzle from the input.
	 *
	 * We need to produce the solutions in the order of input, so we store them
	 * immediately in a queue after having read them.
	 *
	 * @note Assumes that @ref mutex is held.
	 *
	 * @return Pointer to enqueued puzzle, if available, otherwise nullptr.
	 */
	QueueElement* readPuzzle()
	{
		ColorArray array(rows, columns, originRow, originColumn);

		unsigned row = 0, column = 0;
		char digit;

		while (input >> digit) {
			array.setColor(row, column, {digit});

			// Go to next position.
			if (++row != rows)
				continue;
			row = 0;

			if (++column != columns)
				continue;

			// Build the puzzle, enqueue it and return a pointer.
			queue.emplace(array.createGraph(), array.getColors());
			return &queue.back();
		}

		return nullptr;
	}

	/**
	 * Flush results from the front of the queue to output stream.
	 *
	 * @note Assumes that @ref mutex is held.
	 */
	void flushResults()
	{
		while (!queue.empty() && queue.front().done) {
			const std::vector<color_t> &result = queue.front().result;
			const std::vector<std::string> &colors = queue.front().colors;
			for (unsigned move = 1; move < result.size(); ++move)
				output << colors[result[move]];
			output << '\n';
			queue.pop();
		}
	}

private:
	// Protecting the members against data races, but not the queue elements in
	// progress, which are private to the thread that added them to the queue.
	std::mutex mutex;

	// Input and output.
	std::istream &input;
	std::ostream &output;

	// Problem dimensions.
	const unsigned rows, columns;
	const unsigned originRow, originColumn;

	// Puzzle queue.
	std::queue<QueueElement> queue;
};

} // anonymous namespace

static ColorArray readData(std::istream &input)
{
	unsigned rows, columns;
	input >> rows >> columns;
	unsigned originRow, originColumn;
	input >> originRow >> originColumn;

	ColorArray array(rows, columns, originRow, originColumn);
	for (unsigned row = 0; row < rows; ++row) {
		for (unsigned column = 0; column < columns; ++column)
		{
			std::string entry;
			input >> entry;
			array.setColor(row, column, std::move(entry));
		}
	}

	return array;
}

static void solvePuzzle(std::istream &input)
{
	ColorArray array = readData(input);
	Graph graph = array.createGraph();
	graph.reduce();
	std::vector<color_t> result = computeBestSequence(graph);

	std::vector<std::string> colors = array.getColors();
	std::cout << "A shortest sequence of " << result.size() - 1
	          << " moves is given by:\n\n    [" << colors[result[0]] << "]";
	for (unsigned move = 1; move < result.size(); ++move)
		std::cout << " " << colors[result[move]];
	std::cout << '\n';
}

static void solvePuzzleChallenge(
	std::istream &input,
	unsigned rows, unsigned columns,
	unsigned originRow, unsigned originColumn)
{
	PuzzleQueue queue(input, std::cout, rows, columns, originRow, originColumn);

	// Fire up worker threads solving puzzles.
	unsigned numThreads = std::thread::hardware_concurrency();
	std::vector<std::thread> threads;
	threads.reserve(numThreads);
	for (unsigned thread = 0; thread != numThreads; ++thread)
		threads.emplace_back([&queue](){ queue.solve(); });

	// Wait until all are done.
	for (auto &thread : threads)
		thread.join();
}

int main(int argc, char **argv)
{
	if (argc == 2) {
		std::ifstream file(argv[1]);
		if (file.fail()) {
			std::cerr << "Error: could not open file '" << argv[1] << "'.\n";
			return 1;
		}

		solvePuzzle(file);
	}
	else if (argc == 4 || argc == 6) {
		unsigned rows, columns;
		std::istringstream(argv[1]) >> rows;
		std::istringstream(argv[2]) >> columns;

		unsigned originRow = 0, originColumn = 0;
		if (argc == 6) {
			std::istringstream(argv[3]) >> originRow;
			std::istringstream(argv[4]) >> originColumn;
		}

		std::ifstream file(argv[argc-1]);
		if (file.fail()) {
			std::cerr << "Error: could not open file '" << argv[argc-1]
			          << "'.\n";
			return 1;
		}

		solvePuzzleChallenge(file, rows, columns, originRow, originColumn);
	}
	else {
		std::cout <<
			"Usage: " << argv[0] << " filename\n"
			"       " << argv[0] << " rows columns [row column] filename\n"
			"\n"
			"In the first variant, the file should have the number of rows and "
			"columns in the first line, the row and column index of the origin "
			"cell (0-based) in the second, and then the colors of each cell, "
			"all separated by spaces. "
			"The colors are strings of non-whitespace characters.\n"
			"\n"
			"In the second variant, the file may contain multiple puzzles, "
			"given by rows x columns single-character colors. Optionally, the "
			"origin cell may be given by row and column index (0-based), "
			"otherwise (0, 0) is assumed.\n";
		return 1;
	}
}
