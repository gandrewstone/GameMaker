cmake_minimum_required(VERSION 3.20)
project(gamemaker_port C CXX)

set(CMAKE_C_STANDARD 99)
if(MSVC)
  add_compile_options(/Zi)           # debug info for crash reports
  add_link_options(/DEBUG)
endif()

include(FetchContent)
if(EMSCRIPTEN)
  set(PLATFORM Web CACHE STRING "raylib platform" FORCE)   # WebGL canvas instead of a desktop window
endif()
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(raylib)

# ymfm (BSD-3-Clause): cycle-accurate Yamaha FM chips; the OPL2 (YM3812) is the Sound Blaster's FM synthesizer.
# It has no CMake project of its own, so its sources are added directly.
FetchContent_Declare(ymfm
  GIT_REPOSITORY https://github.com/aaronsgiles/ymfm.git
  GIT_TAG 81aec25ccbb98f4873a255f7551ac4dadac59b4a)
FetchContent_GetProperties(ymfm)
if(NOT ymfm_POPULATED)
  FetchContent_Populate(ymfm)
endif()

add_library(gmplat STATIC src/fb.c src/gif.c src/audio.cpp src/dospath.c src/settings.c
  ${ymfm_SOURCE_DIR}/src/ymfm_opl.cpp ${ymfm_SOURCE_DIR}/src/ymfm_adpcm.cpp ${ymfm_SOURCE_DIR}/src/ymfm_pcm.cpp)
target_include_directories(gmplat PRIVATE ${ymfm_SOURCE_DIR}/src)
set_target_properties(gmplat PROPERTIES CXX_STANDARD 17)
target_include_directories(gmplat PUBLIC src)
target_link_libraries(gmplat PUBLIC raylib)

# The web build (Emscripten) is just gmplay; the viewers, converters and self-tests are built natively.
if(EMSCRIPTEN)
  set(GM_DESKTOP_TOOLS OFF)
else()
  set(GM_DESKTOP_TOOLS ON)
endif()

if(GM_DESKTOP_TOOLS)
  add_executable(gmview src/gmview.c)
  target_link_libraries(gmview PRIVATE gmplat)

  # Reader/comparer for frame dumps from the instrumented DOS playgame.
  add_executable(fdump tools/fdump.c)
  target_link_libraries(fdump PRIVATE raylib)
endif()

# ---------------------------------------------------------------------------------------------
# The original GameMaker engine (code/), compiled in place as C++ behind shim/gmcompat.h
# (Borland 16-bit compatibility: 16-bit int, far pointers into an emulated 1 MB address space).
# ---------------------------------------------------------------------------------------------
set(GM_CODE ${CMAKE_CURRENT_SOURCE_DIR}/../code)
# Directories the engine is compiled from. On Windows these are the originals; elsewhere cleaned copies (cmake/IncludeShadow.cmake).
set(GM_SRC_GM ${GM_CODE}/GM)
set(GM_SRC_DRV ${GM_CODE}/DRV)
set(GM_SRC_FLI ${GM_CODE}/Image/FLI)
if(NOT WIN32)
  include(cmake/IncludeShadow.cmake)
  set(GM_SRC_GM ${CMAKE_BINARY_DIR}/gm_src/GM)
  set(GM_SRC_DRV ${CMAKE_BINARY_DIR}/gm_src/DRV)
  set(GM_SRC_FLI ${CMAKE_BINARY_DIR}/gm_src/FLI)
  gm_mirror_dir(${GM_CODE}/GM ${GM_SRC_GM})
  gm_mirror_dir(${GM_CODE}/DRV ${GM_SRC_DRV})
  gm_mirror_dir(${GM_CODE}/Image/FLI ${GM_SRC_FLI})
endif()
set(GM_ASM_REPLACEMENTS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx_asm.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/text_asm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/input_asm.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/vgaobj.cpp)
set(GM_ENGINE_SOURCES
  ${GM_SRC_GM}/PLAYGAME.C ${GM_SRC_GM}/WINDIO.C ${GM_SRC_GM}/GENC.C
  ${GM_SRC_GM}/GIFC.C ${GM_SRC_GM}/PALC.C ${CMAKE_CURRENT_SOURCE_DIR}/src/svga_port.cpp ${GM_SRC_GM}/HISCOREL.C
  ${GM_SRC_GM}/JSTICKC.C ${GM_SRC_GM}/DIRTRECT.C ${GM_SRC_GM}/GRAPHC.C ${GM_SRC_GM}/FINDFILE.CPP
  ${CMAKE_CURRENT_SOURCE_DIR}/src/sound_port.cpp ${GM_SRC_GM}/SBMUSIC.C ${GM_SRC_DRV}/FILECLSS.CPP ${GM_SRC_FLI}/FLI.CPP)
set_source_files_properties(${GM_ENGINE_SOURCES} PROPERTIES LANGUAGE CXX)

