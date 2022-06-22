#pragma once

#include <DirectXMath.h>

enum class Space {
    Local,
    World,
};

class Camera {
public:

    Camera();
    virtual ~Camera();

    void XM_CALLCONV set_LookAt(DirectX::FXMVECTOR eye, DirectX::FXMVECTOR target, DirectX::FXMVECTOR up);
    DirectX::XMMATRIX get_ViewMatrix() const;
    DirectX::XMMATRIX get_InverseViewMatrix() const;

    void set_Projection(float fovy, float aspect, float zNear, float zFar);
    DirectX::XMMATRIX get_ProjectionMatrix() const;
    DirectX::XMMATRIX get_InverseProjectionMatrix() const;

    void set_FoV(float fovy);
    float get_FoV() const;

    void XM_CALLCONV set_Translation(DirectX::FXMVECTOR translation);
    DirectX::XMVECTOR get_Translation() const;

    void XM_CALLCONV set_FocalPoint(DirectX::FXMVECTOR focalPoint);
    DirectX::XMVECTOR get_FocalPoint() const;

    void XM_CALLCONV set_Rotation(DirectX::FXMVECTOR rotation);
    DirectX::XMVECTOR get_Rotation() const;

    void XM_CALLCONV Translate(DirectX::FXMVECTOR translation, Space space = Space::Local);
    void Rotate(DirectX::FXMVECTOR quaternion);
    void XM_CALLCONV MoveFocalPoint(DirectX::FXMVECTOR focal_point, Space space = Space::Local);

protected:
    virtual void UpdateViewMatrix() const;
    virtual void UpdateInverseViewMatrix() const;
    virtual void UpdateProjectionMatrix() const;
    virtual void UpdateInverseProjectionMatrix() const;

    __declspec(align(16)) struct AlignedData {
        DirectX::XMVECTOR m_Translation;
        DirectX::XMVECTOR m_Rotation;
        DirectX::XMVECTOR m_FocalPoint;
        DirectX::XMMATRIX m_ViewMatrix, m_InverseViewMatrix;
        DirectX::XMMATRIX m_ProjectionMatrix, m_InverseProjectionMatrix;
    };
    AlignedData* pData;

    float m_vFoV;
    float m_AspectRatio;
    float m_zNear;
    float m_zFar;

    mutable bool m_ViewDirty;
    mutable bool m_InverseViewDirty;
    mutable bool m_ProjectionDirty;
    mutable bool m_InverseProjectionDirty;
};