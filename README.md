Flood It - Solver
=================

[![Build Status](https://github.com/aaronpuchert/floodit/actions/workflows/build-test.yml/badge.svg)](https://github.com/aaronpuchert/floodit/actions/workflows/build-test.yml)

This program tries to find an optimal solution to the game [Flood It](http://unixpapa.com/floodit/).
The idea is to understand it as a graph problem and use the A* algorithm.

The program can be compiled via `make`. If necessary, set `CXX` to your favorite C++ compiler.
A Debug version can be compiled via setting `VARIANT=debug`.
