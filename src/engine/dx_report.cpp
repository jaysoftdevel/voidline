#ifdef _WIN32
#include "engine/graphics.hpp"
#include "platform/paths.hpp"
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <iterator>
#include <iostream>
#include <stdexcept>

namespace vl {
namespace {
using Microsoft::WRL::ComPtr;

template<class T> bool query(ID3D12Device& device,D3D12_FEATURE feature,T& info) {
    return SUCCEEDED(device.CheckFeatureSupport(feature,&info,sizeof(info)));
}

const char* levelName(D3D_FEATURE_LEVEL level) {
    switch (level) {
    case D3D_FEATURE_LEVEL_12_2: return "12_2";
    case D3D_FEATURE_LEVEL_12_1: return "12_1";
    case D3D_FEATURE_LEVEL_12_0: return "12_0";
    case D3D_FEATURE_LEVEL_11_1: return "11_1";
    case D3D_FEATURE_LEVEL_11_0: return "11_0";
    default: return "unknown";
    }
}

void adapterReport(IDXGIAdapter1& adapter) {
    DXGI_ADAPTER_DESC1 description{};
    if (FAILED(adapter.GetDesc1(&description))) return;
    std::cout<<"\n"<<utf8Path(std::filesystem::path(description.Description))
             <<" | "<<((description.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)?"software":"hardware")
             <<" | "<<description.DedicatedVideoMemory/(1024*1024)<<" MiB dedicated\n";

    ComPtr<ID3D12Device> device;
    if (FAILED(D3D12CreateDevice(&adapter,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(device.GetAddressOf())))) {
        std::cout<<"  Direct3D 12 device unavailable\n";
        return;
    }
    constexpr D3D_FEATURE_LEVEL supported[]{D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,D3D_FEATURE_LEVEL_11_1,D3D_FEATURE_LEVEL_11_0};
    for (size_t first=0;first<std::size(supported);++first) {
        D3D12_FEATURE_DATA_FEATURE_LEVELS levels{static_cast<UINT>(std::size(supported)-first),
                                                supported+first,D3D_FEATURE_LEVEL_11_0};
        if (!query(*device.Get(),D3D12_FEATURE_FEATURE_LEVELS,levels)) continue;
        std::cout<<"  Maximum feature level "<<levelName(levels.MaxSupportedFeatureLevel)<<'\n';
        break;
    }

    // Older runtimes reject shader-model query values they do not recognize.
    for (const unsigned candidate:{0x69u,0x68u,0x67u,0x66u,0x65u,0x64u,0x63u,0x62u,0x61u,0x60u,0x51u}) {
        D3D12_FEATURE_DATA_SHADER_MODEL shader{static_cast<D3D_SHADER_MODEL>(candidate)};
        if (!query(*device.Get(),D3D12_FEATURE_SHADER_MODEL,shader)) continue;
        const unsigned model=shader.HighestShaderModel;
        std::cout<<"  Shader model "<<(model>>4)<<'.'<<(model&15)<<'\n';
        break;
    }
    D3D12_FEATURE_DATA_ROOT_SIGNATURE root{D3D_ROOT_SIGNATURE_VERSION_1_1};
    if (query(*device.Get(),D3D12_FEATURE_ROOT_SIGNATURE,root))
        std::cout<<"  Root signature "<<(root.HighestVersion>=D3D_ROOT_SIGNATURE_VERSION_1_1?"1.1":"1.0")<<'\n';
    D3D12_FEATURE_DATA_D3D12_OPTIONS base{};
    if (query(*device.Get(),D3D12_FEATURE_D3D12_OPTIONS,base)) {
        std::cout<<"  Resource binding tier "<<base.ResourceBindingTier
                 <<" | Tiled resources tier "<<base.TiledResourcesTier
                 <<" | Conservative rasterization tier "<<base.ConservativeRasterizationTier<<'\n';
    }
    D3D12_FEATURE_DATA_FORMAT_SUPPORT hdr{DXGI_FORMAT_R11G11B10_FLOAT};
    if (query(*device.Get(),D3D12_FEATURE_FORMAT_SUPPORT,hdr))
        std::cout<<"  R11G11B10 HDR target blending "<<((hdr.Support1&D3D12_FORMAT_SUPPORT1_BLENDABLE)?"yes":"no")<<'\n';

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 opt5{};
    if (query(*device.Get(),D3D12_FEATURE_D3D12_OPTIONS5,opt5))
        std::cout<<"  DXR tier "<<opt5.RaytracingTier<<" | Render pass tier "<<opt5.RenderPassesTier<<'\n';
    D3D12_FEATURE_DATA_D3D12_OPTIONS6 opt6{};
    if (query(*device.Get(),D3D12_FEATURE_D3D12_OPTIONS6,opt6))
        std::cout<<"  Variable-rate shading tier "<<opt6.VariableShadingRateTier<<'\n';
    D3D12_FEATURE_DATA_D3D12_OPTIONS7 opt7{};
    if (query(*device.Get(),D3D12_FEATURE_D3D12_OPTIONS7,opt7))
        std::cout<<"  Mesh shader tier "<<opt7.MeshShaderTier<<" | Sampler feedback tier "<<opt7.SamplerFeedbackTier<<'\n';
    D3D12_FEATURE_DATA_D3D12_OPTIONS12 opt12{};
    if (query(*device.Get(),D3D12_FEATURE_D3D12_OPTIONS12,opt12))
        std::cout<<"  Enhanced barriers "<<(opt12.EnhancedBarriersSupported?"yes":"no")<<'\n';
}
}

void printDirectXReport() {
    ComPtr<IDXGIFactory4> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf()))))
        throw std::runtime_error("Could not initialize DXGI.");
    std::cout<<"Direct3D 12 adapter capabilities\n";
    ComPtr<IDXGIFactory5> present;
    BOOL tearing=FALSE;
    if (SUCCEEDED(factory.As(&present)) && SUCCEEDED(present->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,&tearing,sizeof(tearing))))
        std::cout<<"DXGI tearing support "<<(tearing?"yes":"no")<<'\n';
    for (UINT i=0;;++i) {
        ComPtr<IDXGIAdapter1> adapter;
        if (FAILED(factory->EnumAdapters1(i,adapter.GetAddressOf()))) break;
        DXGI_ADAPTER_DESC1 info{};
        if (SUCCEEDED(adapter->GetDesc1(&info)) && !(info.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)) adapterReport(*adapter.Get());
    }
    ComPtr<IDXGIAdapter1> warp;
    if (SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(warp.GetAddressOf())))) adapterReport(*warp.Get());
}
}
#endif
