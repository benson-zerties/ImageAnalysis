#include <iostream>
#include <filesystem>

#include "detection_results_v2.pb.h"
#include "data_model.h"

using namespace std;
namespace fs = std::filesystem;

mutex DataModel::init_mutex;

DataModel::DataModel()
{
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

	cout << "creating singleton" << endl;
}

DataModel::~DataModel()
{
}

shared_ptr<DataModel> DataModel::getInstance()
{
	static shared_ptr<DataModel> singleton;
	
	if (singleton == NULL) {
		/* make DataModel object creation thread-safe */
		lock_guard<mutex> lock(init_mutex);
		if (singleton == NULL) {
			cout << "not initialized yet" << endl;
    		singleton = shared_ptr<DataModel>(new DataModel());
		}
	}
	return singleton;
}

void DataModel::open(string fname)
{
    m_path = fs::path(fname).parent_path();
    m_file = make_unique<ifstream>(fname, ios::binary);
}

fs::path DataModel::getItemByIdx(uint64_t idx)
{
    (void)idx;
    const unsigned SIZE_BYTES = 8;
    const unsigned FIELD_DESCR = 1;
    const uint64_t DATA_SIZE_MAX = 5000;
    char size_field[SIZE_BYTES + FIELD_DESCR];
    char data[DATA_SIZE_MAX];
    uint64_t off = 0;
    uint64_t record_size = 0;
    fs::path img_path;

    object_detection::Size size;
    object_detection::Example example;

    if ( m_file->is_open() )
    {
        std::cout << "Read from file" << std::endl;
        m_file->seekg(off);
        m_file->read(size_field, sizeof(size_field));
        size.ParseFromArray(size_field, sizeof(size_field));
        record_size = size.value();
        std::cout << "Size of record: " << record_size << std::endl;

        off += FIELD_DESCR + SIZE_BYTES;
        m_file->seekg(off);
        m_file->read(data, std::min(record_size, DATA_SIZE_MAX));
        example.ParseFromArray(data, record_size);
        std::cout << "Example fname: " << example.filename() << std::endl;
        img_path = m_path / example.filename();
        
    }
    return img_path;
}



