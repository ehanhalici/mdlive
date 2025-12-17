cmake_minimum_required(VERSION 3.14)
project(mdlive CXX)

set(CMAKE_CXX_STANDARD 17)

# --- 1. Yolları ve Paketleri Ayarla ---

# FLTK klasörünün tam yolunu al
set(FLTK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/fltk-1.4.4")

# Kontrol: Klasör gerçekten orada mı?
if(NOT EXISTS "${FLTK_ROOT}/FL")
    message(FATAL_ERROR "HATA: FLTK dosyalari bulunamadi! Yol: ${FLTK_ROOT}/FL \n Lutfen 'dependencies/fltk' klasorunu kontrol et.")
else()
    message(STATUS "FLTK Yolu Dogrulandi: ${FLTK_ROOT}")
endif()

# Kütüphane klasörü
set(FLTK_LIB_DIR "${FLTK_ROOT}/lib")

# OpenGL Bul
find_package(OpenGL REQUIRED)

# --- 2. Include Yolları (Kritik Düzeltme) ---
include_directories(
    src
    ${FLTK_ROOT}      # <FL/Fl.H> diyebilmek için burasi sart
    ${FLTK_ROOT}/src  # Bazen gerekli olabilir
    ${OPENGL_INCLUDE_DIR}
)

# --- 3. Executable ---
set(PROJECT_SRC
    src/Callback.cpp
    src/Editor.cpp
    src/FileSystem.cpp
    src/Highlighter.cpp
    src/main.cpp
    src/Ui.cpp
    src/Parser.cpp

  
)
add_executable(mdlive ${PROJECT_SRC})

target_include_directories(mdlive PRIVATE include)
# --- 4. Linkleme ---
target_link_libraries(mdlive
    PRIVATE
    # FLTK Statik Libleri (Sırayla)
    ${FLTK_LIB_DIR}/libfltk_images.a
    ${FLTK_LIB_DIR}/libfltk_gl.a
    ${FLTK_LIB_DIR}/libfltk.a
    ${FLTK_LIB_DIR}/libfltk_jpeg.a
    ${FLTK_LIB_DIR}/libfltk_png.a
    ${FLTK_LIB_DIR}/libfltk_z.a
    
    # OpenGL
    ${OPENGL_LIBRARIES}
    
    # Sistem (X11 vb.)
    X11 Xext Xft fontconfig Xinerama Xcursor Xrender Xfixes
    
    # Temel
    pthread dl m
)

if(UNIX AND NOT APPLE)
    target_link_options(mdlive PRIVATE -static-libgcc -static-libstdc++)
endif()
