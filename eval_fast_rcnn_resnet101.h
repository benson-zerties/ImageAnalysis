#ifndef EVAL_FAST_RCNN_RESTNET101_H_
#define EVAL_FAST_RCNN_RESTNET101_H_

#include <immintrin.h>
#include <cassert>

#include "detection_results_v2.pb.h"

struct EvalFastRcnnResnet101
{
    typedef object_detection::Example UParser;

    /**
     * \brief Extracts detections results from a single example
     *
     * The classes and scores are assumed to be int8[]
     *
     * \param example
     * \param class_ids The classes, for which number of detections is evaluated
     * \param valid_det Number of detections above threshold
     * \param threshold [0..100]
     */
    static bool calcNumDetections(UParser& example, const std::vector<int>& class_ids, 
            std::vector<int>& valid_det, int threshold)
    {
        static uint32_t num_detections_old = 0;
        static char* scores_aligned = nullptr;
        static char* classes_aligned = nullptr;

        assert(class_ids.size() == valid_det.size() &&
                "For each class-id, we need to the the number of detections");
        uint32_t num_detections = example.num_detections();
        /* we process 32 int8-integers at a time */
        const size_t chunk_size = 32;
        const size_t alignment = 32; // required by AVX256 instructions
        /* calculate number of chucks by ceiling */
        auto rational_ceil = [](size_t num, size_t denom) { return (num + denom - 1) / denom; };
        uint32_t chunks = rational_ceil(num_detections,chunk_size);
        size_t size = chunk_size * chunks;
        if (num_detections != num_detections_old)
        {
            delete scores_aligned;
            delete classes_aligned;
            scores_aligned = (char*)aligned_alloc( alignment, size );
            classes_aligned = (char*)aligned_alloc( alignment, size );
            if (scores_aligned == nullptr || classes_aligned == nullptr)
            {
                // memory allocation failed
                num_detections_old = 0;
                return false;
            }
            num_detections_old = num_detections;
        }
        memset(scores_aligned, 0, size);
        assert(num_detections <= size && "Not enough memory for all detections");
        memcpy(scores_aligned, (example.scores()).c_str(), num_detections);
        memset(classes_aligned, 0, size);
        assert(num_detections <= size && "Not enough memory for all detections");
        memcpy(classes_aligned, (example.classes()).c_str(), num_detections);
        
        for (uint32_t m = 0; m < class_ids.size(); m++) /* for each class */
        {
            int ref_class = class_ids[m];
            valid_det[m] = 0;
            for (size_t n = 0; n < chunks; n++) /* for each chunk is the list of all detections */
            {
                __m256i classes8 = _mm256_load_si256((__m256i_u const *)(classes_aligned + n*chunk_size));
                __m256i scores8  = _mm256_load_si256((__m256i_u const *)(scores_aligned + n*chunk_size));
                /* we work on chunks of 32 x int8_t */
                /* create vector with reference class in each entry */
                __m128i ref_class_128 = _mm_set_epi64x(0, ref_class);
                __m256i ref_class_vec = _mm256_broadcastb_epi8 (ref_class_128);
                /* compare reference class with detected class */
                __m256i class_matches = _mm256_cmpeq_epi8 (ref_class_vec, classes8);

                /* we check if score is above detection threshold */
                __m128i threshold_128 = _mm_set_epi64x(0, threshold);
                __m256i threshold_vec = _mm256_broadcastb_epi8 (threshold_128);
                __m256i score_above_thres32 = _mm256_cmpgt_epi8 (scores8, threshold_vec);

                /* if class matches, and score is above threshold, the result is valid */
                int score_above_thres_mask = _mm256_movemask_epi8(score_above_thres32);
                int class_mask = _mm256_movemask_epi8(class_matches);
                int num_det_tmp = __builtin_popcount(score_above_thres_mask & class_mask);
                valid_det[m] += num_det_tmp;
            }
        }
        return true;
    }
};

#endif /* EVAL_FAST_RCNN_RESTNET101_H_ */
