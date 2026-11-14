# Construcción, depuración y práctica profesional

Referencia didáctica parcial del autor para Programación 2. Se desarrolla junto con la enseñanza del curso; la variante y el aprendizaje del estudiante requieren sus propias prácticas y comprobaciones.

Esta entrega conserva la prueba mínima del entorno en `01_prueba_cmake` e incorpora el primer componente de AssetCheck: lectura y validación de una sola declaración del manifiesto.

El componente comprueba formato, identificador, tipo y tamaño, en ese orden. Al rechazar una línea conserva todos los campos del registro anterior. La lectura de archivos completos, los duplicados y el cliente de consola se incorporarán en etapas posteriores. Esta versión valida declaraciones; no abre ni inspecciona los recursos declarados.

## Archivos de esta etapa

- `01_prueba_cmake/CMakeLists.txt` y `01_prueba_cmake/main.cpp`: fuentes conservadas de la preparación del entorno.
- `AssetCheck/include/assetcheck/registro.hpp`: interfaz pública.
- `AssetCheck/src/registro.cpp`: implementación.
- `AssetCheck/pruebas/casos.cpp`: comprobador independiente.
- `AssetCheck/CMakeLists.txt`: biblioteca, comprobador y registro de la prueba de CTest.

## Construcción y comprobación en Windows

Usa una consola de desarrollo x64 de Visual Studio con MSVC, Windows SDK, CMake y Ninja disponibles en el mismo entorno. Ejecuta las órdenes desde la carpeta que contiene este README.

El CMake de las fuentes declara `cxx_std_20` como mínimo de modo de lenguaje para la biblioteca y lo comunica al comprobador mediante su relación de uso. Ese mínimo permite un modo posterior. En la orden siguiente solicitamos C++23 mediante `CMAKE_CXX_STANDARD=23`; `CMAKE_CXX_STANDARD_REQUIRED=ON` evita que se sustituya esa petición por un estándar anterior. La declaración del mínimo no garantiza por sí sola que el compilador implemente cada característica concreta del estándar.

Las fuentes fijan `CXX_EXTENSIONS OFF` tanto para la biblioteca como para el comprobador. Esto solicita el modo sin extensiones propias del compilador cuando este distingue esos modos.

Ejecuta cada orden por separado y consulta inmediatamente su código. Si configuración o construcción devuelve un código distinto de cero, corrige el primer error antes de continuar; ejecutar un binario anterior no acredita la fuente actual.

```cmd
cmake -S AssetCheck -B AssetCheck/build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=23 -DCMAKE_CXX_STANDARD_REQUIRED=ON
echo CodigoConfiguracion=%ERRORLEVEL%
cmake --build AssetCheck/build --target comprobar_registro
echo CodigoConstruccion=%ERRORLEVEL%
AssetCheck\build\comprobar_registro.exe
echo CodigoComprobador=%ERRORLEVEL%
ctest --test-dir AssetCheck/build --output-on-failure
echo CodigoCTest=%ERRORLEVEL%
```

El comprobador no requiere entrada de consola: utiliza los casos de su código. El resultado esperado es `Casos=28 Fallos=0`, seguido de salto de línea, con retorno cero. La batería reúne veintisiete casos individuales y una secuencia de tres llamadas sobre el mismo destino, contada como otro caso.

CTest registra una prueba que ejecuta ese comprobador. Debe terminar con cero y mostrar una prueba superada; los casos internos del programa no se registran individualmente en CTest.

Los árboles `build` contienen archivos generados y permanecen fuera de la publicación. Las fuentes de la prueba mínima se conservan como preparación anterior del entorno.
