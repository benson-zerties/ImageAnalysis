#include <fstream>
#include <filesystem>

#include "annotations.pb.h"

namespace fs = std::filesystem;
using namespace std;

class Annotations {
    typedef annotations::Annotation Parser;

    public:
        Annotations(fs::path annotationFile);
        ~Annotations(){};
        bool isFlaggedAsMisdetection(const string& filename);
        void setFlagMisdetectByName(const string& filename);
        void clearFlagMisdetectByName(const string& filename);
        bool dumpToFile(const fs::path& filename);

    private:
        bool syncToFile();
        
        fs::path m_filename;
        unique_ptr<Parser> m_parser;
        std::mutex m_parserLck;
};
