#include "scene_visitor.h"

#include "effect_pso.h"
#include "camera.h"
#include "command_list.h"
#include "index_buffer.h"
#include "material.h"
#include "mesh.h"
#include "scene_node.h"

#include <DirectXMath.h>

SceneVisitor::SceneVisitor(CommandList& command_list, const Camera& camera, EffectPSO& pso, bool transparent) : m_command_list(command_list), m_camera(camera), m_lighting_pso(pso), m_transparent_pass(transparent) {}

void SceneVisitor::Visit(Scene& scene) {
    m_lighting_pso.SetViewMatrix(m_camera.get_ViewMatrix());
    m_lighting_pso.SetProjectionMatrix(m_camera.get_ProjectionMatrix());
}

void SceneVisitor::Visit(SceneNode& scene_node) {
    auto world = scene_node.GetWorldTransform();
    m_lighting_pso.SetWorldMatrix(world);
}

void SceneVisitor::Visit(Mesh& mesh) {
    auto material = mesh.GetMaterial();
    if (material->IsTransparent() == m_transparent_pass) {
        m_lighting_pso.SetMaterial(material);

        m_lighting_pso.Apply(m_command_list);
        mesh.Draw(m_command_list);
    }
}