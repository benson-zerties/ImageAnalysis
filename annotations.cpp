#include "annotations.h"

Annotations::Annotations(fs::path annotationFile)
{
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    m_filename = annotationFile;
    m_parser = make_unique<Parser>();
    ifstream file(m_filename, ios::binary | ios::in); // open in binary mode AND set file-ptr to end of stream
    if ( !file )
    {
        std::cout << "New file created: " << annotationFile << std::endl;
    }
    else if ( !m_parser->ParseFromIstream(&file) )
    {
        std::cerr << "Failed to parse file: " << annotationFile << std::endl;
    }
}

bool Annotations::syncToFile()
{
    fstream output(m_filename, ios::out | ios::trunc | ios::binary);
    if (!m_parser->SerializeToOstream(&output)) {
      cerr << "Failed to write address book." << endl;
      return false; 
    }
    return true;
}

bool Annotations::dumpToFile(const fs::path& filename)
{
    static std::mutex filenameLck;
    const std::lock_guard<std::mutex> lock(filenameLck);
    fstream output(filename, ios::out | ios::trunc | ios::binary);
    if ( output.is_open() )
    {
        for (auto & [key, value] : m_parser->examples())
        {
            output << key << std::endl;
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool Annotations::isFlaggedAsMisdetection(const string& filename)
{
    auto examples = m_parser->mutable_examples();
    auto it = examples->find(filename);
    return ( it != examples->end() ) ? true:false;
}

void Annotations::clearFlagMisdetectByName(const string& filename)
{
    const std::lock_guard<std::mutex> lock(m_parserLck);
    auto examples = m_parser->mutable_examples();
    auto it = examples->find(filename);
    if ( it != examples->end() )
    {
        examples->erase(it);
        syncToFile();
    }
}

void Annotations::setFlagMisdetectByName(const string& filename)
{
    const std::lock_guard<std::mutex> lock(m_parserLck);
    auto examples = m_parser->mutable_examples();
    auto it = examples->find(filename);
    if ( it == examples->end() )
    {
        // key was not found -> adding key
        annotations::Annotation::Metadata val;
        val.set_misdetection(true);
        (*examples)[filename] = val;
        syncToFile();
    }
}
