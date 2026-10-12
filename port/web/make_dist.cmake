# Assembles the web distributable: a folder of static files for any web server (run through the web_dist target).
#
#   inputs: GM_BUILD_DIR  where gmplay.js and gmplay.wasm are
#           GM_WEB_SRC    port/web (the page)
#           GM_REPO       repository root (games, licences)
#           GM_GAMES      games to pack: names separated by ";" or ",", "all" (every GameMaker 3.0 game in cd/gameware)
#                         or "none" (players then drop their own game folders on the page)
#           GM_OUT        output folder
cmake_minimum_required(VERSION 3.20)

foreach(var GM_BUILD_DIR GM_WEB_SRC GM_REPO GM_GAMES GM_OUT)
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "make_dist.cmake: ${var} is not set")
  endif()
endforeach()
foreach(f gmplay.js gmplay.wasm)
  if(NOT EXISTS "${GM_BUILD_DIR}/${f}")
    message(FATAL_ERROR "make_dist.cmake: ${GM_BUILD_DIR}/${f} is missing; build the gmplay target first")
  endif()
endforeach()

# GM_STANDALONE=<game> (optional): instead of a site with a game list, make ONE html file holding the page, the engine and
# that single game, which opens from disk with a double click (no web server) and starts with a Play button.
if(NOT DEFINED GM_STANDALONE)
  set(GM_STANDALONE "")
endif()
if(NOT GM_STANDALONE STREQUAL "")
  set(GM_GAMES "${GM_STANDALONE}")
endif()

file(REMOVE_RECURSE "${GM_OUT}")
file(MAKE_DIRECTORY "${GM_OUT}/games")

file(COPY "${GM_WEB_SRC}/index.html" "${GM_WEB_SRC}/gmplay-web.js" DESTINATION "${GM_OUT}")
file(COPY "${GM_BUILD_DIR}/gmplay.js" "${GM_BUILD_DIR}/gmplay.wasm" DESTINATION "${GM_OUT}")

# ---- games ------------------------------------------------------------------------------------------------------------
set(game_root "${GM_REPO}/cd/gameware")

# A GameMaker 3.0 game's .gam file starts with "GM"; the older shareware games (own player program) do not.
function(is_v3_game dir out)
  set(${out} FALSE PARENT_SCOPE)
  file(GLOB gams LIST_DIRECTORIES false "${dir}/*.gam" "${dir}/*.GAM")
  list(LENGTH gams n)
  if(n EQUAL 0)
    return()
  endif()
  list(GET gams 0 gam)
  file(READ "${gam}" magic LIMIT 2 HEX)
  if(magic STREQUAL "474d")
    set(${out} TRUE PARENT_SCOPE)
  endif()
endfunction()

string(REPLACE "," ";" games "${GM_GAMES}")
string(TOLOWER "${games}" games_lc)
set(wanted "")
if(games_lc STREQUAL "all")
  file(GLOB entries LIST_DIRECTORIES true RELATIVE "${game_root}" "${game_root}/*")
  list(SORT entries)
  foreach(e ${entries})
    if(IS_DIRECTORY "${game_root}/${e}")
      is_v3_game("${game_root}/${e}" ok)
      if(ok)
        list(APPEND wanted "${e}")
      endif()
    endif()
  endforeach()
elseif(NOT games_lc STREQUAL "none" AND NOT games STREQUAL "")
  set(wanted ${games})
endif()

set(json "")
set(packed "")
foreach(g ${wanted})
  if(NOT IS_DIRECTORY "${game_root}/${g}")
    message(FATAL_ERROR "make_dist.cmake: there is no game '${g}' in ${game_root} (the shareware games in cd/sharware use an older format that this player cannot run)")
  endif()
  is_v3_game("${game_root}/${g}" ok)
  if(NOT ok)
    message(FATAL_ERROR "make_dist.cmake: ${g} is not a GameMaker 3.0 game (its .gam file does not start with GM)")
  endif()
  # every file of the game except editor backups and DOS programs
  file(GLOB_RECURSE all LIST_DIRECTORIES false RELATIVE "${game_root}/${g}" "${game_root}/${g}/*")
  set(keep "")
  foreach(f ${all})
    if(NOT f MATCHES "\\.([bB][aA][kK]|[eE][xX][eE]|[cC][oO][mM]|[bB][aA][tT]|[tT][mM][pP])$")
      list(APPEND keep "${f}")
    endif()
  endforeach()
  set(zip "${GM_OUT}/games/${g}.zip")
  execute_process(COMMAND "${CMAKE_COMMAND}" -E tar cf "${zip}" --format=zip -- ${keep}
    WORKING_DIRECTORY "${game_root}/${g}" RESULT_VARIABLE rc)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "make_dist.cmake: could not make ${zip}")
  endif()
  file(SIZE "${zip}" bytes)
  string(SUBSTRING "${g}" 0 1 first)
  string(TOUPPER "${first}" first)
  string(SUBSTRING "${g}" 1 -1 rest)
  if(NOT json STREQUAL "")
    string(APPEND json ",\n")
  endif()
  string(APPEND json "  {\"id\": \"${g}\", \"name\": \"${first}${rest}\", \"zip\": \"games/${g}.zip\", \"bytes\": ${bytes}}")
  list(APPEND packed "${g}")
endforeach()
file(WRITE "${GM_OUT}/games.json" "[\n${json}\n]\n")

