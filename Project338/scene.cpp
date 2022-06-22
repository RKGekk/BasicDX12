#include "scene.h"

#include "command_list.h"
#include "device.h"
#include "material.h"
#include "mesh.h"
#include "scene_node.h"
#include "texture.h"
#include "vertex_types.h"
#include "visitor.h"

#include <assimp/config.h>
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/ProgressHandler.hpp>
#include <assimp/anim.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>


void Scene::SetRootNode(std::shared_ptr<SceneNode> node) {
	m_root_node = node;
}

std::shared_ptr<SceneNode> Scene::GetRootNode() const {
	return m_root_node;
}

class ProgressHandler : public Assimp::ProgressHandler {
public:
    ProgressHandler(const Scene& scene, const std::function<bool(float)> progress_callback) : m_scene(scene), m_progress_callback(progress_callback) {}

    virtual bool Update(float percentage) override {
        if (m_progress_callback) {
            return m_progress_callback(percentage);
        }

        return true;
    }

private:
    const Scene& m_scene;
    std::function<bool(float)> m_progress_callback;
};

inline DirectX::BoundingBox CreateBoundingBox(const aiAABB& aabb) {
    DirectX::XMVECTOR min = DirectX::XMVectorSet(aabb.mMin.x, aabb.mMin.y, aabb.mMin.z, 1.0f);
    DirectX::XMVECTOR max = DirectX::XMVectorSet(aabb.mMax.x, aabb.mMax.y, aabb.mMax.z, 1.0f);

    DirectX::BoundingBox bb;
    DirectX::BoundingBox::CreateFromPoints(bb, min, max);

    return bb;
}

bool Scene::LoadSceneFromFile(CommandList& command_list, const std::wstring& file_name, const std::function<bool(float)>& loading_progress) {
    std::filesystem::path file_path = file_name;
    std::filesystem::path export_path = std::filesystem::path(file_path).replace_extension("assbin");

    std::filesystem::path parent_path;
    if (file_path.has_parent_path()) {
        parent_path = file_path.parent_path();
    }
    else {
        parent_path = std::filesystem::current_path();
    }

    Assimp::Importer importer;
    const aiScene* scene;

    importer.SetProgressHandler(new ProgressHandler(*this, loading_progress));

    if (std::filesystem::exists(export_path) && std::filesystem::is_regular_file(export_path)) {
        scene = importer.ReadFile(export_path.string(), aiProcess_GenBoundingBoxes);
    }
    else {
        importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80.0f);
        importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);

        unsigned int preprocess_flags = aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_OptimizeGraph | aiProcess_ConvertToLeftHanded | aiProcess_GenBoundingBoxes;
        scene = importer.ReadFile(file_path.string(), preprocess_flags);

        if (scene) {
            Assimp::Exporter exporter;
            exporter.Export(scene, "assbin", export_path.string(), 0);
        }
    }

    if (!scene) {
        return false;
    }

    ImportScene(command_list, *scene, parent_path);

    return true;
}

bool Scene::LoadSceneFromString(CommandList& command_list, const std::string& scene_str, const std::string& format) {
    Assimp::Importer importer;
    const aiScene* scene = nullptr;

    importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80.0f);
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);

    unsigned int preprocess_flags = aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded | aiProcess_GenBoundingBoxes;

    scene = importer.ReadFileFromMemory(scene_str.data(), scene_str.size(), preprocess_flags, format.c_str());

    if (!scene) {
        return false;
    }

    ImportScene(command_list, *scene, std::filesystem::current_path());

    return true;
}

void Scene::ImportScene(CommandList& command_list, const aiScene& scene, std::filesystem::path parent_path) {

    if (m_root_node) {
        m_root_node.reset();
    }

    m_material_map.clear();
    m_materials.clear();
    m_meshes.clear();

    for (unsigned int i = 0u; i < scene.mNumMaterials; ++i) {
        ImportMaterial(command_list, *(scene.mMaterials[i]), parent_path);
    }
    
    for (unsigned int i = 0; i < scene.mNumMeshes; ++i) {
        ImportMesh(command_list, *(scene.mMeshes[i]));
    }

    m_root_node = ImportSceneNode(command_list, nullptr, scene.mRootNode);
}

