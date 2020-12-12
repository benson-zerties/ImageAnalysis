#ifndef _DATAMODELPROTOBUF_H_
#define _DATAMODELPROTOBUF_H_

#include <memory>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <string>
#include <mutex>
#include <thread>
#include <future>
#include <filesystem>

#include "detection_results_v2.pb.h"
#include "data_model.h"
#include "data_vector.h"
#include "algo.h"

using namespace std;
namespace fs = std::filesystem;

template<typename T_EvalAlgo>
class DataModelProtoBuf: public DataModel<DataModelProtoBuf<T_EvalAlgo>>
{
    typedef T_EvalAlgo T;
    friend class DataModel<DataModelProtoBuf<T_EvalAlgo>>;

	public:
		~DataModelProtoBuf() = default;

        void open(string fname)
        {
            m_path = fs::path(fname).parent_path();
            m_file = make_unique<ifstream>(fname, ios::binary);
            if ( (m_file->rdstate() & std::ifstream::failbit ) != 0 )
            {
                std::cerr << "Error opening file " << fname << std::endl;
            }
            m_dataLoading.clear();
            m_dataLoaded = false;
            m_poisIdentified.clear();
            m_fileOffsets.clear();
            m_fileOffsets.insert(std::make_pair(0,0));
        }

        void load()
        {
            this->parseStuff();
        }

        /**
         * \brief Number of detections per image
         */
        std::vector<int8_t> getNumDetections(uint8_t classIdx)
        {
            std::vector<int8_t> det(0);
            if (classIdx < m_detectsPerClass.size())
            {
                det = m_detectsPerClass[classIdx]->toStdVector();
            }
            return det;
        }

        fs::path getItemByIdx(uint64_t idx)
        {
            auto it = m_fileOffsets.lower_bound(idx);
            if(it != m_fileOffsets.begin()) {
                --it; // it now points at the right element
            }
            idx -= it->first;
            uint64_t off = it->second;

            const unsigned SIZE_BYTES = 8;
            const unsigned FIELD_DESCR = 1;
            const uint64_t DATA_SIZE_MAX = 10000;
            char size_field[SIZE_BYTES + FIELD_DESCR];
            char data[DATA_SIZE_MAX];
            uint64_t record_size = 0;
            fs::path img_path;
            object_detection::Size size;
            object_detection::Example example;
        
            if ( m_file && m_file->is_open() )
            {
                for (unsigned k = 0; k < idx+1; k++) {
                    readFromFile(size_field, sizeof(size_field), off);
                    size.ParseFromArray(size_field, sizeof(size_field));
                    record_size = size.value();
                    off += FIELD_DESCR + SIZE_BYTES;
                    off += record_size;
                }
                size_t n_bytes = std::min(record_size, sizeof(data));
                readFromFile(data, n_bytes, off);
                example.ParseFromArray(data, n_bytes);
                img_path = m_path / example.filename();
            }
            return img_path;
        }

        uint32_t nextPoi(unsigned idx)
        {
            uint32_t idxPoi = m_numExamples;
            if (identifyPois())
            {
                for (auto& classVec : m_poisPerClass) // foreach class
                {
                    for (auto val : *classVec) // foreach point of interest within class
                    {
                        if (val > idx)
                        {
                            if (val < idxPoi)
                                idxPoi = val;
                            break;
                        }
                    }
                }
            }
            return idxPoi;
        }
 
        uint32_t prevPoi(unsigned idx)
        {
            uint32_t idxPoi = 0;
            if (identifyPois())
            {
                for (auto& classVec : m_poisPerClass) // foreach class
                {
                    uint32_t lastVal = 0;
                    for (auto val : *classVec) // foreach point of interest within class
                    {
                        if (val < idx)
                        {
                            lastVal = val;
                        }
                        else
                        {
                            if (lastVal > idxPoi)
                                idxPoi = lastVal;
                            break;
                        }
                        lastVal = val;
                    }
                }
            }
            return idxPoi;
        }
        
	protected:
		shared_ptr<string> m_description;

	private:
		DataModelProtoBuf<T_EvalAlgo>():m_detectsPerClass(0)
        {
            // Verify that the version of the library that we linked against is
            // compatible with the version of the headers we compiled against.
            GOOGLE_PROTOBUF_VERIFY_VERSION;
        }

        bool readFromFile(char* buffer, size_t n, size_t seek_off)
        {
            bool ret_val = false;
            std::lock_guard<std::mutex> lck (m_fileMtx);
            m_file->clear();
            if ( m_file->is_open() )
            {
                m_file->seekg(seek_off);
                if ( !(*m_file) )
                {
                    return ret_val;
                }
                m_file->read(buffer, n);
                if ( !(*m_file) )
                {
                    return ret_val;
                }
                else
                {
                    ret_val = true;
                }
            }
            else
            {
                std::cout << "File unexpetedly closed" << std::endl;
            }
            return ret_val;
        }

