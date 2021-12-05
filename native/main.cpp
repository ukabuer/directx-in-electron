#include <iostream>
#include <cassert>
#include <string>
#include <cstdint>

#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define VIEWPORT_WIDTH 300
#define VIEWPORT_HEIGHT 300
#define WINDOW_CLASS_NAME "MyWindowClass"

// from https://github.com/stream-labs/obs-studio-node/blob/36eeb480f36c9c414fb73223d144f4889e331029/obs-studio-client/source/nodeobs_display.cpp#L32
static BOOL CALLBACK EnumChromeWindowsProc(HWND hwnd, LPARAM lParam)
{
    char buf[256];
    if (GetClassNameA(hwnd, buf, sizeof(buf) / sizeof(*buf))) {
        if (strstr(buf, "Intermediate D3D Window")) {
            LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
            if ((style & WS_CLIPSIBLINGS) == 0) {
                style |= WS_CLIPSIBLINGS;
                SetWindowLongPtr(hwnd, GWL_STYLE, style);
            }
        }
    }
    return TRUE;
}

// from https://github.com/stream-labs/obs-studio-node/blob/36eeb480f36c9c414fb73223d144f4889e331029/obs-studio-client/source/nodeobs_display.cpp#L47
static void FixChromeD3DIssue(HWND chromeWindow)
{
    // auto handle = FindWindowEx(chromeWindow, nullptr, "Intermediate D3D Window", "");
    // assert(handle != nullptr);
    (void)EnumChildWindows(chromeWindow, EnumChromeWindowsProc, 0);

    LONG_PTR style = GetWindowLongPtr(chromeWindow, GWL_STYLE);
    if ((style & WS_CLIPCHILDREN) == 0) {
        style |= WS_CLIPCHILDREN;
        SetWindowLongPtr(chromeWindow, GWL_STYLE, style);
    }
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

static void RegisterWindowClass()
{
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(WNDCLASSEX));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_OWNDC | CS_NOCLOSE | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = nullptr;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = WINDOW_CLASS_NAME;

    RegisterClassEx(&wc);
}

static HWND CreateChildWindow(HWND parent)
{
    RECT rect;
    GetWindowRect(parent, &rect);
    int parentWidth = rect.right - rect.left;
    int parentHeight = rect.bottom - rect.top;

    auto child = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_COMPOSITED,
        WINDOW_CLASS_NAME,
        nullptr,
        WS_VISIBLE | WS_CHILD | WS_POPUP,
        parentWidth / 2 - VIEWPORT_WIDTH / 2,
        parentHeight / 2 - VIEWPORT_HEIGHT / 2,
        VIEWPORT_WIDTH,
        VIEWPORT_HEIGHT,
        parent,
        nullptr,
        nullptr,
        nullptr
    );
    SetParent(child, parent);
    SetLayeredWindowAttributes(child, 0, 255, LWA_ALPHA);

    return child;
}

