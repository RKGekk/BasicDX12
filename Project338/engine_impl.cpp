#include "engine_impl.h"

#include "application.h"
#include "command_queue.h"
#include "material.h"
#include "mesh.h"
#include "scene_node.h"
#include "scene_visitor.h"
#include "texture.h"
#include "utils.h"

#include <wrl.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <DirectXColors.h>
#include <DirectXMath.h>

#include <algorithm>
#include <memory>

DirectX::XMMATRIX XM_CALLCONV LookAtMatrix(DirectX::FXMVECTOR Position, DirectX::FXMVECTOR Direction, DirectX::FXMVECTOR Up) {
	assert(!DirectX::XMVector3Equal(Direction, DirectX::XMVectorZero()));
	assert(!DirectX::XMVector3IsInfinite(Direction));
	assert(!DirectX::XMVector3Equal(Up, DirectX::XMVectorZero()));
	assert(!DirectX::XMVector3IsInfinite(Up));

	DirectX::XMVECTOR R2 = DirectX::XMVector3Normalize(Direction);

	DirectX::XMVECTOR R0 = DirectX::XMVector3Cross(Up, R2);
	R0 = DirectX::XMVector3Normalize(R0);

	DirectX::XMVECTOR R1 = DirectX::XMVector3Cross(R2, R0);

	DirectX::XMMATRIX M(R0, R1, R2, Position);

	return M;
}

EngineImpl::EngineImpl(const std::wstring& name, int width, int height, bool vSync) :
	m_name(name),
	m_width(width),
	m_height(height),
	m_vSync(vSync),
	m_tearing_supported(false),
	m_scissor_rect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX)),
	m_viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height))),
	m_full_screen(false),
	m_allow_fullscreen_toggle(true),
	m_is_content_loaded(false) {}

EngineImpl::~EngineImpl() {}

bool EngineImpl::Initialize() {
	if (!DirectX::XMVerifyCPUSupport()) {
		MessageBoxA(NULL, "Failed to verify DirectX Math library support.", "Error", MB_OK | MB_ICONERROR);
		return false;
	}

	m_pWindow = Application::Get().CreateRenderWindow(m_name, m_width, m_height);
	m_pWindow->Update += UpdateEvent::slot(&EngineImpl::OnUpdate, this);
	m_pWindow->Resize += ResizeEvent::slot(&EngineImpl::OnResize, this);
	m_pWindow->DPIScaleChanged += DPIScaleEvent::slot(&EngineImpl::OnDPIScaleChanged, this);
	m_pWindow->KeyPressed += KeyboardEvent::slot(&EngineImpl::OnKeyPressed, this);
	m_pWindow->KeyReleased += KeyboardEvent::slot(&EngineImpl::OnKeyReleased, this);
	m_pWindow->MouseMoved += MouseMotionEvent::slot(&EngineImpl::OnMouseMoved, this);

	m_adapter_reader = std::make_shared<AdapterReader>();
	m_adapter_reader->Initialize();

	//m_tearing_supported = m_adapter_reader->CheckTearingSupport();
	m_adapter = m_adapter_reader->GetAdapter();
	m_device = Device::Create(m_adapter);

	return true;
}

void EngineImpl::ShowWindow() {
	m_pWindow->Show();
}