# ---- standalone: everything in one html file ----------------------------------------------------------------------------
if(NOT GM_STANDALONE STREQUAL "")
  set(g "${GM_STANDALONE}")
  file(READ "${GM_WEB_SRC}/index.html" page)
  file(READ "${GM_WEB_SRC}/gmplay-web.js" page_js)
  file(READ "${GM_BUILD_DIR}/gmplay.js" engine_js)
  file(READ "${GM_BUILD_DIR}/gmplay.wasm" wasm_hex HEX)       # binary data travels as hex text, decoded by gmHex2Bytes()
  file(READ "${GM_OUT}/games/${g}.zip" zip_hex HEX)
  string(REPLACE "</script" "<\\/script" page_js "${page_js}")  # nothing may end the inline <script> early
  string(REPLACE "</script" "<\\/script" engine_js "${engine_js}")
  string(SUBSTRING "${g}" 0 1 first)
  string(TOUPPER "${first}" first)
  string(SUBSTRING "${g}" 1 -1 rest)
  set(marker "<script src=\"gmplay-web.js\"></script>")
  string(FIND "${page}" "${marker}" marker_at)
  if(marker_at EQUAL -1)
    message(FATAL_ERROR "make_dist.cmake: web/index.html no longer contains ${marker}")
  endif()
  # order matters: the flag first, then the page script (it creates `Module` and sets the page up), then the data the
  # engine needs, then the engine itself, which starts compiling the embedded .wasm as soon as it runs
  string(REPLACE "${marker}"
    "<script>window.GM_STANDALONE = { name: \"${first}${rest}\" };</script>\n<script>\n${page_js}\n</script>\n<script>\nModule.wasmBinary = gmHex2Bytes(\"${wasm_hex}\");\nwindow.GM_GAME_ZIP = gmHex2Bytes(\"${zip_hex}\");\n</script>\n<script>\n${engine_js}\n</script>"
    page "${page}")
  file(WRITE "${GM_OUT}/${g}.html" "${page}")
  file(REMOVE_RECURSE "${GM_OUT}/games" "${GM_OUT}/games.json" "${GM_OUT}/index.html" "${GM_OUT}/gmplay-web.js"
    "${GM_OUT}/gmplay.js" "${GM_OUT}/gmplay.wasm")
endif()

# ---- notices ----------------------------------------------------------------------------------------------------------
file(COPY "${GM_REPO}/LICENSE" DESTINATION "${GM_OUT}")
file(COPY "${GM_REPO}/port/THIRD_PARTY.md" "${GM_REPO}/port/licenses" DESTINATION "${GM_OUT}")
list(LENGTH packed npacked)
string(REPLACE ";" ", " packed_text "${packed}")
if(npacked EQUAL 0)
  set(packed_text "(none: players drop their own game folders on the page)")
endif()
if(NOT GM_STANDALONE STREQUAL "")
  file(WRITE "${GM_OUT}/README.txt"
"${GM_STANDALONE}.html is a complete game in one file: the 1994 GameMaker engine (compiled to WebAssembly) and the game ${GM_STANDALONE}.

To play, open ${GM_STANDALONE}.html in a current Chrome, Edge, Firefox or Safari (double-click it, or put it on any web
server) and press Play. No server and no other files are needed. Scores, saved games and settings are kept in the browser.
Esc is kept for the game in full screen on Chrome and Edge only. Touch screens are not supported.

LICENSE, THIRD_PARTY.md and licenses/ hold the licences of the engine and of what it builds on. The game itself is not
covered by them: publish it only if you have the right to distribute it.
")
else()
file(WRITE "${GM_OUT}/README.txt"
"GameMaker Player for the web
============================

This folder is a complete static web site. Put it on any web server (no special headers or server software needed),
or try it locally:

    cd <this folder>
    python3 -m http.server 8000        then open  http://localhost:8000/

(Opening index.html straight from the disk does not work: browsers do not let a page load .wasm files from file://.)

Games packed into this build: ${packed_text}
Players can also drop a game folder or a .zip onto the page; those files are read in the browser and not uploaded.

Contents
  index.html, gmplay-web.js   the page
  gmplay.js, gmplay.wasm      the player (the 1994 engine compiled to WebAssembly)
  games.json, games/*.zip     the packed games (games.json lists them for the page)
  LICENSE, THIRD_PARTY.md, licenses/   licences of the engine and of what it builds on

Before you publish: the games in games/ are not covered by the licences above. Include only games you have the right to
distribute. Rebuild with a different list:  bash port/build_web.sh --games none   (or --games name1,name2 or --games all)

Browsers: current Chrome, Edge, Firefox and Safari (WebAssembly, DecompressionStream for zip files). Esc is kept for the
game in full screen on Chrome and Edge only. Touch screens are not supported.
")
endif()

# ---- one file to hand around ------------------------------------------------------------------------------------------
get_filename_component(out_name "${GM_OUT}" NAME)
get_filename_component(out_parent "${GM_OUT}" DIRECTORY)
file(REMOVE "${out_parent}/${out_name}.zip")
execute_process(COMMAND "${CMAKE_COMMAND}" -E tar cf "${out_parent}/${out_name}.zip" --format=zip -- .
  WORKING_DIRECTORY "${GM_OUT}" RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(WARNING "make_dist.cmake: could not make ${out_parent}/${out_name}.zip")
endif()

if(NOT GM_STANDALONE STREQUAL "")
  file(SIZE "${GM_OUT}/${GM_STANDALONE}.html" html_bytes)
  message(STATUS "Standalone game ready: ${GM_OUT}/${GM_STANDALONE}.html  (${html_bytes} bytes, opens by double-click)")
else()
  file(SIZE "${GM_OUT}/gmplay.wasm" wasm_bytes)
  message(STATUS "Web distributable ready: ${GM_OUT}  (games: ${packed_text}; gmplay.wasm ${wasm_bytes} bytes)")
endif()
message(STATUS "                         and ${out_parent}/${out_name}.zip")