void Render(HWND hWnd)
{
    struct Vertex { float x, y, z; float color[4]; };
    struct VShaderParams { float time; float pad[3]; };

    ID3D11Device* pDev = nullptr;
    ID3D11DeviceContext* pCtx = nullptr;
    IDXGISwapChain* pSwapchain = nullptr;
    ID3D11RenderTargetView* pRenderTarget = nullptr;
    ID3D11InputLayout* pLayout = nullptr;
    ID3D11VertexShader* pVS = nullptr;
    ID3D11PixelShader* pPS = nullptr;
    ID3D11Buffer* pVertexBuffer = nullptr;
    ID3D11Buffer* pShaderParamsBuffer = nullptr;
    VShaderParams params{};

    // setup device & device context & swapchain
    DXGI_SWAP_CHAIN_DESC scd;
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.Width = VIEWPORT_WIDTH;
    scd.BufferDesc.Height = VIEWPORT_HEIGHT;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    auto hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        0,
        0, // D3D11_CREATE_DEVICE_DEBUG,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &scd,
        &pSwapchain,
        &pDev,
        nullptr,
        &pCtx
    );
    assert(SUCCEEDED(hr) && pDev != nullptr && pCtx != nullptr);

    // setup render targets
    ID3D11Texture2D* tex = nullptr;
    hr = pSwapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&tex));
    assert(SUCCEEDED(hr) && tex != nullptr);
    hr = pDev->CreateRenderTargetView(tex, nullptr, &pRenderTarget);
    assert(SUCCEEDED(hr) && tex != nullptr);
    pCtx->OMSetRenderTargets(1, &pRenderTarget, nullptr);
    tex->Release();

    // Set the viewport
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = VIEWPORT_WIDTH;
    viewport.Height = VIEWPORT_HEIGHT;
    pCtx->RSSetViewports(1, &viewport);

    // compile and setup shaders
    ID3D10Blob* vs, *ps;
    hr = D3DCompileFromFile(L"native\\shaders.hlsl", 0, 0, "VShader", "vs_4_0", 0, 0, &vs, 0);
    assert(SUCCEEDED(hr) && vs != nullptr);
    hr = D3DCompileFromFile(L"native\\shaders.hlsl", 0, 0, "PShader", "ps_4_0", 0, 0, &ps, 0);
    assert(SUCCEEDED(hr) && ps != nullptr);
    pDev->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &pVS);
    pDev->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &pPS);
    pCtx->VSSetShader(pVS, 0, 0);
    pCtx->PSSetShader(pPS, 0, 0);

    // setup input layout
    D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    pDev->CreateInputLayout(ied, 2, vs->GetBufferPointer(), vs->GetBufferSize(), &pLayout);
    pCtx->IASetInputLayout(pLayout);

    // setup vertex buffer
    Vertex vertices[] =
    {
        {0.0f, 0.5f, 0.0f, {1.0f, 0.0f, 0.0f, 1.0f}},
        {0.5f, -0.5, 0.0f, {0.0f, 1.0f, 0.0f, 1.0f}},
        {-0.5f, -0.5f, 0.0f, {0.0f, 0.0f, 1.0f, 1.0f}}
    };
    D3D11_SUBRESOURCE_DATA data;
    ZeroMemory(&data, sizeof(data));
    data.pSysMem = vertices;

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(Vertex) * 3;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    hr = pDev->CreateBuffer(&bd, &data, &pVertexBuffer);
    assert(SUCCEEDED(hr) && pVertexBuffer != nullptr);

    // setup shader params buffer
    D3D11_BUFFER_DESC cbd;
    ZeroMemory(&cbd, sizeof(cbd));
    cbd.Usage = D3D11_USAGE_DEFAULT;
    cbd.ByteWidth = sizeof(VShaderParams);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = pDev->CreateBuffer(&cbd, nullptr, &pShaderParamsBuffer);
    assert(SUCCEEDED(hr) && pShaderParamsBuffer != nullptr);

    MSG msg;
    // enter the render loop
    while (true)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
            {
                break;
            }
        }
        
        float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        pCtx->ClearRenderTargetView(pRenderTarget, color);

        uint32_t stride = sizeof(Vertex);
        UINT offset = 0;
        pCtx->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
        pCtx->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        params.time += 0.001f;
        pCtx->UpdateSubresource(reinterpret_cast<ID3D11Resource*>(pShaderParamsBuffer), 0, nullptr, &params, 0, 0);
        pCtx->VSSetConstantBuffers(0, 1, &pShaderParamsBuffer);

        pCtx->Draw(3, 0);

        pSwapchain->Present(0, 0);
    }

    pLayout->Release();
    pVS->Release();
    pPS->Release();
    pVertexBuffer->Release();
    pSwapchain->Release();
    pDev->Release();
    pRenderTarget->Release();
    pCtx->Release();
}

int main(int argc, const char** argv)
{
    if (argc < 2)
    {
        std::cerr << "HWND should be passed as the second arg";
        return 1;
    }

    auto* arg = argv[1];
    auto hwnd = reinterpret_cast<HWND>(std::stoi(arg, 0, 10));

    RegisterWindowClass();
    FixChromeD3DIssue(hwnd);
    auto child = CreateChildWindow(hwnd);

    Render(child);

    return 0;
}
