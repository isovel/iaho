#include <IAHO/LegacyIni.hpp>
#include <IAHO/Settings.hpp>

#include <string>
#include <unordered_set>

#include <PalCfg/Load.hpp>
#include <PalCfg/Write.hpp>

#include <catch2/catch_test_macros.hpp>

using Ids = std::unordered_set<std::wstring>;

TEST_CASE("the schema loads every key from JSON")
{
    IAHO::Settings S;
    const auto R = PalCfg::LoadFromText(
        IAHO::kFields,
        R"({"enabled": false, "logLevel": "discovery", "extraHiddenRecipes": ["A"], "neverHideRecipes": "B; C"})",
        S);
    CHECK_FALSE(S.Enabled);
    CHECK(S.Verbosity == IAHO::LogVerbosity::Discovery);
    CHECK(S.ExtraHiddenRecipes == Ids{L"A"});
    CHECK(S.NeverHideRecipes == Ids{L"B", L"C"});
    CHECK(R.unknownKeys.empty());
}

TEST_CASE("keys load without regard to case")
{
    IAHO::Settings S;
    PalCfg::LoadFromText(IAHO::kFields, R"({"Enabled": false, "LOGLEVEL": "quiet"})", S);
    CHECK_FALSE(S.Enabled);
    CHECK(S.Verbosity == IAHO::LogVerbosity::Quiet);
}

TEST_CASE("an imported config renders and loads back unchanged")
{
    const auto Imported = IAHO::ImportIni("Enabled = false\nLogLevel = discovery\nExtraHiddenRecipes = A, B\nNeverHideRecipes = C");
    IAHO::Settings Loaded;
    PalCfg::LoadFromText(IAHO::kFields, PalCfg::RenderDocument(IAHO::kFields, Imported), Loaded);
    CHECK(Loaded.Enabled == Imported.Enabled);
    CHECK(Loaded.Verbosity == Imported.Verbosity);
    CHECK(Loaded.ExtraHiddenRecipes == Imported.ExtraHiddenRecipes);
    CHECK(Loaded.NeverHideRecipes == Imported.NeverHideRecipes);
}
