# Construcción, depuración y práctica profesional

Referencia didáctica parcial del autor para Programación 2. Se desarrolla junto con la enseñanza del curso; la variante y el aprendizaje del estudiante requieren sus propias prácticas y comprobaciones.

Esta entrega conserva la prueba mínima del entorno en `01_prueba_cmake` y las etapas de AssetCheck: validar declaraciones, procesar lotes, revisar un manifiesto físico y presentar informes. Incluye cuatro bibliotecas, el cliente original `assetcheck`, un segundo cliente `assetcheck_tipos` que cuenta únicamente admisiones por tipo y cuatro comprobadores independientes. La referencia sigue siendo parcial: completar la variante propia del estudiante y la reproducción en su equipo requiere trabajo y evidencia adicionales.

El lector de un registro comprueba formato, identificador, tipo y tamaño, en ese orden. Al rechazar una línea conserva todos los campos del registro anterior. La biblioteca del lote utiliza ese lector y, sólo si la línea es válida, busca su ID entre las admisiones anteriores. La comparación distingue mayúsculas y minúsculas. Un duplicado no reemplaza el primer registro admitido ni su línea de origen; una línea rechazada localmente no reserva su ID.

El lote guarda admisiones y rechazos por separado, en su orden de llegada. La biblioteca del manifiesto obtiene las líneas físicas y proporciona su texto y número a `procesar_linea`. Controla los terminadores LF/CRLF, el contenido de hasta ochenta bytes por línea y el máximo de cien líneas físicas. Una línea inválida produce un rechazo local; no poder abrir el archivo, un fallo de lectura o una línea 101 impiden completar la revisión. AssetCheck valida declaraciones; no abre ni inspecciona las texturas, mallas o sonidos declarados. El cliente recibe una ruta de manifiesto y decide qué informe puede publicar.

## Archivos de esta etapa

- `01_prueba_cmake/CMakeLists.txt` y `01_prueba_cmake/main.cpp`: fuentes conservadas de la preparación del entorno.
- `AssetCheck/include/assetcheck/registro.hpp`: interfaz del lector y vocabulario de resultados, ampliado con `duplicado` para el lote.
- `AssetCheck/src/registro.cpp`: lectura y validación local de una declaración.
- `AssetCheck/pruebas/casos.cpp`: comprobador conservado de la primera etapa.
- `AssetCheck/include/assetcheck/lote.hpp`: admisiones, rechazos, lote e interfaz de `procesar_linea`.
- `AssetCheck/src/lote.cpp`: admisión y búsqueda de duplicados sin copiar el lector.
- `AssetCheck/pruebas/lote_casos.cpp`: comprobador del estado completo del lote.
- `AssetCheck/include/assetcheck/manifiesto.hpp`: interfaz pública de la revisión completa y de la preparación del informe.
- `AssetCheck/src/manifiesto.cpp`: lectura del flujo o archivo y construcción de las cadenas de salida.
- `AssetCheck/include/assetcheck/estadisticas.hpp` y `AssetCheck/src/estadisticas.cpp`: consulta de los tipos admitidos y construcción de TIPOS/RESUMEN sin escribir en los canales.
- `AssetCheck/src/cliente.cpp`: cliente original de consola `assetcheck`, sin cambios por la ampliación.
- `AssetCheck/src/cliente_tipos.cpp`: segundo cliente `assetcheck_tipos`, reutiliza el lector del manifiesto y publica la estadística sólo tras una revisión completa.
- `AssetCheck/pruebas/estadisticas_casos.cpp`: 40 comparaciones del cálculo, la preparación de admisiones y rechazos, los lotes independientes y los informes esperados.
- `AssetCheck/pruebas/comprobar_tipos_cli.cmake`: nueve ejecuciones del segundo cliente con oráculos independientes para salida, error y código.
- `AssetCheck/muestras/lote.txt`: muestra curricular de ocho líneas, con sus bytes conservados.
- `AssetCheck/muestras/variacion_lector.txt`: muestra de seis líneas para el ensayo parcial del lector, con 94 bytes conservados.
- `AssetCheck/muestras/variante.txt`: tercera muestra, con ocho líneas y expectativas para el contrato planificado de tipos.
- `AssetCheck/docs/variacion_lector.md`: expectativas, resultado observado y receta de captura del ensayo parcial del autor.
- [AssetCheck/docs/contrato_tipos.md](AssetCheck/docs/contrato_tipos.md): contrato, expectativas y alcance verificado del informe por tipo.
- `AssetCheck/pruebas/manifiesto_casos.cpp`: comprobador del manifiesto, con archivos de prueba propios.
- `AssetCheck/pruebas/cliente_casos.cmake`: guion que ejecuta el cliente y compara su código y sus dos canales.
- `AssetCheck/CMakeLists.txt`: cuatro bibliotecas y seis programas: dos clientes y cuatro comprobadores. CTest registra seis pruebas, incluidas dos nuevas para los tipos.
- `.gitattributes`: reglas que conservan los finales de línea de las tres muestras al usar Git.
- `.github/workflows/assetcheck-windows.yml`: compila con MSVC y ejecuta las seis suites en Windows Server 2022, sin reemplazar una prueba manual del estudiante.

