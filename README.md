# Construcción, depuración y práctica profesional

Referencia didáctica parcial del autor para Programación 2. Se desarrolla junto con la enseñanza del curso; la variante y el aprendizaje del estudiante requieren sus propias prácticas y comprobaciones.

Esta entrega conserva la prueba mínima del entorno en `01_prueba_cmake` y las dos primeras etapas de AssetCheck: validar una declaración y procesar un lote en memoria.

El lector de un registro comprueba formato, identificador, tipo y tamaño, en ese orden. Al rechazar una línea conserva todos los campos del registro anterior. La biblioteca del lote utiliza ese lector y, sólo si la línea es válida, busca su ID entre las admisiones anteriores. La comparación distingue mayúsculas y minúsculas. Un duplicado no reemplaza el primer registro admitido ni su línea de origen; una línea rechazada localmente no reserva su ID.

El lote guarda admisiones y rechazos por separado, en su orden de llegada. El código que llama a `procesar_linea` proporciona el texto y su número de línea. Esta etapa no lee un archivo físico ni cuenta sus líneas: la lectura completa, los límites del archivo y el cliente de consola se incorporarán después. AssetCheck valida declaraciones; no abre ni inspecciona los recursos declarados.

## Archivos de esta etapa

- `01_prueba_cmake/CMakeLists.txt` y `01_prueba_cmake/main.cpp`: fuentes conservadas de la preparación del entorno.
- `AssetCheck/include/assetcheck/registro.hpp`: interfaz del lector y vocabulario de resultados, ampliado con `duplicado` para el lote.
- `AssetCheck/src/registro.cpp`: lectura y validación local de una declaración.
- `AssetCheck/pruebas/casos.cpp`: comprobador conservado de la primera etapa.
- `AssetCheck/include/assetcheck/lote.hpp`: admisiones, rechazos, lote e interfaz de `procesar_linea`.
- `AssetCheck/src/lote.cpp`: admisión y búsqueda de duplicados sin copiar el lector.
- `AssetCheck/pruebas/lote_casos.cpp`: comprobador del estado completo del lote.
- `AssetCheck/CMakeLists.txt`: dos bibliotecas, dos comprobadores y sus pruebas de CTest.

## Construcción y comprobación en Windows

Usa una consola de desarrollo x64 de Visual Studio con MSVC, Windows SDK, CMake y Ninja disponibles en el mismo entorno. Ejecuta las órdenes desde la carpeta que contiene este README, donde están `01_prueba_cmake` y `AssetCheck`. Conviene una ruta corta para evitar que los nombres de los archivos generados superen los límites de las herramientas de Windows.

El CMake de las fuentes declara `cxx_std_20` como mínimo de modo de lenguaje para las bibliotecas y lo comunica a sus consumidores mediante sus requisitos de uso. Ese mínimo permite un modo posterior. En la orden siguiente solicitamos C++23 mediante `CMAKE_CXX_STANDARD=23`; `CMAKE_CXX_STANDARD_REQUIRED=ON` evita que se sustituya esa petición por un estándar anterior. La declaración del mínimo no garantiza por sí sola que el compilador implemente cada característica concreta del estándar.

Las fuentes fijan `CXX_EXTENSIONS OFF` para las dos bibliotecas y los dos comprobadores. Esto solicita el modo sin extensiones propias del compilador cuando este distingue esos modos.

Ejecuta cada orden por separado y consulta inmediatamente su código. Las líneas `echo` muestran el código de la orden anterior: cero indica éxito. Si configuración o construcción devuelve otro valor, corrige el primer error antes de continuar; ejecutar un binario anterior no acredita la fuente actual. Si un comprobador o CTest devuelve un valor distinto de cero, revisa sus diagnósticos y vuelve a comprobar tras corregir la causa.

```cmd
cmake -S AssetCheck -B AssetCheck/build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=23 -DCMAKE_CXX_STANDARD_REQUIRED=ON
echo CodigoConfiguracion=%ERRORLEVEL%
cmake --build AssetCheck/build --target comprobar_registro comprobar_lote
echo CodigoConstruccion=%ERRORLEVEL%
AssetCheck\build\comprobar_registro.exe
echo CodigoRegistro=%ERRORLEVEL%
AssetCheck\build\comprobar_lote.exe
echo CodigoLote=%ERRORLEVEL%
ctest --test-dir AssetCheck/build --output-on-failure
echo CodigoCTest=%ERRORLEVEL%
```

La primera orden configura el proyecto y genera los archivos de construcción:

- `-S AssetCheck` selecciona la carpeta de fuentes que contiene `CMakeLists.txt`.
- `-B AssetCheck/build` separa la caché y los archivos generados de las fuentes. Esa carpeta debe pertenecer a este proyecto; no reutilices una caché de otro árbol.
- `-G Ninja` selecciona el generador Ninja.
- `-DCMAKE_BUILD_TYPE=Debug` selecciona la configuración de depuración de este generador.
- `-DCMAKE_CXX_STANDARD=23` y `-DCMAKE_CXX_STANDARD_REQUIRED=ON` solicitan el modo de lenguaje explicado arriba.

La segunda orden construye los dos programas. `--build AssetCheck/build` selecciona el árbol generado y `--target comprobar_registro comprobar_lote` indica los dos objetivos. CMake incluye también las bibliotecas que necesitan. Después, las dos rutas terminadas en `.exe` ejecutan los comprobadores recién construidos.

## Resultados esperados y regresión

Ningún comprobador requiere entrada de consola: ambos utilizan datos y expectativas declarados en su código.

`comprobar_registro.exe` debe escribir:

```text
Casos=28 Fallos=0
```

Esa batería conserva veintisiete casos individuales y una secuencia de tres llamadas sobre el mismo destino, contada como otro caso. Volver a ejecutarla comprueba que la ampliación del proyecto conserva los comportamientos ensayados de la primera etapa.

`comprobar_lote.exe` debe escribir:

```text
Comprobaciones=31 Fallos=0
```

Sus treinta y un ensayos comparan el estado final completo: cantidad y orden de admisiones y rechazos, número de línea, los tres campos de cada registro y la causa y el primer origen de cada rechazo. Cada ensayo empieza con un lote nuevo y conserva ese lote durante todas sus líneas. Se incluyen el lote vacío, IDs únicos, duplicados inmediatos e intermedios, rechazos locales antes de una admisión, diferencias entre mayúsculas y minúsculas y dos lotes nuevos sucesivos. Las expectativas no se calculan llamando al lector que se está comprobando.

Cada resumen termina con un salto de línea. Ambos programas deben dejar vacío el canal de error y terminar con código cero. Si el estado obtenido difiere del esperado, el comprobador del lote muestra el ensayo y ambos estados en el canal de error, acumula el fallo y termina con código uno. Se comparan datos conservados, no sólo conteos.

La última orden usa `--test-dir AssetCheck/build` para localizar el árbol configurado. CTest ejecuta los dos comprobadores registrados; `--output-on-failure` muestra la salida de una prueba si falla. Debe terminar con código cero y mostrar dos pruebas superadas. Las veintiocho y treinta y una comprobaciones internas no se registran individualmente en CTest.

Volver a ejecutar los dos comprobadores después de un cambio permite detectar si se pierde un comportamiento ya ensayado; esa comprobación de regresión queda limitada a los datos y condiciones de estas baterías. Todavía no acredita lectura de archivos, informes del cliente ni una variante propia del estudiante.

Los árboles `build` contienen archivos generados y permanecen fuera de la publicación. Las fuentes de la prueba mínima se conservan como preparación anterior del entorno.
