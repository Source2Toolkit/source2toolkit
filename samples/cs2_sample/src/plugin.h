/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * ======================================================
 * Source2Toolkit Sample Plugin
 * ======================================================
 *
 * One plugin that walks through the toolkit, a section at a time. It started
 * as a port of Metamod:Source's own s2_sample_mm -- the same hooks, the same
 * convars, the same command, the same level callbacks -- and grew a section
 * for every subsystem a plugin usually reaches for first:
 *
 *   1. Lifecycle        Load / Unload / IToolkitListener
 *   2. ConVars          CConVar, CConVarRef, change hook
 *   3. Commands         console + chat commands, arguments, replies, listeners
 *   4. Game events      pre / post hooks, dontBroadcast, firing an event
 *   5. Core events      entity listener, entity outputs, client callbacks,
 *                       GameFrame; a raw KHook where the toolkit has nothing
 *   6. Net messages     building + sending, outgoing and incoming hooks
 *   7. Sounds           one-call emit, sound objects, stopping, the sound hook
 *   8. Game functions   game hooks (TakeDamage, PostThink), calling by signature
 *   9. Entities         schema fields, teleport, items, inputs
 *  10. Timers           next frame, delayed, repeating
 *  11. Transmit         hiding players and entities per viewer, CheckTransmit hook
 *
 * plugin.cpp carries the same numbers in its section banners. Each section is
 * a SetupXxx() called from Load() followed by the handlers it registers, so a
 * section can be lifted out into a plugin of its own as it stands.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * This sample plugin is public domain.
 */

#ifndef _INCLUDE_SOURCE2TOOLKIT_SAMPLE_PLUGIN_H_
#define _INCLUDE_SOURCE2TOOLKIT_SAMPLE_PLUGIN_H_

#include "source2toolkit/IToolkitPlugin.h"
#include "source2toolkit/IToolkitApi.h"

// TOOLKIT_SAVEVARS() fills one global per subsystem, so every subsystem header
// has to be in scope for its interface name.
#include "source2toolkit/IToolkitAddresses.h"
#include "source2toolkit/IToolkitCommands.h"
#include "source2toolkit/IToolkitConVars.h"
#include "source2toolkit/IToolkitCustomHud.h"
#include "source2toolkit/IToolkitEntities.h"
#include "source2toolkit/IToolkitEvents.h"
#include "source2toolkit/IToolkitGameConfig.h"
#include "source2toolkit/IToolkitGameHooks.h"
#include "source2toolkit/IToolkitGameSystems.h"
#include "source2toolkit/IToolkitHTTP.h"
#include "source2toolkit/IToolkitJSON.h"
#include "source2toolkit/IToolkitMemory.h"
#include "source2toolkit/IToolkitMenus.h"
#include "source2toolkit/IToolkitModule.h"
#include "source2toolkit/IToolkitMySQL.h"
#include "source2toolkit/IToolkitNetworkMessages.h"
#include "source2toolkit/IToolkitScheduler.h"
#include "source2toolkit/IToolkitSounds.h"
#include "source2toolkit/IToolkitTrace.h"

#include "source2toolkit/schema/entity/classes/CBaseEntity.h"
#include "source2toolkit/schema/entity/classes/CCSPlayerPawn.h"
#include "source2toolkit/schema/entityio.h"
#include "source2toolkit/schema/takedamageinfo.h"
#include "source2toolkit/schema/takedamageresult.h"
#include "entity2/entitysystem.h"
#include "igameevents.h"
#include "engine2/isource2server.h"
#include "engine2/ivengineserver2.h"

// Generated from plugin-metadata.json by tools/version_gen.py -- the metadata
// below is not written twice.
#include "version_gen.h"

// Above the class: the hook targets below read g_pSource2GameClients,
// g_pToolkitAddresses and g_pServerModule, so the globals have to be declared
// first.
TOOLKIT_GLOBALVARS();