bool EngineImpl::LoadContent() {
	Application& app = Application::Get();

	m_swap_chain = m_device->CreateSwapChain(m_pWindow->GetWindowHandle(), DXGI_FORMAT_R8G8B8A8_UNORM);
	m_gui = m_device->CreateGUI(m_pWindow->GetWindowHandle(), m_swap_chain->GetRenderTarget());

	app.WndProcHandler += WndProcEvent::slot(&GUI::WndProcHandler, m_gui);

	CommandQueue& command_queue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	std::shared_ptr<CommandList> command_list = command_queue.GetCommandList();

	//m_scene = command_list->LoadSceneFromFile(L"feisar/feisar.obj");
	m_scene = command_list->LoadSceneFromFile(L"crate/crate.obj");

	command_queue.ExecuteCommandList(command_list);
	command_queue.Flush();

	m_sphere = command_list->CreateSphere(0.1f);
	m_cone = command_list->CreateCone(0.1f, 0.2f);

	auto fence = command_queue.ExecuteCommandList(command_list);

	const DirectX::XMVECTOR eye_position = DirectX::XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f);
	const DirectX::XMVECTOR focus_point = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	const DirectX::XMVECTOR up_direction = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	float aspect_ratio = m_width / static_cast<float>(m_height);
	m_camera.set_LookAt(eye_position, focus_point, up_direction);
	m_camera.set_Projection(45.0f, aspect_ratio, 0.1f, 100.0f);

	m_lighting_pso = std::make_shared<EffectPSO>(m_device, true, false);
	m_decal_pso = std::make_shared<EffectPSO>(m_device, true, true);
	m_unlit_pso = std::make_shared<EffectPSO>(m_device, false, false);

	const int num_directional_lights = 1;
	static const DirectX::XMVECTORF32 Light_colors[] = {
		DirectX::Colors::White,
		DirectX::Colors::OrangeRed,
		DirectX::Colors::Blue
	};
	m_directional_lights.resize(num_directional_lights);
	DirectionalLight& l = m_directional_lights[0];

	float angle_around_x = DirectX::XM_PIDIV4;
	float angle_around_y = DirectX::XM_PIDIV4;
	DirectX::XMVECTOR direction_ws = DirectX::XMVectorSetW(
		DirectX::XMVector3Normalize(
			DirectX::XMVector3Transform(
				DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
				DirectX::XMMatrixMultiply(
					DirectX::XMMatrixRotationY(angle_around_y),
					DirectX::XMMatrixRotationX(angle_around_x)
				)
			)
		),
		0.0f
	);
	DirectX::XMVECTOR direction_vs = DirectX::XMVectorSetW(
		DirectX::XMVector3Normalize(
			DirectX::XMVector3Transform(
				direction_ws,
				m_camera.get_ViewMatrix()
			)
		),
		0.0f
	);

	XMStoreFloat4(&l.DirectionWS, direction_ws);
	XMStoreFloat4(&l.DirectionVS, direction_vs);
	//XMStoreFloat4(&l.DirectionWS, DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f));
	//XMStoreFloat4(&l.DirectionVS, DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f));
	l.Color = DirectX::XMFLOAT4(Light_colors[0]);

	m_lighting_pso->SetDirectionalLights(m_directional_lights);
	m_decal_pso->SetDirectionalLights(m_directional_lights);


	DXGI_FORMAT back_buffer_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	DXGI_FORMAT depth_buffer_format = DXGI_FORMAT_D32_FLOAT;

	DXGI_SAMPLE_DESC sample_desc = m_device->GetMultisampleQualityLevels(back_buffer_format);

	auto color_desc = CD3DX12_RESOURCE_DESC::Tex2D(back_buffer_format, m_width, m_height, 1, 1, sample_desc.Count, sample_desc.Quality, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	D3D12_CLEAR_VALUE color_clear_value;
	color_clear_value.Format = color_desc.Format;
	color_clear_value.Color[0] = 0.4f;
	color_clear_value.Color[1] = 0.6f;
	color_clear_value.Color[2] = 0.9f;
	color_clear_value.Color[3] = 1.0f;

	auto color_texture = m_device->CreateTexture(color_desc, &color_clear_value);
	color_texture->SetName(L"Color Render Target");

	auto depth_desc = CD3DX12_RESOURCE_DESC::Tex2D(depth_buffer_format, m_width, m_height, 1, 1, sample_desc.Count, sample_desc.Quality, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	D3D12_CLEAR_VALUE depth_clear_value;
	depth_clear_value.Format = depth_desc.Format;
	depth_clear_value.DepthStencil = { 1.0f, 0 };

	auto depth_texture = m_device->CreateTexture(depth_desc, &depth_clear_value);
	depth_texture->SetName(L"Depth Render Target");

	m_render_target.AttachTexture(AttachmentPoint::Color0, color_texture);
	m_render_target.AttachTexture(AttachmentPoint::DepthStencil, depth_texture);

	command_queue.WaitForFenceValue(fence);

	return m_scene != nullptr;
}

void EngineImpl::OnResize(ResizeEventArgs& e) {
	if (e.Width == m_width && e.Height == m_height) return;

	m_width = std::max(1, e.Width);
	m_height = std::max(1, e.Height);

	m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));

	m_render_target.Resize(m_width, m_height);
	m_swap_chain->Resize(m_width, m_height);
}

void EngineImpl::UnloadContent() {
	m_is_content_loaded = false;
}

