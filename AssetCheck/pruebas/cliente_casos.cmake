cmake_minimum_required(VERSION 3.20)

# === PREPARACION_Y_DIRECTORIO ===
# Este guion es para Windows. No configura ni construye el cliente.
# Los codigos 0, 1 y 2 esperados en cada caso pertenecen a assetcheck.
# El guion termina con 0 si todo coincide y con 1 ante un error de CMake,
# preparacion o comparacion. El mensaje identifica PREPARACION o COMPARACION.
function(error_preparacion texto)
    message(FATAL_ERROR "PREPARACION: ${texto}")
endfunction()

if(NOT DEFINED CLIENTE OR NOT EXISTS "${CLIENTE}" OR IS_DIRECTORY "${CLIENTE}")
    error_preparacion("CLIENTE debe ser la ruta del ejecutable ya construido.")
endif()
if(NOT DEFINED RAIZ_PRUEBAS OR NOT IS_DIRECTORY "${RAIZ_PRUEBAS}")
    error_preparacion("RAIZ_PRUEBAS debe ser un directorio existente del build.")
endif()
get_filename_component(CLIENTE "${CLIENTE}" ABSOLUTE)
get_filename_component(RAIZ_PRUEBAS "${RAIZ_PRUEBAS}" ABSOLUTE)

# Este bloqueo coordina las instancias de ESTE guion durante la reserva.
# Su archivo de control se conserva. No protege frente a procesos que lo ignoren.
set(archivo_bloqueo "${RAIZ_PRUEBAS}/cliente_casos.lock")
file(LOCK "${archivo_bloqueo}" GUARD PROCESS TIMEOUT 15 RESULT_VARIABLE bloqueo)
if(NOT "${bloqueo}" STREQUAL "0")
    error_preparacion("No se obtuvo el bloqueo de reserva: ${bloqueo}")
endif()
set(numero 1)
while(EXISTS "${RAIZ_PRUEBAS}/cliente_casos_${numero}")
    math(EXPR numero "${numero} + 1")
endwhile()
set(DIRECTORIO "${RAIZ_PRUEBAS}/cliente_casos_${numero}")
message(STATUS "PREPARACION: creando un directorio nuevo: ${DIRECTORIO}")
file(MAKE_DIRECTORY "${DIRECTORIO}")
if(NOT IS_DIRECTORY "${DIRECTORIO}")
    error_preparacion("No se pudo crear el directorio de esta ejecucion.")
endif()
file(LOCK "${archivo_bloqueo}" RELEASE)

# === ARCHIVOS_DE_ENTRADA ===
# Los textos se escriben tal como estan: sin BOM ni cambio de terminadores.
# Se comprueba la escritura releyendo los bytes; nunca se reusa otro directorio.
function(escribir nombre contenido)
    set(ruta "${DIRECTORIO}/${nombre}")
    if(EXISTS "${ruta}")
        error_preparacion("Ya existe el archivo que se iba a crear: ${ruta}")
    endif()
    set(powershell "$ENV{WINDIR}/System32/WindowsPowerShell/v1.0/powershell.exe")
    if(NOT EXISTS "${powershell}" OR IS_DIRECTORY "${powershell}")
        error_preparacion("No se encontro Windows PowerShell en WINDIR.")
    endif()
    # HEX entrega dos digitos por byte, sin introducir saltos de linea.
    # La escritura binaria no pasa por file(WRITE), que tradujo LF a CRLF.
    string(HEX "${contenido}" esperado)
    string(REPLACE "'" "''" ruta_ps "${ruta}")
    set(orden_ps "$ErrorActionPreference = 'Stop'; try { $hex = '${esperado}'; $bytes = [byte[]]::new($hex.Length / 2); for ($i = 0; $i -lt $bytes.Length; $i += 1) { $bytes[$i] = [Convert]::ToByte($hex.Substring(2 * $i, 2), 16) }; [IO.File]::WriteAllBytes('${ruta_ps}', $bytes); exit 0 } catch { [Console]::Error.WriteLine($_.Exception.Message); exit 1 }")
    # El limite de CreateProcess en Windows es 32767 caracteres. Esta cota
    # conservadora incluye programa, orden, ruta escapada y margen de argumentos.
    string(LENGTH "${powershell}" longitud_programa)
    string(LENGTH "${orden_ps}" longitud_orden)
    math(EXPR longitud_total "${longitud_programa} + ${longitud_orden} + 128")
    if(longitud_total GREATER 30000)
        error_preparacion("Orden demasiado larga para preparar ${nombre}.")
    endif()
    message(STATUS "PREPARACION: escribiendo ${nombre}")
    execute_process(
        COMMAND "${powershell}" -NoProfile -NonInteractive -Command "${orden_ps}"
        RESULT_VARIABLE codigo_preparacion
        OUTPUT_VARIABLE salida_preparacion
        ERROR_VARIABLE error_escritura
        TIMEOUT 15)
    if(NOT "${codigo_preparacion}" STREQUAL "0")
        error_preparacion("No se escribio ${nombre}; codigo=${codigo_preparacion}; ${error_escritura}")
    endif()
    if(NOT EXISTS "${ruta}")
        error_preparacion("PowerShell termino sin crear ${nombre}.")
    endif()
    file(READ "${ruta}" obtenido HEX)
    if(NOT "${obtenido}" STREQUAL "${esperado}")
        error_preparacion("Los bytes escritos no coinciden: ${ruta}")
    endif()