void Scene::ImportMaterial(CommandList& command_list, const aiMaterial& material, std::filesystem::path parent_path) {
    aiString material_name;
    aiString aiTexture_path;
    aiTextureOp aiBlend_operation;
    float blend_factor;
    aiColor4D diffuse_color;
    aiColor4D specular_color;
    aiColor4D ambient_color;
    aiColor4D emissive_color;
    float opacity;
    float index_of_refraction;
    float reflectivity;
    float shininess;
    float bump_intensity;

    std::shared_ptr<Material> pMaterial = std::make_shared<Material>();

    if (material.Get(AI_MATKEY_COLOR_AMBIENT, ambient_color) == aiReturn_SUCCESS) {
        pMaterial->SetAmbientColor(DirectX::XMFLOAT4(ambient_color.r, ambient_color.g, ambient_color.b, ambient_color.a));
    }
    if (material.Get(AI_MATKEY_COLOR_EMISSIVE, emissive_color) == aiReturn_SUCCESS) {
        pMaterial->SetEmissiveColor(DirectX::XMFLOAT4(emissive_color.r, emissive_color.g, emissive_color.b, emissive_color.a));
    }
    if (material.Get(AI_MATKEY_COLOR_DIFFUSE, diffuse_color) == aiReturn_SUCCESS) {
        pMaterial->SetDiffuseColor(DirectX::XMFLOAT4(diffuse_color.r, diffuse_color.g, diffuse_color.b, diffuse_color.a));
    }
    if (material.Get(AI_MATKEY_COLOR_SPECULAR, specular_color) == aiReturn_SUCCESS) {
        pMaterial->SetSpecularColor(DirectX::XMFLOAT4(specular_color.r, specular_color.g, specular_color.b, specular_color.a));
    }
    if (material.Get(AI_MATKEY_SHININESS, shininess) == aiReturn_SUCCESS) {
        pMaterial->SetSpecularPower(shininess);
    }
    if (material.Get(AI_MATKEY_OPACITY, opacity) == aiReturn_SUCCESS) {
        pMaterial->SetOpacity(opacity);
    }
    if (material.Get(AI_MATKEY_REFRACTI, index_of_refraction)) {
        pMaterial->SetIndexOfRefraction(index_of_refraction);
    }
    if (material.Get(AI_MATKEY_REFLECTIVITY, reflectivity) == aiReturn_SUCCESS) {
        pMaterial->SetReflectance(DirectX::XMFLOAT4(reflectivity, reflectivity, reflectivity, reflectivity));
    }
    if (material.Get(AI_MATKEY_BUMPSCALING, bump_intensity) == aiReturn_SUCCESS) {
        pMaterial->SetBumpIntensity(bump_intensity);
    }

    if (material.GetTextureCount(aiTextureType_AMBIENT) > 0 && material.GetTexture(aiTextureType_AMBIENT, 0, &aiTexture_path, nullptr, nullptr, &blend_factor, &aiBlend_operation) == aiReturn_SUCCESS) {
        std::filesystem::path texture_path(aiTexture_path.C_Str());
        auto texture = command_list.LoadTextureFromFile(parent_path / texture_path, true);
        pMaterial->SetTexture(Material::TextureType::Ambient, texture);
    }

    if (material.GetTextureCount(aiTextureType_EMISSIVE) > 0 && material.GetTexture(aiTextureType_EMISSIVE, 0, &aiTexture_path, nullptr, nullptr, &blend_factor, &aiBlend_operation) == aiReturn_SUCCESS) {
        std::filesystem::path texture_path(aiTexture_path.C_Str());
        auto texture = command_list.LoadTextureFromFile(parent_path / texture_path, true);
        pMaterial->SetTexture(Material::TextureType::Emissive, texture);
    }

    if (material.GetTextureCount(aiTextureType_DIFFUSE) > 0 && material.GetTexture(aiTextureType_DIFFUSE, 0, &aiTexture_path, nullptr, nullptr, &blend_factor, &aiBlend_operation) == aiReturn_SUCCESS) {
        std::filesystem::path texture_path(aiTexture_path.C_Str());
        auto texture = command_list.LoadTextureFromFile(parent_path / texture_path, true);
        pMaterial->SetTexture(Material::TextureType::Diffuse, texture);
    }

    if (material.GetTextureCount(aiTextureType_SPECULAR) > 0 && material.GetTexture(aiTextureType_SPECULAR, 0, &aiTexture_path, nullptr, nullptr, &blend_factor, &aiBlend_operation) == aiReturn_SUCCESS) {
        std::filesystem::path texture_path(aiTexture_path.C_Str());
        auto texture = command_list.LoadTextureFromFile(parent_path / texture_path, true);
        pMaterial->SetTexture(Material::TextureType::Specular, texture);
    }

    if (material.GetTextureCount(aiTextureType_SHININESS) > 0 && material.GetTexture(aiTextureType_SHININESS, 0, &aiTexture_path, nullptr, nullptr, &blend_factor, &aiBlend_operation) == aiReturn_SUCCESS) {
        std::filesystem::path texture_path(aiTexture_path.C_Str());
        auto texture = command_list.LoadTextureFromFile(parent_path / texture_path, false);
        pMaterial->SetTexture(Material::TextureType::SpecularPower, texture);
    }

    if (material.GetTextureCount(aiTextureType_OPACITY) > 0 && material.GetTexture(aiTextureType_OPACITY, 0, &aiTexture_path, nullptr, nullptr, &blend_factor, &aiBlend_operation) == aiReturn_SUCCESS) {
        std::filesystem::path texture_path(aiTexture_path.C_Str());
        auto texture = command_list.LoadTextureFromFile(parent_path / texture_path, false);
        pMaterial->SetTexture(Material::TextureType::Opacity, texture);
    }

    if (material.GetTextureCount(aiTextureType_NORMALS) > 0 && material.GetTexture(aiTextureType_NORMALS, 0, &aiTexture_path) == aiReturn_SUCCESS) {
        std::filesystem::path texture_path(aiTexture_path.C_Str());
        auto texture = command_list.LoadTextureFromFile(parent_path / texture_path, false);
        pMaterial->SetTexture(Material::TextureType::Normal, texture);
    }
    else if (material.GetTextureCount(aiTextureType_HEIGHT) > 0 && material.GetTexture(aiTextureType_HEIGHT, 0, &aiTexture_path, nullptr, nullptr, &blend_factor) == aiReturn_SUCCESS) {
        std::filesystem::path texture_path(aiTexture_path.C_Str());
        auto texture = command_list.LoadTextureFromFile(parent_path / texture_path, false);

        Material::TextureType texture_type = (texture->BitsPerPixel() >= 24) ? Material::TextureType::Normal : Material::TextureType::Bump;

        pMaterial->SetTexture(texture_type, texture);
    }

    m_materials.push_back(pMaterial);
}

