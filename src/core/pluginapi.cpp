/**
* vim: set ts=4 sw=4 tw=99 noet:
 * =============================================================================
 * Source2Toolkit
 * Copyright (C) 2025-2026 Michal "Slynx (˙·٠● S l y n x ●٠·˙)" Přikryl.
 * All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, Michal "Slynx (˙·٠● S l y n x ●٠·˙)" Přikryl
 * gives you permission to link the code of this program
 * (as well as its derivative works) to "Counter-Strike 2," "Source 2,"
 * "Steam," and any Game MODs or server software running on software by
 * Valve Corporation. You must obey the GNU General Public License in all
 * respects for all other code used.
 *
 * Additionally, this exception applies to all derivative works unless
 * otherwise stated in LICENSE.txt.
 *
 * Authors:
 *   - Michal "Slynx (˙·٠● S l y n x ●٠·˙)" Přikryl
 *
 * Project: Source2Toolkit
 */
#include "pluginapi.h"

#include "addresses.h"
#include "commands.h"
#include "convars.h"
#include "core/paths.h"
#include "core/scheduler.h"
#include "customhud.h"
#include "entities.h"
#include "events.h"
#include "gamehooks.h"
#include "gamesystems.h"
#include "http.h"
#include "json.h"
#include "menus.h"
#include "module.h"
#include "mysql.h"
#include "networkmessages.h"
#include "permissions.h"
#include "plugin.h"
#include "pluginmanager.h"
#include "raytrace.h"
#include "scripts.h"
#include "sounds.h"
#include "transmit.h"
#include "utils/log.h"

#include <source2toolkit/IToolkitAddresses.h>
#include <source2toolkit/IToolkitCommands.h>
#include <source2toolkit/IToolkitConVars.h>
#include <source2toolkit/IToolkitCustomHud.h>
#include <source2toolkit/IToolkitEntities.h>
#include <source2toolkit/IToolkitEvents.h>
#include <source2toolkit/IToolkitGameConfig.h>
#include <source2toolkit/IToolkitGameSystems.h>
#include <source2toolkit/IToolkitHTTP.h>
#include <source2toolkit/IToolkitMenus.h>
#include <source2toolkit/IToolkitMySQL.h>
#include <source2toolkit/IToolkitNetworkMessages.h>
#include <source2toolkit/IToolkitPlugin.h>
#include <source2toolkit/IToolkitScheduler.h>
#include <source2toolkit/IToolkitSounds.h>
#include <source2toolkit/IToolkitTrace.h>
#include <source2toolkit/IToolkitTransmit.h>

#include <cstring>
#include <stdarg.h>

PluginApi pluginApi;

// The same globals a plugin gets from TOOLKIT_EXPOSE, so the SDK code linked
// into the core (the schema helpers, ADDR_*, ...) reads them exactly as it
// does inside a plugin. Filled by FillToolkitGlobals().
TOOLKIT_DEFINE_GLOBALVARS()

void FillToolkitGlobals()
{
    g_ToolkitAPI = &pluginApi;
    TOOLKIT_FILLVARS(&pluginApi)
}

void PluginApi::Log(IToolkitPlugin* plugin, const char* msg, ...)
{
    va_list ap;
    char buffer[2048];

    va_start(ap, msg);
    FormatArgs(buffer, sizeof(buffer), msg, ap);
    va_end(ap);

    ConPrintf("[%s] %s", plugin->GetName(), buffer);
}

void PluginApi::ConPrint(const char* msg)
{
    g_SMAPI->ConPrint(msg);
}

void PluginApi::ConPrintf(const char* fmt, ...)
{
    va_list ap;
    char buffer[2048];

    va_start(ap, fmt);
    FormatArgs(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);

    g_SMAPI->ConPrint(buffer);
}

void PluginApi::AddListener(IToolkitPlugin* plugin, IToolkitListener* pListener)
{
    pluginManager.AddListener(plugin, pListener);
}

void* PluginApi::QueryInterface(CreateInterfaceFn fn, const char* iface, int min)
{
    char buffer[256]; /* assume no interface will go beyond this */
    size_t len = strlen(iface);
    int ret; /* just in case something doesn't handle NULL properly */

    if (len > sizeof(buffer) - 4)
    {
        return NULL;
    }

    strcpy(buffer, iface);

    if (min != -1)
    {
        char* ptr = &buffer[len - 1];
        int digits = 0;
        while (isdigit(*ptr) && digits <= 3)
        {
            *ptr = '\0';
            digits++;
            ptr--;
        }
        if (digits != 3)
        {
            /* for now, assume this is an error */
            strcpy(buffer, iface);
        }
        else
        {
            char num[4];
            min = (min == 0) ? 1 : min;
            snprintf(num, sizeof(num), "%03d", min);
            strcat(buffer, num);
        }
    }

    return InterfaceSearch(fn, buffer, IFACE_MAXNUM, &ret);
}

