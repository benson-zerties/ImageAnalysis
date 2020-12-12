#include <iostream>
#include <gtest/gtest.h>

#include "algo.h"

#define NUM_DETECTIONS 200

TEST (AlgoTest, TestForFailExample1)
{
    std::vector<uint32_t> grads;
    std::vector<int8_t> seq = {0,0,1,1,0,0,0,0,2,0,2,2,2,2,2,0,0,0,0,0,0,0,0,0,0,1,1,1,2,0,0,0,
                               1,0,0,0,0,0,0,0,1,0};
    Algo::stdVectorDerivative<int8_t>(seq, grads);

    uint32_t gradCnt = 0;
    for (uint32_t k = 0; k <  seq.size()-1; k++)
    {
        if (seq[k] != seq[k+1])
        {
            ASSERT_EQ (k, grads[gradCnt++]);
        }
    }
}
