#pragma once

class Scene;
class SceneNode;
class Mesh;

class Visitor {
public:
	Visitor() = default;
	virtual ~Visitor() = default;

	virtual void Visit(Scene& scene) = 0;
	virtual void Visit(SceneNode& scene_node) = 0;
	virtual void Visit(Mesh& mesh) = 0;
};