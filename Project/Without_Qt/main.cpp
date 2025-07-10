#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "implot.h"
#include <windows.h>
#include <d3d11.h>
#include <vector>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <portaudio.h>

// Link DX libraries
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// Forward declare message handler
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

// Data containers
std::vector<float> x_data;
std::vector<float> y_data;
bool is_recording = false;

// Microphone device index (default 1, change as needed)
int micDeviceIndex = 1;

// Helper: List all PortAudio devices (run at startup to find your mic)
void listAudioDevices() {
    Pa_Initialize();
    int numDevices = Pa_GetDeviceCount();
    printf("=== Audio Input Devices ===\n");
    for (int i = 0; i < numDevices; ++i) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        printf("Device #%d: %s\n", i, info->name);
    }
    Pa_Terminate();
}

// Get RMS amplitude from microphone
float getMicRMS() {
    PaStream *stream;
    PaStreamParameters inputParameters;
    std::vector<float> buffer(256);  // 256 samples
    float rms = 0.0f;

    if (Pa_Initialize() != paNoError)
        return -1;

    inputParameters.device = micDeviceIndex;
    if (inputParameters.device == paNoDevice) {
        Pa_Terminate();
        return -1;
    }
    inputParameters.channelCount = 1;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;

    if (Pa_OpenStream(&stream, &inputParameters, nullptr,
                      44100, 256, paClipOff, nullptr, nullptr) != paNoError) {
        Pa_Terminate();
        return -1;
    }

    Pa_StartStream(stream);
    Pa_ReadStream(stream, buffer.data(), 256);
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    float sum = 0;
    for (float s : buffer)
        sum += s * s;
    rms = std::sqrt(sum / buffer.size());
    return rms;
}

// Add a data point (X = time, Y = mic RMS amplitude)
void add_data_point() {
    static float t = 0;
    t += 0.05f; // e.g., 50 ms per sample
    float amplitude = getMicRMS();
    x_data.push_back(t);
    y_data.push_back(amplitude);
    if (x_data.size() > 200) {
        x_data.erase(x_data.begin());
        y_data.erase(y_data.begin());
    }
}

// Win32 globals
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;
static HWND                     g_hWnd = nullptr;

// Forward declare helpers
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

// Win32 main window procedure
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// Main code
int main(int argc, char **argv)
{
    // Optionally set mic index from command line
    if (argc > 1)
        micDeviceIndex = atoi(argv[1]);

    // List all devices for user info
    listAudioDevices();

    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "ImGui Example", NULL };
    RegisterClassEx(&wc);
    g_hWnd = CreateWindow(wc.lpszClassName, "Without_Qt - ImGui/ImPlot Demo", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(g_hWnd))
    {
        CleanupDeviceD3D();
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(g_hWnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hWnd);

    // Setup ImGui/ImPlot context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages
        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // ----- UI -----
        ImGui::Begin("Real-Time Plot Demo");
        if (ImGui::Button("Start Recording")) is_recording = true;
        ImGui::SameLine();
        if (ImGui::Button("Stop Recording")) is_recording = false;

        if (ImGui::InputInt("Mic Device Index", &micDeviceIndex)) {
            // If user changes index, next point will use the new device
        }

        if (is_recording) add_data_point();

        if (ImPlot::BeginPlot("Amplitude (RMS) vs. Time", "Time (s)", "Amplitude")) {
            ImPlot::SetNextAxesLimits(0, 10, 0, 1.0, ImGuiCond_Once); // e.g., 10s x-axis, amplitude 0-1
            ImPlot::PlotScatter("Mic RMS", x_data.data(), y_data.data(), (int)x_data.size());
            ImPlot::EndPlot();
        }

        ImGui::End();
        // ----- End UI -----

        // Rendering
        ImGui::Render();
        const float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(g_hWnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// DirectX helper functions
bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0 };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 1,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = NULL;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}
