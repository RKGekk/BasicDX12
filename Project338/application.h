#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <atomic>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>

#include "adapter_reader.h"
#include "device.h"
#include "events.h"

class WindowSurface;
class EngineImpl;

using WndProcEvent = Delegate<LRESULT(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)>;

class Application {
public:
    static void Create(HINSTANCE hInst);
    static void Destroy();
    static Application& Get();

    std::shared_ptr<WindowSurface> CreateRenderWindow(const std::wstring& window_name, int client_width, int client_height);
    std::shared_ptr<WindowSurface> GetWindowByName(const std::wstring& window_name);

    int Run(std::shared_ptr<EngineImpl> pEngine);
    void Quit(int exit_code = 0);

    void Stop();
    WndProcEvent WndProcHandler;
    Event Exit;

protected:
    friend LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    Application(HINSTANCE hInst);
    bool Initialize();
    virtual ~Application();

    void RegisterWindowClass(HINSTANCE hInst);
    static void EnableDebugLayer();

    virtual LRESULT OnWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    virtual void OnExit(EventArgs& e);

private:
    Application(const Application& copy) = delete;
    Application& operator=(const Application& other) = delete;

    HINSTANCE m_hInstance;

    std::atomic_bool m_is_running;
    std::atomic_bool m_request_quit;
};