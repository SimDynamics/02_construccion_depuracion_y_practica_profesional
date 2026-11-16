cmake_minimum_required(VERSION 3.20)

# Compara por separado codigo, stdout y stderr, con los saltos finales.
if(NOT DEFINED CLIENTE OR NOT EXISTS "${CLIENTE}" OR IS_DIRECTORY "${CLIENTE}")
    message(FATAL_ERROR "PREPARACION: falta el ejecutable assetcheck_tipos.")
endif()
if(NOT DEFINED DIR OR NOT IS_DIRECTORY "${DIR}")
    message(FATAL_ERROR "PREPARACION: falta el directorio de construccion.")
endif()
get_filename_component(CLIENTE "${CLIENTE}" ABSOLUTE)
get_filename_component(DIR "${DIR}" ABSOLUTE)
set(MUESTRA "${CMAKE_CURRENT_LIST_DIR}/../muestras/variante.txt")
if(NOT EXISTS "${MUESTRA}")
    message(FATAL_ERROR "PREPARACION: falta la muestra de ocho lineas.")
endif()
file(SHA256 "${MUESTRA}" suma)
if(NOT suma STREQUAL "a5f2ce648444ab349717abad2dd829e4c5f3f0afe249e06cf1af2ad8d26efd7c")
    message(FATAL_ERROR "PREPARACION: los bytes de la muestra publicada cambiaron.")
endif()

# No reutilizar ni borrar evidencias de ejecuciones anteriores.
set(bloqueo "${DIR}/comprobar_tipos_cli.lock")
file(LOCK "${bloqueo}" GUARD PROCESS TIMEOUT 15 RESULT_VARIABLE resultado_bloqueo)
if(NOT "${resultado_bloqueo}" STREQUAL "0")
    message(FATAL_ERROR "PREPARACION: bloqueo no obtenido.")
endif()
set(numero 1)
while(EXISTS "${DIR}/comprobar_tipos_cli_${numero}")
    math(EXPR numero "${numero} + 1")
endwhile()
set(TRABAJO "${DIR}/comprobar_tipos_cli_${numero}")
file(MAKE_DIRECTORY "${TRABAJO}")
file(LOCK "${bloqueo}" RELEASE)
if(NOT IS_DIRECTORY "${TRABAJO}")
    message(FATAL_ERROR "PREPARACION: no se creo carpeta aislada.")
endif()

file(WRITE "${TRABAJO}/vacio.txt" "")
file(WRITE "${TRABAJO}/uno.txt" "A malla 2")
file(WRITE "${TRABAJO}/duplicado.txt" "A malla 2\nA sonido 7\n")
file(WRITE "${TRABAJO}/tamano.txt" "Zero textura 0\n")
string(REPEAT "\n" 101 lineas)
file(WRITE "${TRABAJO}/ciento_uno.txt" "${lineas}")
if(EXISTS "${TRABAJO}/ausente.txt")
    message(FATAL_ERROR "PREPARACION: la ruta ausente ya existe.")
endif()

# Los dos escenarios adicionales necesitan bytes exactos, incluido CR sin LF.
# Se preparan en binario y se verifican antes de ejecutar el segundo cliente.
function(escribir_borde nombre contenido)
    set(ruta "${TRABAJO}/${nombre}")
    if(EXISTS "${ruta}")
        message(FATAL_ERROR "PREPARACION: Ya existe el archivo que se iba a crear: ${ruta}")
    endif()
    set(powershell "$ENV{WINDIR}/System32/WindowsPowerShell/v1.0/powershell.exe")
    if(NOT EXISTS "${powershell}" OR IS_DIRECTORY "${powershell}")
        message(FATAL_ERROR "PREPARACION: No se encontro Windows PowerShell en WINDIR.")
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
        message(FATAL_ERROR "PREPARACION: Orden demasiado larga para preparar ${nombre}.")
    endif()
    message(STATUS "PREPARACION: escribiendo ${nombre}")
    execute_process(
        COMMAND "${powershell}" -NoProfile -NonInteractive -Command "${orden_ps}"
        RESULT_VARIABLE codigo_preparacion
        OUTPUT_VARIABLE salida_preparacion
        ERROR_VARIABLE error_escritura
        TIMEOUT 15)
    if(NOT "${codigo_preparacion}" STREQUAL "0")
        message(FATAL_ERROR "PREPARACION: No se escribio ${nombre}; codigo=${codigo_preparacion}; ${error_escritura}")
    endif()
    if(NOT EXISTS "${ruta}")
        message(FATAL_ERROR "PREPARACION: PowerShell termino sin crear ${nombre}.")
    endif()
    file(READ "${ruta}" obtenido HEX)
    if(NOT "${obtenido}" STREQUAL "${esperado}")
        message(FATAL_ERROR "PREPARACION: Los bytes escritos no coinciden: ${ruta}")
    endif()
endfunction()
# Fronteras publicadas en U4L6/U4L7: LF entre lineas y CR aislado al EOF.
set(fronteras "Faro textura 5\n_Mal imagen 0 extra\n_Mal imagen 0\nValido imagen 0\nValido malla 0\nEco sonido 2\r")
string(REPEAT "\n" 99 lineas_limite)
set(limite_prefijo "Faro textura 5\nEco sonido 2\n${lineas_limite}")
escribir_borde("fronteras.dat" "${fronteras}")
escribir_borde("limite_prefijo.dat" "${limite_prefijo}")
file(SHA256 "${TRABAJO}/fronteras.dat" huella_fronteras)
file(SHA256 "${TRABAJO}/limite_prefijo.dat" huella_limite)
if(NOT huella_fronteras STREQUAL "3710009967d4da88e53968a83ba2b19ed37cb23107f002ce1b83659185e3bb99"
    OR NOT huella_limite STREQUAL "a23343a769698e610bf50ac77c2ab6d4e6516d5f4c4298af6858f86e18439e9b")
    message(FATAL_ERROR "PREPARACION: las fronteras no conservan los bytes de U4L6/U4L7.")
