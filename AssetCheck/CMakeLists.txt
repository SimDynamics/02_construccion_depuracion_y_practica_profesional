cmake_minimum_required(VERSION 3.20)
project(AssetCheck LANGUAGES CXX)
add_library(assetcheck_registro STATIC src/registro.cpp)
target_include_directories(assetcheck_registro PUBLIC include)
target_compile_features(assetcheck_registro PUBLIC cxx_std_20)
set_target_properties(assetcheck_registro PROPERTIES CXX_EXTENSIONS OFF)
add_executable(comprobar_registro pruebas/casos.cpp)
target_link_libraries(comprobar_registro PRIVATE assetcheck_registro)
set_target_properties(comprobar_registro PROPERTIES CXX_EXTENSIONS OFF)
enable_testing()
add_test(NAME comprobar_registro COMMAND comprobar_registro)

# Añadir después de definir assetcheck_registro.
add_library(assetcheck_lote STATIC src/lote.cpp)
target_include_directories(assetcheck_lote PUBLIC include)
target_link_libraries(assetcheck_lote PUBLIC assetcheck_registro)
target_compile_features(assetcheck_lote PUBLIC cxx_std_20)
set_target_properties(assetcheck_lote PROPERTIES CXX_EXTENSIONS OFF)

add_executable(comprobar_lote pruebas/lote_casos.cpp)
target_link_libraries(comprobar_lote PRIVATE assetcheck_lote)
add_test(NAME comprobar_lote COMMAND comprobar_lote)
set_target_properties(comprobar_lote PROPERTIES CXX_EXTENSIONS OFF)

add_library(assetcheck_manifiesto STATIC src/manifiesto.cpp)
target_include_directories(assetcheck_manifiesto PUBLIC include)
target_link_libraries(assetcheck_manifiesto PUBLIC assetcheck_lote)
target_compile_features(assetcheck_manifiesto PUBLIC cxx_std_23)
set_target_properties(assetcheck_manifiesto PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)