void Scene::ImportMesh(CommandList& command_list, const aiMesh& aiMesh) {
    auto mesh = std::make_shared<Mesh>();

    std::vector<VertexPositionNormalTangentBitangentTexture> vertex_data(aiMesh.mNumVertices);

    assert(aiMesh.mMaterialIndex < m_materials.size());
    mesh->SetMaterial(m_materials[aiMesh.mMaterialIndex]);

    unsigned int i;
    if (aiMesh.HasPositions()) {
        for (i = 0u; i < aiMesh.mNumVertices; ++i) {
            vertex_data[i].Position = { aiMesh.mVertices[i].x, aiMesh.mVertices[i].y, aiMesh.mVertices[i].z };
        }
    }

    if (aiMesh.HasNormals()) {
        for (i = 0; i < aiMesh.mNumVertices; ++i) {
            vertex_data[i].Normal = { aiMesh.mNormals[i].x, aiMesh.mNormals[i].y, aiMesh.mNormals[i].z };
        }
    }

    if (aiMesh.HasTangentsAndBitangents()) {
        for (i = 0; i < aiMesh.mNumVertices; ++i) {
            vertex_data[i].Tangent = { aiMesh.mTangents[i].x, aiMesh.mTangents[i].y, aiMesh.mTangents[i].z };
            vertex_data[i].Bitangent = { aiMesh.mBitangents[i].x, aiMesh.mBitangents[i].y, aiMesh.mBitangents[i].z };
        }
    }

    if (aiMesh.HasTextureCoords(0)) {
        for (i = 0; i < aiMesh.mNumVertices; ++i) {
            vertex_data[i].TexCoord = { aiMesh.mTextureCoords[0][i].x, aiMesh.mTextureCoords[0][i].y, aiMesh.mTextureCoords[0][i].z };
        }
    }

    auto vertex_buffer = command_list.CopyVertexBuffer(vertex_data);
    mesh->SetVertexBuffer(0, vertex_buffer);

    if (aiMesh.HasFaces()) {
        std::vector<unsigned int> indices;
        for (i = 0; i < aiMesh.mNumFaces; ++i) {
            const aiFace& face = aiMesh.mFaces[i];

            if (face.mNumIndices == 3) {
                indices.push_back(face.mIndices[0]);
                indices.push_back(face.mIndices[1]);
                indices.push_back(face.mIndices[2]);
            }
        }

        if (indices.size() > 0) {
            auto indexBuffer = command_list.CopyIndexBuffer(indices);
            mesh->SetIndexBuffer(indexBuffer);
        }
    }

    mesh->SetAABB(CreateBoundingBox(aiMesh.mAABB));

    m_meshes.push_back(mesh);
}

std::shared_ptr<SceneNode> Scene::ImportSceneNode(CommandList& command_list, std::shared_ptr<SceneNode> parent, const aiNode* aiNode) {
    if (!aiNode) {
        return nullptr;
    }

    auto node = std::make_shared<SceneNode>(DirectX::XMMATRIX(&(aiNode->mTransformation.a1)));
    node->SetParent(parent);

    if (aiNode->mName.length > 0) {
        node->SetName(aiNode->mName.C_Str());
    }

    for (unsigned int i = 0; i < aiNode->mNumMeshes; ++i) {
        assert(aiNode->mMeshes[i] < m_meshes.size());

        std::shared_ptr<Mesh> pMesh = m_meshes[aiNode->mMeshes[i]];
        node->AddMesh(pMesh);
    }

    for (unsigned int i = 0; i < aiNode->mNumChildren; ++i) {
        auto child = ImportSceneNode(command_list, node, aiNode->mChildren[i]);
        node->AddChild(child);
    }

    return node;
}

void Scene::Accept(Visitor& visitor) {
    visitor.Visit(*this);
    if (m_root_node) {
        m_root_node->Accept(visitor);
    }
}

DirectX::BoundingBox Scene::GetAABB() const {
    DirectX::BoundingBox aabb{ { 0, 0, 0 }, { 0, 0, 0 } };

    if (m_root_node) {
        aabb = m_root_node->GetAABB();
    }

    return aabb;
}