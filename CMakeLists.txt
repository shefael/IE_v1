cmake_minimum_required(VERSION 3.20)
project(IntelliEditor LANGUAGES C CXX)

# -------------------------------------------------------------------
# 1. Configuration du compilateur et des flags C/C++
# -------------------------------------------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Détection spécifique à MinGW (utilisé sur Windows)

if(MINGW)
    add_compile_options(-Wall -Wextra -Wpedantic)
    set(CMAKE_C_FLAGS_RELEASE   "-O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
    set(CMAKE_C_FLAGS_DEBUG     "-g -O0")
    set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0")
endif()

# -------------------------------------------------------------------
# 2. Chemins des bibliothèques llama.cpp pré-compilées
# -------------------------------------------------------------------
set(LLAMA_BUILD_DIR ${CMAKE_SOURCE_DIR}/external/llama.cpp/build)

set(LLAMA_LIBS
    ${LLAMA_BUILD_DIR}/src/libllama.a
    ${LLAMA_BUILD_DIR}/common/libllama-common.a
    ${LLAMA_BUILD_DIR}/common/libllama-common-base.a
    ${LLAMA_BUILD_DIR}/ggml/src/ggml.a
    ${LLAMA_BUILD_DIR}/ggml/src/ggml-base.a
    ${LLAMA_BUILD_DIR}/ggml/src/ggml-cpu.a
)

# -------------------------------------------------------------------
# Hunspell — bibliothèque système MinGW
# -------------------------------------------------------------------
set(HUNSPELL_INCLUDE_DIR "/mingw64/include/hunspell")
set(HUNSPELL_LIB         "C:/msys64/mingw64/lib/libhunspell-1.7.a")

# -------------------------------------------------------------------
# 3. Liste des fichiers sources du projet
# -------------------------------------------------------------------
set(PROJECT_SOURCES
    src/main.c
    src/editor/gap_buffer.c
    src/editor/undo_redo.c
    src/ui/main_window.c
    src/ui/scintilla_wrapper.c
    src/llm/llm_interface.c
    src/llm/llm_thread.c
    src/nlp/hunspell_wrap.c
    src/nlp/tokenizer.c
    src/nlp/nlp_engine.c
    src/rules/rule_engine.c
    src/rules/rule_parser.c
    src/utils/encoding.c
    src/utils/config.c
)

# -------------------------------------------------------------------
# 4. Bibliothèques externes
# -------------------------------------------------------------------

# --- cJSON ---
add_library(cjson STATIC external/cJSON/cJSON.c)
target_include_directories(cjson PUBLIC external/cJSON)

# --- Scintilla ---
set(SCINTILLA_SOURCES
    # --- Fichiers src/ (base) ---
    external/scintilla/src/AutoComplete.cxx
    external/scintilla/src/CallTip.cxx
    external/scintilla/src/CaseConvert.cxx        
    external/scintilla/src/CaseFolder.cxx         
    external/scintilla/src/CellBuffer.cxx
    external/scintilla/src/ChangeHistory.cxx
    external/scintilla/src/CharacterCategoryMap.cxx 
    external/scintilla/src/CharacterType.cxx        
    external/scintilla/src/CharClassify.cxx
    external/scintilla/src/ContractionState.cxx
    external/scintilla/src/DBCS.cxx               
    external/scintilla/src/Decoration.cxx
    external/scintilla/src/Document.cxx
    external/scintilla/src/EditModel.cxx
    external/scintilla/src/EditView.cxx
    external/scintilla/src/Editor.cxx
    external/scintilla/src/Geometry.cxx           
    external/scintilla/src/Indicator.cxx
    external/scintilla/src/KeyMap.cxx
    external/scintilla/src/LineMarker.cxx
    external/scintilla/src/MarginView.cxx
    external/scintilla/src/PerLine.cxx
    external/scintilla/src/PositionCache.cxx
    external/scintilla/src/RESearch.cxx
    external/scintilla/src/RunStyles.cxx
    external/scintilla/src/ScintillaBase.cxx      
    external/scintilla/src/Selection.cxx
    external/scintilla/src/Style.cxx
    external/scintilla/src/UndoHistory.cxx
    external/scintilla/src/UniConversion.cxx
    external/scintilla/src/UniqueString.cxx       
    external/scintilla/src/ViewStyle.cxx
    external/scintilla/src/XPM.cxx

    # --- Fichiers win32/ (plateforme Windows) ---
    external/scintilla/win32/HanjaDic.cxx         
    external/scintilla/win32/PlatWin.cxx
    external/scintilla/win32/ListBox.cxx          
    external/scintilla/win32/SurfaceGDI.cxx        
    external/scintilla/win32/SurfaceD2D.cxx        
    external/scintilla/win32/ScintillaWin.cxx
)

add_library(scintilla STATIC ${SCINTILLA_SOURCES})
target_include_directories(scintilla PUBLIC
    external/scintilla/include
    external/scintilla/src
    external/scintilla/win32
)
target_compile_definitions(scintilla PRIVATE
    UNICODE _UNICODE SCI_LEXER _CRT_SECURE_NO_WARNINGS
)
target_compile_options(scintilla PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-include cstdint>
    $<$<CXX_COMPILER_ID:Clang>:-include cstdint>
)
if(MINGW)
    target_compile_options(scintilla PRIVATE
        -Wno-unused-parameter -Wno-pedantic
    )
endif()
set_target_properties(scintilla PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# -------------------------------------------------------------------
# 5. Création de l'exécutable
# -------------------------------------------------------------------
add_executable(IntelliEditor ${PROJECT_SOURCES})

target_include_directories(IntelliEditor PRIVATE
    include
    src
    src/editor
    src/ui
    src/llm
    src/nlp
    src/rules
    src/utils
    external/cJSON
    external/scintilla/include
    external/llama.cpp
    external/llama.cpp/include
    external/llama.cpp/common
    external/llama.cpp/ggml/include
)

# -------------------------------------------------------------------
# 6. Linkage
# -------------------------------------------------------------------
# --- Ajoute ceci près du début ---
find_package(OpenMP REQUIRED)
target_link_options(IntelliEditor PRIVATE
    -static-libgcc
    -static-libstdc++
)
target_link_libraries(IntelliEditor
    cjson
    scintilla
    ${LLAMA_LIBS}
    OpenMP::OpenMP_C
    OpenMP::OpenMP_CXX
    ${HUNSPELL_LIB}
    ws2_32
    user32
    gdi32
    comctl32
    imm32
    ole32
    uuid
)

set_target_properties(IntelliEditor PROPERTIES
    WIN32_EXECUTABLE TRUE
)
