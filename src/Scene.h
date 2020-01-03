#pragma once
class Scene
{
public:
	Scene();
	~Scene();
	virtual void update(float deltaTime);
	virtual void variableUpdate(float deltaTime);
	virtual void render();
};