// libserver, for a pattern of our own (8. game functions). Loaded in Load()
// before KHOOK_INIT(), freed in Unload().
extern IToolkitModule *g_pServerModule;

class SamplePlugin final : public IToolkitPlugin,
                           public IToolkitListener,
                           public IEntityListener,
                           public IEntityIOListener
{
public:
	bool Load(PluginId id, IToolkitAPI *api, char *error, size_t maxlen, bool late) override;
	bool Unload(char *error, size_t maxlen) override;

public: // 1. lifecycle -- IToolkitListener
	void OnAllToolkitPluginsLoaded() override;
	void OnLevelInit(const char *pMapName, const char *pMapEntities, const char *pOldLevel, const char *pLandmarkName, bool loadGame, bool background) override;
	void OnLevelShutdown() override;

public: // 5. core events -- IToolkitListener, IEntityListener, IEntityIOListener
	void OnClientPutInServer(CPlayerSlot slot, const char *pszName, int type, uint64 xuid) override;
	void OnClientSettingsChanged(CPlayerSlot slot) override;
	void OnClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason reason, const char *pszName, uint64 xuid, const char *pszNetworkID) override;
	void OnGameFrame(bool simulating, bool bFirstTick, bool bLastTick) override;

	void OnEntityCreated(CEntityInstance *pEntity) override;
	void OnEntitySpawned(CEntityInstance *pEntity) override;
	void OnEntityDeleted(CEntityInstance *pEntity) override;
	Action OnEntityOutput(const char *pchOutputName, CEntityInstance *pActivator, CEntityInstance *pCaller, float flDelay, bool post) override;

public: // 5. core events -- raw KHook handlers, for what the toolkit has no callback for
	KHook::Return<bool> Hook_ClientConnect(ISource2GameClients *pThis, CPlayerSlot slot, const char *pszName, uint64 xuid, const char *pszNetworkID, bool unk1, CBufferString *pRejectReason);
	KHook::Return<void> Hook_ClientCommand(ISource2GameClients *pThis, CPlayerSlot nSlot, const CCommand &cmd);

public: // 8. game functions -- IToolkitGameHooks handlers, and a raw KHook by signature
	GameHookReturn<TakeDamageContext::Return> OnTakeDamage(TakeDamageContext &ctx, bool post);
	KHook::Return<CBasePlayerWeapon *> Hook_GiveNamedItem(CCSPlayer_ItemServices *pThis, const char *pszItem, int nSubType, CEconItemView *pScriptItem, bool bForce, void *a6);
	KHook::Return<void> Hook_SwitchTeam(CCSPlayerController *pThis, int nTeam);
	KHook::Return<void> Hook_UtilRemove(CEntityInstance *pEntity);
	KHook::Return<void> Hook_SnapViewAngles(CBasePlayerPawn *pThis, QAngle *pAngles);

private: // one per section of plugin.cpp, called from Load() in this order
	void SetupConVars();
	void SetupCommands();
	void SetupGameEvents();
	void SetupCoreEvents();
	void SetupNetMessages();
	void SetupSounds();
	void SetupGameFunctions();
	void SetupEntityCommands();
	void SetupTimers();
	void SetupTransmit();