# Compiler options for code built behind the shim. The 1994 sources have unchecked buffers, signed chars, wrapping
# arithmetic and type punning, so the checks and optimisations that assume otherwise are off.
if(MSVC)
  set(GM_SHIM_OPTIONS /FI${CMAKE_CURRENT_SOURCE_DIR}/shim/gmcompat.h /permissive /w /GS-)
else()
  # GCC and Clang. The flags that only one of them knows are probed rather than assumed.
  include(CheckCXXCompilerFlag)
  set(GM_SHIM_OPTIONS -include ${CMAKE_CURRENT_SOURCE_DIR}/shim/gmcompat.h -w -fsigned-char -fwrapv)
  foreach(flag
      -fpermissive                        # GCC: old-style conversions are warnings
      -fno-strict-aliasing                # the sources pun types
      -fno-delete-null-pointer-checks
      -Wno-c++11-narrowing                # Clang: char tables like {128, 160, ...} (the editors' button pictures)
      -Wno-narrowing                      # GCC: the same
      -fno-operator-names)                # "xor" is a function name in the sources
    string(MAKE_C_IDENTIFIER "GM_HAS${flag}" var)
    check_cxx_compiler_flag(${flag} ${var})
    if(${var})
      list(APPEND GM_SHIM_OPTIONS ${flag})
    endif()
  endforeach()
endif()

# Include directories of the engine, in Borland's search order. On case-sensitive file systems the headers are also
# reachable under every spelling the sources use (see cmake/IncludeShadow.cmake).
set(GM_ENGINE_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/shim ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${GM_SRC_GM} ${GM_SRC_DRV} ${GM_SRC_FLI})
if(NOT WIN32)
  gm_include_shadow(${CMAKE_BINARY_DIR}/gm_include ${GM_SRC_GM} ${GM_SRC_DRV} ${GM_SRC_FLI})
  # the header spellings come before port/src so that "gif.h" means the engine's GIF.H, not the port's own gif.h
  list(INSERT GM_ENGINE_INCLUDES 1 ${CMAKE_CURRENT_SOURCE_DIR}/shim_posix ${CMAKE_BINARY_DIR}/gm_include)
endif()

add_library(engine_objs OBJECT ${GM_ENGINE_SOURCES} ${GM_ASM_REPLACEMENTS})
target_include_directories(engine_objs PRIVATE ${GM_ENGINE_INCLUDES})
target_compile_options(engine_objs PRIVATE ${GM_SHIM_OPTIONS})
set_target_properties(engine_objs PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED OFF)
# Same replay frame-dump hook as the instrumented DOS build (see baseline/README.md).
target_compile_definitions(engine_objs PRIVATE FRAMEDUMP)

# Platform layer (DOS services on raylib) joins the framebuffer library.
target_sources(gmplat PRIVATE src/dosplat.c src/dosfind.c)