endif()

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

set(CASOS 0)
function(probar nombre codigo_esperado stdout_esperado stderr_esperado)
    set(archivo_out "${TRABAJO}/${nombre}.stdout.bin")
    set(archivo_err "${TRABAJO}/${nombre}.stderr.bin")
    execute_process(
        COMMAND "${CLIENTE}" ${ARGN}
        WORKING_DIRECTORY "${TRABAJO}"
        RESULT_VARIABLE obtenido
        OUTPUT_FILE "${archivo_out}"
        ERROR_FILE "${archivo_err}"
        TIMEOUT 20)
    if(NOT "${obtenido}" MATCHES "^-?[0-9]+$")
        message(FATAL_ERROR "PREPARACION: ${nombre} no termino: ${obtenido}")
    endif()
    file(READ "${archivo_out}" stdout_hex HEX)
    file(READ "${archivo_err}" stderr_hex HEX)
    # CRLF a LF solamente; conservar CR sueltos, NUL, espacios y salto final.
    normalizar_crlf("${stdout_hex}" out_normalizado)
    normalizar_crlf("${stderr_hex}" err_normalizado)
    string(HEX "${stdout_esperado}" out_esperado_hex)
    string(HEX "${stderr_esperado}" err_esperado_hex)
    if(NOT "${obtenido}" STREQUAL "${codigo_esperado}"
        OR NOT "${out_normalizado}" STREQUAL "${out_esperado_hex}"
        OR NOT "${err_normalizado}" STREQUAL "${err_esperado_hex}")
        message(FATAL_ERROR "COMPARACION ${nombre}: codigo ${obtenido}/${codigo_esperado}; stdout ${out_normalizado}/${out_esperado_hex}; stderr ${err_normalizado}/${err_esperado_hex}")
    endif()
    math(EXPR siguiente "${CASOS} + 1")
    set(CASOS "${siguiente}" PARENT_SCOPE)
endfunction()

probar("muestra" 1
    "TIPOS textura=1 malla=1 sonido=2 bytes_total=255\nRESUMEN lineas=8 aceptados=4 rechazados=4\n"
    "RECHAZO linea=1 causa=TAMANO\nRECHAZO linea=4 causa=DUPLICADO primera=2\nRECHAZO linea=6 causa=ID\nRECHAZO linea=7 causa=FORMATO\n"
    "${MUESTRA}")
probar("vacio" 0
    "TIPOS textura=0 malla=0 sonido=0 bytes_total=0\nRESUMEN lineas=0 aceptados=0 rechazados=0\n" ""
    "${TRABAJO}/vacio.txt")
probar("una_sin_lf" 0
    "TIPOS textura=0 malla=1 sonido=0 bytes_total=2\nRESUMEN lineas=1 aceptados=1 rechazados=0\n" ""
    "${TRABAJO}/uno.txt")
probar("duplicado" 1
    "TIPOS textura=0 malla=1 sonido=0 bytes_total=2\nRESUMEN lineas=2 aceptados=1 rechazados=1\n"
    "RECHAZO linea=2 causa=DUPLICADO primera=1\n"
    "${TRABAJO}/duplicado.txt")
probar("tamano" 1
    "TIPOS textura=0 malla=0 sonido=0 bytes_total=0\nRESUMEN lineas=1 aceptados=0 rechazados=1\n"
    "RECHAZO linea=1 causa=TAMANO\n"
    "${TRABAJO}/tamano.txt")
probar("limite_global" 2 "" "ERROR causa=LIMITE_LINEAS\n"
    "${TRABAJO}/ciento_uno.txt")
probar("sin_argumentos" 2 "" "ERROR causa=APERTURA\n")
probar("argumentos_extra" 2 "" "ERROR causa=APERTURA\n"
    "${TRABAJO}/uno.txt" extra)
probar("ruta_ausente" 2 "" "ERROR causa=APERTURA\n"
    "${TRABAJO}/ausente.txt")

# Se conservan los nueve casos anteriores y los oraculos de ambos canales.
probar("fronteras" 1
    "TIPOS textura=1 malla=0 sonido=0 bytes_total=5\nRESUMEN lineas=6 aceptados=1 rechazados=5\n"
    "RECHAZO linea=2 causa=FORMATO\nRECHAZO linea=3 causa=ID\nRECHAZO linea=4 causa=TIPO\nRECHAZO linea=5 causa=TAMANO\nRECHAZO linea=6 causa=FORMATO\n"
    "${TRABAJO}/fronteras.dat")
probar("limite_prefijo" 2 "" "ERROR causa=LIMITE_LINEAS\n"
    "${TRABAJO}/limite_prefijo.dat")

message(STATUS "CasosCLI=${CASOS}; directorio=${TRABAJO}")
if(NOT CASOS EQUAL 11)
    message(FATAL_ERROR "COMPARACION: se esperaban once ejecuciones.")
endif()
