#include "Engine3D.h"
#include <GLFW/glfw3.h>

#define TINYVDBIO_USE_SYSTEM_ZLIB
#include <zlib.h>

//#define TINYVDBIO_IMPLEMENTATION
//#include "tinyvdbio.h"

Engine3D::Engine3D(){
	this->startTime = glfwGetTime();
	this->previousUpdateTime = startTime;
	
	/*tinyvdb::VDBHeader header;
	std::string warn;
	std::string err;
	std::string path = RESOURCES_DIR "wdas_cloud_eighth.vdb";
	tinyvdb::VDBStatus status = tinyvdb::ParseVDBHeader(path, &header, &err);

	// 2. Read Grid descriptors
	std::map<std::string, tinyvdb::GridDescriptor> gd_map;

	status = tinyvdb::ReadGridDescriptors(path, header, &gd_map, &err);
	if (status != tinyvdb::TINYVDBIO_SUCCESS) {
		if (!err.empty()) {
			std::cerr << err << std::endl;
		}
	}

	// 3. Read Grids
	status = tinyvdb::ReadGrids(path, header, gd_map, &warn, &err);
	if (!warn.empty()) {
		std::cout << warn << std::endl;
	}
	if (status != tinyvdb::TINYVDBIO_SUCCESS) {
		if (!err.empty()) {
			std::cerr << err << std::endl;
		}
	}



	std::map<std::string, tinyvdb::GridDescriptor>::iterator it = gd_map.begin();

	tinyvdb::GridDescriptor vl = gd_map.at("density");
	std::cout << "=======================" << std::endl;
	*/


}



Engine3D::~Engine3D(){
}
