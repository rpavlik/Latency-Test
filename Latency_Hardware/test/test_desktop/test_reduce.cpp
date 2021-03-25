
#include <unity.h>
#include <numeric>
#include <pairwiseReduce.h>
#include <array>
#include <vector>

const std::array<int, 8> baseVals = {1, 2, 3, 4, 5, 6, 7, 8};
const int simpleSum = ([] { return std::accumulate(baseVals.begin(), baseVals.end(), 0); })();
void test_ints(void)
{
    std::vector<int> vals{baseVals.begin(), baseVals.end()};
    TEST_ASSERT_EQUAL(simpleSum, pairwiseReduce(vals.begin(), vals.end()));
}

void test_floats(void)
{
    std::vector<float> vals{baseVals.begin(), baseVals.end()};
    TEST_ASSERT_EQUAL(simpleSum, pairwiseReduce(vals.begin(), vals.end()));
}


int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_ints);
    RUN_TEST(test_floats);
    UNITY_END();

    return 0;
}