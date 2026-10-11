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
#pragma once

#include <source2toolkit/IToolkitApi.h>

#include <memory>
#include <string>
#include <vector>

struct ToolkitPlugin;

class PluginApi : public IToolkitAPI
{
public:
    /// The plugin whose Load() is running: ToolkitFactory records what it
    /// asks for and names it in its messages. Null outside a Load().
    static void SetLoadingPlugin(ToolkitPlugin* plugin);

    void Log(IToolkitPlugin *plugin, const char* msg, ...) override;
    void ConPrint(const char* msg) override;
    void ConPrintf(const char* fmt, ...) override;

    void AddListener(IToolkitPlugin *plugin, IToolkitListener *pListener) override;

    void* QueryInterface(CreateInterfaceFn fn, const char *iface, int min) override;
    void* InterfaceSearch(CreateInterfaceFn fn, const char *iface, int max, int *ret) override;
    int FormatIface(char iface[], size_t maxlength) override;

    CreateInterfaceFn GetEngineFactory(bool syn=true) override;
    CreateInterfaceFn GetPhysicsFactory(bool syn=true) override;
    CreateInterfaceFn GetFileSystemFactory(bool syn=true) override;
    CreateInterfaceFn GetServerFactory(bool syn=true) override;

    void* ToolkitFactory(const char *iface, int *ret, PluginId *id) override;
    void* MetaFactory(const char *iface, int *ret, PluginId *id) override;

    IToolkitAddresses* Addresses() override;
    IToolkitCommands* Commands() override;
    IToolkitConVars* ConVars() override;
    IToolkitCustomHud* CustomHud() override;
    IToolkitEntities* Entities() override;
    IToolkitEvents* Events() override;
    IToolkitGameConfig* GameConfig() override;
    IToolkitGameSystems* GameSystems() override;
    IToolkitHTTP* HTTP() override;
    IToolkitMenus* Menus() override;
    IToolkitMySQL* MySQL() override;
    IToolkitNetworkMessages* NetworkMessages() override;
    IToolkitScheduler* Scheduler() override;
    IToolkitSounds* Sounds() override;
    IToolkitTrace* Trace() override;
    IToolkitTransmit* Transmit() override;
    IToolkitGameHooks* GameHooks() override;
    IToolkitPaths* Paths() override;

    IToolkitModule* LoadModule(const char* name) override;
    IToolkitModule* LoadModuleFromMemory(uintptr_t ptr) override;
    void FreeModule(IToolkitModule* module) override;

    ISource2Server* GetSource2Server() override;
    ISource2ServerConfig* GetSource2ServerConfig() override;
    ISource2GameClients* GetSource2GameClients() override;
    ISource2GameEntities* GetSource2GameEntities() override;
    IVEngineServer2* GetEngineServer() override;
    ICvar* GetCVar() override;
    CSchemaSystem* GetSchemaSystem() override;
    IGameResourceService* GetGameResourceService() override;
    INetworkServerService* GetNetworkServerService() override;
    INetworkSystem* GetNetworkSystem() override;
    INetworkMessages* GetNetworkMessages() override;
    INetworkStringTableContainer* GetNetworkStringTableServer() override;
    IEngineServiceMgr* GetEngineServiceMgr() override;
    IHostStateMgr* GetHostStateMgr() override;
    ILocalize* GetLocalize() override;
    IFileSystem* GetFileSystem() override;

    IGameEventSystem* GetGameEventSystem() override;
    IGameEventManager2* GetGameEventManager() override;
    CGameEntitySystem* GetEntitySystem() override;
    CGlobalVars* GetGlobalVars() override;

    const char* GetBaseDir() override;

    size_t Format(char *buffer, size_t maxlength, const char *format, ...) override;
    size_t FormatArgs(char *buffer, size_t maxlength, const char *format, va_list ap) override;
};

extern PluginApi pluginApi;

/// Points the SDK's toolkit globals (g_ToolkitAPI, g_pToolkitAddresses, ...)
/// at the core's own interfaces. Call once the game config exists: the
/// game-config interface is the only one that is not a static object.
void FillToolkitGlobals();