endfunction()

escribir("vacio.txt" "")
escribir("una_lf.txt" "A malla 1\n")
escribir("ultima_sin_lf.txt" "A malla 1")
escribir("muestra8.txt" "Atlas textura 00012\r\nPieza malla 30001\r\nPieza malla 3\r\nAtlas textura 20\r\natlas sonido 1\r\n_Eco sonido 2\r\nEco sonido 2 extra\r\nEco sonido 2")
escribir("rechazos_y_duplicado.txt" "A malla 0\nA malla 3\nA sonido 4\n_Eco sonido 2\nT imagen 2\nForm malla 2 extra\n")
escribir("tab.txt" "Tab\tmalla\t00012\n")

# CMake no permite string(ASCII 0). Este caso hace visible el NUL.
# Como las demas entradas, se escribe en binario con PowerShell de Windows 5.1.
set(powershell "$ENV{WINDIR}/System32/WindowsPowerShell/v1.0/powershell.exe")
if(NOT EXISTS "${powershell}")
    error_preparacion("No se encontro Windows PowerShell en WINDIR.")
endif()
set(ruta_nul "${DIRECTORIO}/nul.txt")
string(REPLACE "'" "''" ruta_nul_ps "${ruta_nul}")
# Las comillas simples de la ruta se duplican para el literal de PowerShell.
# GetBytes produce texto ASCII; [byte]0 es NUL y [byte]10 es LF.
set(orden_ps "$ErrorActionPreference = 'Stop'; try { $bytes = [Text.Encoding]::ASCII.GetBytes('Nul malla 1'); $bytes += [byte]0; $bytes += [byte]10; $bytes += [Text.Encoding]::ASCII.GetBytes('Despues sonido 2'); [IO.File]::WriteAllBytes('${ruta_nul_ps}', $bytes); exit 0 } catch { [Console]::Error.WriteLine($_.Exception.Message); exit 1 }")
execute_process(
    COMMAND "${powershell}" -NoProfile -NonInteractive -Command "${orden_ps}"
    RESULT_VARIABLE preparacion_nul
    OUTPUT_VARIABLE salida_preparacion
    ERROR_VARIABLE error_preparacion_nul
    TIMEOUT 15)
if(NOT "${preparacion_nul}" STREQUAL "0")
    error_preparacion("PowerShell no preparo NUL; codigo=${preparacion_nul}; ${error_preparacion_nul}")
endif()
if(NOT EXISTS "${ruta_nul}")
    error_preparacion("PowerShell termino sin crear el fixture NUL.")
endif()
file(READ "${ruta_nul}" nul_hex HEX)
if(NOT "${nul_hex}" STREQUAL "4e756c206d616c6c612031000a4465737075657320736f6e69646f2032")
    error_preparacion("El fixture NUL no contiene los bytes previstos.")
endif()

string(ASCII 127 del)
escribir("del127.txt" "Del malla 1${del}\nDespues sonido 2\n")
string(ASCII 26 ctrl_z)
escribir("ctrl_z.txt" "Corte malla 1${ctrl_z}\nDespues sonido 2\n")

# 14 bytes de texto y 66 espacios: exactamente 80 bytes de contenido.
string(REPEAT " " 66 relleno)
set(b80 "Limite malla 1${relleno}")
string(LENGTH "${b80}" longitud)
if(NOT longitud EQUAL 80)
    error_preparacion("La primera linea de frontera no mide 80 bytes.")
