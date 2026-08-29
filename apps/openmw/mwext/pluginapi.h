#ifndef TES3MP_MWEXT_PLUGINAPI_H
#define TES3MP_MWEXT_PLUGINAPI_H

#include <stdint.h>

#if defined(_WIN32)
#  define MWEXT_CALL __cdecl
#  if defined(__cplusplus)
#    define MWEXT_EXPORT extern "C" __declspec(dllexport)
#  else
#    define MWEXT_EXPORT __declspec(dllexport)
#  endif
#else
#  define MWEXT_CALL
#  if defined(__cplusplus)
#    define MWEXT_EXPORT extern "C" __attribute__((visibility("default")))
#  else
#    define MWEXT_EXPORT __attribute__((visibility("default")))
#  endif
#endif

#define MWEXT_ABI_VERSION 1u
#define MWEXT_PLUGIN_ENTRYPOINT "mwext_plugin_init"

typedef enum MWExtLogLevel
{
    MWEXT_LOG_DEBUG = 0,
    MWEXT_LOG_INFO = 1,
    MWEXT_LOG_WARNING = 2,
    MWEXT_LOG_ERROR = 3
} MWExtLogLevel;

typedef struct MWExtVec3
{
    float x;
    float y;
    float z;
} MWExtVec3;

/*
 * ABI v1 is intentionally tiny. New host functions are appended to the end of
 * this struct in later ABI-compatible revisions; plugins must check structSize.
 */
typedef struct MWExtHostApiV1
{
    uint32_t abiVersion;
    uint32_t structSize;
    void* hostContext;

    void (MWEXT_CALL *log)(void* hostContext, int level, const char* message);
    int (MWEXT_CALL *getPlayerPosition)(void* hostContext, MWExtVec3* outPosition);
} MWExtHostApiV1;

typedef struct MWExtPluginV1
{
    uint32_t abiVersion;
    uint32_t structSize;

    const char* id;
    const char* name;
    void* pluginContext;

    void (MWEXT_CALL *onFrame)(void* pluginContext, float dt);
    void (MWEXT_CALL *onUnload)(void* pluginContext);
} MWExtPluginV1;

typedef int (MWEXT_CALL *MWExtPluginInitFn)(const MWExtHostApiV1* host, MWExtPluginV1* plugin);

#endif