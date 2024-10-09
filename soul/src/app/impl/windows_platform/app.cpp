#include "app/app.h"

#include <Shlobj.h>
#include <Windows.h>
#include <stringapiset.h>
#include <winnls.h>

namespace soul::app
{
  void App::init_storage_path()
  {
    PWSTR roaming_file_path = nullptr;
    HRESULT result =
      SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &roaming_file_path);
    SOUL_ASSERT(0, result == S_OK);
    i32 length =
      WideCharToMultiByte(CP_UTF8, 0, roaming_file_path, -1, nullptr, 0, nullptr, nullptr);
    char* output = new char[length];
    WideCharToMultiByte(CP_UTF8, 0, roaming_file_path, -1, output, length, nullptr, nullptr);
    storage_path_ = Path::From(StringView(output, strlen(output)));
    delete[] output;
    storage_path_ /= name_.cspan();

    if (!std::filesystem::exists(storage_path_))
    {
      std::filesystem::create_directory(storage_path_);
    }
  }
} // namespace soul::app
