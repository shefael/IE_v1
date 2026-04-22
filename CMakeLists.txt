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
    set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
    set(CMAKE_C_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
endif()

# -------------------------------------------------------------------
# 2. Liste des fichiers sources du projet
# -------------------------------------------------------------------
set(PROJECT_SOURCES
    src/main.c
    src/editor/gap_buffer.c
    src/editor/undo_redo.c
    src/ui/main_window.c
    src/ui/scintilla_wrapper.c
    src/llm/llm_interface.c
    src/nlp/hunspell_wrap.c
    src/rules/rule_engine.c
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
    UNICODE
    _UNICODE
    SCI_LEXER
    _CRT_SECURE_NO_WARNINGS
)

# 🔧 Fix UINT8_MAX / uint8_t MinGW
target_compile_options(scintilla PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-include cstdint>
    $<$<CXX_COMPILER_ID:Clang>:-include cstdint>
)

if(MINGW)
    target_compile_options(scintilla PRIVATE
        -Wno-unused-parameter
        -Wno-pedantic
    )
endif()

set_target_properties(scintilla PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# --- llama.cpp ---
option(LLAMA_BUILD_EXAMPLES OFF)
option(LLAMA_BUILD_TESTS OFF)
option(BUILD_SHARED_LIBS OFF)
option(LLAMA_CURL OFF)
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)

add_subdirectory(external/llama.cpp)

# -------------------------------------------------------------------
# 5. Création et configuration de l'exécutable
# -------------------------------------------------------------------
add_executable(IntelliEditor ${PROJECT_SOURCES})

# Inclure les headers de ton projet
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
    external/llama.cpp
)

# Ajout des répertoires pour les headers externes si besoin
target_include_directories(IntelliEditor PRIVATE external/scintilla/include)

# -------------------------------------------------------------------
# 6. Linkage des bibliothèques
# -------------------------------------------------------------------
target_link_libraries(IntelliEditor
    cjson
    scintilla
    llama
    ggml
)

# Ajout des flags spécifiques Windows
if(WIN32)
    set_target_properties(IntelliEditor PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    target_link_libraries(IntelliEditor
        ws2_32      # Winsock2 pour llama.cpp
        user32      # Interface utilisateur Windows
        gdi32       # Graphiques
        comctl32    # Contrôles communs
        imm32       # Gestion des entrées IME
    )
endif()

# -------------------------------------------------------------------
# 7. Copie des DLLs/objets (optionnel)
# -------------------------------------------------------------------
if(MINGW AND WIN32)
    add_custom_command(TARGET IntelliEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:llama>" "$<TARGET_FILE_DIR:IntelliEditor>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:ggml>" "$<TARGET_FILE_DIR:IntelliEditor>"
    )
endif()