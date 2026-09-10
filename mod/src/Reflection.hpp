#pragma once

#include <cstdint>
#include <vector>

#include <File/Macros.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/NameTypes.hpp>
#include <Unreal/UObjectGlobals.hpp>

namespace IAHO
{
    // Resolves a class by its full script path, e.g. STR("/Script/Pal.PalUtility").
    auto FindClass(const RC::CharType* FullClassPath) -> RC::Unreal::UClass*;

    // Class default object of a class path. Blueprint function libraries are called on their CDO.
    auto FindClassDefaultObject(const RC::CharType* FullClassPath) -> RC::Unreal::UObject*;

    // Resolves a UFunction by its full script path, e.g. STR("/Script/Pal.PalUIUtility:FilteringWorkSpaceRecipe").
    auto FindFunction(const RC::CharType* FullFunctionPath) -> RC::Unreal::UFunction*;

    /**
     * Calls a reflected UFunction with its parameter block addressed by parameter name rather than by
     * a hand-written struct, so a layout change in a game patch cannot silently corrupt the call.
     * Every parameter is constructed and destroyed through its own FProperty.
     */
    class FuncCall
    {
      public:
        FuncCall() = default;
        ~FuncCall();

        FuncCall(const FuncCall&) = delete;
        auto operator=(const FuncCall&) -> FuncCall& = delete;

        // Binds by name against the object's class chain. Returns false when the function does not exist.
        auto Bind(RC::Unreal::UObject* Object, const RC::CharType* FunctionName) -> bool;

        auto IsBound() const -> bool
        {
            return m_Function != nullptr;
        }

        template <typename ParamType>
        auto Set(const RC::CharType* ParameterName, const ParamType& Value) -> bool
        {
            auto* Address = AddressOf(ParameterName, sizeof(ParamType));
            if (!Address)
            {
                return false;
            }
            *static_cast<ParamType*>(Address) = Value;
            return true;
        }

        template <typename ParamType>
        auto Get(const RC::CharType* ParameterName, ParamType& OutValue) -> bool
        {
            auto* Address = AddressOf(ParameterName, sizeof(ParamType));
            if (!Address)
            {
                return false;
            }
            OutValue = *static_cast<ParamType*>(Address);
            return true;
        }

        // Executes against the object the call was bound to.
        auto Call() -> bool;

        // Raw parameter address, for reading a field out of a returned USTRUCT by reflected offset.
        auto Raw(const RC::CharType* ParameterName) -> void*;

      private:
        auto AddressOf(const RC::CharType* ParameterName, size_t ExpectedSize) -> void*;
        auto PropertyOf(const RC::CharType* ParameterName) -> RC::Unreal::FProperty*;
        auto ReleaseParams() -> void;

        RC::Unreal::UObject* m_Object{};
        RC::Unreal::UFunction* m_Function{};
        std::vector<uint8_t> m_Params{};
    };
} // namespace IAHO
