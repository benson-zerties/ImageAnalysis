#ifndef ALGO_H_
#define ALGO_H_

#include <cstdio>
#include <immintrin.h>
#include "detection_results_v2.pb.h"

using namespace std;

namespace Algo
{
    template<typename T>
    void stdVectorDerivative(const std::vector<T>& data, std::vector<uint32_t>& nonZeroGrad)
    {
        /* always fail at compile-time */
        /* TODO: do we really need both lines?? */
        static_assert(is_integral<T>::value == true,"Datatype not supported");
        static_assert(is_integral<T>::value == false,"Datatype not supported");
    }

    template<>
    void stdVectorDerivative<int8_t>(const std::vector<int8_t>& data, std::vector<uint32_t>& nonZeroGrad)
    {
        static char* data_aligned_a = nullptr;
        static char* data_aligned_b = nullptr;
        static uint32_t num_samples_old = 0;

        uint32_t num_samples = data.size();
        nonZeroGrad.resize(num_samples);
        const size_t chunk_size = 32;
        const size_t alignment = 32; // required by AVX256 instructions
        /* calculate number of chucks by ceiling */
        auto rational_ceil = [](size_t num, size_t denom) { return (num + denom - 1) / denom; };
        uint32_t chunks = rational_ceil(num_samples*sizeof(data[0]),chunk_size);
        size_t size = chunk_size * chunks;
        if (num_samples != num_samples_old)
        {
            std::cout << "calcNumDetections(): allocting aligned memory" << std::endl;
            delete data_aligned_a;
            delete data_aligned_b;
            data_aligned_a = (char*)aligned_alloc( alignment, size );
            data_aligned_b = (char*)aligned_alloc( alignment, size );
            num_samples_old = num_samples;
        }
        memset(data_aligned_a + num_samples * sizeof(data[0]), 0, (size - num_samples)*sizeof(data[0]));
        memset(data_aligned_b + (num_samples-1) * sizeof(data[0]), 0, (size - num_samples + 1)*sizeof(data[0]));
        assert(num_samples <= size && "Not enough memory for all detections");
        memcpy(data_aligned_a, &data[0], num_samples);
        assert(num_samples <= size && "Not enough memory for all detections");
        memcpy(data_aligned_b, &data[1], num_samples - 1);

        uint32_t cntNonZeroGrads = 0;
        for (size_t n = 0; n < chunks; n++) /* for each chunk is the list of all detections */
        {
            __m256i data_a8 = _mm256_load_si256((__m256i_u const *)(data_aligned_a + n*chunk_size));
            __m256i data_b8 = _mm256_load_si256((__m256i_u const *)(data_aligned_b + n*chunk_size));
            __m256i result  = _mm256_sub_epi8 (data_b8, data_a8); // b - a
            __m256i cmp_zeros_res = _mm256_cmpeq_epi8(result, _mm256_setzero_si256());

            int bitmask = ~_mm256_movemask_epi8(cmp_zeros_res);
            while (bitmask){
                int i = __builtin_ctz(bitmask);  /* Count the number of trailing zero bits in bitmask. */
                nonZeroGrad[cntNonZeroGrads++] = (i + n*chunk_size);
                bitmask = bitmask & (bitmask-1); /* Clear the lowest set bit in k.                     */
            }
        }
        nonZeroGrad.resize(cntNonZeroGrads);
    }
};

#endif /* ALGO_H_ */
