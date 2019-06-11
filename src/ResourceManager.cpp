#include "ResourceManager.h"

ResourceManager *ResourceManager::self = 0;

ResourceManager* ResourceManager::getSelf(){
	if (self == 0)
		self = new ResourceManager();

	return self;
}

ResourceManager::ResourceManager() {
}


ResourceManager::~ResourceManager() {
}


