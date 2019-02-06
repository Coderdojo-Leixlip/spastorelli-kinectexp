#!/bin/sh

make clean || exit 1

# Generate baseline coverage
make all COVERAGE=ON || exit 1
lcov --capture --initial \
     --directory ./build/libs/ \
     --output-file ./build/coverage/.coverage.base

# Run tests and generate coverage
make run-tests COVERAGE=ON || exit 1
lcov --capture \
     --directory ./build/libs/ \
     --output-file ./build/coverage/.coverage.run

# Combine coverage tracefiles
lcov --add-tracefile ./build/coverage/.coverage.base \
     --add-tracefile ./build/coverage/.coverage.run \
     --output-file ./build/coverage/.coverage.total

# Extract only the coverage we need
lcov --extract ./build/coverage/.coverage.total \
      "$(PWD)/src/*" \
     --output-file ./build/coverage/.coverage.final

# Generate HTML output
genhtml --quiet --no-branch-coverage --output-directory=./build/coverage/ \
     ./build/coverage/.coverage.final