void* PluginApi::InterfaceSearch(CreateInterfaceFn fn, const char* iface, int max, int* ret)
{
    char _if[256]; /* assume no interface goes beyond this */
    size_t len = strlen(iface);
    int num = 0;
    void* pf = NULL;

    if (max > 999)
    {
        max = 999;
    }

    if (len + 4 > sizeof(_if))
    {
        if (ret)
        {
            *ret = META_IFACE_FAILED;
        }
        return NULL;
    }

    strcpy(_if, iface);

    do
    {
        if ((pf = (fn)(_if, ret)) != NULL)
        {
            break;
        }
        if (num > max)
        {
            break;
        }
    }
    while ((num = FormatIface(_if, len + 1)));

    return pf;
}

int PluginApi::FormatIface(char iface[], size_t maxlength)
{
    size_t length = strlen(iface);
    size_t i;
    int num = 0;

    for (i = length - 1; i + 1 > 0; i--)
    {
        if (!isdigit(iface[i]))
        {
            if (i != length - 1)
            {
                num = 1;
            }
            break;
        }
    }

    if ((num && (maxlength <= length)) || (!num && (maxlength <= length + 3)))
    {
        return -1;
    }

    if (i != length - 1)
    {
        num = atoi(&(iface[++i]));
    }

    num++;

    snprintf(&(iface[i]), 4, "%03d", num);

    return num;
}

CreateInterfaceFn PluginApi::GetEngineFactory(bool syn/* =true */)
{
    return g_SMAPI->GetEngineFactory(syn);
}

CreateInterfaceFn PluginApi::GetPhysicsFactory(bool syn/* =true */)
{
    return g_SMAPI->GetPhysicsFactory(syn);
}

CreateInterfaceFn PluginApi::GetFileSystemFactory(bool syn/* =true */)
{
    return g_SMAPI->GetFileSystemFactory(syn);
}

CreateInterfaceFn PluginApi::GetServerFactory(bool syn/* =true */)
{
    return g_SMAPI->GetServerFactory(syn);
}

static ToolkitPlugin* s_pLoadingPlugin = nullptr;

void PluginApi::SetLoadingPlugin(ToolkitPlugin* plugin)
{
    s_pLoadingPlugin = plugin;
}

namespace
{
    struct CurrentInterface
    {
        const char* iface;
        void* ptr;
    };

    // Every toolkit interface at its current revision. Built on first use,
    // after the core's Load() filled the pointers in.
    const CurrentInterface* CurrentInterfaces(size_t& count)
    {
        static const CurrentInterface s_current[] = {
            { TOOLKIT_ADDRESSES_INTERFACE, &addresses::toolkitAddresses },
            { TOOLKIT_COMMANDS_INTERFACE, &commands::commandsManager },
            { TOOLKIT_CONVARS_INTERFACE, &convars::convarsManager },
            { TOOLKIT_CUSTOMHUD_INTERFACE, &customhud::customHudManager },
            { TOOLKIT_ENTITIES_INTERFACE, &entities::entitiesManager },
            { TOOLKIT_EVENTS_INTERFACE, &events::eventManager },
            { TOOLKIT_GAMECONFIG_INTERFACE, shared::g_pGameConfig },
            { TOOLKIT_GAMESYSTEMS_INTERFACE, &gamesystems::gameSystemsManager },
            { TOOLKIT_GAMEHOOKS_INTERFACE, &gamehooks::gameHooksManager },
            { TOOLKIT_HTTP_INTERFACE, &http::httpManager },
            { TOOLKIT_JSON_INTERFACE, &json::jsonManager },
            { TOOLKIT_MENUS_INTERFACE, &menus::menuManager },
            { TOOLKIT_MYSQL_INTERFACE, &mysql::mysqlManager },
            { TOOLKIT_NETWORKMESSAGES_INTERFACE, &networkmessages::networkMessagesManager },
            { TOOLKIT_SCHEDULER_INTERFACE, &scheduler::schedulerManager },
            { TOOLKIT_SCRIPTS_INTERFACE, &scripts::scriptsManager },
            { TOOLKIT_SOUNDS_INTERFACE, &sounds::soundsManager },
            { TOOLKIT_TRACE_INTERFACE, &raytrace::rayTrace },
            { TOOLKIT_TRANSMIT_INTERFACE, &transmit::transmitManager },
            { TOOLKIT_PATHS_INTERFACE, &paths::pathsManager },
            { TOOLKIT_PERMISSIONS_INTERFACE, &permissions::permissionsManager },
        };
        count = sizeof(s_current) / sizeof(s_current[0]);
        return s_current;
    }

