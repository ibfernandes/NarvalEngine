#pragma once
class Scene
{
public:
	Scene();
	~Scene();
	bool shouldLoad = true;
	virtual void load();
	virtual void update(float deltaTime);
	virtual void variableUpdate(float deltaTime);
	virtual void render();
};

