#include <mi/mdl_sdk.h>
#include <mi/base/miwindows.h>

#include "core/dev_util.h"
#include "core/type.h"

HMODULE g_dso_handle = 0;

inline mi::neuraylib::INeuray* load_and_get_ineuray(const char* filename = nullptr)
{
    if (!filename)
        filename = "libmdl_sdk" MI_BASE_DLL_FILE_EXT;


    HMODULE handle = LoadLibraryA(filename);
    if (!handle) {
        // fall back to libraries in a relative lib folder, relevant for install targets
        std::string fallback = std::string("../../../lib/") + filename;
        handle = LoadLibraryA(fallback.c_str());
    }
    if (!handle) {
        LPTSTR buffer = 0;
        LPCTSTR message = TEXT("unknown failure");
        DWORD error_code = GetLastError();
        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS, 0, error_code,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buffer, 0, 0))
            message = buffer;
        fprintf(stderr, "Failed to load %s library (%u): %ls",
            filename, soul::cast<uint32>(error_code), message);
        if (buffer)
            LocalFree(buffer);
        return 0;
    }
    void* symbol = GetProcAddress(handle, "mi_factory");
    if (!symbol) {
        LPTSTR buffer = 0;
        LPCTSTR message = TEXT("unknown failure");
        DWORD error_code = GetLastError();
        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS, 0, error_code,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buffer, 0, 0))
            message = buffer;
        fprintf(stderr, "GetProcAddress error (%u): %ls", soul::cast<uint32>(error_code), message);
        if (buffer)
            LocalFree(buffer);
        return 0;
    }

    g_dso_handle = handle;

    mi::neuraylib::INeuray* neuray = mi::neuraylib::mi_factory<mi::neuraylib::INeuray>(symbol);
    if (!neuray)
    {
        mi::base::Handle<mi::neuraylib::IVersion> version(
            mi::neuraylib::mi_factory<mi::neuraylib::IVersion>(symbol));
        if (!version)
            fprintf(stderr, "Error: Incompatible library.\n");
        else
            fprintf(stderr, "Error: Library version %s does not match header version %s.\n",
                version->get_product_version(), MI_NEURAYLIB_PRODUCT_VERSION_STRING);
        return 0;
    }
    return neuray;
}

bool unload_neuray()
{
    BOOL result = FreeLibrary(g_dso_handle);
    if (!result) {
        LPTSTR buffer = 0;
        LPCTSTR message = TEXT("unknown failure");
        DWORD error_code = GetLastError();
        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS, 0, error_code,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buffer, 0, 0))
            message = buffer;
        fprintf(stderr, "Failed to unload library (%u): %ls", error_code, message);
        if (buffer)
            LocalFree(buffer);
        return false;
    }
    return true;
}

int main()
{
    // Get the INeuray interface in a suitable smart pointer.
    mi::base::Handle<mi::neuraylib::INeuray> neuray(load_and_get_ineuray());
    if (!neuray.is_valid_interface())
        SOUL_EXIT_FAILURE("Error: The MDL SDK library failed to load and to provide "
            "the mi::neuraylib::INeuray interface.");

    mi::base::Handle<mi::neuraylib::IMdl_configuration> mdl_config(
        neuray->get_api_component<mi::neuraylib::IMdl_configuration>());

    mdl_config->add_mdl_path();

    mi::Sint32 result = neuray->start(true);
    if (result != 0)
        SOUL_EXIT_FAILURE("Failed to initialize the SDK. Result code: %d", result);

    // scene graph manipulations and rendering calls go here, none in this example.
    // ...
    // Shutting the MDL SDK down in blocking mode. Again, a return code of 0 indicates success.
    if (neuray->shutdown(true) != 0)
        ("Failed to shutdown the SDK.");
    // Unload the MDL SDK
    neuray = nullptr; // free the handles that holds the INeuray instance
    if (!unload_neuray())
        SOUL_EXIT_FAILURE("Failed to unload the SDK.");
    return 0;
}