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

#include <engine2/inetworkserverservice.h>
#include <engine2/isource2server.h>
#include <engine2/ivengineserver2.h>
#include <engine2/networkgameserverbase.h>
#include <ISmmPluginExt.h>

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
// WinSock2.h first: Windows.h otherwise pulls in the older winsock.h, and a
// translation unit that includes this header and then core/mysql.h (which
// needs WinSock2.h) gets every socket type and function defined twice.
#include <WinSock2.h>
#include <Windows.h>
using LibHandle = HMODULE;
#else
#include <dlfcn.h>
using LibHandle = void*;
#endif

class IToolkitPlugin;
class IToolkitListener;

struct ToolkitPlugin
{
    int id;
    std::string path;
    LibHandle lib;
    // Where the library is mapped: a handler's code address in it makes the
    // handler this plugin's (PluginManager::OwnerOf).
    const void* moduleBase = nullptr;
    IToolkitPlugin* api;
    // What the plugin was built against (TOOLKIT_PLAPI_VERSION then): the
    // engine callbacks came with 2, so a listener from an older plugin is
    // not asked for them.
    int apiVersion;
    // The toolkit interfaces the plugin asked ToolkitFactory for while it
    // loaded, revision and all -- what "toolkit info" shows.
    std::vector<std::string> ifaces;
    std::vector<IToolkitListener*> listeners;
};

class PluginManager
{
public:
    bool IsPluginLoaded(const std::string& fullPath);

    bool LoadPlugin(const char* path, char* error, size_t maxlen);

    // Public because the deferred reload in pluginmanager.cpp runs from a
    // next-frame task rather than from inside the manager.
    bool LoadPluginFromPath(const char* fullPath, char* error, size_t maxlen, bool hotReload);

    bool UnloadPlugin(int id);
    bool ReloadPlugin(int id);

    bool LoadMissing();

    bool LoadAll();
    void UnloadAll();

    void StartFileWatcher();
    void StopFileWatcher();

    void SetAllLoaded();
    void FireMetamodLoaded();

    void AddListener(IToolkitPlugin* plugin, IToolkitListener* listener);
public:
    // Metamod's own plugin events, not the toolkit's. Named apart because they
    // carry a Metamod plugin id, which is a different numbering from the one
    // OnPluginLoad/OnPluginUnload report for .stx plugins.
    void FireMetamodPluginLoaded(SourceMM::PluginId id);
    void FireMetamodPluginUnloaded(SourceMM::PluginId id);
    void OnLevelInit(char const* pMapName, char const* pMapEntities, char const* pOldLevel, char const* pLandmarkName, bool loadGame, bool background);
    void OnLevelShutdown();

    // Engine callbacks (plugin API 2), fanned out from virtualhooks.
    void OnGameFrame(bool simulating, bool firstTick, bool lastTick);
    void OnStartupServer(const GameSessionConfiguration_t& config, ISource2WorldSession* session, const char* mapName);
    void OnClientPutInServer(CPlayerSlot slot, const char* name, int type, uint64 xuid);
    void OnClientVoice(CPlayerSlot slot);
    void OnClientSettingsChanged(CPlayerSlot slot);
    void OnClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason reason, const char* name, uint64 xuid, const char* networkId);
    void OnGameServerSteamAPIActivated();
    void OnGameServerSteamAPIDeactivated();
    void OnLoadEventsFromFile(class IGameEventManager2* manager, const char* filename, bool searchAll);
    void OnClientAuthorized(CPlayerSlot slot, uint64 steamId);
    void OnClientAuthorizeFailed(CPlayerSlot slot, uint64 steamId);
    void OnPermissionsChanged(uint64 steamId);

    /// Deferred unload / reload. A plugin cannot take its own KHook hooks
    /// down from inside a dispatch of the hooked function (the capsule's
    /// shared lock is held for the whole pre -> original -> post run and
    /// RemoveHook wants it exclusively), and GameFrame is the function most
    /// plugins hook. So nothing here unloads from a game callback: the
    /// request is queued and "toolkit _pending" is pushed into the server
    /// command buffer, which the engine runs between frames, outside every
    /// hooked function. RequestUnload is false when no plugin has that id.
    bool RequestUnload(int id);
    void RequestReload(const std::string& fullPath);

    /// Runs the queued requests; only from the "toolkit _pending" command.
    void RunPending();

    /// The plugin's GetName(), or a placeholder for an id that is not loaded.
    const char* NameOf(int id) const;

    /// Whose code an address is: the plugin whose library it lies in, or 0
    /// (the core, or no plugin) -- how every toolkit call that takes a handler
    /// finds its owner (ToolkitCallback::Origin()).
    int OwnerOf(const void* address);
public:
    std::vector<std::unique_ptr<ToolkitPlugin>> m_plugins;
    int m_nextId = 1;
public:
    bool ReloadPluginByPath(const std::string& fullPath);
private:

    std::thread m_watcherThread;
    std::atomic<bool> m_stopWatcher{false};

    // Set once LoadAll() has run; every plugin loaded after that point is a
    // late load and is told so through IToolkitPlugin::Load's `late`.
    bool m_bStartupLoadDone = false;

    // OwnerOf(): module base per address seen, dropped when the list changes.
    std::unordered_map<const void*, int> m_ownerCache;
    size_t m_listGeneration = 0;        // bumped on every add / remove
    size_t m_ownerCacheGeneration = 0;

    std::vector<int> m_unloadRequests;
    std::vector<std::string> m_reloadRequests;
    bool m_bPendingQueued = false;

    void QueuePending();
};

extern PluginManager pluginManager;