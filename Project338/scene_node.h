#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <DirectXMath.h>
#include <DirectXCollision.h>

class Mesh;
class CommandList;
class Visitor;

class SceneNode : public std::enable_shared_from_this<SceneNode> {
public:
	explicit SceneNode(const DirectX::XMMATRIX& local_transform = DirectX::XMMatrixIdentity());
	virtual ~SceneNode();

	const std::string& GetName() const;
	void SetName(const std::string& name);

	DirectX::XMMATRIX GetLocalTransform() const;
	void SetLocalTransform(const DirectX::XMMATRIX& local_transform);
	DirectX::XMMATRIX GetInverseLocalTransform() const;

	DirectX::XMMATRIX GetWorldTransform() const;
	DirectX::XMMATRIX GetInverseWorldTransform() const;

	void AddChild(std::shared_ptr<SceneNode> child_node);
	void RemoveChild(std::shared_ptr<SceneNode> child_node);
	void SetParent(std::shared_ptr<SceneNode> parent_node);

	size_t AddMesh(std::shared_ptr<Mesh> mesh);
	void RemoveMesh(std::shared_ptr<Mesh> mesh);

	std::shared_ptr<Mesh> GetMesh(size_t index = 0);

	const DirectX::BoundingBox& GetAABB() const;

	void Accept(Visitor& visitor);

	DirectX::XMMATRIX GetParentWorldTransform() const;

private:
	using NodePtr = std::shared_ptr<SceneNode>;
	using NodeList = std::vector<NodePtr>;
	using NodeNameMap = std::multimap<std::string, NodePtr>;
	using MeshList = std::vector<std::shared_ptr<Mesh>>;

	std::string m_name;

	struct alignas(16) AlignedData {
		DirectX::XMMATRIX m_local_transform;
		DirectX::XMMATRIX m_inverse_transform;
	}* m_aligned_data;

	std::weak_ptr<SceneNode> m_parent_node;
	NodeList m_children;
	NodeNameMap m_children_by_name;
	MeshList m_meshes;

	DirectX::BoundingBox m_AABB;
};