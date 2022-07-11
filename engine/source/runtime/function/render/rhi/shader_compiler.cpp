#include "shader_compiler.h"
#include "runtime/core/base/macro.h"
#include "runtime/platform/file_service/file_service.h"

#define VERIFY_DXC_API(expr) \
    do \
    { \
        HRESULT hr = expr; \
        if (FAILED(hr)) \
        { \
            throw DxcException(hr); \
        } \
    } while (false)

const char* DxcException::GetErrorType() const noexcept { return "[Dxc]"; }

std::string DxcException::GetError() const
{
#define DXCERR(x) \
    case x: \
        Error = #x; \
        break

    std::string Error;
    switch (ErrorCode)
    {
        DXCERR(E_FAIL);
        DXCERR(E_INVALIDARG);
        DXCERR(E_OUTOFMEMORY);
        DXCERR(E_NOTIMPL);
        DXCERR(E_NOINTERFACE);
        default: {
            char Buffer[64] = {};
            sprintf_s(Buffer, "HRESULT of 0x%08X", static_cast<UINT>(ErrorCode));
            Error = Buffer;
        }
        break;
    }
#undef DXCERR
    return Error;
}

ShaderCompiler::ShaderCompiler() : ShaderModel(RHI_SHADER_MODEL::ShaderModel_6_6)
{
    VERIFY_DXC_API(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&Compiler3)));
    VERIFY_DXC_API(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&Utils)));
    VERIFY_DXC_API(Utils->CreateDefaultIncludeHandler(&DefaultIncludeHandler));
}

void ShaderCompiler::SetShaderModel(RHI_SHADER_MODEL ShaderModel) noexcept { this->ShaderModel = ShaderModel; }

Shader ShaderCompiler::CompileShader(RHI_SHADER_TYPE              ShaderType,
                                     const std::filesystem::path& Path,
                                     const ShaderCompileOptions&  Options) const
{
    std::wstring ProfileString = ShaderProfileString(ShaderType);

    std::vector<DxcDefine> Defines;
    Defines.reserve(Options.Defines.size());
    for (const auto& Define : Options.Defines)
    {
        Defines.emplace_back(DxcDefine {Define.first.data(), Define.second.data()});
    }

    ShaderCompilationResult Result = Compile(Path, Options.EntryPoint, ProfileString.data(), Defines);
    return {ShaderType, Result.ShaderHash, Result.Binary, Result.Pdb};
}

Library ShaderCompiler::CompileLibrary(const std::filesystem::path& Path) const
{
    std::wstring ProfileString = LibraryProfileString();

    ShaderCompilationResult Result = Compile(Path, L"", ProfileString.data(), {});
    return {Result.ShaderHash, Result.Binary, Result.Pdb};
}

std::wstring ShaderCompiler::GetShaderModelString() const
{
    std::wstring ShaderModelString;
    switch (ShaderModel)
    {
        case RHI_SHADER_MODEL::ShaderModel_6_5:
            ShaderModelString = L"6_5";
            break;
        case RHI_SHADER_MODEL::ShaderModel_6_6:
            ShaderModelString = L"6_6";
            break;
    }

    return ShaderModelString;
}

std::wstring ShaderCompiler::ShaderProfileString(RHI_SHADER_TYPE ShaderType) const
{
    std::wstring ProfileString;
    switch (ShaderType)
    {
        case RHI_SHADER_TYPE::Vertex:
            ProfileString = L"vs_";
            break;
        case RHI_SHADER_TYPE::Hull:
            ProfileString = L"hs_";
            break;
        case RHI_SHADER_TYPE::Domain:
            ProfileString = L"ds_";
            break;
        case RHI_SHADER_TYPE::Geometry:
            ProfileString = L"gs_";
            break;
        case RHI_SHADER_TYPE::Pixel:
            ProfileString = L"ps_";
            break;
        case RHI_SHADER_TYPE::Compute:
            ProfileString = L"cs_";
            break;
        case RHI_SHADER_TYPE::Amplification:
            ProfileString = L"as_";
            break;
        case RHI_SHADER_TYPE::Mesh:
            ProfileString = L"ms_";
            break;
    }

    return ProfileString + GetShaderModelString();
}

std::wstring ShaderCompiler::LibraryProfileString() const { return L"lib_" + GetShaderModelString(); }

