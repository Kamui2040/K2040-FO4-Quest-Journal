-- Standalone, opt-in OG quest-event observer; NOT included in main xmake.lua.
set_xmakever("3.0.0")
set_project("K2040_Quest_Staged_Init")
set_version("0.0.1")
set_languages("c++23")
add_rules("mode.releasedbg")

local common = os.getenv("K2040_COMMONLIBF4_ROOT")
local sdk = os.getenv("K2040_WINDOWS_SDK_ROOT")
local out = os.getenv("K2040_STAGED_INIT_OUTPUT_DIR")
local required = "12beba2a89fe117a14f1707b88c99ecb1b12f8c0"
if not common or not sdk or not out or common == "" or sdk == "" or out == "" then
    raise("Set CommonLib, Windows SDK, and isolated probe output directories")
end
common = path.absolute(common)
sdk = path.absolute(sdk)
out = path.absolute(out)
if not os.isdir(common) or not os.isfile(path.join(common, "xmake.lua")) then
    raise("Missing CommonLib checkout")
end
if out:find("SteamLibrary", 1, true) or out:find("Data/F4SE", 1, true) then
    raise("Refuse output staging into game or mod deployment")
end
includes(common)

if is_mode("releasedbg") then
    target("commonlib-shared")
        set_strip("none")
    target("commonlibf4")
        set_strip("none")
end

target("K2040_Quest_Staged_Init")
    set_kind("shared")
    set_filename("K2040_Quest_Staged_Init.dll")
    set_symbols("debug")
    set_targetdir(out)
    on_load(function(_)
        local git = import("devel.git")
        local revision = git.lastcommit({repodir=common})
        if revision ~= required then
            raise("Unexpected CommonLibF4 version")
        end
    end)
    add_rules("commonlibf4.plugin", {
        name = "K2040_Quest_Staged_Init",
        author = "K2040",
        version = "0.0.1"
    })
    add_files("NativeStagedInit.cpp")
    add_includedirs(path.join(common,"include"),
                    path.join(common,"lib","commonlib-shared","include"))
    add_defines("WIN32_LEAN_AND_MEAN", "NOMINMAX", "COMMONLIB_RUNTIMECOUNT=3")
    if is_host("linux") and is_plat("windows") then
        local versions = os.dirs(path.join(sdk, "Windows Kits", "10", "Include", "*"))
        table.sort(versions)
        local version = versions[#versions]
        if not version then raise("Missing Windows SDK headers") end
        for _,component in ipairs({"shared", "um", "ucrt"}) do
            add_includedirs(path.join(version, component), {system=true})
        end
    end
    if is_plat("windows") then
        add_cxflags("/permissive-", "/wd4200", "/wd4201", "/wd4324", "/bigobj", "/FS")
        add_syslinks("version", "ole32", "oleaut32", "user32",
                     "bcrypt", "crypt32")
    end
