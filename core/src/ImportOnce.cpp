#include <IAHO/ImportOnce.hpp>

#include <IAHO/LegacyIni.hpp>
#include <PalCfg/File.hpp>

namespace IAHO
{
    ImportResult ImportOnce(ConfigFile& File, const std::string& IniPath)
    {
        ImportResult Result;
        std::string Text;
        if (PalCfg::StatFile(File.Path()).exists || !PalCfg::ReadFileText(IniPath, Text))
        {
            return Result;
        }

        Result.Imported = true;
        Result.Values = ImportIni(Text);
        Result.Saved = File.Save(Result.Values);
        return Result;
    }
}
