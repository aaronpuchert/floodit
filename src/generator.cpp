#include <iostream>
#include <random>
#include <sstream>

int main(int argc, char **argv)
{
	if (argc != 4 && argc != 6) {
		std::cout << "Usage: " << argv[0]
		          << " rows columns [row column] number-of-colors\n";
		return 1;
	}

	// Read parameters
	int rows, columns, originRow = 0, originColumn = 0, numColors;
	std::istringstream(argv[1]) >> rows;
	std::istringstream(argv[2]) >> columns;
	if (argc == 6) {
		std::istringstream(argv[3]) >> originRow;
		std::istringstream(argv[4]) >> originColumn;
	}
	std::istringstream(argv[argc-1]) >> numColors;

	// Initialize random number generator
	std::random_device r;
	std::seed_seq seed1{r(), r(), r(), r(), r(), r(), r(), r()};
	std::mt19937 mt(seed1);
	std::uniform_int_distribution<int> uniform_dist(0, numColors - 1);

	// Generate puzzle. For now with origin (0, 0).
	std::cout << rows << " " << columns << '\n'
	          << originRow << " " << originColumn << '\n';
	for (int i = 0; i < rows; ++i)
	{
		for (int j = 0; j < columns; ++j)
			std::cout << uniform_dist(mt) << " ";
		std::cout << '\n';
	}
}
