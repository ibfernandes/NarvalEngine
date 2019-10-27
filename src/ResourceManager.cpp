#include "ResourceManager.h"

ResourceManager *ResourceManager::self = 0;

ResourceManager* ResourceManager::getSelf(){
	if (self == 0) {
		openvdb::initialize();
		self = new ResourceManager();
	}

	return self;
}

ResourceManager::ResourceManager() {
}


ResourceManager::~ResourceManager() {
}