private:
	// Raw KHook hooks, for what the toolkit has no callback for: two engine
	// calls (turning a connection away, swallowing a client command) and game
	// functions found by signature, one per way of getting the address.
	// Everything
	// else this plugin listens to comes through the toolkit: IToolkitListener
	// for the client and frame callbacks, IToolkitGameHooks for game
	// functions such as TakeDamage. Prefer those -- a plugin with hooks of its
	// own is "raw tier" in `toolkit list` and needs a rebuild whenever KHook
	// or the engine changes, a plugin without them only a core update.
	//
	// KHook is metamod's detour library, the one engine the toolkit shares
	// with metamod, so these land next to every other plugin's hooks. Each
	// holds the function it hooks, the context (this) and the Pre/Post
	// callbacks (Pre runs before the original, Post after; nullptr leaves a
	// side empty). The macro takes the hook's type from the handler it names
	// (which is why the handlers come first), KHOOK_INIT() in Load() installs
	// it, KHOOK_DESTRUCT() in Unload() takes it down. The interface pointer is
	// read at KHOOK_INIT(), not here -- it is still null when this object is
	// constructed.
	KHOOK_VIRTUAL(m_hClientConnect, &ISource2GameClients::ClientConnect, &g_pSource2GameClients, &SamplePlugin::Hook_ClientConnect, nullptr);
	KHOOK_VIRTUAL(m_hClientCommand, &ISource2GameClients::ClientCommand, &g_pSource2GameClients, &SamplePlugin::Hook_ClientCommand, nullptr);

	// Game functions by signature. KHOOK_MEMBER is a method (the handler gets
	// `this` first), KHOOK_FUNCTION a free function (the handler gets the
	// function's parameters and nothing else). The target is evaluated at
	// KHOOK_INIT(), so it may use the toolkit's globals. Look in
	// IToolkitGameHooks first: what is there needs no hook of your own.
	//
	// 1. An address the toolkit already resolved -- IToolkitAddresses, as the
	//    virtual getter ...
	KHOOK_MEMBER(m_hGiveNamedItem, g_pToolkitAddresses->CCSPlayer_ItemServices_GiveNamedItem(), &SamplePlugin::Hook_GiveNamedItem, nullptr);
	//    ... or as its ADDR_* macro, here on a free function.
	KHOOK_FUNCTION(m_hUtilRemove, ADDR_UTIL_REMOVE(), &SamplePlugin::Hook_UtilRemove, nullptr);
	//
	// 2. The name of a gamedata entry -- the toolkit's or one you added. Its
	//    library is read and the function found by exported symbol or by
	//    pattern, whichever the entry has; a game update is a gamedata update.
	KHOOK_MEMBER(m_hSwitchTeam, "CCSPlayerController::SwitchTeam", &SamplePlugin::Hook_SwitchTeam, nullptr);
	//
	// 3. A pattern of your own, hardcoded: WIN_LINUX picks the platform's,
	//    FindPattern scans the module. Only for what nothing else resolves --
	//    it needs a rebuild whenever the game moves the function.
	KHOOK_MEMBER(m_hSnapViewAngles,
	             g_pServerModule->FindPattern(WIN_LINUX(
	                 "48 89 7C 24 ? 55 48 8B EC 48 81 EC ? ? ? ? 48 8B F9 48 8D 4D ? E8 ? ? ? ? F2 0F 10 87 C8 0B 00 00",
	                 "55 48 89 E5 41 57 49 89 FF 48 89 F7 41 56 41 55 41 54 53 48 81 EC ? ? ? ? E8")),
	             &SamplePlugin::Hook_SnapViewAngles, nullptr);

private:
	// 6. net messages -- one bit per player slot, the same layout the hooks get
	// their recipients in.
	uint64_t m_NoShakeMask = 0;     // players who turned screen shakes off (!noshake)
	uint64_t m_VoiceMutedMask = 0;  // players whose voice is dropped (sample_mute)

	// 10. timers -- a Timer* is only good while the timer lives; see SetupTimers().
	Timer *m_pCountdownTimer = nullptr;
	int m_nCountdown = 0;

public:
	const char *GetAuthor() override { return PLUGIN_AUTHOR; }
	const char *GetName() override { return PLUGIN_DISPLAY_NAME; }
	const char *GetDescription() override { return PLUGIN_DESCRIPTION; }
	const char *GetVersion() override { return PLUGIN_FULL_VERSION; }
};

extern SamplePlugin g_Plugin;

#endif //_INCLUDE_SOURCE2TOOLKIT_SAMPLE_PLUGIN_H_
