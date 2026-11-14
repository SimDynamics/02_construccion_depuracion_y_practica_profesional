# Ensayo parcial del autor: otra entrada para el lector

Esta sesión utiliza el cliente de referencia y cambia sus datos de entrada. Documenta una comprobación concreta; no acredita una modificación del programa ni la variante propia o el dominio del estudiante.

## Datos y expectativas previas

`muestras/variacion_lector.txt` contiene estas seis líneas:

```text
Faro sonido 0
Faro sonido 7
Puente malla 24
Faro textura 5
faro textura 3
Brisa sonido 12
```

Son 94 bytes ASCII: cinco parejas CRLF separan las líneas; la última termina en `2`, sin terminador final y sin BOM. Su SHA-256 es `f07edd4fb9b32286778df69abc143d7db3bd06f8a2d98eb357724142626ed222`. La regla `AssetCheck/muestras/variacion_lector.txt -text` de `.gitattributes` conserva esos bytes al utilizar Git.

Antes de ejecutar, se esperaba que la línea 1 fuera rechazada por tamaño cero y que no reservara `Faro`. La primera admisión de ese ID quedaría en la línea 2 con `sonido` y tamaño 7. La línea 4 sería un duplicado de la 2, sin reemplazarla. `faro` es otro ID porque la comparación distingue mayúsculas y minúsculas. La última línea debe procesarse aunque no termine en LF.

stdout esperado, incluido el salto final:

```text
ACEPTADO linea=2 id=Faro tipo=sonido bytes=7
ACEPTADO linea=3 id=Puente tipo=malla bytes=24
ACEPTADO linea=5 id=faro tipo=textura bytes=3
ACEPTADO linea=6 id=Brisa tipo=sonido bytes=12
RESUMEN lineas=6 aceptados=4 rechazados=2
```

stderr esperado, incluido el salto final:

```text
RECHAZO linea=1 causa=TAMANO
RECHAZO linea=4 causa=DUPLICADO primera=2
```

El código esperado es **1**: la revisión termina completa con cuatro admisiones y dos rechazos locales. No es un error global ni un fallo de construcción.

## Reproducir y conservar los dos canales

Usa Windows PowerShell 5.1 con el entorno de desarrollo x64 de Visual Studio. Ejecuta estas órdenes desde la carpeta **AssetCheck**, donde están `muestras` y `build`. El cliente `build/assetcheck.exe` debe haberse construido con la receta del [README de la raíz](../../README.md). Esa receta de construcción usa CMD desde la raíz del repositorio; las órdenes de este documento usan PowerShell desde AssetCheck.

La siguiente secuencia conserva cualquier revisión anterior y reserva un hijo numerado nuevo dentro de `build`. Es una reserva para este ensayo secuencial, no un bloqueo universal frente a otras ejecuciones. CMD recibe una sola orden con redirecciones a dos archivos distintos; `/d` omite sus órdenes AutoRun y `/c` ejecuta esa orden y termina. El valor de `$LASTEXITCODE` se guarda inmediatamente después de CMD y se conserva en `$codigo`:

```powershell
if (-not (Test-Path -LiteralPath '.\build\assetcheck.exe' -PathType Leaf)) {
    throw 'Falta .\build\assetcheck.exe. Completa la construccion desde las fuentes actuales antes de capturar.'
}
if (-not (Test-Path -LiteralPath '.\muestras\variacion_lector.txt' -PathType Leaf)) {
    throw 'Falta .\muestras\variacion_lector.txt. Crea y comprueba primero la muestra.'
}
$numero = 1
while (Test-Path -LiteralPath ".\build\revision_variacion_$numero") {
    $numero += 1
}
$revision = ".\build\revision_variacion_$numero"
New-Item -ItemType Directory -Path $revision -ErrorAction Stop | Out-Null
$orden = '.\build\assetcheck.exe ".\muestras\variacion_lector.txt" 1>"' + $revision + '\stdout.bin" 2>"' + $revision + '\stderr.bin"'
cmd /d /c $orden
$codigo = $LASTEXITCODE
$codigo
```

Consulta los canales por separado en la misma sesión de PowerShell. `$revision` identifica la carpeta recién creada; `$codigo` conserva el resultado del cliente:

```powershell
Get-Content -LiteralPath (Join-Path $revision 'stdout.bin') -Encoding ASCII
Get-Content -LiteralPath (Join-Path $revision 'stderr.bin') -Encoding ASCII
$codigo
```

La primera consulta debe mostrar las cinco líneas de stdout, la segunda las dos de stderr y la última el código 1. Conserva la carpeta de revisión y sus archivos para compararlos; repetir la captura reserva otro número.

## Resultado observado y alcance

El ensayo registrado del autor utilizó el ejecutable reproducido de la revisión `c9c1c216903157cd352d8bd936ec56ef5ed9c637`, con herramientas preinstaladas. Obtuvo exactamente los textos anteriores: 232 bytes en stdout y 73 en stderr, con terminadores CRLF y código 1. El manifiesto conservó sus 94 bytes y su SHA-256. Las expectativas se fijaron antes de esa ejecución; aquí se documentan después sus resultados.

La evidencia corresponde a esta sesión de seis líneas. No demuestra pruebas exhaustivas, una instalación en Windows vacío ni un fallo real de lectura del archivo dentro del proceso del cliente. Las demás comprobaciones y ese límite se describen en el README. AssetCheck revisa las declaraciones del manifiesto; este ensayo no abre los recursos declarados.
