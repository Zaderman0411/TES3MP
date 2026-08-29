#ifndef OPENMW_MWEXT_PLUGINMANAGER_H
#define OPENMW_MWEXT_PLUGINMANAGER_H

#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>

#include "pluginapi.h"

namespace MWExt
{
    class PluginManager
    {
        public:
            explicit PluginManager(const boost::filesystem::path& pluginDirectory);
            ~PluginManager();

            PluginManager(const PluginManager&) = delete;
            PluginManager& operator=(const PluginManager&) = delete;

            void loadAll();
            void onFrame(float dt);

        private:
            struct LoadedPlugin
            {
                void* handle = nullptr;
                MWExtPluginV1 api{};
                std::string path;
            };

            static void MWEXT_CALL hostLog(void* context, int level, const char* message);
            static int MWEXT_CALL hostGetPlayerPosition(void* context, MWExtVec3* outPosition);

            bool loadOne(const boost::filesystem::path& path);
            void unloadAll();
            bool isPluginLibrary(const boost::filesystem::path& path) const;

            boost::filesystem::path mPluginDirectory;
            MWExtHostApiV1 mHostApi{};
            std::vector<LoadedPlugin> mPlugins;
    };
}

#endif