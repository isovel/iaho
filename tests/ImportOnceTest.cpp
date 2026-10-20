#include <unistd.h>
#include <IAHO/ImportOnce.hpp>

#include <filesystem>
#include <fstream>
#include <string>

#include <PalCfg/File.hpp>

#include <catch2/catch_test_macros.hpp>

namespace
{
    // A fresh directory per test, removed on the way out.
    struct TempDir
    {
        std::filesystem::path Path;

        TempDir()
        {
            static int Counter = 0;
            Path = std::filesystem::temp_directory_path() / ("iaho-import-" + std::to_string(::getpid()) + "-" + std::to_string(++Counter));
            std::filesystem::create_directories(Path);
        }
        ~TempDir() { std::filesystem::remove_all(Path); }

        std::string File(const char* Name) const { return (Path / Name).string(); }
    };

    void Write(const std::string& Path, const std::string& Text)
    {
        std::ofstream(Path, std::ios::binary) << Text;
    }
}

TEST_CASE("an ini beside an existing json is ignored")
{
    TempDir Dir;
    Write(Dir.File("config.json"), R"({"enabled": true})");
    Write(Dir.File("config.ini"), "Enabled = false");
    IAHO::ConfigFile File(IAHO::kFields, Dir.File("config.json"));

    const auto Result = IAHO::ImportOnce(File, Dir.File("config.ini"));

    CHECK_FALSE(Result.Imported);
    std::string Json;
    REQUIRE(PalCfg::ReadFileText(Dir.File("config.json"), Json));
    CHECK(Json == R"({"enabled": true})");
}

TEST_CASE("an ini with no json is saved as the json and put live")
{
    TempDir Dir;
    Write(Dir.File("config.ini"), "Enabled = false\nNeverHideRecipes = A");
    IAHO::ConfigFile File(IAHO::kFields, Dir.File("config.json"));

    const auto Result = IAHO::ImportOnce(File, Dir.File("config.ini"));

    CHECK(Result.Imported);
    CHECK(Result.Saved);
    CHECK(PalCfg::StatFile(Dir.File("config.json")).exists);
    CHECK_FALSE(File.Get()->Enabled);
    CHECK(File.Get()->NeverHideRecipes.contains(L"A"));
}

TEST_CASE("a json that cannot be written still hands back the imported values")
{
    TempDir Dir;
    Write(Dir.File("config.ini"), "Enabled = false");
    // A file where the json's folder should be, so no folder can be made there.
    Write(Dir.File("blocker"), "");
    IAHO::ConfigFile File(IAHO::kFields, (Dir.Path / "blocker" / "config.json").string());

    const auto Result = IAHO::ImportOnce(File, Dir.File("config.ini"));

    CHECK(Result.Imported);
    CHECK_FALSE(Result.Saved);
    CHECK_FALSE(Result.Values.Enabled);
}

TEST_CASE("no ini imports nothing")
{
    TempDir Dir;
    IAHO::ConfigFile File(IAHO::kFields, Dir.File("config.json"));

    CHECK_FALSE(IAHO::ImportOnce(File, Dir.File("config.ini")).Imported);
    CHECK_FALSE(PalCfg::StatFile(Dir.File("config.json")).exists);
}
