#include "Config.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

#include "Log.hpp"

namespace IAHO
{
    using RC::File::StringType;

    namespace
    {
        auto Trim(StringType Value) -> StringType
        {
            const auto NotSpace = [](RC::CharType C) {
                return C != STR(' ') && C != STR('\t') && C != STR('\r') && C != STR('\n');
            };
            Value.erase(Value.begin(), std::find_if(Value.begin(), Value.end(), NotSpace));
            Value.erase(std::find_if(Value.rbegin(), Value.rend(), NotSpace).base(), Value.end());
            return Value;
        }

        auto ToLower(StringType Value) -> StringType
        {
            std::transform(Value.begin(), Value.end(), Value.begin(), [](RC::CharType C) {
                return static_cast<RC::CharType>(std::towlower(C));
            });
            return Value;
        }

        auto IsTruthy(const StringType& Value) -> bool
        {
            const auto Lowered = ToLower(Value);
            return Lowered == STR("1") || Lowered == STR("true") || Lowered == STR("yes") || Lowered == STR("on");
        }
    } // namespace

    auto Config::ParseIdList(const StringType& Value, std::unordered_set<StringType>& Out) -> void
    {
        StringType Current{};
        for (const auto Char : Value)
        {
            if (Char == STR(',') || Char == STR(';'))
            {
                auto Id = Trim(Current);
                if (!Id.empty())
                {
                    Out.insert(std::move(Id));
                }
                Current.clear();
                continue;
            }
            Current.push_back(Char);
        }
        auto Id = Trim(Current);
        if (!Id.empty())
        {
            Out.insert(std::move(Id));
        }
    }

    auto Config::Load(const std::filesystem::path& ModDirectory) -> void
    {
        const auto ConfigPath = ModDirectory / "config.ini";
        std::wifstream Stream{ConfigPath};
        if (!Stream.is_open())
        {
            Log(STR("No config.ini at {}, using defaults\n"), ConfigPath.wstring());
            return;
        }

        StringType Line{};
        while (std::getline(Stream, Line))
        {
            Line = Trim(Line);
            if (Line.empty() || Line.front() == STR(';') || Line.front() == STR('#') || Line.front() == STR('['))
            {
                continue;
            }

            const auto Separator = Line.find(STR('='));
            if (Separator == StringType::npos)
            {
                continue;
            }

            const auto Key = ToLower(Trim(Line.substr(0, Separator)));
            const auto Value = Trim(Line.substr(Separator + 1));

            if (Key == STR("enabled"))
            {
                m_Enabled = IsTruthy(Value);
            }
            else if (Key == STR("loglevel"))
            {
                const auto Lowered = ToLower(Value);
                if (Lowered == STR("quiet"))
                {
                    m_Verbosity = LogVerbosity::Quiet;
                }
                else if (Lowered == STR("discovery"))
                {
                    m_Verbosity = LogVerbosity::Discovery;
                }
                else
                {
                    m_Verbosity = LogVerbosity::Normal;
                }
            }
            else if (Key == STR("extrahiddenrecipes"))
            {
                ParseIdList(Value, m_ExtraHidden);
            }
            else if (Key == STR("neverhiderecipes"))
            {
                ParseIdList(Value, m_NeverHidden);
            }
        }

        Log(STR("Config: Enabled={} LogLevel={} ExtraHidden={} NeverHide={}\n"),
            m_Enabled,
            static_cast<int32_t>(m_Verbosity),
            m_ExtraHidden.size(),
            m_NeverHidden.size());
    }

    auto Config::IsForceHidden(const RC::Unreal::FName& ProductId) const -> bool
    {
        if (m_ExtraHidden.empty())
        {
            return false;
        }
        return m_ExtraHidden.contains(RC::Unreal::FName{ProductId}.ToString());
    }

    auto Config::IsNeverHidden(const RC::Unreal::FName& ProductId) const -> bool
    {
        if (m_NeverHidden.empty())
        {
            return false;
        }
        return m_NeverHidden.contains(RC::Unreal::FName{ProductId}.ToString());
    }

    auto GetConfig() -> Config&
    {
        static Config Instance{};
        return Instance;
    }
} // namespace IAHO
