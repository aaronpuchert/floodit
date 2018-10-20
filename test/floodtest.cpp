#include <gtest/gtest.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <utility>
#include <vector>
#include "floodit.hpp"

struct FlooditTestParam
{
	std::vector<color_t> colors;
	std::vector<std::pair<unsigned, unsigned>> edges;
	unsigned numMoves;
};

class FlooditTest : public testing::TestWithParam<FlooditTestParam> {};

TEST_P(FlooditTest, Solve)
{
	const FlooditTestParam& param = GetParam();

	// Verify that the graph is reduced.
	for (auto edge : param.edges)
		EXPECT_NE(param.colors[edge.first], param.colors[edge.second]);

	// We duplicate, invert and sort the relations for easy access.
	std::vector<std::pair<unsigned, unsigned>> edges(param.edges);
	std::transform(
		param.edges.begin(), param.edges.end(), std::back_inserter(edges),
		[](const std::pair<unsigned, unsigned>& edge)
		{ return std::make_pair(edge.second, edge.first); }
	);
	std::sort(edges.begin(), edges.end());

	// Build graph.
	Graph graph(param.colors.size());

	for (unsigned i = 0; i != param.colors.size(); ++i)
		graph.setColor(i, param.colors[i]);

	for (std::pair<unsigned, unsigned> edge : param.edges)
		graph.addEdge(edge.first, edge.second);

	// Compute solution.
	std::vector<color_t> solution = computeBestSequence(graph);

	// Verify first (pseudo-)move.
	EXPECT_EQ(param.colors[0], solution[0]);

	// Verify solution. (This only works for reduced graphs.)
	std::vector<bool> filled(param.colors.size());
	filled[0] = true;
	for (color_t color : solution) {
		for (unsigned i = 0; i != param.colors.size(); ++i) {
			if (filled[i]) {
				auto begin = std::lower_bound(
					param.edges.begin(), param.edges.end(),
					std::make_pair(i, 0u));
				auto end = std::lower_bound(
					param.edges.begin(), param.edges.end(),
					std::make_pair(i+1, 0u));
				for (auto cur = begin; cur != end; ++cur)
					if (param.colors[cur->second] == color)
						filled[cur->second] = true;
			}
		}
	}

	for (unsigned i = 0; i != filled.size(); ++i)
		EXPECT_TRUE(filled[i]) << "Field " << i << " not filled";

	// Check number of moves.
	EXPECT_EQ(param.numMoves, solution.size() - 1);
}

static const FlooditTestParam flooditTestParams[] = {
	{
		{0},
		{},
		0
	},
	{
		{0, 1},
		{{0, 1}},
		1
	},
	{
		{0, 1, 0},
		{{0, 1}, {1, 2}},
		2
	},
	{
		{0, 1, 2},
		{{0, 1}, {0, 2}, {1, 2}},
		2
	},
	{
		{0, 1, 1, 0},
		{{0, 1}, {0, 2}, {1, 3}, {2, 3}},
		2
	},
	{
		{0, 1, 2, 0},
		{{0, 1}, {0, 2}, {1, 3}, {2, 3}},
		3
	},
	{
		{0, 1, 2, 0},
		{{0, 1}, {0, 2}, {1, 2}, {2, 3}},
		3
	},
	{
		{0, 1, 2, 1},
		{{0, 1}, {0, 2}, {1, 2}, {2, 3}},
		2
	},
	{
		{0, 1, 2, 0},
		{{0, 1}, {0, 2}, {1, 2}, {1, 3}, {2, 3}},
		3
	},
	{
		{0, 1, 2, 3},
		{{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}},
		3
	},
};

INSTANTIATE_TEST_CASE_P(
	FloodTest, FlooditTest, ::testing::ValuesIn(flooditTestParams));
