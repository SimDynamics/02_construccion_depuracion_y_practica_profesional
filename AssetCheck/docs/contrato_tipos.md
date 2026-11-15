# Contrato del resumen por tipo

**Estado: REFERENCIA IMPLEMENTADA Y COMPROBADA EN LOS CASOS DECLARADOS.** La interfaz `include/assetcheck/estadisticas.hpp` y las funciones `contar_tipos` y `salida_tipos` de `src/estadisticas.cpp` forman la biblioteca estática `assetcheck_estadisticas`. El segundo cliente `src/cliente_tipos.cpp` se construye como `assetcheck_tipos` y conserva el cliente original. El comprobador C++ realiza 40 comparaciones y el guion de procesos contrasta nueve invocaciones del segundo cliente. Las expectativas siguientes proceden del contrato; la referencia y las pruebas del autor no acreditan una variante propia ni el dominio del estudiante.

## Proteger el cliente existente

En revisiones completas, `assetcheck` conserva su interfaz publicada: filas `ACEPTADO` y `RESUMEN` en stdout, filas `RECHAZO` en stderr y código 0 sin rechazos o 1 con rechazos. Ante un error global conserva stdout vacío, un diagnóstico en stderr y código 2. Se conservan sus fuentes, bibliotecas, configuración de CMake y pruebas actuales. La nueva presentación corresponde al segundo cliente `assetcheck_tipos`, que reutiliza la misma decisión de lectura y validación.

## Interfaz y precondiciones de las funciones

`contar_tipos(const Lote& lote)` devuelve un objeto `Estadisticas` con `texturas`, `mallas`, `sonidos` y `bytes_total`. Los cuatro campos comienzan en cero en cada llamada. La función recorre sólo `lote.admitidos`: cada admisión aumenta el contador de su tipo exacto y aporta su tamaño una vez. La referencia evita copiar el lote; `const` impide modificarlo a través de ese parámetro.

La precondición es que el lote proceda de las bibliotecas del contrato: hasta 100 líneas físicas y admisiones ya validadas, con tipos permitidos y tamaños entre 1 y 30000. Esta consulta no vuelve a leer ni validar el manifiesto.

`salida_tipos(const Estadisticas& datos, const Lote& lote)` devuelve una cadena con TIPOS y RESUMEN. `datos` debe proceder de `contar_tipos(lote)` y ese mismo lote debe permanecer sin cambios entre ambas llamadas. Así los recuentos y los bytes corresponden a las admisiones que se resumen. La cadena se construye con configuración regional clásica y saltos lógicos finales. La función no imprime, no abre archivos ni decide sobre errores globales; esa decisión corresponde al segundo cliente antes de publicar cualquier informe.

## Qué se cuenta

Se cuentan únicamente las admisiones del lote completo, por tipo exacto `textura`, `malla` o `sonido`. Se suman sus `tamano_bytes` declarados. Un rechazo local, incluido un duplicado, no aumenta el recuento ni la suma. La primera admisión y su origen permanecen sin cambios; los identificadores distinguen mayúsculas y minúsculas.

Los tamaños son declaraciones: este informe no comprueba que existan los recursos ni que sus archivos tengan el tamaño declarado. Se conservan las reglas del manifiesto, sus límites de 80 bytes por línea y 100 líneas y las causas de rechazo existentes. Con 100 admisiones de 30000 bytes, la suma declarada máxima es 3000000.

## Formato y códigos

Tras una revisión completa, el segundo cliente escribe exactamente dos filas en stdout, cada una con un salto de línea lógico al final:

```text
TIPOS textura=T malla=M sonido=S bytes_total=B
RESUMEN lineas=L aceptados=A rechazados=R
```

Las expectativas representan cada salto de línea con LF. Una captura en Windows puede contener CRLF por la biblioteca de ejecución. Para comparar las salidas, convierte únicamente CRLF a LF y conserva los demás bytes, incluidos CR sueltos, NUL, espacios y el salto final.