    // "IToolkitMenus002" -> revision 2, and the length of the name before it.
    bool SplitRevision(const char* iface, size_t& nameLen, int& revision)
    {
        const size_t len = strlen(iface);
        if (len < 4)
            return false;
        for (size_t i = len - 3; i < len; i++)
            if (iface[i] < '0' || iface[i] > '9')
                return false;
        nameLen = len - 3;
        revision = atoi(iface + nameLen);
        return true;
    }
}

void* PluginApi::ToolkitFactory(const char* iface, int* ret, PluginId* id)
{
    void* ptr = nullptr;
    const char* who = s_pLoadingPlugin && s_pLoadingPlugin->api ? s_pLoadingPlugin->api->GetName() : "A plugin";

    // The one detour engine on the server, as metamod handed it to the toolkit.
    if (!strcmp(iface, TOOLKIT_KHOOK_INTERFACE)) ptr = KHook::__exported__khook;
    // ... and which KHook that is: the commit the core was compiled against
    // (metamod's third_party/khook, the engine itself -- see the SDK's
    // CMakeLists.txt). A plugin's KHOOK_INIT() compares it with the commit
    // of the SDK's vendor/khook it was built from and refuses to load on a
    // mismatch.
    else if (!strcmp(iface, TOOLKIT_KHOOK_VERSION_INTERFACE))
    {
        static const char s_khookCommit[] = TOOLKIT_KHOOK_COMMIT;
        ptr = const_cast<char*>(s_khookCommit);
    }
    else
    {
        size_t count = 0;
        const CurrentInterface* current = CurrentInterfaces(count);

        for (size_t i = 0; i < count && !ptr; i++)
        {
            if (!strcmp(iface, current[i].iface))
                ptr = current[i].ptr;
        }

        // Not the current revision: the same interface at another one? The
        // name carries the revision ("IToolkitMenus002"); the C++ name
        // never changes, so the plugin asks by the string it was built with.
        // The answer says which side has to move.
        size_t nameLen = 0;
        int wanted = 0;
        if (!ptr && SplitRevision(iface, nameLen, wanted))
        {
            for (size_t i = 0; i < count; i++)
            {
                size_t curLen = 0;
                int have = 0;
                if (!SplitRevision(current[i].iface, curLen, have) || curLen != nameLen || strncmp(iface, current[i].iface, nameLen) != 0)
                    continue;

                if (wanted < have)
                    FP_ERROR("{} was built against {}, this core serves {}: rebuild the plugin against the current SDK", who, iface, current[i].iface);
                else
                    FP_ERROR("{} asks for {}, newer than this core's {}: update the core", who, iface, current[i].iface);
                break;
            }
        }
    }

    if (ptr)
    {
        if (s_pLoadingPlugin)
            s_pLoadingPlugin->ifaces.push_back(iface);
        if (ret) *ret = TOOLKIT_IFACE_OK;
        return ptr;
    }

    for (auto& p : pluginManager.m_plugins)
    {
        for (auto* l : p->listeners)
        {
            if (auto res = l->OnToolkitQuery(iface, ret))
            {
                if (id) *id = p->id;
                return res;
            }
        }
    }

    if (ret) *ret = TOOLKIT_IFACE_FAILED;
    return nullptr;
}

void* PluginApi::MetaFactory(const char* iface, int* ret, PluginId* id)
{
    return g_SMAPI->MetaFactory(iface, ret, id);
}

IToolkitAddresses* PluginApi::Addresses()
{
    return &addresses::toolkitAddresses;
}

IToolkitCommands* PluginApi::Commands()
{
    return &commands::commandsManager;
}

IToolkitConVars* PluginApi::ConVars()
{
    return &convars::convarsManager;
}

IToolkitCustomHud* PluginApi::CustomHud()
{
    return &customhud::customHudManager;
}

IToolkitSounds* PluginApi::Sounds()
{
    return &sounds::soundsManager;
}