ShaderCompilationResult ShaderCompiler::Compile(const std::filesystem::path&  Path,
                                                std::wstring_view             EntryPoint,
                                                std::wstring_view             Profile,
                                                const std::vector<DxcDefine>& ShaderDefines) const
{
    ShaderCompilationResult Result = {};

    std::filesystem::path PdbPath = Path;
    PdbPath.replace_extension(L"pdb");

    // https://developer.nvidia.com/dx12-dos-and-donts
    LPCWSTR Arguments[] = {
        // Use the /all_resources_bound / D3DCOMPILE_ALL_RESOURCES_BOUND compile flag if possible
        // This allows for the compiler to do a better job at optimizing texture accesses. We have
        // seen frame rate improvements of > 1 % when toggling this flag on.
        L"-all_resources_bound",
        L"-WX", // Warnings as errors
        L"-Zi", // Debug info
        L"-Fd",
        PdbPath.c_str(), // Shader Pdb
#ifdef _DEBUG
        L"-Od", // Disable optimization
#else
        L"-O3", // Optimization level 3
#endif
        L"-Zss", // Compute shader hash based on source
    };

    // Build arguments
    Microsoft::WRL::ComPtr<IDxcCompilerArgs> DxcCompilerArgs;
    VERIFY_DXC_API(Utils->BuildArguments(Path.c_str(),
                                         EntryPoint.data(),
                                         Profile.data(),
                                         Arguments,
                                         ARRAYSIZE(Arguments),
                                         ShaderDefines.data(),
                                         static_cast<UINT32>(ShaderDefines.size()),
                                         &DxcCompilerArgs));

    Microsoft::WRL::ComPtr<IDxcBlobEncoding> Source;
    VERIFY_DXC_API(Utils->LoadFile(Path.c_str(), nullptr, &Source));

    DxcBuffer SourceBuffer = {
        Source->GetBufferPointer(), Source->GetBufferSize(), DXC_CP_ACP};
    Microsoft::WRL::ComPtr<IDxcResult> DxcResult;
    VERIFY_DXC_API(Compiler3->Compile(&SourceBuffer,
                                      DxcCompilerArgs->GetArguments(),
                                      DxcCompilerArgs->GetCount(),
                                      DefaultIncludeHandler.Get(),
                                      IID_PPV_ARGS(&DxcResult)));

    HRESULT Status = S_FALSE;
    if (SUCCEEDED(DxcResult->GetStatus(&Status)))
    {
        if (FAILED(Status))
        {
            Microsoft::WRL::ComPtr<IDxcBlobUtf8>  Errors;
            Microsoft::WRL::ComPtr<IDxcBlobUtf16> OutputName;
            VERIFY_DXC_API(DxcResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&Errors), &OutputName));
            LOG_ERROR("Shader compile error: {0}", (char*)Errors->GetBufferPointer());
            throw std::runtime_error("Failed to compile shader");
        }
    }

    DxcResult->GetResult(&Result.Binary);

    if (DxcResult->HasOutput(DXC_OUT_PDB))
    {
        Microsoft::WRL::ComPtr<IDxcBlobUtf16> Name;
        DxcResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&Result.Pdb), &Name);
        if (Name)
        {
            Result.PdbName = Name->GetStringPointer();
        }

        // Save pdb
        Pilot::FileStream Stream(PdbPath, Pilot::FileMode::Create, Pilot::FileAccess::Write);
        Pilot::BinaryWriter Writer(Stream);
        Writer.write(Result.Pdb->GetBufferPointer(), Result.Pdb->GetBufferSize());
    }

    if (DxcResult->HasOutput(DXC_OUT_SHADER_HASH))
    {
        Microsoft::WRL::ComPtr<IDxcBlob>      ShaderHash;
        Microsoft::WRL::ComPtr<IDxcBlobUtf16> OutputName;
        DxcResult->GetOutput(DXC_OUT_SHADER_HASH, IID_PPV_ARGS(&ShaderHash), &OutputName);
        if (ShaderHash)
        {
            assert(ShaderHash->GetBufferSize() == sizeof(DxcShaderHash));
            Result.ShaderHash = *static_cast<DxcShaderHash*>(ShaderHash->GetBufferPointer());
        }
    }

    return Result;
}
