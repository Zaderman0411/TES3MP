#include "pluginmanager.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>

#include <boost/filesystem.hpp>

#include <SDL_loadso.h>

#include <components/debug/debuglog.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/ptr.hpp"

namespace
{
    const char* safeString(const char* value)
    {
        return value ? value : "<unnamed>";
    }
}

MWExt::PluginManager::PluginManager(const boost::filesystem::path& pluginDirectory)
    : mPluginDirectory(pluginDirectory)
{
    mHostApi.abiVersion = MWEXT_ABI_VERSION;
    mHostApi.structSize = sizeof(MWExtHostApiV1);
    mHostApi.hostContext = this;
    mHostApi.log = &PluginManager::hostLog;
    mHostApi.getPlayerPosition = &PluginManager::hostGetPlayerPosition;
}

MWExt::PluginManager::~PluginManager()
{
    unloadAll();
}

bool MWExt::PluginManager::isPluginLibrary(const boost::filesystem::path& path) const
{
    if (!boost::filesystem::is_regular_file(path))
        return false;

    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

#if defined(_WIN32)
    return extension == ".dll";
#elif defined(__APPLE__)
    return extension == ".dylib" || extension == ".so";
#else
    return extension == ".so";
#endif
}

void MWExt::PluginManager::loadAll()
{
    if (!boost::filesystem::exists(mPluginDirectory))
    {
        boost::system::error_code ec;
        boost::filesystem::create_directories(mPluginDirectory, ec);
        if (ec)
        {
            Log(Debug::Warning) << "MWExt: could not create client plugin directory '"
                                << mPluginDirectory.string() << "': " << ec.message();
            return;
        }
    }

    Log(Debug::Info) << "MWExt: scanning client plugins in " << mPluginDirectory.string();

    std::vector<boost::filesystem::path> candidates;
    for (boost::filesystem::directory_iterator it(mPluginDirectory), end; it != end; ++it)
    {
        if (isPluginLibrary(it->path()))
            candidates.push_back(it->path());
    }

    std::sort(candidates.begin(), candidates.end());
    for (const boost::filesystem::path& path : candidates)
        loadOne(path);

    Log(Debug::Info) << "MWExt: loaded " << mPlugins.size() << " client plugin(s)";
}

bool MWExt::PluginManager::loadOne(const boost::filesystem::path& path)
{
    void* handle = SDL_LoadObject(path.string().c_str());
    if (!handle)
    {
        Log(Debug::Warning) << "MWExt: failed loading '" << path.string() << "': " << SDL_GetError();
        return false;
    }

    void* symbol = SDL_LoadFunction(handle, MWEXT_PLUGIN_ENTRYPOINT);
    if (!symbol)
    {
        Log(Debug::Warning) << "MWExt: '" << path.filename().string()
                            << "' has no " << MWEXT_PLUGIN_ENTRYPOINT << " entry point";
        SDL_UnloadObject(handle);
        return false;
    }

    MWExtPluginInitFn init = reinterpret_cast<MWExtPluginInitFn>(symbol);
    MWExtPluginV1 plugin;
    std::memset(&plugin, 0, sizeof(plugin));

    int initialized = 0;
    try
    {
        initialized = init(&mHostApi, &plugin);
    }
    catch (...)
    {
        Log(Debug::Error) << "MWExt: plugin threw an exception during initialization: " << path.string();
    }

    if (!initialized)
    {
        Log(Debug::Warning) << "MWExt: plugin initialization failed: " << path.string();
        SDL_UnloadObject(handle);
        return false;
    }

    if (plugin.abiVersion != MWEXT_ABI_VERSION || plugin.structSize < sizeof(MWExtPluginV1))
    {
        Log(Debug::Warning) << "MWExt: incompatible plugin ABI in '" << path.filename().string()
                            << "' (plugin ABI " << plugin.abiVersion
                            << ", host ABI " << MWEXT_ABI_VERSION << ")";
        SDL_UnloadObject(handle);
        return false;
    }

    LoadedPlugin loaded;
    loaded.handle = handle;
    loaded.api = plugin;
    loaded.path = path.string();
    mPlugins.push_back(loaded);

    Log(Debug::Info) << "MWExt: loaded client plugin " << safeString(plugin.name)
                     << " [" << safeString(plugin.id) << "]";
    return true;
}

void MWExt::PluginManager::onFrame(float dt)
{
    for (LoadedPlugin& plugin : mPlugins)
    {
        if (!plugin.api.onFrame)
            continue;

        try
        {
            plugin.api.onFrame(plugin.api.pluginContext, dt);
        }
        catch (...)
        {
            Log(Debug::Error) << "MWExt: exception escaped onFrame from " << safeString(plugin.api.name);
        }
    }
}

void MWExt::PluginManager::unloadAll()
{
    for (auto it = mPlugins.rbegin(); it != mPlugins.rend(); ++it)
    {
        if (it->api.onUnload)
        {
            try
            {
                it->api.onUnload(it->api.pluginContext);
            }
            catch (...)
            {
                Log(Debug::Error) << "MWExt: exception escaped onUnload from " << safeString(it->api.name);
            }
        }

        if (it->handle)
            SDL_UnloadObject(it->handle);
    }
    mPlugins.clear();
}

void MWEXT_CALL MWExt::PluginManager::hostLog(void*, int level, const char* message)
{
    switch (level)
    {
        case MWEXT_LOG_DEBUG:   Log(Debug::Debug) << "MWExt plugin: " << safeString(message); break;
        case MWEXT_LOG_WARNING: Log(Debug::Warning) << "MWExt plugin: " << safeString(message); break;
        case MWEXT_LOG_ERROR:   Log(Debug::Error) << "MWExt plugin: " << safeString(message); break;
        case MWEXT_LOG_INFO:
        default:                Log(Debug::Info) << "MWExt plugin: " << safeString(message); break;
    }
}

int MWEXT_CALL MWExt::PluginManager::hostGetPlayerPosition(void*, MWExtVec3* outPosition)
{
    if (!outPosition)
        return 0;

    MWBase::World* world = MWBase::Environment::get().getWorld();
    if (!world)
        return 0;

    const MWWorld::Ptr player = world->getPlayerPtr();
    if (player.isEmpty())
        return 0;

    const auto& position = player.getRefData().getPosition();
    outPosition->x = position.pos[0];
    outPosition->y = position.pos[1];
    outPosition->z = position.pos[2];
    return 1;
}