void EngineImpl::OnUpdate(UpdateEventArgs& e) {
	const DirectX::XMVECTOR eye_position = DirectX::XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f);
	const DirectX::XMVECTOR focus_point = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	const DirectX::XMVECTOR up_direction = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	float aspect_ratio = m_width / static_cast<float>(m_height);
	m_camera.set_LookAt(eye_position, focus_point, up_direction);
	m_camera.set_Projection(45.0f, aspect_ratio, 0.1f, 100.0f);

	m_lighting_pso->SetDirectionalLights(m_directional_lights);
	m_decal_pso->SetDirectionalLights(m_directional_lights);

	float angle = static_cast<float>(e.TotalTime * 45.0);
	const DirectX::XMVECTOR rotation_axis = DirectX::XMVectorSetW(DirectX::XMVector3Normalize(DirectX::XMVectorSet(0.0f, 1.0f, 1.0f, 0.0f)), 0.0f);
	DirectX::XMMATRIX model_matrix = DirectX::XMMatrixRotationAxis(rotation_axis, DirectX::XMConvertToRadians(angle));
	m_scene->GetRootNode()->SetLocalTransform(model_matrix);

	OnRender();
}

void EngineImpl::OnRender() {
	Application& app = Application::Get();

	m_pWindow->SetFullscreen(m_full_screen);

	auto& command_queue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto command_list = command_queue.GetCommandList();

	const auto& render_target = m_render_target;

	SceneVisitor opaque_pass(*command_list, m_camera, *m_lighting_pso, false);
	SceneVisitor transparent_pass(*command_list, m_camera, *m_decal_pso, true);
	SceneVisitor unlit_pass(*command_list, m_camera, *m_unlit_pso, false);

	{
		FLOAT clear_color[] = { 0.4f, 0.6f, 0.9f, 1.0f };

		command_list->ClearTexture(render_target.GetTexture(AttachmentPoint::Color0), clear_color);
		command_list->ClearDepthStencilTexture(render_target.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
	}

	command_list->SetViewport(m_viewport);
	command_list->SetScissorRect(m_scissor_rect);
	command_list->SetRenderTarget(m_render_target);

	m_scene->Accept(opaque_pass);
	m_scene->Accept(transparent_pass);

	MaterialProperties light_material = Material::Black;
	for (const auto& l : m_point_lights) {
		light_material.Emissive = l.Color;
		auto light_pos = XMLoadFloat4(&l.PositionWS);
		auto world_matrix = DirectX::XMMatrixTranslationFromVector(light_pos);

		m_sphere->GetRootNode()->SetLocalTransform(world_matrix);
		m_sphere->GetRootNode()->GetMesh()->GetMaterial()->SetMaterialProperties(light_material);
		m_sphere->Accept(unlit_pass);
	}

	for (const auto& l : m_spot_lights) {
		light_material.Emissive = l.Color;
		DirectX::XMVECTOR light_pos = DirectX::XMLoadFloat4(&l.PositionWS);
		DirectX::XMVECTOR light_dir = DirectX::XMLoadFloat4(&l.DirectionWS);
		DirectX::XMVECTOR up = DirectX::XMVectorSet(0, 1, 0, 0);

		auto rotation_matrix = DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(-90.0f));
		auto world_matrix = DirectX::XMMatrixMultiply(rotation_matrix, LookAtMatrix(light_pos, light_dir, up));

		m_cone->GetRootNode()->SetLocalTransform(world_matrix);
		m_cone->GetRootNode()->GetMesh()->GetMaterial()->SetMaterialProperties(light_material);
		m_cone->Accept(unlit_pass);
	}

	auto swap_chain_back_buffer = m_swap_chain->GetRenderTarget().GetTexture(AttachmentPoint::Color0);
	auto msaa_render_target = m_render_target.GetTexture(AttachmentPoint::Color0);

	command_list->ResolveSubresource(swap_chain_back_buffer, msaa_render_target);

	OnGUI(command_list, m_swap_chain->GetRenderTarget());

	command_queue.ExecuteCommandList(command_list);

	m_swap_chain->Present();
}

void EngineImpl::OnKeyPressed(KeyEventArgs& e) {
	switch (e.Key) {
		case WindowKey::Escape:
			Application::Get().Quit(0);
			break;
		case WindowKey::Enter:
			if (e.Alt) {
		case WindowKey::F11:
			m_pWindow->ToggleFullscreen();
			break;
			}
	}
}

void EngineImpl::OnKeyReleased(KeyEventArgs& e) {}

void EngineImpl::OnMouseMoved(MouseMotionEventArgs& e) {}

void EngineImpl::OnDPIScaleChanged(DPIScaleEventArgs& e) {}

void EngineImpl::OnGUI(const std::shared_ptr<CommandList>& command_list, const RenderTarget& render_target) {
	m_gui->NewFrame();

	if (ImGui::Begin("Menu")) {
		ImGui::Text("Hello World");

		ImGui::End();
	}

	m_gui->Render(command_list, render_target);
}