`TIPOS` y `RESUMEN` son rótulos fijos. T, M y S se sustituyen por los recuentos de admisiones de cada tipo; B es la suma de sus bytes declarados. A es el número de admisiones, R el de rechazos y L incluye todas las líneas físicas revisadas, también las rechazadas. Los rechazos se escriben en stderr con el formato, causa, origen y orden ya publicados. No se promete un orden conjunto entre ambos canales.

Una revisión completa sin rechazos devuelve 0; con rechazos locales, 1. Un error global `APERTURA`, `LECTURA` o `LIMITE_LINEAS` devuelve 2: stdout queda vacío y stderr contiene únicamente `ERROR causa=...`, con la causa correspondiente y un salto de línea lógico al final. No se publican TIPOS, RESUMEN, admisiones ni datos de una revisión parcial o anterior. Si falta la ruta o se entregan argumentos adicionales, el cliente escribe `ERROR causa=APERTURA` y devuelve 2 antes de intentar abrir un archivo: es una decisión de su interfaz.

## Muestra y expectativas independientes

`muestras/variante.txt` contiene estas ocho líneas, sin los números usados en la explicación curricular:

```text
Faro textura 0
Faro textura 00005
Puente malla 240
Faro sonido 7
faro sonido 8
_Eco sonido 2
Eco sonido 2 extra
Eco sonido 2
```

El archivo tiene 131 bytes ASCII, siete parejas CRLF, ningún BOM y ningún terminador tras la última línea. Su SHA-256 es `a5f2ce648444ab349717abad2dd829e4c5f3f0afe249e06cf1af2ad8d26efd7c`. La regla `AssetCheck/muestras/variante.txt -text` de `.gitattributes` conserva sus bytes al usar Git.

La línea 1 se rechaza por tamaño cero y no reserva `Faro`. Se admiten `Faro textura 5` en la línea 2, `Puente malla 240` en la 3, `faro sonido 8` en la 5 y `Eco sonido 2` en la 8. La línea 4 es un duplicado cuyo primer origen es 2; las líneas 6 y 7 se rechazan por ID y FORMATO. La línea 8 debe procesarse aunque no termine en LF.

Las cuatro admisiones dan una textura, una malla y dos sonidos. Su suma es 5 + 240 + 8 + 2 = 255; no se añaden los 7 bytes del duplicado. Estos valores se fijan a partir del contrato, independientemente de la implementación.

stdout esperado de `assetcheck_tipos`, con un salto final tras cada fila:

```text
TIPOS textura=1 malla=1 sonido=2 bytes_total=255
RESUMEN lineas=8 aceptados=4 rechazados=4
```

stderr esperado, también con salto final:

```text
RECHAZO linea=1 causa=TAMANO
RECHAZO linea=4 causa=DUPLICADO primera=2
RECHAZO linea=6 causa=ID
RECHAZO linea=7 causa=FORMATO
```

Código esperado: **1**. Son cuatro admisiones y cuatro rechazos en una revisión completa. El cliente actual `assetcheck` debe seguir presentando sus cuatro filas ACEPTADO y el mismo RESUMEN para esta entrada; no se le añadirá la fila TIPOS.

## Componentes y comprobaciones

La configuración de CMake conserva `assetcheck_estadisticas` después de manifiesto y antes de los programas. `assetcheck_tipos` compila `src/cliente_tipos.cpp` y enlaza con `assetcheck_estadisticas` y `assetcheck_manifiesto` mediante PRIVATE. Las cuatro bibliotecas mantienen sus dependencias y los valores comunes de lenguaje. El proyecto tiene cuatro bibliotecas, seis programas y seis pruebas CTest: cuatro comprobadores C++ y dos guiones de procesos. Se conservan el cliente original, las demás fuentes y las tres muestras. El detalle de construcción y las herramientas observadas están en el [README](../../README.md).


## Comprobador de estadísticas

`pruebas/estadisticas_casos.cpp` se construye como `comprobar_estadisticas` y tiene su propia prueba CTest. Contrasta los cuatro campos de estadísticas, la preparación del lote y las cadenas completas; no ejecuta ninguno de los clientes.