endif()
escribir("b80_crlf.txt" "${b80}\r\nDespues sonido 2\n")
escribir("b80_crcrlf.txt" "${b80}\r\r\nDespues sonido 2\n")
escribir("b80_crxlf.txt" "${b80}\rX\nDespues sonido 2\n")
escribir("b81_lf.txt" "${b80} \nDespues sonido 2\n")
escribir("mezcla.txt" "A malla 1\nB textura 2\r\nC sonido 3")
escribir("vacia_intermedia.txt" "A malla 1\n\nB textura 2\n")
string(REPEAT "\n" 100 cien_lineas)
escribir("cien.txt" "${cien_lineas}")
escribir("cientouna_lf.txt" "${cien_lineas}\n")
escribir("cientouna_sin_lf.txt" "${cien_lineas}A malla 1")
escribir("ruta con espacios.txt" "Espacio sonido 2\n")

# === NORMALIZAR_SOLO_CRLF ===
# Se leen canales en HEX para conservar incluso un NUL que el cliente emita.
# Cada paso consume pares completos de digitos: 0d seguido de 0a -> 0a.
# No se eliminan CR sueltos, espacios, NUL ni el salto final.
function(normalizar_crlf hexadecimal destino)
    string(LENGTH "${hexadecimal}" longitud)
    set(posicion 0)
    set(normalizado "")
    while(posicion LESS longitud)
        string(SUBSTRING "${hexadecimal}" ${posicion} 2 byte_actual)
        math(EXPR siguiente "${posicion} + 2")
        if("${byte_actual}" STREQUAL "0d" AND siguiente LESS longitud)
            string(SUBSTRING "${hexadecimal}" ${siguiente} 2 byte_siguiente)
            if("${byte_siguiente}" STREQUAL "0a")
                string(APPEND normalizado "0a")
                math(EXPR posicion "${posicion} + 4")
                continue()
            endif()
        endif()
        string(APPEND normalizado "${byte_actual}")
        math(EXPR posicion "${posicion} + 2")
    endwhile()
    set(${destino} "${normalizado}" PARENT_SCOPE)
endfunction()

# === EJECUTAR_Y_COMPARAR ===
# Una llamada independiente por proceso. ARGN contiene sus argumentos separados.
# Cada proceso se compara en tres dimensiones; un caso cuenta una sola vez.
set(CASOS 0)
set(FALLOS 0)
function(probar nombre codigo_esperado stdout_esperado stderr_esperado)
    set(captura_out "${DIRECTORIO}/${nombre}.stdout.bin")
    set(captura_err "${DIRECTORIO}/${nombre}.stderr.bin")
    if(EXISTS "${captura_out}" OR EXISTS "${captura_err}")
        error_preparacion("Ya existen archivos de captura para ${nombre}.")
    endif()
    execute_process(
        COMMAND "${CLIENTE}" ${ARGN}
        WORKING_DIRECTORY "${DIRECTORIO}"
        RESULT_VARIABLE codigo_obtenido
        OUTPUT_FILE "${captura_out}"
        ERROR_FILE "${captura_err}"
        TIMEOUT 15)
    if(NOT "${codigo_obtenido}" MATCHES "^-?[0-9]+$")
        error_preparacion("No se completo ${nombre}: ${codigo_obtenido}")
    endif()
    if(NOT EXISTS "${captura_out}" OR NOT EXISTS "${captura_err}")
        error_preparacion("Falta un canal capturado de ${nombre}.")
    endif()
    file(READ "${captura_out}" stdout_hex HEX)
    file(READ "${captura_err}" stderr_hex HEX)
    normalizar_crlf("${stdout_hex}" stdout_normalizado)
    normalizar_crlf("${stderr_hex}" stderr_normalizado)
    string(HEX "${stdout_esperado}" stdout_previsto)
    string(HEX "${stderr_esperado}" stderr_previsto)
    set(coincide TRUE)
    if(NOT "${codigo_obtenido}" STREQUAL "${codigo_esperado}")
        message("COMPARACION ${nombre}: codigo esperado=${codigo_esperado}; obtenido=${codigo_obtenido}")
        set(coincide FALSE)
    endif()
    if(NOT "${stdout_normalizado}" STREQUAL "${stdout_previsto}")
        message("COMPARACION ${nombre}: stdout HEX esperado=${stdout_previsto}; obtenido=${stdout_normalizado}; bruto=${captura_out}")
        set(coincide FALSE)
    endif()
    if(NOT "${stderr_normalizado}" STREQUAL "${stderr_previsto}")
        message("COMPARACION ${nombre}: stderr HEX esperado=${stderr_previsto}; obtenido=${stderr_normalizado}; bruto=${captura_err}")
        set(coincide FALSE)
    endif()
    math(EXPR nuevos_casos "${CASOS} + 1")
    set(CASOS ${nuevos_casos} PARENT_SCOPE)
    if(NOT coincide)
        math(EXPR nuevos_fallos "${FALLOS} + 1")
        set(FALLOS ${nuevos_fallos} PARENT_SCOPE)
    endif()
