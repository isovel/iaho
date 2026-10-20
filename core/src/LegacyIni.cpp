#include <IAHO/LegacyIni.hpp>

#include <string>

#include <PalCfg/Utf.hpp>

namespace IAHO
{
    namespace
    {
        std::string_view Trim(std::string_view Value)
        {
            constexpr std::string_view kSpace = " \t\r\n";
            const auto First = Value.find_first_not_of(kSpace);
            if (First == std::string_view::npos) return {};
            return Value.substr(First, Value.find_last_not_of(kSpace) - First + 1);
        }

        std::string ToLower(std::string_view Value)
        {
            std::string Lowered(Value);
            for (char& C : Lowered)
            {
                if (C >= 'A' && C <= 'Z') C = static_cast<char>(C - 'A' + 'a');
            }
            return Lowered;
        }

        bool IsTruthy(std::string_view Value)
        {
            const std::string Lowered = ToLower(Value);
            return Lowered == "1" || Lowered == "true" || Lowered == "yes" || Lowered == "on";
        }

        void AddIds(std::string_view Value, std::unordered_set<std::wstring>& Out)
        {
            while (!Value.empty())
            {
                const auto Separator = Value.find_first_of(",;");
                const std::string_view Id = Trim(Value.substr(0, Separator));
                if (!Id.empty()) Out.insert(PalCfg::Utf8ToWide(Id));
                if (Separator == std::string_view::npos) break;
                Value.remove_prefix(Separator + 1);
            }
        }
    }

    Settings ImportIni(std::string_view Text)
    {
        if (Text.starts_with("\xEF\xBB\xBF")) Text.remove_prefix(3);

        Settings Result;
        while (!Text.empty())
        {
            const auto End = Text.find('\n');
            const std::string_view Line = Trim(Text.substr(0, End));
            Text.remove_prefix(End == std::string_view::npos ? Text.size() : End + 1);

            if (Line.empty() || Line.front() == ';' || Line.front() == '#' || Line.front() == '[') continue;

            const auto Separator = Line.find('=');
            if (Separator == std::string_view::npos) continue;

            const std::string Key = ToLower(Trim(Line.substr(0, Separator)));
            const std::string_view Value = Trim(Line.substr(Separator + 1));

            if (Key == "enabled")
            {
                Result.Enabled = IsTruthy(Value);
            }
            else if (Key == "loglevel")
            {
                const std::string Level = ToLower(Value);
                Result.Verbosity = Level == "quiet"       ? LogVerbosity::Quiet
                                   : Level == "discovery" ? LogVerbosity::Discovery
                                                          : LogVerbosity::Normal;
            }
            else if (Key == "extrahiddenrecipes")
            {
                AddIds(Value, Result.ExtraHiddenRecipes);
            }
            else if (Key == "neverhiderecipes")
            {
                AddIds(Value, Result.NeverHideRecipes);
            }
        }
        return Result;
    }
}
