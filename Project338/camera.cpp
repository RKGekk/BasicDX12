#include "camera.h"

Camera::Camera() : m_ViewDirty(true), m_InverseViewDirty(true), m_ProjectionDirty(true), m_InverseProjectionDirty(true), m_vFoV(45.0f), m_AspectRatio(1.0f), m_zNear(0.1f), m_zFar(100.0f) {
    pData = (AlignedData*)_aligned_malloc(sizeof(AlignedData), 16);
    pData->m_Translation = DirectX::XMVectorZero();
    pData->m_Rotation = DirectX::XMQuaternionIdentity();
    pData->m_FocalPoint = DirectX::XMVectorZero();
}

Camera::~Camera() {
    _aligned_free(pData);
}

void XM_CALLCONV Camera::set_LookAt(DirectX::FXMVECTOR eye, DirectX::FXMVECTOR target, DirectX::FXMVECTOR up) {
    pData->m_ViewMatrix = DirectX::XMMatrixLookAtLH(eye, target, up);

    pData->m_Translation = eye;
    pData->m_Rotation = XMQuaternionRotationMatrix(XMMatrixTranspose(pData->m_ViewMatrix));

    m_InverseViewDirty = true;
    m_ViewDirty = false;
}

DirectX::XMMATRIX Camera::get_ViewMatrix() const {
    if (m_ViewDirty) {
        UpdateViewMatrix();
    }
    return pData->m_ViewMatrix;
}

DirectX::XMMATRIX Camera::get_InverseViewMatrix() const {
    if (m_ViewDirty || m_InverseViewDirty) {
        pData->m_InverseViewMatrix = DirectX::XMMatrixInverse(nullptr, get_ViewMatrix());
        m_InverseViewDirty = false;
    }

    return pData->m_InverseViewMatrix;
}

void Camera::set_Projection(float fovy, float aspect, float zNear, float zFar) {
    m_vFoV = fovy;
    m_AspectRatio = aspect;
    m_zNear = zNear;
    m_zFar = zFar;

    m_ProjectionDirty = true;
    m_InverseProjectionDirty = true;
}

DirectX::XMMATRIX Camera::get_ProjectionMatrix() const {
    if (m_ProjectionDirty) {
        UpdateProjectionMatrix();
    }

    return pData->m_ProjectionMatrix;
}

DirectX::XMMATRIX Camera::get_InverseProjectionMatrix() const {
    if (m_ProjectionDirty || m_InverseProjectionDirty) {
        UpdateInverseProjectionMatrix();
    }

    return pData->m_InverseProjectionMatrix;
}

void Camera::set_FoV(float fovy) {
    if (m_vFoV != fovy) {
        m_vFoV = fovy;
        m_ProjectionDirty = true;
        m_InverseProjectionDirty = true;
    }
}

float Camera::get_FoV() const {
    return m_vFoV;
}

void XM_CALLCONV Camera::set_Translation(DirectX::FXMVECTOR translation) {
    pData->m_Translation = translation;
    m_ViewDirty = true;
}

DirectX::XMVECTOR Camera::get_Translation() const {
    return pData->m_Translation;
}

void XM_CALLCONV Camera::set_FocalPoint(DirectX::FXMVECTOR focalPoint) {
    pData->m_FocalPoint = focalPoint;
    m_ViewDirty = true;
}

DirectX::XMVECTOR Camera::get_FocalPoint() const {
    return pData->m_FocalPoint;
}

void Camera::set_Rotation(DirectX::FXMVECTOR rotation) {
    pData->m_Rotation = rotation;
    m_ViewDirty = true;
}

DirectX::XMVECTOR Camera::get_Rotation() const {
    return pData->m_Rotation;
}

void XM_CALLCONV Camera::Translate(DirectX::FXMVECTOR translation, Space space) {
    using namespace DirectX;
    switch (space) {
        case Space::Local:
        {
            pData->m_Translation += DirectX::XMVector3Rotate(translation, pData->m_Rotation);
        }
        break;
        case Space::World:
        {
            pData->m_Translation += translation;
        }
        break;
    }

    pData->m_Translation = DirectX::XMVectorSetW(pData->m_Translation, 1.0f);

    m_ViewDirty = true;
}

void XM_CALLCONV Camera::MoveFocalPoint(DirectX::FXMVECTOR focalPoint, Space space) {
    using namespace DirectX;
    switch (space) {
        case Space::Local:
        {
            pData->m_FocalPoint += DirectX::XMVector3Rotate(focalPoint, pData->m_Rotation);
        }
        break;
        case Space::World:
        {
            pData->m_FocalPoint += focalPoint;
        }
        break;
    }

    pData->m_FocalPoint = DirectX::XMVectorSetW(pData->m_FocalPoint, 1.0f);

    m_ViewDirty = true;
}


void Camera::Rotate(DirectX::FXMVECTOR quaternion) {
    pData->m_Rotation = DirectX::XMQuaternionMultiply(quaternion, pData->m_Rotation);

    m_ViewDirty = true;
}

void Camera::UpdateViewMatrix() const {
    using namespace DirectX;
    DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationQuaternion(pData->m_Rotation));
    DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslationFromVector(-(pData->m_Translation));
    DirectX::XMMATRIX focalMatrix = DirectX::XMMatrixTranslationFromVector(-(pData->m_FocalPoint));

    pData->m_ViewMatrix = focalMatrix * rotationMatrix * translationMatrix;

    m_InverseViewDirty = true;
    m_ViewDirty = false;
}

void Camera::UpdateInverseViewMatrix() const {
    if (m_ViewDirty) {
        UpdateViewMatrix();
    }

    pData->m_InverseViewMatrix = DirectX::XMMatrixInverse(nullptr, pData->m_ViewMatrix);
    m_InverseViewDirty = false;
}

void Camera::UpdateProjectionMatrix() const {
    pData->m_ProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(m_vFoV), m_AspectRatio, m_zNear, m_zFar);

    m_ProjectionDirty = false;
    m_InverseProjectionDirty = true;
}

void Camera::UpdateInverseProjectionMatrix() const {
    pData->m_InverseProjectionMatrix = DirectX::XMMatrixInverse(nullptr, get_ProjectionMatrix());
    m_InverseProjectionDirty = false;
}