endfunction()

# === CASOS_Y_EXPECTATIVAS ===
# Los resultados siguientes se fijan a partir del contrato, no de la biblioteca
# ni de una ejecucion previa. Cada salto final esperado forma parte del dato.
probar("01_vacio" 0
    "RESUMEN lineas=0 aceptados=0 rechazados=0\n" ""
    "${DIRECTORIO}/vacio.txt")
probar("02_una_lf" 0
    "ACEPTADO linea=1 id=A tipo=malla bytes=1\nRESUMEN lineas=1 aceptados=1 rechazados=0\n" ""
    "${DIRECTORIO}/una_lf.txt")
probar("03_ultima_sin_lf" 0
    "ACEPTADO linea=1 id=A tipo=malla bytes=1\nRESUMEN lineas=1 aceptados=1 rechazados=0\n" ""
    "${DIRECTORIO}/ultima_sin_lf.txt")
probar("04_muestra8" 1
    "ACEPTADO linea=1 id=Atlas tipo=textura bytes=12\nACEPTADO linea=3 id=Pieza tipo=malla bytes=3\nACEPTADO linea=5 id=atlas tipo=sonido bytes=1\nACEPTADO linea=8 id=Eco tipo=sonido bytes=2\nRESUMEN lineas=8 aceptados=4 rechazados=4\n"
    "RECHAZO linea=2 causa=TAMANO\nRECHAZO linea=4 causa=DUPLICADO primera=1\nRECHAZO linea=6 causa=ID\nRECHAZO linea=7 causa=FORMATO\n"
    "${DIRECTORIO}/muestra8.txt")
probar("05_rechazos_y_duplicado" 1
    "ACEPTADO linea=2 id=A tipo=malla bytes=3\nRESUMEN lineas=6 aceptados=1 rechazados=5\n"
    "RECHAZO linea=1 causa=TAMANO\nRECHAZO linea=3 causa=DUPLICADO primera=2\nRECHAZO linea=4 causa=ID\nRECHAZO linea=5 causa=TIPO\nRECHAZO linea=6 causa=FORMATO\n"
    "${DIRECTORIO}/rechazos_y_duplicado.txt")
probar("06_tab" 0
    "ACEPTADO linea=1 id=Tab tipo=malla bytes=12\nRESUMEN lineas=1 aceptados=1 rechazados=0\n" ""
    "${DIRECTORIO}/tab.txt")
probar("07_nul" 1
    "ACEPTADO linea=2 id=Despues tipo=sonido bytes=2\nRESUMEN lineas=2 aceptados=1 rechazados=1\n"
    "RECHAZO linea=1 causa=FORMATO\n" "${DIRECTORIO}/nul.txt")
probar("08_del127" 1
    "ACEPTADO linea=2 id=Despues tipo=sonido bytes=2\nRESUMEN lineas=2 aceptados=1 rechazados=1\n"
    "RECHAZO linea=1 causa=FORMATO\n" "${DIRECTORIO}/del127.txt")
probar("09_b80_crlf" 0
    "ACEPTADO linea=1 id=Limite tipo=malla bytes=1\nACEPTADO linea=2 id=Despues tipo=sonido bytes=2\nRESUMEN lineas=2 aceptados=2 rechazados=0\n" ""
    "${DIRECTORIO}/b80_crlf.txt")
probar("10_b80_crcrlf" 1
    "ACEPTADO linea=2 id=Despues tipo=sonido bytes=2\nRESUMEN lineas=2 aceptados=1 rechazados=1\n"
    "RECHAZO linea=1 causa=FORMATO\n" "${DIRECTORIO}/b80_crcrlf.txt")
