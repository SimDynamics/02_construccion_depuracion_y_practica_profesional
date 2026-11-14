# Construcción, depuración y práctica profesional

Referencia didáctica parcial del autor para Programación 2. Se desarrolla junto con la enseñanza del curso; la variante y el aprendizaje del estudiante requieren sus propias prácticas y comprobaciones.

Esta entrega conserva la prueba mínima del entorno en `01_prueba_cmake` y las dos primeras etapas de AssetCheck: validar una declaración y procesar un lote en memoria. Incluye una tercera biblioteca para revisar un manifiesto desde un flujo o un archivo binario y preparar las cadenas de su informe, y un cliente de consola que presenta el resultado de una revisión completa.

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
- `AssetCheck/src/cliente.cpp`: cliente de consola `assetcheck`.
- `AssetCheck/muestras/lote.txt`: muestra curricular de ocho líneas, con sus bytes conservados.
- `AssetCheck/CMakeLists.txt`: tres bibliotecas y tres programas: el cliente y los dos comprobadores. CTest registra sólo los dos comprobadores.
- `.gitattributes`: regla que conserva los finales de línea de la muestra al usar Git.

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

Las bibliotecas de registro y lote declaran `cxx_std_20` como mínimo de modo de lenguaje; la nueva biblioteca del manifiesto declara `cxx_std_23`. Comunican esos requisitos a sus consumidores. Un mínimo permite un modo posterior. En la orden siguiente solicitamos C++23 mediante `CMAKE_CXX_STANDARD=23`; `CMAKE_CXX_STANDARD_REQUIRED=ON` evita que se sustituya esa petición por un estándar anterior. La nueva biblioteca y el cliente fijan también `CXX_STANDARD_REQUIRED ON`. Declarar un mínimo no garantiza por sí solo que el compilador implemente cada característica concreta del estándar.

Las fuentes fijan `CXX_EXTENSIONS OFF` para las tres bibliotecas y los tres programas. Esto solicita el modo sin extensiones propias del compilador cuando este distingue esos modos.

Ejecuta cada orden por separado y consulta inmediatamente su código. Las líneas `echo` muestran el código de la orden anterior: cero indica éxito. Si configuración o construcción devuelve otro valor, corrige el primer error antes de continuar; ejecutar un binario anterior no acredita la fuente actual. Si un comprobador o CTest devuelve un valor distinto de cero, revisa sus diagnósticos y vuelve a comprobar tras corregir la causa.

```cmd
cmake -S AssetCheck -B AssetCheck/build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=23 -DCMAKE_CXX_STANDARD_REQUIRED=ON
echo CodigoConfiguracion=%ERRORLEVEL%
cmake --build AssetCheck/build --target assetcheck comprobar_registro comprobar_lote
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

La segunda orden construye los tres programas. `--build AssetCheck/build` selecciona el árbol generado; `--target assetcheck comprobar_registro comprobar_lote` indica el cliente y los dos comprobadores. CMake construye también sus dependencias: el cliente necesita la biblioteca del manifiesto, que enlaza con lote y registro. Las tres bibliotecas son estáticas y no se ejecutan por su cuenta. Después, las dos rutas terminadas en `.exe` ejecutan los comprobadores recién construidos.

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

La orden de CTest usa `--test-dir AssetCheck/build` para localizar el árbol configurado. CTest ejecuta los dos comprobadores registrados; `--output-on-failure` muestra la salida de una prueba si falla. Debe terminar con código cero y mostrar dos pruebas superadas. Las veintiocho y treinta y una comprobaciones internas no se registran individualmente en CTest.

Volver a ejecutar los dos comprobadores después de un cambio permite detectar si se pierde un comportamiento ya ensayado; esa comprobación de regresión queda limitada a los datos y condiciones de las baterías de registro y lote. Esta entrega no incorpora un tercer comprobador registrado para la biblioteca del manifiesto: las pruebas focales de esa biblioteca se realizan por separado y no forman parte de CTest en este repositorio. Los resultados 28/31 y las dos pruebas de CTest no acreditan por sí solos la lectura física ni las salidas del cliente. La ejecución de la muestra siguiente comprueba ese recorrido concreto; la variante propia del estudiante requiere sus propias decisiones y pruebas.

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

Los árboles `build` contienen archivos generados y permanecen fuera de la publicación. Las fuentes de la prueba mínima se conservan como preparación anterior del entorno.