add_executable(gmplay src/gmplay_main.cpp $<TARGET_OBJECTS:engine_objs>)
set_target_properties(gmplay PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_link_libraries(gmplay PRIVATE gmplat)
if(NOT WIN32 AND NOT EMSCRIPTEN)
  find_package(Threads REQUIRED)                  # the hang watchdog (GM_WATCHDOG) runs on its own thread
  target_link_libraries(gmplay PRIVATE Threads::Threads ${CMAKE_DL_LIBS} m)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(gmplay PRIVATE -rdynamic)  # function names in crash backtraces
  endif()
endif()

option(GM_EDITORS "build the design tools (menu, utility, palchos, ...)" ON)
if(GM_EDITORS)
  include(cmake/Editors.cmake)
endif()

if(EMSCRIPTEN)
  # Every wait in the engine is a loop that calls gm_pump(); Asyncify lets that code park itself so that the browser
  # (or Node) can run in between (see gm_os_yield in src/dosfind.c).
  #   GM_WEB_TARGET=node     runs under Node with the real file system: used to replay recordings and compare with DOS
  #   GM_WEB_TARGET=browser  a page (web/) that loads a game into an in-memory file system; "web_dist" assembles it
  set(GM_WEB_TARGET "node" CACHE STRING "node or browser")
  set_target_properties(gmplay PROPERTIES SUFFIX ".js")
  target_link_options(gmplay PRIVATE -sASYNCIFY=1 -sASYNCIFY_STACK_SIZE=524288 -sSTACK_SIZE=2097152
    -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=33554432 -sEXIT_RUNTIME=1 -sUSE_GLFW=3 -sASSERTIONS=1)
  if(GM_WEB_TARGET STREQUAL "node")
    target_link_options(gmplay PRIVATE -sENVIRONMENT=node -sNODERAWFS=1 --pre-js ${CMAKE_CURRENT_SOURCE_DIR}/src/web_node_env.js)
    # <build dir>/gmplay-node runs it like a native program (baseline/check_states.sh: GMPLAY=<build dir>/gmplay-node)
    file(WRITE ${CMAKE_BINARY_DIR}/gmplay-node "#!/bin/sh\nexec node \"${CMAKE_BINARY_DIR}/gmplay.js\" \"$@\"\n")
    file(CHMOD ${CMAKE_BINARY_DIR}/gmplay-node PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  endif()
  if(GM_WEB_TARGET STREQUAL "browser")
    # The page decides when the engine starts (INVOKE_RUN=0) and which files it sees; IDBFS keeps scores and settings.
    target_link_options(gmplay PRIVATE -sENVIRONMENT=web -sINVOKE_RUN=0 -sFORCE_FILESYSTEM=1 -lidbfs.js
      -sINCOMING_MODULE_JS_API=arguments,canvas,noInitialRun,noExitRuntime,onAbort,onExit,onRuntimeInitialized,postRun,preRun,print,printErr,locateFile,wasmBinary,instantiateWasm,setStatus,thisProgram,ENVIRONMENT,preinitializedWebGLContext,elementPointerLock,onFullScreen   # incl. wasmBinary: the single-file build embeds the .wasm
      -sEXIT_RUNTIME=0                   # the page keeps working after the game ends; the engine reports its end itself (gmplay_main.cpp)
      "-sEXPORTED_FUNCTIONS=_main,_gm_web_command" "-sEXPORTED_RUNTIME_METHODS=FS,callMain,HEAPF32,HEAP32,HEAPU8,HEAPU32,HEAP16,HEAPU16"
      --pre-js ${CMAKE_CURRENT_SOURCE_DIR}/web/web_pre.js)

    # One step to a folder that can be put on any static web server:  cmake --build <dir> --target web_dist
    #   -DGM_WEB_GAMES=sample          games packed into it: names separated by ";" or ",", "all" or "none"
    #   -DGM_WEB_OUT=<folder>          where it goes (default: <build dir>/web)
    set(GM_WEB_GAMES "sample" CACHE STRING "games to pack into the web distributable (names, all or none)")
    set(GM_WEB_OUT "${CMAKE_BINARY_DIR}/web" CACHE PATH "web distributable folder")
    #   -DGM_WEB_STANDALONE=<game>     one html file with the player and that game inside, no server needed (see make_dist.cmake)
    set(GM_WEB_STANDALONE "" CACHE STRING "make a single self-contained html file for this game")
    add_custom_target(web_dist ALL
      COMMAND ${CMAKE_COMMAND} -DGM_BUILD_DIR=${CMAKE_BINARY_DIR} -DGM_WEB_SRC=${CMAKE_CURRENT_SOURCE_DIR}/web
        -DGM_REPO=${CMAKE_CURRENT_SOURCE_DIR}/.. "-DGM_GAMES=${GM_WEB_GAMES}" -DGM_OUT=${GM_WEB_OUT}
        "-DGM_STANDALONE=${GM_WEB_STANDALONE}"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/web/make_dist.cmake
      DEPENDS gmplay
      COMMENT "Assembling the web distributable"
      VERBATIM)
  endif()
endif()

if(GM_DESKTOP_TOOLS)
# Timeline of a captured WAV (GM_WAV=...), to check audio output without listening.
add_executable(wavstat tools/wavstat.c)
if(NOT MSVC)
  target_link_libraries(wavstat PRIVATE m)
endif()

# Offline CMF renderer: the engine's own music code (SBMUSIC.C + FM driver) on a manual clock, writing a WAV.
add_executable(cmf2wav tools/cmf2wav.cpp tools/cmf2wav_main.cpp src/sound_port.cpp ${GM_SRC_GM}/SBMUSIC.C)
set_source_files_properties(tools/cmf2wav.cpp src/sound_port.cpp ${GM_SRC_GM}/SBMUSIC.C PROPERTIES
  LANGUAGE CXX COMPILE_OPTIONS "${GM_SHIM_OPTIONS}")
target_include_directories(cmf2wav PRIVATE ${GM_ENGINE_INCLUDES})
target_link_libraries(cmf2wav PRIVATE gmplat)

# Offline VOC decoder test: runs a .voc through the mixer and writes a WAV.
add_executable(voc2wav tools/voc2wav.cpp)
target_include_directories(voc2wav PRIVATE src)
target_link_libraries(voc2wav PRIVATE gmplat)

# Self-checking audio test (speaker pitch, OPL2 note, VOC playback); no audio device needed.
add_executable(audiotest tools/audiotest.cpp)
target_include_directories(audiotest PRIVATE src)
target_link_libraries(audiotest PRIVATE gmplat)

# Self-checking test of the platform pieces that need no window or audio device (settings, picture placement,
# DOS-style file names, directory listing, volume). Run it in a scratch folder: it writes ./pt_work.
add_executable(platformtest tools/platformtest.c)
target_link_libraries(platformtest PRIVATE gmplat)

endif()  # GM_DESKTOP_TOOLS