Las expectativas independientes para los cuatro campos, en orden texturas/mallas/sonidos/bytes_total, son:

Vacío: 0/0/0/0.
Mezcla con duplicado: 1/2/3/27.
Rechazos anteriores a admisiones: 1/0/1/13.
Identificadores A y a: 1/1/0/8.
Sólo rechazos: 0/0/0/0.
Cien texturas de 30000 bytes: 100/0/0/3000000.
Consulta repetida de la mezcla: 1/2/3/27.
Vacío después de la mezcla: 0/0/0/0.

Cada fila origina cuatro comparaciones. Cinco verificaciones adicionales contrastan el lote preparado completo: tamaños y orden de ambos vectores, líneas, ID, tipo, tamaño, causa y primer origen. Tres verificaciones comparan las cadenas enteras de vacío, mezcla y máximo. El resumen normal es `Comparaciones=40 Fallos=0`, stderr vacío y código 0. Una discrepancia devuelve 1; una preparación incorrecta detiene el resto y se distingue por su nombre en el diagnóstico. No se utilizan resultados de la propia función como esperados.

El comprobador de estadísticas conserva las bibliotecas, ambos clientes, las tres muestras y las pruebas anteriores. Sus comprobaciones en memoria complementan los guiones de procesos; no provocan un fallo real LECTURA del archivo en un cliente. Ese comportamiento conserva su alcance pendiente.


## Comprobación del segundo cliente como proceso

`pruebas/comprobar_tipos_cli.cmake` y su `add_test` forman una de las seis pruebas CTest del proyecto. El guion observa nueve procesos de `assetcheck_tipos`; no es otro cliente ni una biblioteca.

Las filas siguientes resumen los valores del contrato. Las cadenas literales completas, con sus rótulos, espacios y saltos finales, están en el guion.

Muestra publicada de 131 bytes: texturas 1, mallas 1, sonidos 2 y bytes_total 255; resumen de 8 líneas, 4 admisiones y 4 rechazos. stderr conserva los rechazos de las líneas 1, 4, 6 y 7; código 1.
Archivo vacío: los cuatro campos estadísticos y los tres del resumen valen 0; stderr vacío; código 0.
Una línea A malla 2 sin terminador: texturas 0, mallas 1, sonidos 0 y bytes_total 2; resumen 1/1/0; stderr vacío; código 0.
Dos líneas A malla 2 y A sonido 7: estadísticas 0/1/0/2; resumen 2/1/1; rechazo DUPLICADO en la línea 2 con primera igual a 1; código 1.
Una línea Zero textura 0: estadísticas 0/0/0/0; resumen 1/0/1; rechazo TAMANO en la línea 1; código 1.
Ciento una líneas vacías: stdout vacío; ERROR causa=LIMITE_LINEAS en stderr; código 2.
Sin ruta: stdout vacío; ERROR causa=APERTURA en stderr; código 2.
Una ruta y un argumento adicional: stdout vacío; ERROR causa=APERTURA en stderr; código 2.
Ruta inexistente: stdout vacío; ERROR causa=APERTURA en stderr; código 2.

En las estadísticas abreviadas, el orden es texturas/mallas/sonidos/bytes_total; en el resumen es líneas/admitidos/rechazados. El guion compara por separado stdout y stderr y normaliza exclusivamente CRLF a LF, recorriendo parejas de bytes. Conserva NUL, CR sueltos, espacios y salto final. Los códigos esperados del cliente no son los del guion ni los de CTest.

El guion se detiene ante la primera discrepancia. PREPARACION señala que no se pudo preparar o completar el ensayo; no acredita un caso correcto. COMPARACION identifica resultados distintos. Sólo tras los nueve veredictos correctos muestra CasosCLI=9 y devuelve 0. Es una prueba CTest para esos nueve procesos. No acredita una LECTURA real del proceso ni atribuye al estudiante la autoría o el dominio de la referencia.
