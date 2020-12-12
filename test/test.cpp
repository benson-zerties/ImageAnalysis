#include <iostream>
#include <gtest/gtest.h>

#include "eval_fast_rcnn_resnet101.h"

#define NUM_DETECTIONS 200

TEST (EvalFastRcnnResnet101Test, Example1)
{
    EvalFastRcnnResnet101::UParser example;
    example.set_num_detections(NUM_DETECTIONS);
  
    const int threshold = 30;
    const char scores[NUM_DETECTIONS]  = {80, 40, 80, 0, 20, 10, 5, 0, 60 }; 
    const char classes[NUM_DETECTIONS] = { 2,  1,  2, 1,  1,  1, 2, 2,  2 };
    example.set_scores(std::string(reinterpret_cast<const char*>(scores), sizeof(scores)));
    example.set_classes(std::string(reinterpret_cast<const char*>(classes), sizeof(classes)));

    const std::vector<int> class_ids{1,2};
    std::vector<int> valid_det(2);
    EvalFastRcnnResnet101::calcNumDetections(example, class_ids, valid_det, threshold);
    // detections for class 1
    ASSERT_EQ (valid_det[0], 1); 
    // detections for class 2
    ASSERT_EQ (valid_det[1], 3); 
    for (auto i = valid_det.begin(); i != valid_det.end(); ++i)
    {
        std::cout << *i << ' ';
    }
}


int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
