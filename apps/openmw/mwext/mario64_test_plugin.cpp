#include "pluginapi.h"

#include <cstdio>
#include <cstring>

namespace
{
    struct Mario64TestState
    {
        const MWExtHostApiV1* host = nullptr;
        float timer = 0.f;
    };

    Mario64TestState gState;

    void MWEXT_CALL onFrame(void* context, float dt)
    {
        Mario64TestState* state = static_cast<Mario64TestState*>(context);
        if (!state || !state->host)
            return;

        state->timer += dt;
        if (state->timer < 3.f)
            return;
        state->timer = 0.f;

        MWExtVec3 position{};
        if (!state->host->getPlayerPosition
            || !state->host->getPlayerPosition(state->host->hostContext, &position))
            return;

        char message[256];
        std::snprintf(message, sizeof(message),
            "[Mario64.dll] addon callback alive; player=(%.2f, %.2f, %.2f)",
            position.x, position.y, position.z);
        state->host->log(state->host->hostContext, MWEXT_LOG_INFO, message);
    }

    void MWEXT_CALL onUnload(void* context)
    {
        Mario64TestState* state = static_cast<Mario64TestState*>(context);
        if (state && state->host && state->host->log)
            state->host->log(state->host->hostContext, MWEXT_LOG_INFO,
                "[Mario64.dll] unloaded cleanly");
    }
}

MWEXT_EXPORT int MWEXT_CALL mwext_plugin_init(const MWExtHostApiV1* host, MWExtPluginV1* plugin)
{
    if (!host || !plugin || host->abiVersion != MWEXT_ABI_VERSION
        || host->structSize < sizeof(MWExtHostApiV1))
        return 0;

    gState.host = host;
    gState.timer = 0.f;

    std::memset(plugin, 0, sizeof(*plugin));
    plugin->abiVersion = MWEXT_ABI_VERSION;
    plugin->structSize = sizeof(MWExtPluginV1);
    plugin->id = "org.tes3mp.mario64.test";
    plugin->name = "Mario64 Addon Architecture Test";
    plugin->pluginContext = &gState;
    plugin->onFrame = &onFrame;
    plugin->onUnload = &onUnload;

    if (host->log)
        host->log(host->hostContext, MWEXT_LOG_INFO,
            "[Mario64.dll] loaded through MWExt ABI v1; no engine movement changes active");

    return 1;
}