probar("11_b80_crxlf" 1
    "ACEPTADO linea=2 id=Despues tipo=sonido bytes=2\nRESUMEN lineas=2 aceptados=1 rechazados=1\n"
    "RECHAZO linea=1 causa=FORMATO\n" "${DIRECTORIO}/b80_crxlf.txt")
probar("12_b81_lf" 1
    "ACEPTADO linea=2 id=Despues tipo=sonido bytes=2\nRESUMEN lineas=2 aceptados=1 rechazados=1\n"
    "RECHAZO linea=1 causa=FORMATO\n" "${DIRECTORIO}/b81_lf.txt")
probar("13_mezcla" 0
    "ACEPTADO linea=1 id=A tipo=malla bytes=1\nACEPTADO linea=2 id=B tipo=textura bytes=2\nACEPTADO linea=3 id=C tipo=sonido bytes=3\nRESUMEN lineas=3 aceptados=3 rechazados=0\n" ""
    "${DIRECTORIO}/mezcla.txt")
probar("14_vacia_intermedia" 1
    "ACEPTADO linea=1 id=A tipo=malla bytes=1\nACEPTADO linea=3 id=B tipo=textura bytes=2\nRESUMEN lineas=3 aceptados=2 rechazados=1\n"
    "RECHAZO linea=2 causa=FORMATO\n" "${DIRECTORIO}/vacia_intermedia.txt")

# Cien LF son cien lineas vacias: el contrato fija FORMATO para cada una.
# Este bucle expande solamente esas expectativas conocidas de 1 a 100.
set(cien_rechazos "")
foreach(linea RANGE 1 100)
    string(APPEND cien_rechazos "RECHAZO linea=${linea} causa=FORMATO\n")
endforeach()
probar("15_cien" 1 "RESUMEN lineas=100 aceptados=0 rechazados=100\n"
    "${cien_rechazos}" "${DIRECTORIO}/cien.txt")
probar("16_cientouna_lf" 2 "" "ERROR causa=LIMITE_LINEAS\n"
    "${DIRECTORIO}/cientouna_lf.txt")
probar("17_cientouna_sin_lf" 2 "" "ERROR causa=LIMITE_LINEAS\n"
    "${DIRECTORIO}/cientouna_sin_lf.txt")

# Sin argumento y con dos argumentos, APERTURA es la decision del contrato
# de argc: no se afirma que se haya intentado abrir un archivo.
probar("18_sin_argumentos" 2 "" "ERROR causa=APERTURA\n")
probar("19_argumentos_extra" 2 "" "ERROR causa=APERTURA\n"
    "${DIRECTORIO}/una_lf.txt" "extra")
# Este nombre no se crea. Su ausencia se comprueba antes de usarlo.
if(EXISTS "${DIRECTORIO}/no_existe.txt")
    error_preparacion("La ruta prevista como ausente existe.")
endif()
probar("20_ruta_ausente" 2 "" "ERROR causa=APERTURA\n"
    "${DIRECTORIO}/no_existe.txt")
probar("21_ruta_con_espacios" 0
    "ACEPTADO linea=1 id=Espacio tipo=sonido bytes=2\nRESUMEN lineas=1 aceptados=1 rechazados=0\n" ""
    "${DIRECTORIO}/ruta con espacios.txt")
# CTRL-Z dentro de la primera linea no debe impedir leer la segunda en binario.
probar("22_ctrl_z" 1
    "ACEPTADO linea=2 id=Despues tipo=sonido bytes=2\nRESUMEN lineas=2 aceptados=1 rechazados=1\n"
    "RECHAZO linea=1 causa=FORMATO\n" "${DIRECTORIO}/ctrl_z.txt")

# === RESUMEN ===
message(STATUS "DirectorioPruebas=${DIRECTORIO}")
message(STATUS "CasosCLI=${CASOS} Fallos=${FALLOS}")
if(NOT CASOS EQUAL 22)
    error_preparacion("El guion no recorrio los 22 casos previstos.")
endif()
if(FALLOS GREATER 0)
    message(FATAL_ERROR "COMPARACION: ${FALLOS} de ${CASOS} casos no coinciden.")
endif()
# No se provoca LECTURA real del ejecutable: un fallo de disco no es portable.
# La biblioteca ya prueba LECTURA mediante un streambuf que falla tras prefijo.
