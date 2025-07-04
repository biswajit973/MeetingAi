#include <windows.h>
#include <d3d11.h>
#include "imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#include "imgui/backends/imgui_impl_dx11.h"
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "d3d11.lib")

#include <vector>
#include <string>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <fstream>
#include "lohmann/json.hpp"
#include "audio_device_json.hpp"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HWND hwnd;
ID3D11Device* g_pd3dDevice = NULL;
ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
IDXGISwapChain* g_pSwapChain = NULL;
ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

// Store screen size globally
int g_screenWidth = 0;
int g_screenHeight = 0;

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = g_screenWidth;
    sd.BufferDesc.Height = g_screenHeight;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
        createDeviceFlags, featureLevelArray, 1, D3D11_SDK_VERSION,
        &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    if (res != S_OK) return false;
    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pd3dDeviceContext) g_pd3dDeviceContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();
}

// Helper to run a command and get output
std::vector<std::string> GetAudioDevices() {
    std::vector<std::string> devices;
    std::array<char, 256> buffer;
    std::string result;
    // Use the built EXE from PyInstaller (adjust path if needed)
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen("dist/audio_transcriber.exe --list-devices", "r"), _pclose);
    if (!pipe) return devices;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    size_t pos = 0, end;
    while ((end = result.find('\n', pos)) != std::string::npos) {
        std::string line = result.substr(pos, end - pos);
        if (!line.empty()) devices.push_back(line);
        pos = end + 1;
    }
    return devices;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // Get screen size
    g_screenWidth = GetSystemMetrics(SM_CXSCREEN);
    g_screenHeight = GetSystemMetrics(SM_CYSCREEN);

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, WndProc, 0L, 0L,
        GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
        L"ImGuiWindowClass", NULL };
    RegisterClassExW(&wc);

    hwnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        wc.lpszClassName, L"ImGui Overlay", WS_POPUP, 0, 0, g_screenWidth, g_screenHeight,
        NULL, NULL, wc.hInstance, NULL);

    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    // Prevent overlay from being captured by screen sharing/capture APIs
    SetWindowDisplayAffinity(hwnd, 0x11); // WDA_EXCLUDEFROMCAPTURE

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    ImGui::StyleColorsDark();

    // Set ImGui display size to screen size
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)g_screenWidth, (float)g_screenHeight);

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    bool show_overlay = true;
    while (msg.message != WM_QUIT && show_overlay) {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Only update device list JSON on first run or when user requests refresh
        static std::vector<AudioDevice> devices;
        static int current_device = 0;
        static bool transcribing = false;
        static std::string transcript;
        static char transcript_buf[8192] = "";
        static bool devices_loaded = false;
        if (!devices_loaded) {
            system("dist/audio_transcriber.exe --list-devices-json");
            devices = LoadAudioDevicesFromJson();
            devices_loaded = true;
        }

        ImGuiWindowFlags window_flags = 0; // Allow resize, move, collapse, close
        ImGui::SetNextWindowSize(ImVec2(600, 700), ImGuiCond_FirstUseEver);
        ImGui::Begin("AI Transcription", &show_overlay, window_flags);
        ImGui::Text("AI Transcription");
        ImGui::Separator();

        // Audio input dropdown
        ImGui::Text("Select Audio Input");
        ImGui::SameLine();
        if (ImGui::BeginCombo("##AudioDevice", devices.empty() ? "None" : devices[current_device].name.c_str(), 0)) {
            for (int n = 0; n < static_cast<int>(devices.size()); n++) {
                bool is_selected = (current_device == n);
                if (ImGui::Selectable(devices[n].name.c_str(), is_selected))
                    current_device = n;
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // Start Transcribing button (large, blue)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.13f, 0.55f, 0.95f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.65f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.45f, 0.85f, 1.0f));
        // Use plain text label to avoid Unicode escape errors
        if (ImGui::Button(transcribing ? "Stop Transcribing" : "Start Transcribing", ImVec2(400, 0))) {
            if (!transcribing) {
                std::string cmd = "start /B Debug/audio_transcriber.exe --device " + std::to_string(devices[current_device].index) + " > transcription.txt";
                system(cmd.c_str());
                transcribing = true;
            } else {
                transcribing = false;
            }
        }
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
        if (ImGui::Button("Refresh", ImVec2(100, 0))) {
            system("Debug/audio_transcriber.exe --list-devices-json");
            devices = LoadAudioDevicesFromJson();
        }

        ImGui::Separator();
        ImGui::Spacing();

        // Q&A Chat-like area (responsive, touches bottom)
        ImGui::BeginChild("QA_Scroll", ImVec2(0, ImGui::GetContentRegionAvail().y), true, ImGuiWindowFlags_AlwaysUseWindowPadding);
        std::ifstream qa_infile("qa_transcription.json");
        transcript.clear();
        float bubbleWidth = ImGui::GetContentRegionAvail().x;
        if (qa_infile) {
            try {
                nlohmann::json j;
                qa_infile >> j;
                for (const auto& entry : j) {
                    if (entry.contains("question") && entry.contains("answer")) {
                        // --- User (You:) bubble, full width, fit height ---
                        const std::string& q = entry["question"].get<std::string>();
                        ImVec2 qTextSize = ImGui::CalcTextSize(q.c_str(), nullptr, false, bubbleWidth - 32.0f);
                        float qBubbleHeight = qTextSize.y + ImGui::GetStyle().FramePadding.y * 4 + ImGui::GetTextLineHeight();
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
                        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.13f, 0.55f, 0.95f, 0.35f));
                        ImGui::BeginChild(ImGui::GetID((q + "_q").c_str()), ImVec2(bubbleWidth, qBubbleHeight), true, ImGuiWindowFlags_AlwaysUseWindowPadding);
                        ImGui::TextColored(ImVec4(1,1,1,1), "You:");
                        ImGui::SameLine();
                        ImGui::PushTextWrapPos(0.0f);
                        ImGui::TextUnformatted(q.c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::EndChild();
                        ImGui::PopStyleColor();
                        ImGui::PopStyleVar();
                        ImGui::Spacing();
                        // --- AI (AI:) bubble, full width, fit height ---
                        const std::string& a = entry["answer"].get<std::string>();
                        ImVec2 aTextSize = ImGui::CalcTextSize(a.c_str(), nullptr, false, bubbleWidth - 32.0f);
                        float aBubbleHeight = aTextSize.y + ImGui::GetStyle().FramePadding.y * 4 + ImGui::GetTextLineHeight();
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
                        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.18f, 0.18f, 0.20f, 0.85f));
                        ImGui::BeginChild(ImGui::GetID((a + "_a").c_str()), ImVec2(bubbleWidth, aBubbleHeight), true, ImGuiWindowFlags_AlwaysUseWindowPadding);
                        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "AI:");
                        ImGui::SameLine();
                        ImGui::PushTextWrapPos(0.0f);
                        ImGui::TextUnformatted(a.c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::EndChild();
                        ImGui::PopStyleColor();
                        ImGui::PopStyleVar();
                        ImGui::Spacing();
                    }
                }
            } catch (...) {
                ImGui::TextColored(ImVec4(1,0,0,1), "[Error reading qa_transcription.json]");
            }
        } else {
            ImGui::Text("No Q&A transcript available.");
        }
        ImGui::EndChild();
        ImGui::End();

        if (!show_overlay) {
            PostQuitMessage(0);
        }

        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        const float clear_color[4] = { 0.0f,0.0f,0.0f,0.0f };
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

