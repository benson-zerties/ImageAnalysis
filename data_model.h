#ifndef DATAMODEL_H_
#define DATAMODEL_H_

#include <memory>
#include <fstream>
#include <string>
#include <mutex>
#include <future>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

template <class T>
class DataModel
{
    // TODO: checks if member is available but does not check is signature is same
    static constexpr bool hasMember = std::is_member_function_pointer<decltype(&T::getNumDetections)>::value;

    public:
        // workaround to avoid virtual functions using CRTP
        std::vector<int8_t> getNumDetections(uint8_t classIdx)
        {
            return static_cast<T*>(this)->getNumDetections(classIdx);
        }

        void open(string fname)
        {
            return static_cast<T*>(this)->open(fname);
        }

        void load()
        {
            return static_cast<T*>(this)->load();
        }

        fs::path getItemByIdx(uint64_t idx)
        {
            return static_cast<T*>(this)->getItemByIdx(idx);
        }

        uint32_t nextPoi(unsigned idx)
        {
            return static_cast<T*>(this)->nextPoi(idx);
        }

        uint32_t prevPoi(unsigned idx)
        {
            return static_cast<T*>(this)->prevPoi(idx);
        }

        int testFunc(int a)
        {
            return static_cast<T*>(this)->testFunc(a);
        }

        DataModel()
        {
            static_assert(hasMember);
        }

		~DataModel() {};

        static shared_ptr<DataModel> getInstance()
        {
            static mutex init_mutex;
        	
        	if (DataModel<T>::singleton == NULL) {
        		/* make DataModel object creation thread-safe */
        		lock_guard<mutex> lock(init_mutex);
        		if (DataModel<T>::singleton == NULL) {
            		DataModel<T>::singleton = shared_ptr<T>(new T());
        		}
        	}
        	return DataModel<T>::singleton;
        }
	
    private:
        static shared_ptr<T> singleton;
};
        
template <class T>
shared_ptr<T> DataModel<T>::singleton = nullptr;

#endif /* DATAMODEL_H_ */
