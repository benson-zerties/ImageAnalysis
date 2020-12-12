#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include <memory>

using namespace std;
namespace fs = std::filesystem;

class Controller
{
	public:
        explicit Controller();
        void setModel(unique_ptr<Model> model);

	protected:

	private:
};

#endif /* _CONTROLLER_H_ */
