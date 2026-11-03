-- K2040 Quest Journal. Windows x64 F4SE plugin, built from Linux or Windows.
-- Reuse the working Quick Attach Menu's cross-runtime CommonLibF4 toolchain.
-- Builds stage under dist/Data; installation/deployment is always explicit.

set_xmakever("3.0.0")
set_project("K2040_Quest_Journal")
set_version("0.0.2")
set_languages("c++23")
add_rules("mode.debug", "mode.releasedbg", "mode.release")

-- Non-shipping explicit QA variant. Production default OFF.
option("quest_dryrun_qa")
    set_default(false)
    set_showmenu(true)
    set_description("Build isolated read-only one-shot tracking QA test DLL")
option_end()

-- No INI controls: a separately built, explicit UI-consent QA preview.
option("quest_integrated_tracking_qa")
    set_default(false)
    set_showmenu(true)
    set_description("QA-only integrated track/untrack with in-Journal enable")
option_end()

-- Dedicated, standalone read-only selector inspection. Never links
-- integrated QA, native write QA, Dogmeat writer, or dry-run writer.
option("quest_selector_readonly_qa")
    set_default(false)
    set_showmenu(true)
    set_description("OG163 research-only: bounded command selector code capture; NO quest writes")
option_end()

-- Schema-only augmentation must be paired with the existing one-shot
-- selected-entry read-only flavor, never enabled on a release target.
option("quest_pipboy_selection_schema_readonly_qa")
    set_default(false)
    set_showmenu(true)
    set_description("OG163 ONLY: anonymous selectedEntry schema census; no quest writes")
option_end()

-- Separate read-only vanilla Pip-Boy selected-entry observer.
-- No native writes, no old selector byte captures, no modified SWF.
option("quest_pipboy_selection_readonly_qa")
    set_default(false)
    set_showmenu(true)
    set_description("OG163-only: single UI-thread GFx selectedEntry metadata observation; no writes")
option_end()

-- Distinct experiment: one exact single Misc objective via vanilla helper.
-- Requires integrated QA and is impossible in normal/release builds.
option("quest_single_misc_write_qa")
    set_default(false)
    set_showmenu(true)
    set_description("NON-SHIPPING: one guarded single-Misc vanilla dispatch on explicit UI ARM/EXECUTE")
option_end()

-- First one-click RC, still isolated and OFF in production. Exactly one
-- verified vanilla quest ID, objective, instance and process attempt.
option("quest_single_misc_rc")
    set_default(false)
    set_showmenu(true)
    set_description("NON-SHIPPING: one-click verified DogmeatQuest QA release candidate")
option_end()

-- Independently compiled, opt-in OG163 candidate. NO QA infrastructure.
-- Normal/default Journal still denies all Pip-Boy native writes.
option("quest_dogmeat_production_candidate")
    set_default(false)
    set_showmenu(true)
    set_description("OPT-IN OG163: only the previously game-tested DogmeatQuest instance")
option_end()

-- Additional, distinctly packaged experimental QA mode. NEVER default on.
option("quest_native_write_qa")
    set_default(false)
    set_showmenu(true)
    set_description("EXPERIMENTAL: one real SetActive call behind two INI consents")
option_end()

option("quest_generic_misc_questwide_qa")
    set_default(false)
    set_showmenu(true)
    set_description("ISOLATED: one native quest-wide Misc SetActive call, instance-checked")
option_end()

option("quest_repeatable_misc_qa")
    set_default(false)
    set_showmenu(true)
    set_description("ISOLATED: repeatable Misc SetActive gated on independent refresh")
option_end()

-- No QA infrastructure, but still an explicitly staged, nondefault build.
-- Supports eligible Misc TESQuest-wide actions only; not a general release.
option("quest_questwide_production_candidate")
    set_default(false)
    set_showmenu(true)
    set_description("OPT-IN OG163 candidate: verified repeatable Misc quest-wide tracking")
option_end()