IToolkitGameHooks* PluginApi::GameHooks()
{
    return &gamehooks::gameHooksManager;
}

IToolkitTransmit* PluginApi::Transmit()
{
    return &transmit::transmitManager;
}

IToolkitEntities* PluginApi::Entities()
{
    return &entities::entitiesManager;
}

IToolkitEvents* PluginApi::Events()
{
    return &events::eventManager;
}

IToolkitGameConfig* PluginApi::GameConfig()
{
    return shared::g_pGameConfig;
}

IToolkitGameSystems* PluginApi::GameSystems()
{
    return &gamesystems::gameSystemsManager;
}

IToolkitHTTP* PluginApi::HTTP()
{
    return &http::httpManager;
}

IToolkitMenus* PluginApi::Menus()
{
    return &menus::menuManager;
}

IToolkitMySQL* PluginApi::MySQL()
{
    return &mysql::mysqlManager;
}

IToolkitNetworkMessages* PluginApi::NetworkMessages()
{
    return &networkmessages::networkMessagesManager;
}

IToolkitScheduler* PluginApi::Scheduler()
{
    return &scheduler::schedulerManager;
}

IToolkitTrace* PluginApi::Trace()
{
    return &raytrace::rayTrace;
}

IToolkitPaths* PluginApi::Paths()
{
    return &paths::pathsManager;
}

IToolkitModule* PluginApi::LoadModule(const char* name)
{
    return new ToolkitModule(name);
}

IToolkitModule* PluginApi::LoadModuleFromMemory(uintptr_t ptr)
{
    return new ToolkitModule(ptr);
}

void PluginApi::FreeModule(IToolkitModule* module)
{
    delete module;
}

ISource2Server* PluginApi::GetSource2Server()
{
    return g_pSource2Server;
}

ISource2ServerConfig* PluginApi::GetSource2ServerConfig()
{
    return g_pSource2ServerConfig;
}

ISource2GameClients* PluginApi::GetSource2GameClients()
{
    return g_pSource2GameClients;
}

ISource2GameEntities* PluginApi::GetSource2GameEntities()
{
    return g_pSource2GameEntities;
}

IVEngineServer2* PluginApi::GetEngineServer()
{
    return g_pEngineServer;
}

ICvar* PluginApi::GetCVar()
{
    return g_pCVar;
}

// interfaces.h only knows the interface type; plugins want the concrete class,
// so the downcast happens here rather than in every plugin.
CSchemaSystem* PluginApi::GetSchemaSystem()
{
    return shared::g_pSchemaSystem;
}

IGameResourceService* PluginApi::GetGameResourceService()
{
    return g_pGameResourceServiceServer;
}

INetworkServerService* PluginApi::GetNetworkServerService()
{
    return g_pNetworkServerService;
}

INetworkSystem* PluginApi::GetNetworkSystem()
{
    return g_pNetworkSystem;
}

INetworkMessages* PluginApi::GetNetworkMessages()
{
    return g_pNetworkMessages;
}

INetworkStringTableContainer* PluginApi::GetNetworkStringTableServer()
{
    return g_pNetworkStringTableServer;
}

IEngineServiceMgr* PluginApi::GetEngineServiceMgr()
{
    return g_pEngineServiceMgr;
}

IHostStateMgr* PluginApi::GetHostStateMgr()
{
    return g_pHostStateMgr;
}

ILocalize* PluginApi::GetLocalize()
{
    return g_pLocalize;
}

IFileSystem* PluginApi::GetFileSystem()
{
    return g_pFullFileSystem;
}

IGameEventSystem* PluginApi::GetGameEventSystem()
{
    return shared::g_pGameEventSystem;
}

IGameEventManager2* PluginApi::GetGameEventManager()
{
    return shared::g_pGameEventManager;
}

CGameEntitySystem* PluginApi::GetEntitySystem()
{
    return shared::g_pEntitySystem;
}

CGlobalVars* PluginApi::GetGlobalVars()
{
    return shared::getGlobalVars();
}

const char* PluginApi::GetBaseDir()
{
    return g_SMAPI->GetBaseDir();
}

size_t PluginApi::Format(char* buffer, size_t maxlength, const char* format, ...)
{
    va_list ap;
    size_t result;

    va_start(ap, format);
    result = FormatArgs(buffer, maxlength, format, ap);
    va_end(ap);

    return result;
}

size_t PluginApi::FormatArgs(char* buffer, size_t maxlength, const char* format, va_list ap)
{
    return g_SMAPI->FormatArgs(buffer, maxlength, format, ap);
}
