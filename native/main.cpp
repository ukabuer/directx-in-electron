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

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define VIEWPORT_WIDTH 300
#define VIEWPORT_HEIGHT 300
#define HEADLESS_MODE 1
#define WINDOW_CLASS_NAME "MyWindowClass"

static DWORD sTargetProcessId = 0;

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
    auto window = CreateWindowEx(
        0, //WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_COMPOSITED,
        WINDOW_CLASS_NAME,
        "Test",
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

#if HEADLESS_MODE
void Render()
#else
void Render(HWND hWnd)
#endif
{
    struct Vertex { float x, y, z; float color[4]; };
    struct VShaderParams { float time; float pad[3]; };

    ID3D11Device* pDev = nullptr;
    ID3D11DeviceContext* pCtx = nullptr;
    IDXGISwapChain* pSwapchain = nullptr;

#if HEADLESS_MODE
    auto hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        0,
        0, // D3D11_CREATE_DEVICE_DEBUG,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &pDev,
        nullptr,
        &pCtx
    );
#else
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
#endif

    D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    VShaderParams params{};
    
    ID3D11Texture2D *pTexture = nullptr;
    ID3D11ShaderResourceView* pSrv = nullptr;
    ID3D11RenderTargetView* pRenderTarget = nullptr;
    ID3D11Buffer* pVertexBuffer = nullptr;
    ID3D11Buffer* pShaderParamsBuffer = nullptr;
    ID3D11VertexShader* pVS = nullptr;
    ID3D11PixelShader* pPS = nullptr;
    ID3D11InputLayout* pLayout = nullptr;
    IDXGIKeyedMutex* pDxgiMutex = nullptr;
    // render triangle data
    {
        D3D11_TEXTURE2D_DESC texDesc = {
            .Width = VIEWPORT_WIDTH,
            .Height = VIEWPORT_HEIGHT,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
            .SampleDesc = { .Count = 1, .Quality = 0 },
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
            .CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE,
            .MiscFlags = D3D11_RESOURCE_MISC_SHARED_NTHANDLE | D3D11_RESOURCE_MISC_SHARED 
        };
        pDev->CreateTexture2D(&texDesc, NULL, &pTexture);
        assert(SUCCEEDED(hr) && pTexture != nullptr);

        IDXGIResource1* pDXGIResource = NULL;
        hr = pTexture->QueryInterface(__uuidof(IDXGIResource1), (LPVOID*) &pDXGIResource);
        assert(SUCCEEDED(hr) && pDXGIResource != nullptr);

        HANDLE localHandle = 0;
        hr = pDXGIResource->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ, L"SharedTexture", &localHandle);
        assert(SUCCEEDED(hr) && localHandle != nullptr);

        HANDLE hProc = OpenProcess(PROCESS_DUP_HANDLE, false, (DWORD)sTargetProcessId);
        assert(hProc != 0);

        HANDLE sharedHandle;
        hr = DuplicateHandle(GetCurrentProcess(), localHandle, hProc, &sharedHandle, 0, false, DUPLICATE_SAME_ACCESS);
        assert(SUCCEEDED(hr) && sharedHandle != 0);
        CloseHandle(hProc);
        std::cout << "Texture shared handle: " << sharedHandle << std::endl;

        hr = pDev->CreateRenderTargetView(pTexture, nullptr, &pRenderTarget);
        assert(SUCCEEDED(hr) && pRenderTarget != nullptr);
        pCtx->OMSetRenderTargets(1, &pRenderTarget, nullptr);

        hr = pDev->CreateShaderResourceView(pTexture, nullptr, &pSrv);
        assert(SUCCEEDED(hr) && pSrv != nullptr);

        pTexture->Release();

        // compile and setup shaders
        ID3D10Blob* vs, *ps;
        hr = D3DCompileFromFile(L"native\\shaders.hlsl", 0, 0, "VShader", "vs_4_0", 0, 0, &vs, 0);
        assert(SUCCEEDED(hr) && vs != nullptr);
        hr = D3DCompileFromFile(L"native\\shaders.hlsl", 0, 0, "PShader", "ps_4_0", 0, 0, &ps, 0);
        assert(SUCCEEDED(hr) && ps != nullptr);
        pDev->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &pVS);
        pDev->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &pPS);

        // setup input layout
        pDev->CreateInputLayout(ied, 2, vs->GetBufferPointer(), vs->GetBufferSize(), &pLayout);

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
    }

#if !HEADLESS_MODE
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
        hr = D3DCompileFromFile(L"native\\shaders.hlsl", 0, 0, "VShader", "vs_4_0", 0, 0, &vs, 0);
        assert(SUCCEEDED(hr) && vs != nullptr);
        hr = D3DCompileFromFile(L"native\\shaders.hlsl", 0, 0, "PShaderCopy", "ps_4_0", 0, 0, &ps, 0);
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
#endif

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
        
        frame++;

        // render triangle
        {
            pCtx->OMSetRenderTargets(1, &pRenderTarget, nullptr);
            float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            pCtx->ClearRenderTargetView(pRenderTarget, color);

            pCtx->IASetInputLayout(pLayout);
            uint32_t stride = sizeof(Vertex);
            UINT offset = 0;
            pCtx->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
            pCtx->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            
            pCtx->VSSetShader(pVS, 0, 0);
            pCtx->PSSetShader(pPS, 0, 0);

            params.time += 0.001f;
            pCtx->UpdateSubresource(reinterpret_cast<ID3D11Resource*>(pShaderParamsBuffer), 0, nullptr, &params, 0, 0);
            pCtx->VSSetConstantBuffers(0, 1, &pShaderParamsBuffer);

            pCtx->Draw(3, 0);

            pCtx->OMSetRenderTargets(0, nullptr, nullptr);
        }
        
        // copy to backbuffer
#if !HEADLESS_MODE
        {
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
        }

        hr = pSwapchain->Present(0, 0);
#else
        pCtx->Flush();
#endif

        assert(SUCCEEDED(hr));
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
        std::cerr << "handle should be passed as the second arg";
        return 1;
    }

    auto* arg = argv[1];
    sTargetProcessId = static_cast<DWORD>(std::stoi(arg, 0, 10));
    std::cout << "Get Electron app's main process id: " << sTargetProcessId << std::endl;

#if HEADLESS_MODE
    Render();
#else
    RegisterWindowClass();
    auto window = CreateMyWindow();

    Render(window);
#endif

    return 0;
}
