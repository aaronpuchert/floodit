#include <iostream>
#include <random>
#include <sstream>

int main(int argc, char **argv)
{
	if (argc != 4) {
		std::cout << "Usage: " << argv[0] << " rows columns number-of-colors\n";
		return 1;
	}

	// Read parameters
	int rows, columns, numColors;
	std::istringstream(argv[1]) >> rows;
	std::istringstream(argv[2]) >> columns;
	std::istringstream(argv[3]) >> numColors;

	// Initialize random number generator
	std::random_device r;
	std::seed_seq seed1{r(), r(), r(), r(), r(), r(), r(), r()};
	std::mt19937 mt(seed1);
	std::uniform_int_distribution<int> uniform_dist(0, numColors - 1);

	// Generate puzzle.
	std::cout << rows << " " << columns << '\n';
	for (int i = 0; i < rows; ++i)
	{
		for (int j = 0; j < columns; ++j)
			std::cout << uniform_dist(mt) << " ";
		std::cout << '\n';
	}
}
