// A plugin library: a shared object loaded into the compiler, whose providers register as its
// static objects are constructed (MC4 §3, `library`). Its references bind to the names the compiler
// offers first -- the SDK's, MSA's, the C and C++ runtime's (on Linux mcxx is a -static-pie program
// exporting them, and openkal-musl's dlopen binds a loaded object to them) -- so what it registers
// joins this process's catalog, and the copies of those libraries it carries are never reached.
module;

#include <dlfcn.h>

module mcxx.plugin.host;

import std;
import mcxx.base;
import mcxx.plugin;

namespace mcxx::plugin::host {

std::string load_library(std::string_view name, const std::filesystem::path& file) {
    const std::string path { file.generic_string() };
    // What it registers while it loads is held until its SDK is known (MC4-3-7).
    hold_registrations();
    // RTLD_LAZY: the copy of the C library a library built by mcpp carries names compiler builtins
    // it never calls, which RTLD_NOW would refuse; a function nothing defines ends the program if
    // it is ever called. RTLD_LOCAL: one plugin's names are not another's.
    void* handle { ::dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL) };
    if (handle == nullptr) {
        (void)release_registrations(false);
        const char* why { ::dlerror() };
        return std::format("plugin {} ({}) could not be loaded: {}; nothing it gates was checked", name, path, why != nullptr ? why : "no reason given");
    }
    const auto* abi { static_cast<const int*>(::dlsym(handle, "mcxx_plugin_sdk_abi")) };
    if (abi == nullptr) {
        (void)release_registrations(false);
        return std::format("plugin {} ({}) is not a plugin library: it was not built against the plugin SDK (no mcxx_plugin_sdk_abi); "
                           "nothing it gates was checked", name, path);
    }
    if (*abi != SDK_ABI) {
        (void)release_registrations(false);
        return std::format("plugin {} ({}) was built against plugin SDK ABI {} and this compiler's is {}: rebuild it against this "
                           "compiler's SDK; nothing it gates was checked", name, path, *abi, SDK_ABI);
    }
    if (release_registrations(true) == 0)
        return std::format("plugin {} ({}) registered no provider with this compiler: its references reached its own copy of the SDK, "
                           "which happens when the compiler does not offer its names (on Linux, mcxx linked -static-pie with "
                           "--export-dynamic); nothing it gates was checked", name, path);
    base::trace::count("plugins.loaded");
    return {};
}

} // namespace mcxx::plugin::host
