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

    -- Generated headers/resources (Phase 4 will swap hardcoded version for git-describe)
    set_configvar("PROJECT_NAME", "HighFPSPhysicsFix")
    set_configvar("PROJECT_VERSION", "0.8.13")
    set_configvar("PROJECT_VERSION_MAJOR", 0)
    set_configvar("PROJECT_VERSION_MINOR", 8)
    set_configvar("PROJECT_VERSION_PATCH", 13)
    set_configvar("PROJECT_VERSION_DESCRIBE", "0.8.13")
    set_configvar("CMAKE_CURRENT_SOURCE_DIR", (os.projectdir():gsub("\\", "/")))
    set_configdir("$(builddir)/HighFPSPhysicsFix.gen")
    add_configfiles("cmake/Version.h.in", { filename = "include/Version.h", pattern = "@(.-)@" })
    add_configfiles("cmake/version.rc.in", { filename = "version.rc", pattern = "@(.-)@" })
    add_configfiles("cmake/resources.rc.in", { filename = "resources.rc", pattern = "@(.-)@" })
    add_includedirs("$(builddir)/HighFPSPhysicsFix.gen/include")
    add_files(
        "$(builddir)/HighFPSPhysicsFix.gen/version.rc",
        "$(builddir)/HighFPSPhysicsFix.gen/resources.rc"
    )
