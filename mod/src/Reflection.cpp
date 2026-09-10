#include "Reflection.hpp"

#include <Unreal/CoreUObject/UObject/UnrealType.hpp>

#include "Log.hpp"

namespace IAHO
{
    using namespace RC;
    using namespace RC::Unreal;

    auto FindClass(const CharType* FullClassPath) -> UClass*
    {
        return UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, FullClassPath);
    }

    auto FindClassDefaultObject(const CharType* FullClassPath) -> UObject*
    {
        auto* Class = FindClass(FullClassPath);
        if (!Class)
        {
            return nullptr;
        }
        return Class->GetClassDefaultObject();
    }

    auto FindFunction(const CharType* FullFunctionPath) -> UFunction*
    {
        return UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, FullFunctionPath);
    }

    FuncCall::~FuncCall()
    {
        ReleaseParams();
    }

    auto FuncCall::Bind(UObject* Object, const CharType* FunctionName) -> bool
    {
        ReleaseParams();
        m_Object = nullptr;
        m_Function = nullptr;

        if (!Object)
        {
            return false;
        }

        auto* Function = Object->GetFunctionByNameInChain(FunctionName);
        if (!Function)
        {
            return false;
        }

        m_Object = Object;
        m_Function = Function;
        m_Params.assign(Function->GetParmsSize(), 0);

        // Out params such as TArray or FString must be live objects before ProcessEvent touches them.
        for (auto* Property : TFieldRange<FProperty>(m_Function, EFieldIterationFlags::None))
        {
            if (!Property->HasAllPropertyFlags(CPF_Parm))
            {
                continue;
            }
            Property->InitializeValue_InContainer(m_Params.data());
        }
        return true;
    }

    auto FuncCall::ReleaseParams() -> void
    {
        if (!m_Function || m_Params.empty())
        {
            m_Params.clear();
            return;
        }

        for (auto* Property : TFieldRange<FProperty>(m_Function, EFieldIterationFlags::None))
        {
            if (!Property->HasAllPropertyFlags(CPF_Parm))
            {
                continue;
            }
            Property->DestroyValue_InContainer(m_Params.data());
        }
        m_Params.clear();
    }

    auto FuncCall::PropertyOf(const CharType* ParameterName) -> FProperty*
    {
        if (!m_Function || m_Params.empty())
        {
            return nullptr;
        }

        auto* Property = m_Function->FindProperty(FName{ParameterName, FNAME_Add});
        if (!Property)
        {
            Log<LogLevel::Error>(STR("Parameter '{}' not found on '{}'\n"), ParameterName, m_Function->GetName());
        }
        return Property;
    }

    auto FuncCall::Raw(const CharType* ParameterName) -> void*
    {
        auto* Property = PropertyOf(ParameterName);
        if (!Property)
        {
            return nullptr;
        }
        return m_Params.data() + static_cast<size_t>(Property->GetOffset_Internal());
    }

    auto FuncCall::AddressOf(const CharType* ParameterName, size_t ExpectedSize) -> void*
    {
        auto* Property = PropertyOf(ParameterName);
        if (!Property)
        {
            return nullptr;
        }

        const auto Offset = static_cast<size_t>(Property->GetOffset_Internal());
        const auto Size = static_cast<size_t>(Property->GetElementSize());
        if (Size != ExpectedSize || Offset + Size > m_Params.size())
        {
            Log<LogLevel::Error>(STR("Parameter '{}' size mismatch on '{}': game {} bytes at {}, mod {} bytes\n"),
                                 ParameterName,
                                 m_Function->GetName(),
                                 Size,
                                 Offset,
                                 ExpectedSize);
            return nullptr;
        }
        return m_Params.data() + Offset;
    }

    auto FuncCall::Call() -> bool
    {
        if (!m_Object || !m_Function)
        {
            return false;
        }
        m_Object->ProcessEvent(m_Function, m_Params.data());
        return true;
    }
} // namespace IAHO
