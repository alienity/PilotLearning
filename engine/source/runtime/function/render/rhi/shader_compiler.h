#pragma once
#include "shader.h"

class DxcException : public std::exception
{
public:
    DxcException(HRESULT ErrorCode) : ErrorCode(ErrorCode) {}

    const char* GetErrorType() const noexcept;
    std::string GetError() const;

private:
    const HRESULT ErrorCode;
};

struct ShaderCompilationResult
{
    Microsoft::WRL::ComPtr<IDxcBlob> CompiledShaderBlob;
    std::wstring                     CompiledShaderPdbName;
    Microsoft::WRL::ComPtr<IDxcBlob> CompiledShaderPdb;
    DxcShaderHash                    ShaderHash;
    Microsoft::WRL::ComPtr<IDxcBlob> RootSignatureBlob;
};

class ShaderCompiler
{
public:
    ShaderCompiler();

    void SetShaderModel(RHI_SHADER_MODEL ShaderModel) noexcept;

    [[nodiscard]] Shader CompileShader(RHI_SHADER_TYPE              ShaderType,
                                       const std::filesystem::path& Path,
                                       const ShaderCompileOptions&  Options) const;

    [[nodiscard]] Library CompileLibrary(const std::filesystem::path& Path) const;

private:
    [[nodiscard]] std::wstring GetShaderModelString() const;

    [[nodiscard]] std::wstring ShaderProfileString(RHI_SHADER_TYPE ShaderType) const;

    [[nodiscard]] std::wstring LibraryProfileString() const;

    [[nodiscard]] ShaderCompilationResult Compile(const std::filesystem::path&  Path,
                                                  std::wstring_view             EntryPoint,
                                                  std::wstring_view             Profile,
                                                  const std::vector<DxcDefine>& ShaderDefines,
                                                  bool ExtractRootSignature = false) const;

private:
    Microsoft::WRL::ComPtr<IDxcCompiler3>      Compiler3;
    Microsoft::WRL::ComPtr<IDxcUtils>          Utils;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> DefaultIncludeHandler;
    RHI_SHADER_MODEL                           ShaderModel;
};
