#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <Shlwapi.h>
#include <dxgidebug.h>

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "DirectXTex.lib")
#pragma comment(lib, "assimp-vc142-mtd.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Shlwapi.lib")

#include "application.h"
#include "engine_impl.h"

void ReportLiveObjects() {
    IDXGIDebug1* dxgiDebug;
    DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));

    HRESULT hr = dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    dxgiDebug->Release();
}

int main() {
    int retCode = 0;

    WCHAR path[MAX_PATH];
    HMODULE hModule = GetModuleHandleW(NULL);
    if (GetModuleFileNameW(hModule, path, MAX_PATH) > 0) {
        PathRemoveFileSpecW(path);
        SetCurrentDirectoryW(path);
    }

    Application::Create(hModule);

    {
        std::shared_ptr<EngineImpl> demo = std::make_shared<EngineImpl>(L"Learning DirectX 12 - Lesson 2", 1280, 720);
        retCode = Application::Get().Run(demo);
    }
    Application::Destroy();

    atexit(&ReportLiveObjects);

    return retCode;
}