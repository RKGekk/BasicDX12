#pragma once

#include "visitor.h"

class Camera;
class EffectPSO;
class CommandList;

class SceneVisitor : public Visitor {
public:
    SceneVisitor(CommandList& commandList, const Camera& camera, EffectPSO& pso, bool transparent);

    virtual void Visit(Scene& scene) override;
    virtual void Visit(SceneNode& scene_node) override;
    virtual void Visit(Mesh& mesh) override;

private:
    CommandList& m_command_list;
    const Camera& m_camera;
    EffectPSO& m_lighting_pso;
    bool m_transparent_pass;
};