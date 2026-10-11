/**
* vim: set ts=4 sw=4 tw=99 noet:
 * =============================================================================
 * Source2Toolkit
 * Copyright (C) 2025-2026 Michal "Slynx (˙·٠● S l y n x ●٠·˙)" Přikryl.
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
 * As a special exception, Michal "Slynx (˙·٠● S l y n x ●٠·˙)" Přikryl
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
 *   - Michal "Slynx (˙·٠● S l y n x ●٠·˙)" Přikryl
 *
 * Project: Source2Toolkit
 */
#pragma once
#include <dynlibutils/memaddr.hpp>
#include <dynlibutils/module.hpp>
#include <source2toolkit/IToolkitModule.h>

#include <string>
#include <string_view>

/// CModule::InitFromName, except that the game's server module is found
/// through its CreateInterface. Metamod's loader is a server.dll /
/// libserver.so of its own and is loaded first; on Windows GetModuleHandle
/// returns that one. Every by-name module lookup in the core goes through here.
bool InitModuleFromName(DynLibUtils::CModule& module, std::string_view name, bool extension = false);

class ToolkitModule : public IToolkitModule
{
public:
    ToolkitModule() = default;
    explicit ToolkitModule(const char* name);
    explicit ToolkitModule(uintptr_t ptr);

    bool InitFromName(const char* name, bool extension = false) override;
    bool InitFromMemory(uintptr_t ptr) override;

    IToolkitMemory FindPattern(const char* pattern, IToolkitMemory startAddress = IToolkitMemory()) const override;
    IToolkitMemory FindPatternInSection(const char* pattern, const char* section, IToolkitMemory startAddress = IToolkitMemory()) const override;

    IToolkitMemory GetVirtualTableByName(const char* name, bool decorated = false) const override;
    IToolkitMemory GetFunctionByName(const char* name) const override;

    SectionInfo GetSectionByName(const char* name) const override;
    void* GetModuleHandle() const override;
    IToolkitMemory GetModuleBase() const override;
    const char* GetModulePath() const override;
    const char* GetModuleName() const override;

    IToolkitMemory GetVirtualTableByOffset(const char* name, std::ptrdiff_t offset, bool decorated = false) const override;
    IToolkitMemory GetVirtualTableByBase(const char* name, const char* baseName, bool decorated = false) const override;

private:
    DynLibUtils::CModule m_module;
    mutable std::string  m_cachedName;
};
