#include "celero/Celero.h"

#include <vector>
#include <numeric>
using namespace std;

// From https://www.bfilipek.com/2016/01/micro-benchmarking-libraries-for-c.html#comparison

auto IntToStringConversionTest(int count)
{
    vector<int> inputNumbers(count);
    vector<string> outNumbers;

    iota(begin(inputNumbers), end(inputNumbers), 0);
    for (auto& num : inputNumbers) outNumbers.push_back(to_string(num));

    return outNumbers;
}

auto DoubleToStringConversionTest(int count)
{
    vector<double> inputNumbers(count);
    vector<string> outNumbers;

    iota(begin(inputNumbers), end(inputNumbers), 0.12345);
    for (auto& num : inputNumbers) outNumbers.push_back(to_string(num));

    return outNumbers;
}

CELERO_MAIN;

BASELINE(IntToStringTest, Baseline10, 10, 100)
{
    IntToStringConversionTest(10);
}

BENCHMARK(IntToStringTest, Baseline1000, 10, 100)
{
    IntToStringConversionTest(1000);
}

BASELINE(DoubleToStringTest, Baseline10, 10, 100)
{
    DoubleToStringConversionTest(10);
}

BENCHMARK(DoubleToStringTest, Baseline1000, 10, 100)
{
    DoubleToStringConversionTest(1000);
}
