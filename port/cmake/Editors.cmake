# The GameMaker design tools (code/GM), built like the player: the 1994 sources behind shim/gmcompat.h, with the
# assembly modules replaced by C++ (src/*_asm.cpp). The editors are compiled with GM_EDITOR, which turns off the
# playgame-only "old mouse" mode of GENC.C / WINDIO.C (see shim/gmcompat.h).
#
#   gm_editor(<name> [NEWMOUSE] MAIN <file in code/GM> SOURCES <files in code/GM> [PORT <files in port/src>])
#   NEWMOUSE: the program uses the TRANMOUS.HPP mouse stack instead of the INT 33h wrapper (OLDMOUSE.ASM)
#
# makes the target gm<name>. Sources are built per program, because the same files are compiled differently for the
# player (MOUSE) and the editors.

set(GM_EDITOR_ASM_REPLACEMENTS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx_asm.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/text_asm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/input_asm.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/editor_globals.cpp)

function(gm_editor name)
  cmake_parse_arguments(E "NEWMOUSE" "MAIN" "SOURCES;PORT" ${ARGN})
  set(srcs)
  foreach(f ${E_SOURCES})
    list(APPEND srcs ${GM_SRC_GM}/${f})
  endforeach()
  foreach(f ${E_PORT})
    list(APPEND srcs ${CMAKE_CURRENT_SOURCE_DIR}/src/${f})
  endforeach()
  # <main>.C followed by the entry point (edit_entry.h): one generated translation unit
  set(entry ${CMAKE_BINARY_DIR}/editors/${name}_entry.cpp)
  file(WRITE ${entry} "#include \"${GM_SRC_GM}/${E_MAIN}\"\n#include \"edit_entry.h\"\n")
  add_library(gm${name}_objs OBJECT ${entry} ${srcs} ${GM_EDITOR_ASM_REPLACEMENTS})
  set_source_files_properties(${entry} ${srcs} PROPERTIES LANGUAGE CXX)
  target_include_directories(gm${name}_objs PRIVATE ${GM_ENGINE_INCLUDES})
  target_compile_options(gm${name}_objs PRIVATE ${GM_SHIM_OPTIONS})
  target_compile_definitions(gm${name}_objs PRIVATE GM_EDITOR=1)
  if(E_NEWMOUSE)
    target_compile_definitions(gm${name}_objs PRIVATE GM_NEWMOUSE=1)
  endif()
  set_target_properties(gm${name}_objs PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED OFF)
  add_executable(gm${name} ${CMAKE_CURRENT_SOURCE_DIR}/src/gmedit_main.cpp $<TARGET_OBJECTS:gm${name}_objs>)
  set_target_properties(gm${name} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
  target_link_libraries(gm${name} PRIVATE gmplat)
  if(NOT WIN32 AND NOT EMSCRIPTEN)
    target_link_libraries(gm${name} PRIVATE ${CMAKE_DL_LIBS} m)
  endif()
  if(EMSCRIPTEN)
    # as for gmplay (CMakeLists.txt): every wait is a loop that calls gm_pump(), which Asyncify lets park itself
    set(GM_WEB_TARGET "node" CACHE STRING "node or browser")
    set_target_properties(gm${name} PROPERTIES SUFFIX ".js")
    target_link_options(gm${name} PRIVATE -sASYNCIFY=1 -sASYNCIFY_STACK_SIZE=524288 -sSTACK_SIZE=2097152
      -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=33554432 -sEXIT_RUNTIME=1 -sUSE_GLFW=3 -sASSERTIONS=1)
    if(GM_WEB_TARGET STREQUAL "node")
      target_link_options(gm${name} PRIVATE -sENVIRONMENT=node -sNODERAWFS=1
        --pre-js ${CMAKE_CURRENT_SOURCE_DIR}/src/web_node_env.js)
      file(WRITE ${CMAKE_BINARY_DIR}/gm${name}-node "#!/bin/sh\nexec node \"${CMAKE_BINARY_DIR}/gm${name}.js\" \"$@\"\n")
      file(CHMOD ${CMAKE_BINARY_DIR}/gm${name}-node PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
    if(GM_WEB_TARGET STREQUAL "browser")
      # One ES module per program (web/gmedit-web.js loads them one at a time and shares the files through IndexedDB).
      # The page starts the program (INVOKE_RUN=0) and is told when it ends (gmedit_main.cpp, page_notify).
      target_link_options(gm${name} PRIVATE -sENVIRONMENT=web -sINVOKE_RUN=0 -sFORCE_FILESYSTEM=1 -lidbfs.js
        -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createGmProgram
        -sINCOMING_MODULE_JS_API=arguments,canvas,noInitialRun,noExitRuntime,onAbort,onExit,onRuntimeInitialized,postRun,preRun,print,printErr,locateFile,wasmBinary,instantiateWasm,setStatus,thisProgram,ENVIRONMENT,preinitializedWebGLContext,elementPointerLock,onFullScreen
        -sEXIT_RUNTIME=0
        "-sEXPORTED_FUNCTIONS=_main,_gm_web_command" "-sEXPORTED_RUNTIME_METHODS=FS,callMain,HEAPF32,HEAP32,HEAPU8,HEAPU32,HEAP16,HEAPU16"
        --pre-js ${CMAKE_CURRENT_SOURCE_DIR}/web/editor_pre.js)
    endif()
  endif()
endfunction()

# Programs that use the old (INT 33h) mouse interface
gm_editor(menu    MAIN MENU.C    SOURCES WINDIO.C GENC.C GRAPHC.C FINDFILE.CPP)
gm_editor(utility MAIN UTILITY.C SOURCES WINDIO.C GENC.C GRAPHC.C FINDFILE.CPP JSTICKC.C)

# Programs that use the new (interrupt-driven, class based) mouse and input stack of TRANMOUS.HPP / GENINPUT.CPP
set(GM_EDITOR_INPUT_SOURCES GENCLASS.CPP GASCLASS.CPP GENINPUT.CPP TIMER.CPP WINDCLSS.CPP FACELIFT.C)
gm_editor(palchos NEWMOUSE MAIN PALCHOS.C
  SOURCES WINDIO.C GENC.C GRAPHC.C FINDFILE.CPP PALC.C ${GM_EDITOR_INPUT_SOURCES}
  PORT newmouse_port.cpp)

set(GM_EDITOR_COMMON WINDIO.C GENC.C GRAPHC.C FINDFILE.CPP PALC.C BLOCC.C ${GM_EDITOR_INPUT_SOURCES})
gm_editor(blocedit NEWMOUSE MAIN BLOCEDIT.C SOURCES ${GM_EDITOR_COMMON} PORT newmouse_port.cpp)
gm_editor(mapmaker NEWMOUSE MAIN MAPMAKER.C SOURCES ${GM_EDITOR_COMMON} MAPC.C PORT newmouse_port.cpp clrbloc_port.cpp)
gm_editor(monedit  NEWMOUSE MAIN MONEDIT.C  SOURCES ${GM_EDITOR_COMMON} PORT newmouse_port.cpp)
gm_editor(charedit NEWMOUSE MAIN CHAREDIT.C SOURCES ${GM_EDITOR_COMMON} SOUNDC.C JSTICKC.C PORT newmouse_port.cpp sound_port.cpp)
gm_editor(image    NEWMOUSE MAIN IMAGE.C    SOURCES ${GM_EDITOR_COMMON} GIFC.C PORT newmouse_port.cpp)
gm_editor(grator   NEWMOUSE MAIN GRATOR.C   SOURCES ${GM_EDITOR_COMMON} GRAMAP.C PORT newmouse_port.cpp)
gm_editor(sndedit  NEWMOUSE MAIN SNDEDIT.C  SOURCES ${GM_EDITOR_COMMON} SOUNDC.C JSTICKC.C PORT newmouse_port.cpp sound_port.cpp)

# The launcher: native replacement of GM.EXE (runs menu and the tools by exit code, see src/gmlaunch_main.cpp)
if(NOT EMSCRIPTEN)
add_executable(gmlaunch src/gmlaunch_main.cpp)
set_target_properties(gmlaunch PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
add_dependencies(gmlaunch gmmenu gmutility gmpalchos gmblocedit gmmonedit gmmapmaker gmcharedit gmimage gmsndedit gmgrator)
endif()

# The player as one more program of the design tools page (web/gmedit-web.js "Play"): the same engine objects as gmplay,
# linked like an editor (one ES module that the page starts after it has mounted the shared folder). Only the browser target.
if(EMSCRIPTEN AND GM_WEB_TARGET STREQUAL "browser")
  add_executable(gmplayer src/gmplay_main.cpp $<TARGET_OBJECTS:engine_objs>)
  set_target_properties(gmplayer PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON SUFFIX ".js")
  target_compile_definitions(gmplayer PRIVATE GM_PLAYER_MODULE=1)
  target_link_libraries(gmplayer PRIVATE gmplat)
  target_link_options(gmplayer PRIVATE -sASYNCIFY=1 -sASYNCIFY_STACK_SIZE=524288 -sSTACK_SIZE=2097152
    -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=33554432 -sUSE_GLFW=3 -sASSERTIONS=1
    -sENVIRONMENT=web -sINVOKE_RUN=0 -sFORCE_FILESYSTEM=1 -lidbfs.js
    -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createGmProgram
    -sINCOMING_MODULE_JS_API=arguments,canvas,noInitialRun,noExitRuntime,onAbort,onExit,onRuntimeInitialized,postRun,preRun,print,printErr,locateFile,wasmBinary,instantiateWasm,setStatus,thisProgram,ENVIRONMENT,preinitializedWebGLContext,elementPointerLock,onFullScreen
    -sEXIT_RUNTIME=0
    "-sEXPORTED_FUNCTIONS=_main,_gm_web_command" "-sEXPORTED_RUNTIME_METHODS=FS,callMain,HEAPF32,HEAP32,HEAPU8,HEAPU32,HEAP16,HEAPU16"
    --pre-js ${CMAKE_CURRENT_SOURCE_DIR}/web/editor_pre.js)
endif()
