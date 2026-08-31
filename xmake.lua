set_project("windrose-rcon")
set_version("1.1.4")

add_rules("mode.release")

target("version")
    set_kind("shared")
    add_files("src/version.cpp", 
              "src/windrose_engine.cpp",
              "src/rcon/rcon_server.cpp",
              "src/crypto/aes_crypto.cpp",
              "src/utils/pattern_finder.cpp",
              "src/ban/ban_list.cpp",
              "src/commands/help.cpp",
              "src/commands/info.cpp",
              "src/commands/showplayers.cpp",
              "src/commands/kick.cpp",
              "src/commands/ban.cpp",
              "src/commands/unban.cpp",
              "src/commands/banlist.cpp",
              "src/commands/getpos.cpp",
              "src/commands/shutdown.cpp",
              "src/commands/uptime.cpp",
              "src/commands/playerinfo.cpp")
    add_headerfiles("src/offsets.h",
                    "src/windrose_engine.h",
                    "src/config/config.h",
                    "src/rcon/rcon_server.h",
                    "src/crypto/aes_crypto.h",
                    "src/utils/pattern_finder.h",
                    "src/ban/ban_list.h",
                    "src/commands/commands.h")
    add_includedirs("src")
    set_filename("version.dll")
    set_targetdir("dist")
    
    if is_plat("windows") then
        add_syslinks("kernel32", "user32", "ws2_32", "bcrypt")
    end
