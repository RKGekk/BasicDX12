#include "scene_node.h"

#include "mesh.h"
#include "visitor.h"

#include <cstdlib>

SceneNode::SceneNode(const DirectX::XMMATRIX& local_transform) : m_name("SceneNode"), m_AABB({ 0, 0, 0 }, { 0, 0, 0 }) {
    m_aligned_data = (AlignedData*)_aligned_malloc(sizeof(AlignedData), 16);
    m_aligned_data->m_local_transform = local_transform;
    m_aligned_data->m_inverse_transform = DirectX::XMMatrixInverse(nullptr, local_transform);
}

SceneNode::~SceneNode() {
    _aligned_free(m_aligned_data);
}

const std::string& SceneNode::GetName() const {
    return m_name;
}

void SceneNode::SetName(const std::string& name) {
    m_name = name;
}

DirectX::XMMATRIX SceneNode::GetLocalTransform() const {
    return m_aligned_data->m_local_transform;
}

void SceneNode::SetLocalTransform(const DirectX::XMMATRIX& local_transform) {
    m_aligned_data->m_local_transform = local_transform;
    m_aligned_data->m_inverse_transform = DirectX::XMMatrixInverse(nullptr, local_transform);
}

DirectX::XMMATRIX SceneNode::GetInverseLocalTransform() const {
    return m_aligned_data->m_inverse_transform;
}

DirectX::XMMATRIX SceneNode::GetWorldTransform() const {
    return m_aligned_data->m_local_transform* GetParentWorldTransform();
}

DirectX::XMMATRIX SceneNode::GetInverseWorldTransform() const {
    return DirectX::XMMatrixInverse(nullptr, GetWorldTransform());
}

DirectX::XMMATRIX SceneNode::GetParentWorldTransform() const {
    DirectX::XMMATRIX parent_transform = DirectX::XMMatrixIdentity();
    if (auto parent_node = m_parent_node.lock()) {
        parent_transform = parent_node->GetWorldTransform();
    }

    return parent_transform;
}

void SceneNode::AddChild(std::shared_ptr<SceneNode> child_node) {
    if (child_node) {
        NodeList::iterator iter = std::find(m_children.begin(), m_children.end(), child_node);
        if (iter == m_children.end()) {
            DirectX::XMMATRIX world_transform = child_node->GetWorldTransform();
            child_node->m_parent_node = shared_from_this();
            DirectX::XMMATRIX local_transform = world_transform * GetInverseWorldTransform();
            child_node->SetLocalTransform(local_transform);
            m_children.push_back(child_node);
            if (!child_node->GetName().empty()) {
                m_children_by_name.emplace(child_node->GetName(), child_node);
            }
        }
    }
}

void SceneNode::RemoveChild(std::shared_ptr<SceneNode> child_node) {
    if (child_node) {
        NodeList::const_iterator iter = std::find(m_children.begin(), m_children.end(), child_node);
        if (iter != m_children.cend()) {
            child_node->SetParent(nullptr);
            m_children.erase(iter);

            NodeNameMap::iterator iter2 = m_children_by_name.find(child_node->GetName());
            if (iter2 != m_children_by_name.end()) {
                m_children_by_name.erase(iter2);
            }
        }
        else {
            for (auto child : m_children) {
                child->RemoveChild(child_node);
            }
        }
    }
}

void SceneNode::SetParent(std::shared_ptr<SceneNode> parent_node) {
    std::shared_ptr<SceneNode> me = shared_from_this();
    if (parent_node) {
        parent_node->AddChild(me);
    }
    else if (auto parent = m_parent_node.lock()) {
        auto world_transform = GetWorldTransform();
        parent->RemoveChild(me);
        m_parent_node.reset();
        SetLocalTransform(world_transform);
    }
}

size_t SceneNode::AddMesh(std::shared_ptr<Mesh> mesh) {
    size_t index = (size_t)-1;
    if (mesh) {
        MeshList::const_iterator iter = std::find(m_meshes.begin(), m_meshes.end(), mesh);
        if (iter == m_meshes.cend()) {
            index = m_meshes.size();
            m_meshes.push_back(mesh);

            DirectX::BoundingBox::CreateMerged(m_AABB, m_AABB, mesh->GetAABB());
        }
        else {
            index = iter - m_meshes.begin();
        }
    }

    return index;
}

void SceneNode::RemoveMesh(std::shared_ptr<Mesh> mesh) {
    if (mesh) {
        MeshList::const_iterator iter = std::find(m_meshes.begin(), m_meshes.end(), mesh);
        if (iter != m_meshes.end()) {
            m_meshes.erase(iter);
        }
    }
}

std::shared_ptr<Mesh> SceneNode::GetMesh(size_t pos) {
    std::shared_ptr<Mesh> mesh = nullptr;

    if (pos < m_meshes.size()) {
        mesh = m_meshes[pos];
    }

    return mesh;
}

const DirectX::BoundingBox& SceneNode::GetAABB() const {
    return m_AABB;
}

void SceneNode::Accept(Visitor& visitor) {
    visitor.Visit(*this);

    for (auto& mesh : m_meshes) {
        mesh->Accept(visitor);
    }

    for (auto& child : m_children) {
        child->Accept(visitor);
    }
}