La ampliación por tipos está integrada en CMake y dispone de su cliente y comprobaciones. La revisión `822e84d7` superó [las seis pruebas CTest en GitHub Actions](https://github.com/SimDynamics/02_construccion_depuracion_y_practica_profesional/actions/runs/43668838549) con MSVC 19.44.35230.0 en Windows Server 2022, incluidos el comprobador de 40 comparaciones y los guiones de 22 y nueve procesos. Esta ejecución automatizada corresponde a esa revisión de la referencia; no acredita capturas, preparación desde un Windows vacío ni reproducción o dominio del estudiante.

## Interfaz de la biblioteca del manifiesto

`revisar_flujo(std::istream& origen, Lote& destino, ErrorGlobal& error)` consume el flujo recibido. Puede trabajar con un flujo en memoria para las pruebas. `revisar_archivo(const std::filesystem::path& ruta, Lote& destino, ErrorGlobal& error)` intenta abrir la ruta en modo binario y utiliza el mismo revisor del flujo; así evita traducciones de los bytes de los terminadores en Windows.

Al retornar `true`, la revisión está completa, `error` vale `ErrorGlobal::ninguno` y `destino` recibe el lote nuevo, aunque contenga rechazos locales. Un archivo vacío revisado correctamente sustituye el destino por un lote vacío. Al retornar `false`, `error` identifica `apertura`, `lectura` o `limite_lineas` y el destino anterior permanece completo. Conservarlo no permite presentarlo como resultado del intento fallido. La biblioteca no imprime durante la lectura.

El bloque `try` sólo envuelve `origen.get()`. Una `std::bad_alloc` que escape de esa extracción se propaga. Para las demás excepciones, `bad()` activo produce `LECTURA` y retorno `false`; `eof()` verdadero sin `bad()` permite terminar normalmente; sin ninguna de esas dos marcas, se vuelve a lanzar la excepción. Al propagarse no hay retorno booleano y `destino` sigue intacto. Las operaciones sobre el lote candidato quedan fuera de esa captura. El propio `get()` puede convertir una excepción interna en `badbit` según la máscara del flujo: no se promete que toda falta de memoria del origen escape.

Cuatro funciones auxiliares devuelven datos para que el cliente decida cuándo y dónde presentarlos:

- `salida_admisiones(lote)` devuelve las filas de admisión y el resumen final, cada una con salto de línea.
- `salida_rechazos(lote)` devuelve las filas de rechazo, con el primer origen cuando la causa es duplicado; devuelve una cadena vacía si no hay rechazos.
- `diagnostico_global(error)` devuelve la línea `ERROR causa=...` correspondiente, con salto final, o una cadena vacía para `ninguno`.
- `codigo_terminacion(error, lote)` devuelve el entero 2 ante un error global; sin error global, devuelve 0 si no hay rechazos o 1 si los hay. No termina el proceso por su cuenta.

Las funciones de salida preparan cadenas con formato decimal y configuración regional clásica, sin escribir en `std::cout` ni `std::cerr`. El cliente llama a `revisar_archivo` antes de presentar admisiones, rechazos o resumen. Si la revisión devuelve `false`, presenta únicamente el diagnóstico global y termina con código 2.

## Construcción y comprobación en Windows

Usa una consola de desarrollo x64 de Visual Studio basada en CMD, con MSVC, Windows SDK, CMake y Ninja disponibles en el mismo entorno. Ejecuta las órdenes desde la carpeta que contiene este README, donde están `01_prueba_cmake` y `AssetCheck`. Conviene una ruta corta para evitar que los nombres de los archivos generados superen los límites de las herramientas de Windows.

El guion del cliente necesita además Windows PowerShell 5.1. Utiliza el ejecutable del sistema situado en `%WINDIR%\System32\WindowsPowerShell\v1.0\powershell.exe`, donde `%WINDIR%` es la carpeta de Windows del equipo; el propio guion comprueba que exista. Se usa para escribir todos los archivos de entrada mediante `WriteAllBytes` y conservar sus bytes exactos, incluidos LF, CRLF y NUL. No depende de herramientas guardadas en datos privados de QA ni de otro runtime. La llamada usa `-NoProfile -NonInteractive -Command`: omite los perfiles, no solicita interacción y ejecuta las instrucciones indicadas sin crear un `.ps1` ni cambiar la política de ejecución.

`AssetCheck/CMakeLists.txt` fija estos valores comunes antes de crear las bibliotecas y los programas, para que cada objetivo reciba esa configuración al crearse:

- `CMAKE_CXX_STANDARD 23` solicita el modo de lenguaje C++23.
- `CMAKE_CXX_STANDARD_REQUIRED ON` evita que se sustituya esa petición por un estándar anterior.
- `CMAKE_CXX_EXTENSIONS OFF` solicita el modo sin extensiones propias del compilador cuando este distingue esos modos.

Las cuatro bibliotecas declaran además `cxx_std_23` con alcance `PUBLIC`: comunican ese mínimo de modo de lenguaje a sus consumidores. Un mínimo permite un modo posterior y no garantiza por sí solo que el compilador implemente cada característica concreta del estándar. La orden de configuración siguiente utiliza los valores ya definidos en el proyecto.

Ejecuta cada orden por separado y consulta inmediatamente su código. Las líneas `echo` muestran el código de la orden anterior: cero indica éxito. Si configuración o construcción devuelve otro valor, corrige el primer error antes de continuar; ejecutar un binario anterior no acredita la fuente actual. Si un comprobador o CTest devuelve un valor distinto de cero, revisa sus diagnósticos y vuelve a comprobar tras corregir la causa.

```cmd
cmake -S AssetCheck -B AssetCheck/build -G Ninja -DCMAKE_BUILD_TYPE=Debug
echo CodigoConfiguracion=%ERRORLEVEL%
cmake --build AssetCheck/build --target assetcheck assetcheck_tipos comprobar_registro comprobar_lote comprobar_manifiesto comprobar_estadisticas
echo CodigoConstruccion=%ERRORLEVEL%
AssetCheck\build\comprobar_registro.exe
echo CodigoRegistro=%ERRORLEVEL%
AssetCheck\build\comprobar_lote.exe
echo CodigoLote=%ERRORLEVEL%
AssetCheck\build\comprobar_manifiesto.exe "AssetCheck\build"
echo CodigoManifiesto=%ERRORLEVEL%
AssetCheck\build\comprobar_estadisticas.exe
echo CodigoEstadisticas=%ERRORLEVEL%
cmake "-DCLIENTE=%CD%\AssetCheck\build\assetcheck.exe" "-DRAIZ_PRUEBAS=%CD%\AssetCheck\build" -P AssetCheck/pruebas/cliente_casos.cmake
echo CodigoPruebasCliente=%ERRORLEVEL%
cmake "-DCLIENTE=%CD%\AssetCheck\build\assetcheck_tipos.exe" "-DDIR=%CD%\AssetCheck\build" -P AssetCheck/pruebas/comprobar_tipos_cli.cmake
echo CodigoPruebasTipos=%ERRORLEVEL%
ctest --test-dir AssetCheck/build --output-on-failure
echo CodigoCTest=%ERRORLEVEL%
```

La primera orden configura el proyecto y genera los archivos de construcción:

- `-S AssetCheck` selecciona la carpeta de fuentes que contiene `CMakeLists.txt`.
- `-B AssetCheck/build` separa la caché y los archivos generados de las fuentes. Esa carpeta debe pertenecer a este proyecto; no reutilices una caché de otro árbol.
- `-G Ninja` selecciona el generador Ninja.
- `-DCMAKE_BUILD_TYPE=Debug` selecciona la configuración de depuración de este generador.

La segunda orden construye los seis programas. `--build AssetCheck/build` selecciona el árbol generado y `--target` enumera ambos clientes y los cuatro comprobadores. CMake construye también las cuatro bibliotecas de las que dependen. El comprobador del manifiesto recibe el árbol de construcción como directorio padre; los demás no requieren argumento. Los dos guiones de CMake comprueban cada cliente como proceso independiente, con stdout, stderr y código por separado. Por último, CTest ejecuta los cuatro comprobadores y los dos guiones: son seis pruebas registradas, no una prueba por cada comparación interna.

## Ampliación del informe por tipos

El segundo cliente comparte las bibliotecas de registro, lote y manifiesto con el original, pero calcula un resultado distinto a partir de las admisiones de una revisión completa. La función `contar_tipos` devuelve contadores de textura, malla y sonido y la suma de `tamano_bytes` declarados. `salida_tipos` forma únicamente dos filas en stdout: TIPOS y RESUMEN. Los rechazos locales se muestran en stderr y mantienen el código 1; los errores globales conservan ambos canales según el contrato y retornan 2 sin publicar un informe parcial.

`comprobar_estadisticas.exe` debe finalizar con `Comparaciones=40 Fallos=0` y código cero.

La batería actual consulta ocho estados y contrasta sus cuatro campos por separado (32 comparaciones). También comprueba cinco preparaciones completas antes de consultar sus lotes y tres cadenas de informe fijadas a partir del contrato. La mezcla de una textura, dos mallas y tres sonidos distingue categorías intercambiadas. Los rechazos anteriores a una admisión, los identificadores `A` y `a`, cien tamaños máximos y las consultas repetidas tienen sus propios valores esperados. Si falla una preparación, el comprobador termina con código 1 antes de consultar ese lote; el resumen puede entonces contener menos de 40 comparaciones. Las tres cadenas se comparan en memoria y no sustituyen las pruebas del proceso cliente. El guion `comprobar_tipos_cli.cmake` ejecuta nueve escenarios: muestra de ocho líneas con CRLF, archivo vacío, última línea sin LF, duplicado, tamaño inválido, línea 101, ausencia o exceso de argumentos y ruta inexistente. Comprueba por separado cada código y los bytes de stdout y stderr. El normalizador recorre pares completos de dígitos hexadecimales y cambia únicamente CRLF a LF; conserva CR sueltos, NUL, espacios y el salto final. Los datos de `muestras/variante.txt` se preservan y su SHA-256 se comprueba antes de ejecutarla.

Durante la primera validación CI en Windows, la batería antigua `cliente_casos.cmake` no pudo crear `nul.txt`: Windows interpreta `NUL` como un nombre reservado de dispositivo, incluso con extensión. Se corrigió solamente el nombre del archivo temporal a `nulo_interno.txt`, sin cambiar el byte NUL contenido en la prueba ni sus expectativas. Tras repetir la validación, las seis suites pasaron. Este fallo fue real en la preparación de pruebas, no un error deliberado del lector ni una modificación del cliente.

## Resultados esperados y regresión

Los tres comprobadores utilizan datos y expectativas declarados en su código, sin leer entrada de consola. El comprobador del manifiesto prepara además sus archivos de prueba dentro del directorio que se explica más abajo.

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

Cada resumen termina con un salto de línea. Los comprobadores de registro y lote deben dejar vacío el canal de error y terminar con código cero. Si el estado obtenido difiere del esperado, el comprobador del lote muestra el ensayo y ambos estados en el canal de error, acumula el fallo y termina con código uno. Se comparan datos conservados, no sólo conteos.

### Comprobador del manifiesto y sus archivos de prueba

En la ruta normal, `comprobar_manifiesto.exe` realiza **56 comparaciones**. Contrasta el retorno, `ErrorGlobal` y el lote completo, incluidas cantidades, orden, campos y líneas de origen. También compara las cadenas enteras de informe, los diagnósticos y los códigos esperados. Sus entradas incluyen los límites de 80/81 bytes y 100/101 líneas, los terminadores, bytes inválidos, EOF con distintas máscaras, un fallo real de lectura tras un prefijo y una configuración regional artificial. Bajo esa configuración, también comprueba que `30000` siga siendo un tamaño válido y que `1_2` sea rechazado como `TAMANO`: el separador regional artificial no forma parte de los dígitos admitidos. Las expectativas se declaran independientemente del lector y de los serializadores.

La ejecución directa de la receta pasa `"AssetCheck\build"` como directorio padre. Debe existir. El comprobador reserva allí un hijo nuevo, con un nombre como `manifiesto_casos_1`. Si ese directorio ya existe, lo conserva y prueba el siguiente número; no borra ni reutiliza los archivos de ejecuciones anteriores. Escribe únicamente sus archivos nuevos dentro de la carpeta reservada y comprueba los resultados de creación, escritura y cierre.

La salida normal contiene dos líneas:

```text
DirectorioPruebas=<ruta del directorio nuevo>
Comprobaciones=56 Fallos=0
```

La primera línea muestra la ruta real elegida: el directorio padre y el número disponible cambian entre equipos o ejecuciones. Los marcadores entre `<` y `>` de este ejemplo no se escriben literalmente. La segunda línea es el resumen esperado cuando pasan las 56 comparaciones. Ambas terminan con salto de línea.

Para este comprobador, el código **0** indica comparaciones correctas y stderr vacío. El código **1** indica al menos una discrepancia, cuyos datos esperados y obtenidos aparecen en stderr. El código **2** indica uso incorrecto o un problema que impide completar la preparación o ejecución del comprobador, por ejemplo no poder escribir o cerrar su archivo. Las carpetas ya creadas se conservan también al fallar.

Sin un argumento de directorio, el comprobador utiliza su carpeta de trabajo actual. CTest lo ejecuta sin argumentos desde el árbol de construcción: por eso la invocación de CTest que sigue a la ejecución directa reserva otra carpeta nueva dentro de `AssetCheck/build`. Repetir esa orden conserva ambas carpetas y reserva una más. Ninguna de estas pruebas depende de archivos privados de QA ni modifica la muestra publicada.

### Guion que comprueba el cliente como proceso

`cliente_casos.cmake` inicia **22 procesos del cliente**. Cada caso fija de forma independiente tres resultados esperados: su código de terminación, todo stdout y todo stderr. Un caso con diferencias en varios de esos resultados cuenta como un solo caso fallido. Las expectativas se escriben a partir del contrato; no se obtienen llamando al lector ni copiando la salida que se acaba de producir.

La orden manual de la receta se ejecuta desde la raíz del repositorio. En CMD, `%CD%` se sustituye por esa carpeta actual:

```cmd
cmake "-DCLIENTE=%CD%\AssetCheck\build\assetcheck.exe" "-DRAIZ_PRUEBAS=%CD%\AssetCheck\build" -P AssetCheck/pruebas/cliente_casos.cmake
echo CodigoPruebasCliente=%ERRORLEVEL%
```

- `-DCLIENTE=...` entrega la ruta del ejecutable ya construido. Las comillas conservan la ruta como un solo argumento si contiene espacios.
- `-DRAIZ_PRUEBAS=...` entrega el directorio padre donde preparar las entradas y guardar las capturas. Debe existir y pertenecer al árbol de construcción de este proyecto.
- Ambos parámetros `-D` aparecen antes de `-P`. `-P AssetCheck/pruebas/cliente_casos.cmake` ejecuta ese archivo como un guion de CMake; no configura el proyecto, genera un árbol ni construye el ejecutable.

El guion conserva un archivo de control llamado `cliente_casos.lock` en el directorio padre. Intenta obtener durante un máximo de quince segundos el bloqueo que coordina la reserva entre ejecuciones del mismo guion. Después busca un hijo libre como `cliente_casos_1`, lo crea y libera el bloqueo. Si el nombre ya existe, lo conserva y prueba el siguiente número. Es un bloqueo consultivo: no impide que otro proceso que ignore ese acuerdo intervenga en la carpeta. No se borra ni reutiliza ninguna ejecución anterior.

Dentro del hijo nuevo prepara sus propios manifiestos con `WriteAllBytes` de PowerShell y verifica sus bytes. El escritor general convierte el texto a hexadecimal y transforma cada pareja en un byte, sin traducir los terminadores. El archivo con NUL conserva su construcción explícita: texto ASCII, un byte cero, un LF y la segunda línea válida. Se comprueban el resultado de esa preparación y el contenido hexadecimal completo antes de iniciar los ensayos. Los casos incluyen:

- Archivo vacío, una línea con LF y última línea sin terminador.
- La muestra de ocho líneas, rechazos locales, conservación del primer origen de un duplicado y TAB permitido entre campos.
- NUL, byte 127 y CTRL-Z en una línea, seguidos de otra válida que debe conservarse. CTRL-Z no debe terminar antes de tiempo la lectura binaria.
- Ochenta bytes con CRLF, los rechazos de CRCRLF, CRXLF y 81 bytes, siempre con una segunda línea válida; terminadores mezclados y una línea vacía intermedia.
- Cien líneas vacías con sus cien rechazos locales; una línea 101 con o sin LF que impide publicar resultados parciales.
- Ausencia o exceso de argumentos, ruta inexistente y una ruta con espacios.

Cada proceso se inicia sin pasar por una consola intermedia. Sus argumentos se entregan por separado. stdout y stderr se guardan en archivos brutos distintos, con nombres como `04_muestra8.stdout.bin` y `04_muestra8.stderr.bin`. Esos archivos quedan conservados dentro de la carpeta de la ejecución para revisar una discrepancia.

Para comparar un canal, el guion lee sus bytes como parejas de dígitos hexadecimales: por ejemplo, `00` representa NUL, `0d` representa CR y `0a` representa LF. Recorre parejas completas y cambia únicamente la pareja consecutiva CR/LF por LF. Conserva los CR sueltos, NUL, espacios y el salto final; no recorta ni elimina otros caracteres. Compara ese resultado con los bytes del texto esperado. Así tolera la diferencia de terminadores CRLF/LF sin ocultar un byte adicional, un canal equivocado o una salida incompleta.

Los mensajes `PREPARACION` identifican las operaciones de reserva y escritura y, si aparece un error, el problema que impide preparar o completar el ensayo. No se contabiliza como caso superado una preparación fallida. Los mensajes `COMPARACION` identifican el caso y la diferencia de código o de canal, con los bytes esperados y obtenidos y la ruta de su captura. Después de recorrer los ensayos, la ruta normal termina con este resumen, además de los mensajes de preparación anteriores:

```text
-- DirectorioPruebas=<ruta del hijo nuevo cliente_casos_N>
-- CasosCLI=22 Fallos=0
```

El **guion** devuelve 0 cuando coinciden los 22 procesos. Un error de preparación, de CMake o una discrepancia hace terminar el guion con un valor distinto de cero; sus errores controlados usan `FATAL_ERROR` y código 1. Los textos `PREPARACION` y `COMPARACION` permiten distinguir sus causas. Son códigos distintos de los del **cliente** observado: 0 para revisión sin rechazos, 1 para revisión completa con rechazos locales y 2 para error global o cantidad incorrecta de argumentos. Obtener el 1 o el 2 esperado del cliente hace pasar ese caso; no significa que haya fallado el guion.

Estos ensayos acreditan `APERTURA` y `LIMITE_LINEAS` del ejecutable, pero **no provocan ni acreditan un fallo real de lectura del archivo en el proceso del cliente**. La biblioteca comprueba `LECTURA` mediante un origen controlado que falla tras un prefijo; esa prueba no se presenta como ejecución de un dispositivo o disco que falle. No se alteran servicios, red, perfiles ni permisos para fingir ese fallo.

### Ejecutar las seis pruebas con CTest

La orden de CTest usa `--test-dir AssetCheck/build` para localizar el árbol configurado. CTest ejecuta cuatro comprobadores y dos guiones de cliente; `--output-on-failure` muestra el detalle de cualquier fallo. Debe terminar con código cero y mostrar seis pruebas superadas. Las 28, 31, 56 y 40 comparaciones internas, los 22 procesos del cliente original y las nueve ejecuciones del cliente de tipos pertenecen a estas seis suites; no son pruebas CTest adicionales.

Al registrar `comprobar_cliente`, CMake resuelve `$<TARGET_FILE:assetcheck>` a la ruta del cliente y la entrega a `-DCLIENTE`; también pasa el directorio binario a `-DRAIZ_PRUEBAS`. Esa expresión se resuelve al generar el proyecto, no dentro del guion ejecutado con `-P`. CTest no construye el programa: la construcción de la receta debe haber terminado correctamente. Como el guion manual ya reservó una carpeta, CTest conserva esa carpeta y reserva otra para sus 22 procesos.

Volver a ejecutar las seis pruebas después de un cambio permite detectar si se pierde un comportamiento ya ensayado. Esa comprobación de regresión queda limitada a los datos y condiciones declarados: registro y lote en memoria, lectura del manifiesto, errores globales, preparación de las cadenas de informe y los 22 escenarios del proceso del cliente. La ejecución de la muestra siguiente permite observar además su recorrido y sus canales directamente; la variante propia del estudiante requiere sus propias decisiones y pruebas.

## Ejecutar el cliente con la muestra

El archivo `AssetCheck/muestras/lote.txt` contiene estos ocho textos, en este orden:

```text
Atlas textura 00012
Pieza malla 30001
Pieza malla 3
Atlas textura 20
atlas sonido 1
_Eco sonido 2
Eco sonido 2 extra
Eco sonido 2
```

El archivo tiene 136 bytes: siete parejas CRLF separan sus líneas y la última termina después del carácter `2`, sin terminador final. No contiene BOM. La regla `AssetCheck/muestras/lote.txt -text` de `.gitattributes` desactiva la normalización de finales de línea de Git para ese archivo. Permite conservar los CRLF de la muestra en la publicación y en los checkouts.

Desde la misma consola CMD y la carpeta que contiene este README, ejecuta ambas órdenes por separado:

```cmd
AssetCheck\build\assetcheck.exe "AssetCheck\muestras\lote.txt"
echo CodigoAssetCheck=%ERRORLEVEL%
```

La primera orden ejecuta el cliente construido; el argumento entre comillas es la ruta del manifiesto. Las comillas mantienen la ruta como un solo argumento también cuando contiene espacios. La segunda orden muestra inmediatamente el código devuelto por ese proceso.

El cliente debe escribir estas cinco líneas en stdout:

```text
ACEPTADO linea=1 id=Atlas tipo=textura bytes=12
ACEPTADO linea=3 id=Pieza tipo=malla bytes=3
ACEPTADO linea=5 id=atlas tipo=sonido bytes=1
ACEPTADO linea=8 id=Eco tipo=sonido bytes=2
RESUMEN lineas=8 aceptados=4 rechazados=4
```

Y estas cuatro líneas en stderr:

```text
RECHAZO linea=2 causa=TAMANO
RECHAZO linea=4 causa=DUPLICADO primera=1
RECHAZO linea=6 causa=ID
RECHAZO linea=7 causa=FORMATO
```

Cada fila de salida termina con salto de línea, incluida la última. Los dos canales conservan su propio orden, pero una consola puede combinarlos visualmente sin mostrar un orden conjunto garantizado. El código es **1**: se revisaron las ocho líneas y cuatro fueron rechazadas. Ese valor no significa que fallara la compilación ni que la lectura quedara incompleta.

Una revisión completa sin rechazos devuelve **0**. Un error global de apertura, lectura o límite de líneas devuelve **2**, deja stdout vacío y presenta una única línea de diagnóstico global en stderr. No publica un informe parcial ni otro informe anterior.

El contrato del cliente exige exactamente una ruta además del nombre del programa: `argc` debe valer 2. Cuando falta esa ruta o se proporcionan argumentos adicionales, este cliente escribe `ERROR causa=APERTURA` y devuelve **2** antes de intentar abrir un archivo. Esa etiqueta para la cantidad incorrecta de argumentos es una decisión de esta interfaz; no demuestra que se haya intentado abrir una ruta.

El [ensayo parcial con seis líneas](AssetCheck/docs/variacion_lector.md) documenta otra entrada y conserva sus expectativas y resultados. Sus órdenes usan Windows PowerShell 5.1 desde **AssetCheck**; las recetas de construcción de este README usan CMD desde la **raíz del repositorio**.

Los árboles `build` contienen archivos generados y permanecen fuera de la publicación. Las fuentes de la prueba mínima se conservan como preparación anterior del entorno.

## Entorno local observado

La comprobación del autor del 14 de noviembre de 2026 utilizó Windows x64, MSVC 19.51.36260.0, CMake 4.3.1-msvc1, Ninja 1.13.2 y Windows PowerShell 5.1.26100.9711 sin perfiles. Con la petición C++23 del proyecto, las órdenes observadas de MSVC usaron `-std:c++latest -MDd`. Ese indicador observado se distingue del mínimo solicitado en CMake.

La configuración y la construcción terminaron con código 0, sin avisos. CTest pasó sus seis pruebas: registro 28, lote 31, manifiesto 56, estadísticas 40 y guiones de 22 y nueve procesos. La invocación manual del segundo guion también contrastó sus nueve procesos, por separado, con los códigos y canales esperados. Estos resultados se obtuvieron con herramientas ya instaladas y no acreditan la preparación de un Windows vacío ni la variante propia del estudiante.
