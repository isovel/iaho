#include <IAHO/LegacyIni.hpp>

#include <string>
#include <unordered_set>

#include <catch2/catch_test_macros.hpp>

using IAHO::ImportIni;
using IAHO::LogVerbosity;
using Ids = std::unordered_set<std::wstring>;

namespace
{
    // mod/payload/config.ini as it shipped before config.json replaced it.
    constexpr const char* kShipped = R"INI(
; I Already Have One
;
; Hides crafting recipes whose product is already sitting in the player's
; key item (Essential) inventory. Consumables kept there, such as dungeon keys,
; stay craftable.

[IAlreadyHaveOne]

; Master switch. Set to false to restore vanilla crafting menus without
; removing the mod.
Enabled = true

; quiet     - errors only
; normal    - lifecycle plus one line per recipe hidden
; discovery - full per-call tracing of the recipe list and every hide decision,
;             then set it back to normal
LogLevel = normal

; Comma-separated item ids to hide once owned even if the rule above would
; not catch them.
ExtraHiddenRecipes =

; Comma-separated item ids that must always stay craftable.
NeverHideRecipes =
)INI";
}

TEST_CASE("an empty file imports the defaults")
{
    const auto S = ImportIni("");
    CHECK(S.Enabled);
    CHECK(S.Verbosity == LogVerbosity::Normal);
    CHECK(S.ExtraHiddenRecipes.empty());
    CHECK(S.NeverHideRecipes.empty());
}

TEST_CASE("the shipped config.ini imports the defaults")
{
    const auto S = ImportIni(kShipped);
    CHECK(S.Enabled);
    CHECK(S.Verbosity == LogVerbosity::Normal);
    CHECK(S.ExtraHiddenRecipes.empty());
    CHECK(S.NeverHideRecipes.empty());
}

TEST_CASE("keys match without regard to case")
{
    CHECK_FALSE(ImportIni("ENABLED = false").Enabled);
    CHECK(ImportIni("loglevel = quiet").Verbosity == LogVerbosity::Quiet);
}

TEST_CASE("enabled reads the truthy spellings")
{
    for (const char* Yes : {"1", "true", "YES", "on"})
    {
        CHECK(ImportIni(std::string("Enabled = ") + Yes).Enabled);
    }
    for (const char* No : {"0", "no", "off", "false", "maybe"})
    {
        CHECK_FALSE(ImportIni(std::string("Enabled = ") + No).Enabled);
    }
}

TEST_CASE("logLevel maps quiet and discovery, anything else to normal")
{
    CHECK(ImportIni("LogLevel = quiet").Verbosity == LogVerbosity::Quiet);
    CHECK(ImportIni("LogLevel = Discovery").Verbosity == LogVerbosity::Discovery);
    CHECK(ImportIni("LogLevel = verbose").Verbosity == LogVerbosity::Normal);
    CHECK(ImportIni("LogLevel =").Verbosity == LogVerbosity::Normal);
}

TEST_CASE("id lists split on commas and semicolons and trim each id")
{
    const auto S = ImportIni("NeverHideRecipes = A; B ,C,,\nExtraHiddenRecipes=Lamp");
    CHECK(S.NeverHideRecipes == Ids{L"A", L"B", L"C"});
    CHECK(S.ExtraHiddenRecipes == Ids{L"Lamp"});
}

TEST_CASE("comments, sections and lines without '=' are skipped")
{
    const auto S = ImportIni("; Enabled = false\n# Enabled = false\n[Enabled = false]\njunk\n  ; LogLevel = quiet");
    CHECK(S.Enabled);
    CHECK(S.Verbosity == LogVerbosity::Normal);
}

TEST_CASE("CRLF endings and a UTF-8 BOM are tolerated")
{
    const auto S = ImportIni("\xEF\xBB\xBF" "Enabled = false\r\nLogLevel = quiet\r\nNeverHideRecipes = A\r\n");
    CHECK_FALSE(S.Enabled);
    CHECK(S.Verbosity == LogVerbosity::Quiet);
    CHECK(S.NeverHideRecipes == Ids{L"A"});
}

TEST_CASE("a later line overrides an earlier one for scalars and adds to lists")
{
    const auto S = ImportIni("Enabled = false\nEnabled = true\nLogLevel = quiet\nLogLevel = discovery\n"
                             "NeverHideRecipes = A\nNeverHideRecipes = B");
    CHECK(S.Enabled);
    CHECK(S.Verbosity == LogVerbosity::Discovery);
    CHECK(S.NeverHideRecipes == Ids{L"A", L"B"});
}
