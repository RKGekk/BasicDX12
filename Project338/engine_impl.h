#pragma once

#include <vector>

#include <d3d12.h>
#include <DirectXMath.h>
#include <memory>
#include <wrl.h>

#include "adapter_reader.h"
#include "camera.h"
#include "command_list.h"
#include "device.h"
#include "effect_pso.h"
#include "events.h"
#include "gui.h"
#include "light.h"
#include "pipeline_state_object.h"
#include "render_target.h"
#include "root_signature.h"
#include "scene.h"
#include "swap_chain.h"
#include "window_surface.h"

class EngineImpl{
public:
    EngineImpl(const std::wstring& name, int width, int height, bool vSync = true);
    ~EngineImpl();

    bool Initialize();
    void ShowWindow();
    virtual bool LoadContent();
    virtual void UnloadContent();

    void OnUpdate(UpdateEventArgs& e);
    void OnResize(ResizeEventArgs& e);
    void OnRender();
    void OnKeyPressed(KeyEventArgs& e);
    void OnKeyReleased(KeyEventArgs& e);
    void OnMouseMoved(MouseMotionEventArgs& e);
    void OnDPIScaleChanged(DPIScaleEventArgs& e);

    void OnGUI(const std::shared_ptr<CommandList>& commandList, const RenderTarget& renderTarget);

private:
    std::shared_ptr<WindowSurface> m_pWindow;
    std::shared_ptr<AdapterReader> m_adapter_reader;
    AdapterData::AdapterDataPtr m_adapter;

    std::wstring m_name;
    int m_width;
    int m_height;
    bool m_vSync;
    bool m_tearing_supported;
    bool m_full_screen;
    bool m_allow_fullscreen_toggle;

    std::shared_ptr<Device> m_device;
    std::shared_ptr<SwapChain> m_swap_chain;
    std::shared_ptr<GUI> m_gui;

    std::shared_ptr<Scene> m_scene;

    std::shared_ptr<Scene> m_sphere;
    std::shared_ptr<Scene> m_cone;
    std::shared_ptr<Scene> m_axis;

    std::shared_ptr<EffectPSO> m_lighting_pso;
    std::shared_ptr<EffectPSO> m_decal_pso;
    std::shared_ptr<EffectPSO> m_unlit_pso;

    RenderTarget m_render_target;

    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissor_rect;

    Camera m_camera;

    bool m_is_content_loaded;

    std::vector<PointLight> m_point_lights;
    std::vector<SpotLight>  m_spot_lights;
    std::vector<DirectionalLight> m_directional_lights;
};