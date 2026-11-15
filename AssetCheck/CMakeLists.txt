cmake_minimum_required(VERSION 3.20)
project(AssetCheck LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_library(assetcheck_registro STATIC src/registro.cpp)
target_include_directories(assetcheck_registro PUBLIC include)
target_compile_features(assetcheck_registro PUBLIC cxx_std_23)

add_library(assetcheck_lote STATIC src/lote.cpp)
target_include_directories(assetcheck_lote PUBLIC include)
target_link_libraries(assetcheck_lote PUBLIC assetcheck_registro)
target_compile_features(assetcheck_lote PUBLIC cxx_std_23)

add_library(assetcheck_manifiesto STATIC src/manifiesto.cpp)
target_include_directories(assetcheck_manifiesto PUBLIC include)
target_link_libraries(assetcheck_manifiesto PUBLIC assetcheck_lote)
target_compile_features(assetcheck_manifiesto PUBLIC cxx_std_23)

add_library(assetcheck_estadisticas STATIC src/estadisticas.cpp)
target_include_directories(assetcheck_estadisticas PUBLIC include)
target_link_libraries(assetcheck_estadisticas PUBLIC assetcheck_lote)
target_compile_features(assetcheck_estadisticas PUBLIC cxx_std_23)

add_executable(assetcheck src/cliente.cpp)
target_link_libraries(assetcheck PRIVATE assetcheck_manifiesto)

add_executable(assetcheck_tipos src/cliente_tipos.cpp)
target_link_libraries(assetcheck_tipos PRIVATE assetcheck_estadisticas assetcheck_manifiesto)

add_executable(comprobar_registro pruebas/casos.cpp)
target_link_libraries(comprobar_registro PRIVATE assetcheck_registro)
add_test(NAME comprobar_registro COMMAND comprobar_registro)

add_executable(comprobar_lote pruebas/lote_casos.cpp)
target_link_libraries(comprobar_lote PRIVATE assetcheck_lote)
add_test(NAME comprobar_lote COMMAND comprobar_lote)

add_executable(comprobar_manifiesto pruebas/manifiesto_casos.cpp)
target_link_libraries(comprobar_manifiesto PRIVATE assetcheck_manifiesto)
add_test(NAME comprobar_manifiesto COMMAND comprobar_manifiesto)

add_executable(comprobar_estadisticas pruebas/estadisticas_casos.cpp)
target_link_libraries(comprobar_estadisticas PRIVATE assetcheck_estadisticas)
add_test(NAME comprobar_estadisticas COMMAND comprobar_estadisticas)

add_test(NAME comprobar_tipos_cli
    COMMAND "${CMAKE_COMMAND}"
        "-DCLIENTE=$<TARGET_FILE:assetcheck_tipos>"
        "-DDIR=${CMAKE_CURRENT_BINARY_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/pruebas/comprobar_tipos_cli.cmake")

add_test(NAME comprobar_cliente
    COMMAND "${CMAKE_COMMAND}"
        "-DCLIENTE=$<TARGET_FILE:assetcheck>"
        "-DRAIZ_PRUEBAS=${CMAKE_CURRENT_BINARY_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/pruebas/cliente_casos.cmake")