-- Normal product compilation of the already in-game accepted R2 path.
-- Still explicit and false by default until the separate final release
-- gate; no QA UI, experimental INI, or automatic installation.
option("quest_questwide_normal_tracking")
    set_default(false)
    set_showmenu(true)
    set_description("Explicit OG163 normal-Journal build: R2-accepted quest-wide tracking")
option_end()

local required_commonlib_revision = "12beba2a89fe117a14f1707b88c99ecb1b12f8c0"
local required_shared_revision = "f0b1670ee9caac2e349497f6f3c08a69633a8ea7"
local commonlib = os.getenv("K2040_COMMONLIBF4_ROOT")
if not commonlib or commonlib == "" then
    raise("Set K2040_COMMONLIBF4_ROOT to the pinned CommonLibF4 xmake checkout")
end
commonlib = path.absolute(commonlib)
if not os.isdir(commonlib) or not os.isfile(path.join(commonlib,"xmake.lua")) then
    raise("CommonLibF4 xmake checkout not found: " .. commonlib)
end

local prisma_header = path.join(os.scriptdir(),"external","prismaui_f4","PrismaUI_F4_Modern_API.h")
if not os.isfile(prisma_header) then
    raise("Provide unmodified official PrismaUI_F4_Modern_API.h: see external/prismaui_f4/README.md")
end

