# Contrato del resumen por tipo: integración verificada en CI Windows

**Estado: IMPLEMENTACIÓN INTEGRADA Y VALIDADA EN CI WINDOWS.** La biblioteca de estadísticas, `assetcheck_tipos`, las 22 comparaciones de unidad y los nueve escenarios de consola están publicados. [GitHub Actions comprobó seis suites CTest](https://github.com/SimDynamics/02_construccion_depuracion_y_practica_profesional/actions/runs/43654009027) con MSVC 19.44 en Windows Server 2022. Las salidas previstas se fijaron a partir del contrato antes de ejecutar el segundo cliente. Sigue pendiente la reproducción y captura en el Windows local del estudiante y la autoría de su propia variante.

## Proteger el cliente existente

En revisiones completas, `assetcheck` conserva su interfaz publicada: filas `ACEPTADO` y `RESUMEN` en stdout, filas `RECHAZO` en stderr y código 0 sin rechazos o 1 con rechazos. Ante un error global conserva stdout vacío, un diagnóstico en stderr y código 2. Se conservan sus fuentes, bibliotecas, configuración de CMake y pruebas actuales. La nueva presentación corresponde al segundo cliente `assetcheck_tipos`, que reutiliza la misma decisión de lectura y validación.

## Interfaz y precondiciones de las funciones

`contar_tipos(const Lote& lote)` devuelve un objeto `Estadisticas` con `texturas`, `mallas`, `sonidos` y `bytes_total`. Los cuatro campos comienzan en cero en cada llamada. La función recorre sólo `lote.admitidos`: cada admisión aumenta el contador de su tipo exacto y aporta su tamaño una vez. La referencia evita copiar el lote; `const` impide modificarlo a través de ese parámetro.

La precondición es que el lote proceda de las bibliotecas del contrato: hasta 100 líneas físicas y admisiones ya validadas, con tipos permitidos y tamaños entre 1 y 30000. Esta consulta no vuelve a leer ni validar el manifiesto.

`salida_tipos(const Estadisticas& datos, const Lote& lote)` devuelve una cadena con TIPOS y RESUMEN. `datos` debe proceder de `contar_tipos(lote)` y ese mismo lote debe permanecer sin cambios entre ambas llamadas. Así los recuentos y los bytes corresponden a las admisiones que se resumen. La cadena se construye con configuración regional clásica y saltos lógicos finales. La función no imprime, no abre archivos ni decide sobre errores globales; esa decisión corresponde al segundo cliente, sin alterar las responsabilidades de la biblioteca.

## Qué se contará

Se contarán únicamente las admisiones del lote completo, por tipo exacto `textura`, `malla` o `sonido`. Se sumarán sus `tamano_bytes` declarados. Un rechazo local, incluido un duplicado, no aumenta el recuento ni la suma. La primera admisión y su origen permanecen sin cambios; los identificadores distinguen mayúsculas y minúsculas.

Los tamaños son declaraciones: este informe no comprueba que existan los recursos ni que sus archivos tengan el tamaño declarado. Se conservan las reglas del manifiesto, sus límites de 80 bytes por línea y 100 líneas y las causas de rechazo existentes. Con 100 admisiones de 30000 bytes, la suma declarada máxima es 3000000.

## Formato y códigos previstos

Tras una revisión completa, el segundo cliente escribe exactamente dos filas en stdout, cada una con un salto de línea lógico al final:

```text
TIPOS textura=T malla=M sonido=S bytes_total=B
RESUMEN lineas=L aceptados=A rechazados=R
```

Las expectativas representan cada salto de línea con LF. Una captura en Windows puede contener CRLF por la biblioteca de ejecución. Para comparar las salidas, convierte únicamente CRLF a LF y conserva los demás bytes, incluidos CR sueltos, NUL, espacios y el salto final.

`TIPOS` y `RESUMEN` son rótulos fijos. T, M y S se sustituyen por los recuentos de admisiones de cada tipo; B es la suma de sus bytes declarados. A es el número de admisiones, R el de rechazos y L incluye todas las líneas físicas revisadas, también las rechazadas. Los rechazos se escribirán en stderr con el formato, causa, origen y orden ya publicados. No se promete un orden conjunto entre ambos canales.

Una revisión completa sin rechazos devolverá 0; con rechazos locales, 1. Un error global `APERTURA`, `LECTURA` o `LIMITE_LINEAS` devolverá 2: stdout quedará vacío y stderr contendrá únicamente `ERROR causa=...`, con la causa correspondiente y un salto de línea lógico al final. No se publicarán TIPOS, RESUMEN, admisiones ni datos de una revisión parcial o anterior.

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

Las cuatro admisiones dan una textura, una malla y dos sonidos. Su suma es 5 + 240 + 8 + 2 = 255; no se añaden los 7 bytes del duplicado. Estos valores se fijan a partir del contrato y no mediante una llamada a la futura implementación.

stdout del caso comprobado de `assetcheck_tipos`, con un salto final tras cada fila:

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

## Alcance de esta etapa

Las dos funciones de estadísticas y el segundo cliente están integrados en CMake junto a las bibliotecas anteriores. Se construyen cuatro bibliotecas y seis programas, y CTest registra seis suites. Se conservan el cliente original y sus 22 casos, las tres muestras con sus bytes exactos y las pruebas anteriores. La verificación automatizada de Windows Server 2022 no constituye una instalación desde cero en el equipo personal del estudiante ni acredita su variante propia. El siguiente trabajo corresponde a las acciones y capturas Windows reales que exige la lección, y a la práctica autónoma del estudiante.
