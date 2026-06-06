#include <iostream>
#include <cassert>
#include <string>
#include <cstdint>

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

static DWORD sTargetProcessId = 0;

void Render()
{
    struct Vertex { float x, y, z; float color[4]; };
    struct VShaderParams { float time; float pad[3]; };

    ID3D11Device* pDev = nullptr;
    ID3D11DeviceContext* pCtx = nullptr;

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

        pCtx->Flush();
    }

    pLayout->Release();
    pVS->Release();
    pPS->Release();
    pVertexBuffer->Release();
    pDev->Release();
    pRenderTarget->Release();
    pCtx->Release();
}

int main(int argc, const char** argv)
{
    if (argc < 2)
    {
        std::cerr << "process id should be passed as the second arg";
        return 1;
    }

    auto* arg = argv[1];
    sTargetProcessId = static_cast<DWORD>(std::stoi(arg, 0, 10));
    std::cout << "Get Electron app's main process id: " << sTargetProcessId << std::endl;

    Render();

    return 0;
}