local windows_sdk_includes = {}
if is_host("linux") and is_plat("windows") then
    local sdk = get_config("sdk")
    if not sdk or sdk == "" then
        raise("Set --sdk to an extracted Windows SDK for the Linux clang-cl build")
    end
    local versions = os.dirs(path.join(sdk,"Windows Kits","10","Include","*"))
    table.sort(versions)
    local root = versions[#versions]
    if root then
        for _,component in ipairs({"shared","um","ucrt"}) do
            local folder = path.join(root,component)
            if os.isdir(folder) then table.insert(windows_sdk_includes,folder) end
        end
    end
    if #windows_sdk_includes ~= 3 then
        raise("Windows SDK must provide shared, um and ucrt include roots")
    end
end

includes(commonlib)

if is_mode("releasedbg") then
    -- Do not pass final linker stripping options to CommonLib static archives.
    target("commonlib-shared")
        set_strip("none")
    target("commonlibf4")
        set_strip("none")
end

target("K2040_Quest_Journal")
    set_kind("shared")
    set_filename("K2040_Quest_Journal.dll")
    set_symbols("debug")
    on_load(function(_)
        local git = import("devel.git")
        local revision = git.lastcommit({repodir=commonlib})
        if revision ~= required_commonlib_revision then
            raise("Unexpected CommonLibF4 revision " .. revision .. "; expected " .. required_commonlib_revision)
        end
        local shared = git.lastcommit({repodir=path.join(commonlib,"lib","commonlib-shared")})
        if shared ~= required_shared_revision then
            raise("Unexpected commonlib-shared revision " .. shared .. "; expected " .. required_shared_revision)
        end
    end)

    add_rules("commonlibf4.plugin", {
        name="K2040_Quest_Journal", author="K2040", version="0.0.2"
    })
    add_includedirs("src","external/prismaui_f4",
        path.join(commonlib,"include"),path.join(commonlib,"lib","commonlib-shared","include"))
    for _,folder in ipairs(windows_sdk_includes) do add_includedirs(folder,{system=true}) end
    add_files("src/**.cpp")
    if has_config("quest_pipboy_selection_schema_readonly_qa") and
       not has_config("quest_pipboy_selection_readonly_qa") then
        raise("GFx schema inspection requires isolated read-only observer QA")
    end
    if has_config("quest_pipboy_selection_readonly_qa") then
        if has_config("quest_selector_readonly_qa") or
           has_config("quest_dryrun_qa") or
           has_config("quest_integrated_tracking_qa") or
           has_config("quest_native_write_qa") or
           has_config("quest_generic_misc_questwide_qa") or
           has_config("quest_single_misc_write_qa") or
           has_config("quest_single_misc_rc") or
           has_config("quest_dogmeat_production_candidate") then
            raise("Pipboy GFx observer QA MUST exclude all writer and prior research flavors")
        end
        local stage=os.getenv("K2040_PIPBOY_GFX_QA_STAGE")
        if not stage or stage=="" or
           stage:find("SteamLibrary",1,true) or
           stage:find("steamapps",1,true) or
           stage:find("Amethyst",1,true) or
           stage:find("Data/F4SE",1,true) then
            raise("GFx observer QA requires isolated K2040_PIPBOY_GFX_QA_STAGE")
        end
        add_defines("K2040_PIPBOY_SELECTION_READONLY_QA=1")
        if has_config("quest_pipboy_selection_schema_readonly_qa") then
            add_defines("K2040_PIPBOY_SELECTION_SCHEMA_READONLY_QA=1")
        end
        add_files("research/native_pipboy_selection/UnwiredGFxAdapter.cpp")
        -- Compile-check unwired quest-wide read-only adapter only under
        -- the isolated, default-OFF selected-entry AND schema QA flavor.
        if has_config("quest_pipboy_selection_schema_readonly_qa") then
            add_files("research/native_questwide/NativeQuestWideSnapshot.cpp")
        end
        set_targetdir(path.join(stage,"bin"))
    end
    if has_config("quest_selector_readonly_qa") and
       (has_config("quest_dryrun_qa") or
        has_config("quest_integrated_tracking_qa") or
        has_config("quest_native_write_qa") or
        has_config("quest_single_misc_write_qa") or
        has_config("quest_single_misc_rc") or
        has_config("quest_dogmeat_production_candidate")) then
        raise("Selector-only research MUST exclude every quest-writing and QA writer flavor")
    end
    if has_config("quest_selector_readonly_qa") then
        local stage=os.getenv("K2040_SELECTOR_READONLY_STAGE")
        if not stage or stage=="" or
           stage:find("SteamLibrary",1,true) or
           stage:find("steamapps",1,true) or
           stage:find("Amethyst",1,true) or
           stage:find("Data/F4SE",1,true) then
            raise("Selector-only QA requires isolated K2040_SELECTOR_READONLY_STAGE")
        end
        add_defines("K2040_SELECTOR_READONLY_QA=1")
        set_targetdir(path.join(stage,"bin"))
    end
    if has_config("quest_integrated_tracking_qa") and
        (not has_config("quest_dryrun_qa") or has_config("quest_native_write_qa")) then
        raise("Integrated QA requires dryrun base and excludes legacy write-once QA")
    end
    if has_config("quest_single_misc_write_qa") and
       (not has_config("quest_integrated_tracking_qa") or
        not has_config("quest_dryrun_qa") or has_config("quest_native_write_qa")) then
        raise("Single Misc experimental writer requires isolated integrated QA; no legacy writer")
    end
    if has_config("quest_single_misc_rc") and
       (not has_config("quest_single_misc_write_qa") or
        not has_config("quest_integrated_tracking_qa") or
        has_config("quest_native_write_qa")) then
        raise("Single-Misc RC requires isolated single-Misc QA and integrated QA")
    end
    if has_config("quest_dogmeat_production_candidate") and
       (has_config("quest_dryrun_qa") or
        has_config("quest_integrated_tracking_qa") or
        has_config("quest_single_misc_write_qa") or
        has_config("quest_single_misc_rc") or
        has_config("quest_native_write_qa")) then
        raise("Dogmeat production candidate may not compile any QA writer, UI or log")
    end
    if has_config("quest_dogmeat_production_candidate") then
        local stage=os.getenv("K2040_DOGMEAT_PRODUCTION_STAGE")
        if not stage or stage=="" or
           stage:find("SteamLibrary",1,true) or
           stage:find("steamapps",1,true) or
           stage:find("Amethyst",1,true) or
           stage:find("Data/F4SE",1,true) then
            raise("Dogmeat candidate requires safe isolated K2040_DOGMEAT_PRODUCTION_STAGE")
        end
        add_defines("K2040_DOGMEAT_PRODUCTION_CANDIDATE=1")
        set_targetdir(path.join(stage,"bin"))
    end
    if has_config("quest_questwide_production_candidate") and
       has_config("quest_questwide_normal_tracking") then
        raise("Two quest-wide output flavors cannot share a single build")
    end
    if (has_config("quest_questwide_production_candidate") or
        has_config("quest_questwide_normal_tracking")) and
       (has_config("quest_dryrun_qa") or
        has_config("quest_native_write_qa") or
        has_config("quest_generic_misc_questwide_qa") or
        has_config("quest_repeatable_misc_qa") or
        has_config("quest_integrated_tracking_qa") or
        has_config("quest_single_misc_write_qa") or
        has_config("quest_single_misc_rc") or
        has_config("quest_dogmeat_production_candidate") or
        has_config("quest_selector_readonly_qa") or
        has_config("quest_pipboy_selection_readonly_qa")) then
        raise("Quest-wide candidate must exclude all research/QA writers and read-only experimental flavors")
    end
    if has_config("quest_questwide_production_candidate") or
       has_config("quest_questwide_normal_tracking") then
        local stage = has_config("quest_questwide_normal_tracking")
            and path.join(os.scriptdir(),"dist")
            or os.getenv("K2040_QUESTWIDE_CANDIDATE_STAGE")
        if not stage or stage=="" or
           stage:find("SteamLibrary",1,true) or
           stage:find("steamapps",1,true) or
           stage:find("Amethyst",1,true) or
           stage:find("Data/F4SE",1,true) then
            raise("Quest-wide tracking requires a safe dedicated build stage")
        end
        -- Preserve the exact R2-accepted native implementation and
        -- frontend event binding; only the normal output path changes.
        add_defines("K2040_QUESTWIDE_PRODUCTION_CANDIDATE=1")
        add_files("research/production_candidate/QuestWideProductionCandidate.cpp")
        add_files("research/production_candidate/OrdinaryQuestEvidence.cpp")
        add_files("research/native_one_shot/OneShotOgDryRun.cpp")
        add_files("research/native_one_shot/GenericMiscEvidence.cpp")
        set_targetdir(path.join(stage,"bin"))
    end
    if has_config("quest_repeatable_misc_qa") and
       (not has_config("quest_dryrun_qa") or
        has_config("quest_native_write_qa") or
        has_config("quest_generic_misc_questwide_qa") or
        has_config("quest_integrated_tracking_qa") or
        has_config("quest_single_misc_write_qa") or
        has_config("quest_single_misc_rc") or
        has_config("quest_dogmeat_production_candidate")) then
        raise("Repeatable Misc QA needs its own isolated writer-free dryrun flavor")
    end
    if has_config("quest_generic_misc_questwide_qa") and
       (not has_config("quest_native_write_qa") or
        not has_config("quest_dryrun_qa") or
        has_config("quest_integrated_tracking_qa") or
        has_config("quest_single_misc_write_qa") or
        has_config("quest_single_misc_rc") or
        has_config("quest_dogmeat_production_candidate")) then
        raise("Generic Misc QA must be isolated with legacy one-shot writer and dryrun only")
    end
    if has_config("quest_native_write_qa") and not has_config("quest_dryrun_qa") then
        raise("Native write QA requires explicit isolated dry-run QA build")
    end
    if has_config("quest_dryrun_qa") then
        local qa_stage = os.getenv("K2040_DRYRUN_QA_STAGE")
        if not qa_stage or qa_stage == "" or
           qa_stage:find("SteamLibrary",1,true) or
           qa_stage:find("steamapps",1,true) or
           qa_stage:find("Data/F4SE",1,true) then
            raise("QA requires safe isolated K2040_DRYRUN_QA_STAGE")
        end
        add_defines("K2040_TRACKING_DRYRUN_QA=1")
        add_files("research/native_one_shot/OneShotOgDryRun.cpp")
        if has_config("quest_integrated_tracking_qa") then
            add_defines("K2040_TRACKING_INTEGRATED_QA=1")
            add_files("research/native_one_shot/OneShotOgIntegratedQA.cpp")
            if has_config("quest_single_misc_write_qa") then
                add_defines("K2040_SINGLE_MISC_WRITE_QA=1")
                if has_config("quest_single_misc_rc") then
                    add_defines("K2040_SINGLE_MISC_RC=1")
                end
            end
        end
        if has_config("quest_repeatable_misc_qa") then
            add_defines("K2040_REPEATABLE_MISC_QA=1")
            add_files("research/native_one_shot/RepeatableOgWriteQA.cpp")
            add_files("research/native_one_shot/GenericMiscEvidence.cpp")
        end
        if has_config("quest_native_write_qa") then
            add_defines("K2040_TRACKING_NATIVE_WRITE_QA=1")
            add_files("research/native_one_shot/OneShotOgWriteQA.cpp")
            if has_config("quest_generic_misc_questwide_qa") then
                add_defines("K2040_GENERIC_MISC_QUESTWIDE_QA=1")
                add_files("research/native_one_shot/GenericMiscEvidence.cpp")
            end
        end
        set_targetdir(path.join(qa_stage,"bin"))
    end
    add_defines("WIN32_LEAN_AND_MEAN","NOMINMAX","COMMONLIB_RUNTIMECOUNT=3")
    if is_plat("windows") then
        add_cxflags("/permissive-","/wd4200","/wd4201","/wd4324","/bigobj","/FS")
        add_syslinks("version","ole32","oleaut32","user32","bcrypt","crypt32")
    end

    after_build(function(target)
        local base = has_config("quest_dryrun_qa")
            and os.getenv("K2040_DRYRUN_QA_STAGE")
            or (has_config("quest_questwide_normal_tracking")
                and path.join(os.scriptdir(),"dist")
            or (has_config("quest_questwide_production_candidate")
                and os.getenv("K2040_QUESTWIDE_CANDIDATE_STAGE")
                or (has_config("quest_dogmeat_production_candidate")
                and os.getenv("K2040_DOGMEAT_PRODUCTION_STAGE")
                or (has_config("quest_pipboy_selection_readonly_qa")
                    and os.getenv("K2040_PIPBOY_GFX_QA_STAGE")
                    or (has_config("quest_selector_readonly_qa")
                        and os.getenv("K2040_SELECTOR_READONLY_STAGE")
                        or path.join(os.scriptdir(),"dist"))))))
        local plugins = path.join(base,"Data","F4SE","Plugins")
        os.mkdir(plugins)
        if has_config("quest_native_write_qa") or
           has_config("quest_repeatable_misc_qa") then
            os.cp(path.join(os.scriptdir(),"research","native_one_shot",
                "K2040_Quest_Journal_Write_QA.ini"),plugins)
        end
        os.cp(target:targetfile(),plugins)
        local assets = path.join(base,"Data","PrismaUI_F4","views","K2040QuestJournal")
        os.mkdir(assets)
        for _,asset in ipairs(os.files(path.join(os.scriptdir(),"view","*"))) do
            os.cp(asset,assets)
        end
        -- Every K2040 mod carries the same canonical parent MCM menu.
        -- Child controls remain scoped to their own MCM folder.
        for _,name in ipairs({"K2040_Mods","K2040_Quest_Journal"}) do
            local source=path.join(os.scriptdir(),"mcm","Data","MCM","Config",name)
            local target=path.join(base,"Data","MCM","Config",name)
            os.mkdir(target)
            os.cp(path.join(source,"*"),target)
        end
        print("Staged self-contained Quest Journal DLL, views and MCM parent/child under dist/Data")
    end)
