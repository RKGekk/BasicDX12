#pragma once

#include "utils.h"

#include <DirectXCollision.h>

#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/ProgressHandler.hpp>
#include <assimp/anim.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>

struct aiMaterial;
struct aiMesh;
struct aiNode;
struct aiScene;
class CommandList;
class Device;
class SceneNode;
class Mesh;
class Material;
class Visitor;

class Scene {
public:
	Scene() = default;
	~Scene() = default;

	void SetRootNode(std::shared_ptr<SceneNode> node);
	std::shared_ptr<SceneNode> GetRootNode() const;

	DirectX::BoundingBox GetAABB() const;

	virtual void Accept(Visitor& visitor);

	friend class CommandList;

	bool LoadSceneFromFile(CommandList& command_list, const std::wstring& file_name, const std::function<bool(float)>& loading_progress);
	bool LoadSceneFromString(CommandList& command_list, const std::string& scene_str, const std::string& format);

private:
	void ImportScene(CommandList& command_list, const aiScene& scene, std::filesystem::path parent_path);
	void ImportMaterial(CommandList& command_list, const aiMaterial& material, std::filesystem::path parent_path);
	void ImportMesh(CommandList& command_list, const aiMesh& mesh);
	std::shared_ptr<SceneNode> ImportSceneNode(CommandList& command_list, std::shared_ptr<SceneNode> parent, const aiNode* aiNode);

	using MaterialMap = std::map<std::string, std::shared_ptr<Material>>;
	using MaterialList = std::vector<std::shared_ptr<Material>>;
	using MeshList = std::vector<std::shared_ptr<Mesh>>;

	MaterialMap m_material_map;
	MaterialList m_materials;
	MeshList m_meshes;

	std::shared_ptr<SceneNode> m_root_node;

	std::wstring m_scene_file;
};
