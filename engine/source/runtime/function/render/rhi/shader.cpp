#include "shader.h"

Shader::Shader(RHI_SHADER_TYPE                  ShaderType,
               DxcShaderHash                    ShaderHash,
               Microsoft::WRL::ComPtr<IDxcBlob> CompiledShaderBlob,
               Microsoft::WRL::ComPtr<IDxcBlob> CompiledShaderPdb) :
    ShaderType(ShaderType),
    ShaderHash(ShaderHash), CompiledShaderBlob(CompiledShaderBlob), CompiledShaderPdb(CompiledShaderPdb)
{}

void* Shader::GetPointer() const noexcept { return CompiledShaderBlob->GetBufferPointer(); }

size_t Shader::GetSize() const noexcept { return CompiledShaderBlob->GetBufferSize(); }

DxcShaderHash Shader::GetShaderHash() const noexcept { return ShaderHash; }

Library::Library(DxcShaderHash                    ShaderHash,
                 Microsoft::WRL::ComPtr<IDxcBlob> CompiledLibraryBlob,
                 Microsoft::WRL::ComPtr<IDxcBlob> CompiledLibraryPdb) :
    ShaderHash(ShaderHash),
    CompiledLibraryBlob(CompiledLibraryBlob), CompiledLibraryPdb(CompiledLibraryPdb)
{}

void* Library::GetPointer() const noexcept { return CompiledLibraryBlob->GetBufferPointer(); }

size_t Library::GetSize() const noexcept { return CompiledLibraryBlob->GetBufferSize(); }

DxcShaderHash Library::GetShaderHash() const noexcept { return ShaderHash; }
