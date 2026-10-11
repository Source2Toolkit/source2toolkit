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
#include "hookid.h"
#include "shared.h"
#include "virtualhooks.h"

#include <source2toolkit/IToolkitCommands.h>

#include <functional>

class CCommand;
class CCommandContext;

namespace commands {
    struct CommandEntry
    {
        PluginId owner;
        ToolkitHookId id;           // what the Register* call returned; the three aliases share it
        CommandHandler handler;
        bool post;
        ChatHandler chatSource;     // a chat command / ConCommand: what it was registered with, for UnregisterX(handler)
        std::string command;        // a chat command / ConCommand: its bare name, what "Overrides" is keyed by
        std::string permission;     // what a player needs to run it; "" for everybody
    };

    inline CommandHandler WrapVoidHandler(const ChatHandler& fn)
    {
        return [fn](const ToolkitCommandContext& ctx, const ToolkitCommandArgs& args, bool post) -> Action
        {
            fn(ctx, args, post);
            return Action::Ignore;
        };
    }

    void InitCommands();
    void DestructCommands();

    void ConCommandRouter(const CCommandContext &ctx, const CCommand &args);
    /// `source` is where it was typed: the console, or chat through one of
    /// the triggers (the chat path in virtualhooks.cpp says which).
    Action DispatchConsoleListener(const CCommandContext &ctx, const CCommand &args, bool post,
                                   ToolkitCommandSource source = ToolkitCommandSource::Console);

    class CommandsManager : public IToolkitCommands
    {
    public:

        ToolkitHookId RegisterChatListener(const char* pchName, ChatHandler handler, const char* pchPermission) override { const PluginId owner = hookid::OwnerOfHandler(handler); return RegisterChatListener(owner, pchName, std::move(handler), pchPermission); }
        bool UnregisterChatListener(const char* pchName, const ChatHandler& handler) override { return UnregisterChatListener(hookid::OwnerOfHandler(handler), pchName, handler); }
        bool UnregisterChatListener(ToolkitHookId id) override { return Unregister(id); }
        ToolkitHookId RegisterConCommand(const char* pchName, ChatHandler handler, const char* pchPermission) override { const PluginId owner = hookid::OwnerOfHandler(handler); return RegisterConCommand(owner, pchName, std::move(handler), pchPermission); }
        bool UnregisterConCommand(const char* pchName, const ChatHandler& handler) override { return UnregisterConCommand(hookid::OwnerOfHandler(handler), pchName, handler); }
        bool UnregisterConCommand(ToolkitHookId id) override { return Unregister(id); }
        ToolkitHookId RegisterConListener(const char* pchName, CommandHandler handler, bool post) override { const PluginId owner = hookid::OwnerOfHandler(handler); return RegisterConListener(owner, pchName, std::move(handler), post); }
        bool UnregisterConListener(const char* pchName, const CommandHandler& handler, bool post) override { return UnregisterConListener(hookid::OwnerOfHandler(handler), pchName, handler, post); }
        bool UnregisterConListener(ToolkitHookId id) override { return Unregister(id); }
        void ReplyToCommand(const ToolkitCommandContext& ctx, const char* pszMessage) override;

        // The same with the owner spelled out: what the calls above resolve
        // to, and what the core itself calls.
        ToolkitHookId RegisterChatListener(PluginId owner, const char* pchName, ChatHandler handler, const char* pchPermission = nullptr);
        bool UnregisterChatListener(PluginId owner, const char* pchName, const ChatHandler& handler);
        ToolkitHookId RegisterConCommand(PluginId owner, const char* pchName, ChatHandler handler, const char* pchPermission = nullptr);
        bool UnregisterConCommand(PluginId owner, const char* pchName, const ChatHandler& handler);
        ToolkitHookId RegisterConListener(PluginId owner, const char* pchName, CommandHandler handler, bool post);
        bool UnregisterConListener(PluginId owner, const char* pchName, const CommandHandler& handler, bool post);
        bool Unregister(ToolkitHookId id);
    public:
        void RemoveAllForPlugin(PluginId id);
        void UnlockConCommands();

    private:
        // The three names a chat command / ConCommand listens on.
        static void AddAliases(PluginId owner, ToolkitHookId id, const char* pchName, const ChatHandler& handler, const char* pchPermission);
        static bool RemoveAliases(PluginId owner, const char* pchName, const ChatHandler& handler);
    };

    extern CommandsManager commandsManager;
}
