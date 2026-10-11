/**
 * vim: set ts=4 sw=4 tw=99 noet:
 * =============================================================================
 * Source2Toolkit
 * =============================================================================
 *
 * The ids Register* / Hook* hand out (ToolkitHookId): one counter for every
 * interface, so an id is never shared by two registrations anywhere -- and
 * HookList, the handler list most managers keep.
 *
 * Project: Source2Toolkit
 */
#pragma once

#include <source2toolkit/IToolkitTypes.h>

#include <algorithm>
#include <utility>
#include <vector>

namespace hookid
{
    /// Whose code the address is (PluginManager::OwnerOf); 0 for the core.
    PluginId OwnerOf(const void* address);

    /// The owner of a handler: the plugin its code lies in.
    template <typename HANDLER>
    PluginId OwnerOfHandler(const HANDLER& handler)
    {
        return OwnerOf(handler.Origin());
    }

    inline ToolkitHookId Next()
    {
        static ToolkitHookId s_last = 0;
        return ++s_last;
    }

    /// The handlers of one hook point: who registered each, under which id.
    /// Several per plugin; removed by id, by handler (a function or an object
    /// and a method) or all of a plugin's at once.
    template <typename HANDLER>
    class HookList
    {
    public:
        struct Entry
        {
            PluginId owner;
            ToolkitHookId id;
            HANDLER handler;
        };

        ToolkitHookId Add(PluginId owner, HANDLER handler)
        {
            if (!handler)
                return 0;

            const ToolkitHookId id = Next();
            m_entries.push_back({ owner, id, std::move(handler) });
            return id;
        }

        bool RemoveId(ToolkitHookId id)
        {
            return std::erase_if(m_entries, [id](const Entry& e) { return e.id == id; }) > 0;
        }

        bool RemoveHandler(PluginId owner, const HANDLER& handler)
        {
            if (!handler.HasIdentity())
                return false;

            return std::erase_if(m_entries, [owner, &handler](const Entry& e)
            {
                return e.owner == owner && handler.SameAs(e.handler);
            }) > 0;
        }

        void RemoveOwner(PluginId owner)
        {
            std::erase_if(m_entries, [owner](const Entry& e) { return e.owner == owner; });
        }

        void Clear() { m_entries.clear(); }

        bool empty() const { return m_entries.empty(); }
        size_t size() const { return m_entries.size(); }

        /// Copies of the handlers, for a dispatch: a handler is free to hook
        /// or unhook while it runs.
        std::vector<HANDLER> Snapshot() const
        {
            std::vector<HANDLER> out;
            out.reserve(m_entries.size());
            for (const Entry& e : m_entries)
                out.push_back(e.handler);
            return out;
        }

    private:
        std::vector<Entry> m_entries;
    };
}
