#include <iostream>
#include <cassert>
#include <string>
#include <cstdint>
#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_2.h>
#include <processthreadsapi.h>
#include <handleapi.h>
#include <format>
#include <errhandlingapi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define VIEWPORT_WIDTH 300
#define VIEWPORT_HEIGHT 300
#define WINDOW_CLASS_NAME "MyWindowClass"

static HANDLE s_handle = 0;

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
    // wc.style = CS_OWNDC | CS_NOCLOSE | CS_HREDRAW | CS_VREDRAW;
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = nullptr;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = WINDOW_CLASS_NAME;

    RegisterClassEx(&wc);
}

static HWND CreateMyWindow()
{
    RECT rect;

    auto window = CreateWindowEx(
        0, //WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_COMPOSITED,
        WINDOW_CLASS_NAME,
        "Test2",
        WS_VISIBLE | WS_OVERLAPPEDWINDOW,
        0,
        0,
        VIEWPORT_WIDTH,
        VIEWPORT_HEIGHT,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    );
    // SetLayeredWindowAttributes(window, 0, 255, LWA_ALPHA);

    return window;
}

void Render(HWND hWnd)
{
    struct Vertex { float x, y, z; float color[4]; };

    ID3D11Device* pDev = nullptr;
    ID3D11DeviceContext* pCtx = nullptr;
    IDXGISwapChain* pSwapchain = nullptr;

    // setup device & device context & swapchain
    DXGI_SWAP_CHAIN_DESC scd;
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));
    scd.BufferCount = 2;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.Width = VIEWPORT_WIDTH;
    scd.BufferDesc.Height = VIEWPORT_HEIGHT;
    scd.BufferDesc.RefreshRate.Numerator = 0;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    // scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
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

    D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    ID3D11Texture2D *pTexture = nullptr;
    ID3D11ShaderResourceView* pSrv = nullptr;
    {
      std::cout << s_handle << std::endl;
        IDXGIResource1* pDXGIResource = NULL;
        hr = pDev->OpenSharedResource(
            s_handle, 
            __uuidof(ID3D11Texture2D), 
              (void**)&pTexture);
        printf("%x", hr);
        std::cout << std::format("{:x}", hr) << std::endl;
        if (!SUCCEEDED(hr))
        {
          std::cout << "error: " << GetLastError() << std::endl;
        }

        // assert(SUCCEEDED(hr) && pDXGIResource != nullptr);
        // pDXGIResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)(&pTexture));
        assert(SUCCEEDED(hr) && pTexture != nullptr);
        pDXGIResource->Release();

        hr = pDev->CreateShaderResourceView(pTexture, nullptr, &pSrv);
        assert(SUCCEEDED(hr) && pSrv != nullptr);

        pTexture->Release();
    }

    ID3D11RenderTargetView* pBackbuffer = nullptr;
    ID3D11Buffer* pCopyVertexBuffer = nullptr;
    ID3D11VertexShader* pCopyVS = nullptr;
    ID3D11PixelShader* pCopyPS = nullptr;
    ID3D11InputLayout* pCopyLayout = nullptr;
    {
        ID3D11Texture2D* tex = nullptr;
        hr = pSwapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&tex));
        assert(SUCCEEDED(hr) && tex != nullptr);
        hr = pDev->CreateRenderTargetView(tex, nullptr, &pBackbuffer);
        assert(SUCCEEDED(hr) && pBackbuffer != nullptr);
        tex->Release();

        ID3D10Blob* vs, *ps;
        hr = D3DCompileFromFile(L"native\\copy.hlsl", 0, 0, "VShader", "vs_4_0", 0, 0, &vs, 0);
        assert(SUCCEEDED(hr) && vs != nullptr);
        hr = D3DCompileFromFile(L"native\\copy.hlsl", 0, 0, "PShader", "ps_4_0", 0, 0, &ps, 0);
        assert(SUCCEEDED(hr) && ps != nullptr);
        pDev->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &pCopyVS);
        pDev->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &pCopyPS);

        // setup input layout
        pDev->CreateInputLayout(ied, 2, vs->GetBufferPointer(), vs->GetBufferSize(), &pCopyLayout);
        assert(SUCCEEDED(hr) && pCopyLayout != nullptr);

        Vertex vertices[] =
        {
            {-1.0f, 1.0f, 0.0f, {1.0f, 0.0f, 0.0f, 1.0f}},
            {1.0f,  1.0f, 0.0f, {0.0f, 1.0f, 0.0f, 1.0f}},
            {-1.0f, -1.0f, 0.0f, {0.0f, 0.0f, 1.0f, 1.0f}},

            {-1.0f, -1.0f, 0.0f, {0.0f, 0.0f, 1.0f, 1.0f}},
            { 1.0f,  1.0f, 0.0f, {0.0f, 0.0f, 1.0f, 1.0f}},
            { 1.0f, -1.0f, 0.0f, {0.0f, 0.0f, 1.0f, 1.0f}}
        };
        D3D11_SUBRESOURCE_DATA data;
        ZeroMemory(&data, sizeof(data));
        data.pSysMem = vertices;

        D3D11_BUFFER_DESC bd;
        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(Vertex) * 6;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        hr = pDev->CreateBuffer(&bd, &data, &pCopyVertexBuffer);
        assert(SUCCEEDED(hr) && pCopyVertexBuffer != nullptr);
    }

    // Set the viewport
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = VIEWPORT_WIDTH;
    viewport.Height = VIEWPORT_HEIGHT;
    pCtx->RSSetViewports(1, &viewport);

    MSG msg;
    // enter the render loop
    uint32_t frame = 0;
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

        // copy to backbuffer
        pCtx->OMSetRenderTargets(1, &pBackbuffer, nullptr);
        float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        pCtx->ClearRenderTargetView(pBackbuffer, color);

        pCtx->IASetInputLayout(pCopyLayout);
        uint32_t stride = sizeof(Vertex);
        UINT offset = 0;
        pCtx->IASetVertexBuffers(0, 1, &pCopyVertexBuffer, &stride, &offset);
        pCtx->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        pCtx->VSSetShader(pCopyVS, 0, 0);
        pCtx->PSSetShader(pCopyPS, 0, 0);

        pCtx->PSSetShaderResources(0, 1, &pSrv);

        pCtx->Draw(6, 0);

        hr = pSwapchain->Present(0, 0);
        assert(SUCCEEDED(hr));
    }

    pSwapchain->Release();
    pDev->Release();
    pCtx->Release();
}

int main(int argc, const char** argv)
{

  auto proc_id = GetCurrentProcessId();
  std::cout << "Current Process: ";
  std::cout << proc_id << std::endl;

  int value = 0;
  std::cin >> value;
  if (value < 0)
  {
      std::cerr << "handle should be passed as arg";
      return 1;
  }

  std::cout << "Shared Handle: " << value << std::endl;

  s_handle = reinterpret_cast<HANDLE>(value);
  std::cout << s_handle << std::endl;

    RegisterWindowClass();
    // FixChromeD3DIssue(hwnd);
    // auto window = CreateChildWindow(hwnd);
    auto window = CreateMyWindow();

    Render(window);

    return 0;
}
