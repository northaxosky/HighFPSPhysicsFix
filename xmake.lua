set_xmakever("3.0.0")

set_project("HighFPSPhysicsFix")
set_version("0.8.13")
set_license("MIT")

set_languages("c++23")
set_encodings("utf-8")
set_runtimes("MT")

add_rules("mode.debug", "mode.releasedbg")

-- Enable xbyak in commonlibf4 (used by our trampoline JIT in src/dllmain.cpp).
set_config("commonlib_xbyak", true)

includes("extern/CommonLibF4")

add_requires("directxtk")

option("deploy_dir")
    set_default("")
    set_showmenu(true)
    set_description("MO2/game mod folder to copy the built DLL + INI into after build (empty = skip)")
option_end()

target("HighFPSPhysicsFix")
    set_kind("shared")
    set_arch("x64")

    add_deps("commonlibf4")
    add_packages("directxtk")

    add_files("src/**.cpp")
    add_headerfiles("include/**.h", "include/**.hpp")
    add_includedirs("include")
    set_pcxxheader("include/PCH.h")

    add_defines(
        "F4SE_SUPPORT_XBYAK",
        "_UNICODE",
        "_SKMP_DISABLE_BOOST_SERIALIZATION"
    )

    -- Generated headers/resources. Version values are computed in on_load
    -- below from `git describe` (with 0.8.13 fallback for tarball builds).
    set_configvar("PROJECT_NAME", "HighFPSPhysicsFix")
    set_configvar("PROJECT_VERSION", "0.8.13")
    set_configvar("PROJECT_VERSION_MAJOR", 0)
    set_configvar("PROJECT_VERSION_MINOR", 8)
    set_configvar("PROJECT_VERSION_PATCH", 13)
    set_configvar("PROJECT_VERSION_DESCRIBE", "0.8.13")

    on_load(function (target)
        local FALLBACK = "0.8.13"
        local version = FALLBACK
        local describe = FALLBACK
        if os.isdir(".git") then
            local out
            try {
                function ()
                    out = os.iorun("git describe --tags --always --dirty --match \"[0-9]*\"")
                end
            }
            if out and #out > 0 then
                describe = out:gsub("%s+$", "")
                local mj, mn, pt = describe:match("^(%d+)%.(%d+)%.(%d+)")
                if mj and mn and pt then
                    version = mj .. "." .. mn .. "." .. pt
                end
            end
        end
        local mj, mn, pt = version:match("^(%d+)%.(%d+)%.(%d+)")
        target:set("configvar", "PROJECT_VERSION", version)
        target:set("configvar", "PROJECT_VERSION_MAJOR", tonumber(mj))
        target:set("configvar", "PROJECT_VERSION_MINOR", tonumber(mn))
        target:set("configvar", "PROJECT_VERSION_PATCH", tonumber(pt))
        target:set("configvar", "PROJECT_VERSION_DESCRIBE", describe)
        print("HighFPSPhysicsFix version: " .. version .. " (describe: " .. describe .. ")")
    end)
    set_configvar("CMAKE_CURRENT_SOURCE_DIR", (os.projectdir():gsub("\\", "/")))
    set_configdir("$(builddir)/HighFPSPhysicsFix.gen")
    add_configfiles("templates/Version.h.in", { filename = "include/Version.h", pattern = "@(.-)@" })
    add_configfiles("templates/version.rc.in", { filename = "version.rc", pattern = "@(.-)@" })
    add_configfiles("templates/resources.rc.in", { filename = "resources.rc", pattern = "@(.-)@" })
    add_includedirs("$(builddir)/HighFPSPhysicsFix.gen/include")
    add_files(
        "$(builddir)/HighFPSPhysicsFix.gen/version.rc",
        "$(builddir)/HighFPSPhysicsFix.gen/resources.rc"
    )

    -- Compile / link flag parity with the retiring CMake build (CMakeLists.txt:212-256).
    add_cxxflags(
        "/sdl",
        "/W4",
        "/WX",
        "/GS",
        "/guard:cf",
        "/Brepro",
        "/utf-8",
        "/Zi",
        "/permissive-",
        "/Zc:preprocessor",
        "/Zc:__cplusplus",
        "/Zc:throwingNew",
        "/Zc:inline",
        "/wd4200",
        "/wd4100",
        { force = true, tools = { "cl" } }
    )
    add_ldflags(
        "/CETCOMPAT",
        "/DYNAMICBASE",
        "/NXCOMPAT",
        "/HIGHENTROPYVA",
        "/guard:cf",
        { force = true, tools = { "link" } }
    )

    if is_mode("releasedbg") then
        add_cxxflags("/JMC-", "/Ob3", "/GL", "/Gw", "/Gy", { force = true, tools = { "cl" } })
        add_ldflags(
            "/INCREMENTAL:NO",
            "/DEBUG:FULL",
            "/OPT:REF",
            "/OPT:ICF",
            "/LTCG",
            { force = true, tools = { "link" } }
        )
    end

    -- Post-build deploy: copy DLL + PDB + INI into <deploy_dir>/F4SE/Plugins/.
    -- Configure with: xmake f --deploy_dir="C:/Games/Modding/.../mods/High FPS Physics Fix"
    -- Empty (default) skips the deploy step entirely (CI's state).
    after_build(function (target)
        local deploy_dir = get_config("deploy_dir")
        if not deploy_dir or deploy_dir == "" then
            return
        end
        local plugins_dir = path.join(deploy_dir, "F4SE", "Plugins")
        os.mkdir(plugins_dir)
        os.cp(target:targetfile(), plugins_dir)
        local pdb = path.join(target:targetdir(), target:name() .. ".pdb")
        if os.isfile(pdb) then
            os.cp(pdb, plugins_dir)
        end
        if os.isfile("HighFPSPhysicsFix.ini") then
            os.cp("HighFPSPhysicsFix.ini", plugins_dir)
        end
        cprint("${bright green}deploy: ${clear}copied to %s", plugins_dir)
    end)

-- Stage a redistributable mod package for CI artifact upload / local zipping.
-- Layout matches the Nexus / MO2 expectation: <root>/F4SE/Plugins/{dll,pdb,ini}.
-- Run with: xmake package
task("package")
    set_menu({
        usage = "xmake package",
        description = "Stage F4SE/Plugins/ mod-package layout under build/package/HighFPSPhysicsFix/",
    })
    on_run(function ()
        import("core.project.project")
        local target = project.target("HighFPSPhysicsFix")
        if not target then
            raise("target 'HighFPSPhysicsFix' not found - run `xmake` first")
        end
        -- Find the most recently built DLL across mode subdirs (the task
        -- doesn't always see the same mode config used by `xmake`).
        local dll
        local pdb
        local search_root = path.join(os.projectdir(), "build", "windows", "x64")
        for _, mode in ipairs({ "releasedbg", "release", "debug" }) do
            local candidate = path.join(search_root, mode, "HighFPSPhysicsFix.dll")
            if os.isfile(candidate) then
                dll = candidate
                pdb = path.join(search_root, mode, "HighFPSPhysicsFix.pdb")
                break
            end
        end
        if not dll then
            -- Fall back to the targetfile xmake reports.
            dll = target:targetfile()
            pdb = path.join(target:targetdir(), target:name() .. ".pdb")
        end
        if not os.isfile(dll) then
            raise("DLL not built: " .. dll .. " - run `xmake` first")
        end
        local stage = path.join(os.projectdir(), "build", "package", "HighFPSPhysicsFix")
        os.tryrm(stage)
        local plugins_dir = path.join(stage, "F4SE", "Plugins")
        os.mkdir(plugins_dir)
        os.cp(dll, plugins_dir)
        if pdb and os.isfile(pdb) then
            os.cp(pdb, plugins_dir)
        end
        if os.isfile("HighFPSPhysicsFix.ini") then
            os.cp("HighFPSPhysicsFix.ini", plugins_dir)
        end
        cprint("${bright green}package: ${clear}staged at %s", stage)
    end)