        bool identifyPois()
        {
            if (m_dataLoaded)
            {
                if (!m_poisIdentified.test_and_set())
                {
                    m_poisPerClass.resize(m_detectsPerClass.size());
                    for (unsigned classIdx = 0; classIdx < m_detectsPerClass.size(); classIdx++)
                    {
                        m_poisPerClass[classIdx] = make_unique< std::vector<uint32_t> >();
                        auto det = m_detectsPerClass[classIdx]->toStdVector();
                        Algo::stdVectorDerivative<int8_t>(det, *m_poisPerClass[classIdx]);
                    }
                }
                return true;
            }
            else
            {
                return false;
            }
        }
        
        void parseStuff()
        {
            if (m_dataLoading.test_and_set())
            {
                /* file has already been loaded */
                return;
            }
            size_t off = 0;
            object_detection::Size size;
            typename T_EvalAlgo::UParser example;
            uint64_t record_size = 0;
            const unsigned SIZE_BYTES = 8;
            const unsigned FIELD_DESCR = 1;
            const uint64_t DATA_SIZE_MAX = 10000;
            char size_field[SIZE_BYTES + FIELD_DESCR];
            char data[DATA_SIZE_MAX];
            std::vector<int> valid_det(2);
            const int threshold = 10;
            m_numExamples = 0;

            m_detectsPerClass.resize(0);
            for (unsigned idx = 0; idx < class_ids.size(); idx++)
            {
                m_detectsPerClass.push_back(make_unique<DataVector<int8_t, 128>>());
            }

            if ( m_file->is_open() )
            {
                for (unsigned n_rec = 0; n_rec < std::numeric_limits<decltype(n_rec)>::max(); n_rec++) {
                    if ( !readFromFile(size_field, sizeof(size_field), off) )
                    {
                        std::cout << "Reading header failed" << std::endl;
                        break;
                    }
                    if ( !size.ParseFromArray(size_field, sizeof(size_field)) )
                    {
                        std::cout << "Parsing header failed" << std::endl;
                        break;
                    }
                    record_size = size.value();
                    off += FIELD_DESCR + SIZE_BYTES;

                    size_t n_bytes = std::min(record_size, sizeof(data));
                    if (record_size > sizeof(data)) {
                        std::cerr << "Record-size of " << record_size << " bytes exceeds buffer" << std::endl;
                        break;
                    }
                    if ( !readFromFile(data, n_bytes, off) )
                    {
                        std::cout << "Reading payload failed" << std::endl;
                        break;
                    }
                    if ( !example.ParseFromArray(data, n_bytes) )
                    {
                        std::cerr << "Parsing payload failed at record: " << n_rec << std::endl;
                        break;
                    }
                    if ( !T_EvalAlgo::calcNumDetections(example, class_ids, valid_det, threshold) )
                    {
                        return;
                    }
                    for (uint32_t i = 0; i < valid_det.size(); ++i)
                    {
                        m_detectsPerClass[i]->push_back(valid_det[i]);
                    }

                    off += record_size;
                    m_numExamples++;

                    // store file offsets for later use
                    if (m_numExamples % 1024 == 0)
                    {
                        m_fileOffsets.insert(std::make_pair(m_numExamples, off));
                    }
                }
            }
            m_file->clear();
            std::cout << "Found " << m_numExamples << " images" << std::endl;
            m_dataLoaded = true;
        }

        const size_t vectorReservationChunksize = 512;

        unique_ptr<ifstream> m_file;
        std::map<uint32_t,uint32_t> m_fileOffsets;
        std::mutex m_fileMtx;
        fs::path m_path;
        std::atomic_flag m_dataLoading = ATOMIC_FLAG_INIT;
        std::atomic_flag m_poisIdentified = ATOMIC_FLAG_INIT;
        std::atomic<bool> m_dataLoaded = false;

        uint32_t m_numExamples;
        std::vector< unique_ptr<DataVector<int8_t, 128>> > m_detectsPerClass;
        std::vector< unique_ptr< std::vector<uint32_t> > > m_poisPerClass;
        const std::vector<int> class_ids{1,2};
        DataVector<uint8_t, 128> m_numDetections;
        uint8_t m_scoreThreshold = 60;
        std::mutex fileMtx;
};

#endif /* _DATAMODELPROTOBUF_H_ */
