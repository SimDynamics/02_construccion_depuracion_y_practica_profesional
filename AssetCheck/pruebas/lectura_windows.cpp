#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>

#include "assetcheck/manifiesto.hpp"

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <new>
#include <share.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace {
namespace fs = std::filesystem;
constexpr std::size_t bytes_prefijo = 4096;
constexpr LONGLONG limite_captura = 4 * 1024 * 1024;
constexpr ULONGLONG espera_cliente_ms = 30000;

void exigir(bool condicion, const std::string& mensaje) {
    if (!condicion) {
        throw std::runtime_error(mensaje);
    }
}

[[noreturn]] void error_windows(const char* operacion,
                               DWORD codigo = GetLastError()) {
    throw std::runtime_error(std::string(operacion) + ": Win32=" +
                             std::to_string(codigo));
}

class Handle {
public:
    explicit Handle(HANDLE valor = INVALID_HANDLE_VALUE) noexcept : valor_(valor) {}
    ~Handle() { cerrar(); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    Handle(Handle&& otro) noexcept
        : valor_(std::exchange(otro.valor_, INVALID_HANDLE_VALUE)) {}

    HANDLE get() const noexcept { return valor_; }
    void cerrar() noexcept {
        if (valor_ != INVALID_HANDLE_VALUE && valor_ != nullptr) {
            CloseHandle(valor_);
            valor_ = INVALID_HANDLE_VALUE;
        }
    }

private:
    HANDLE valor_;
};

Handle abrir_lectura(const fs::path& ruta) {
    // Sin permiso de escritura: tambien permite que el CRT abra para lectura.
    Handle archivo(CreateFileW(ruta.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (archivo.get() == INVALID_HANDLE_VALUE) {
        error_windows("CreateFileW lectura");
    }
    return archivo;
}

void escribir_nuevo(const fs::path& ruta, const std::string& contenido) {
    Handle archivo(CreateFileW(ruta.c_str(), GENERIC_WRITE, 0, nullptr,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (archivo.get() == INVALID_HANDLE_VALUE) {
        error_windows("CreateFileW manifiesto CREATE_NEW");
    }
    std::size_t escritos = 0;
    while (escritos < contenido.size()) {
        DWORD cantidad = 0;
        if (!WriteFile(archivo.get(), contenido.data() + escritos,
                       static_cast<DWORD>(contenido.size() - escritos),
                       &cantidad, nullptr)) {
            error_windows("WriteFile manifiesto");
        }
        exigir(cantidad != 0, "WriteFile no avanzo");
        escritos += cantidad;
    }
    // El handle se cierra antes de abrir el que mantendra el bloqueo.
}

fs::path crear_directorio_propio(const fs::path& padre) {
    exigir(fs::is_directory(padre), "La carpeta padre de QA debe existir");
    const std::wstring nombre = L"assetcheck_lectura_" +
        std::to_wstring(GetCurrentProcessId()) + L"_" +
        std::to_wstring(GetTickCount64()) + L"_";
    for (unsigned intento = 0; intento < 128; ++intento) {
        const fs::path candidato = padre / (nombre + std::to_wstring(intento));
        std::error_code error;
        if (fs::create_directory(candidato, error)) {
            return candidato;
        }
        if (error && error != std::errc::file_exists) {
            throw std::system_error(error, "crear directorio exclusivo de QA");
        }
        // Un nombre existente nunca se reutiliza ni se borra.
    }
    throw std::runtime_error("No se encontro un nombre libre para QA");
}

struct Datos {
    std::string prefijo;
    std::string cola = "Final malla 7\n";
    std::string admisiones;
};

Datos preparar_datos() {
    Datos datos;
    for (unsigned i = 0; i < 64; ++i) {
        const std::string id = "R" + std::string(i < 10 ? "0" : "") +
            std::to_string(i);
        std::string linea = id + " textura 5";
        linea.resize(63, ' ');
        datos.prefijo += linea + '\n';
        // Oraculo independiente: no llama a los generadores de AssetCheck.
        datos.admisiones += "ACEPTADO linea=" + std::to_string(i + 1) +
            " id=" + id + " tipo=textura bytes=5\r\n";
    }
    exigir(datos.prefijo.size() == bytes_prefijo, "Prefijo distinto de 4096 bytes");
    datos.admisiones += "ACEPTADO linea=65 id=Final tipo=malla bytes=7\r\n"
        "RESUMEN lineas=65 aceptados=65 rechazados=0\r\n";
    return datos;
}

class BloqueoCola {
public:
    BloqueoCola(const fs::path& ruta, DWORD longitud)
        : archivo_(abrir_lectura(ruta)), longitud_(longitud) {
        posicion_.Offset = static_cast<DWORD>(bytes_prefijo);
        if (!LockFileEx(archivo_.get(),
                        LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY,
                        0, longitud_, 0, &posicion_)) {
            error_windows("LockFileEx cola");
        }
        activo_ = true;
    }
    ~BloqueoCola() {
        if (activo_) {
            UnlockFileEx(archivo_.get(), 0, longitud_, 0, &posicion_);
        }
    }
    BloqueoCola(const BloqueoCola&) = delete;
    BloqueoCola& operator=(const BloqueoCola&) = delete;

    void desbloquear() {
        if (!UnlockFileEx(archivo_.get(), 0, longitud_, 0, &posicion_)) {
            error_windows("UnlockFileEx cola");
        }
        activo_ = false;
    }

private:
    Handle archivo_;
    DWORD longitud_;
    OVERLAPPED posicion_{};
    bool activo_ = false;
};

void comprobar_lectura_nativa(const fs::path& ruta, const std::string& prefijo) {
    // Debe ser otro handle: el propietario del bloqueo puede leer su rango.
    Handle lector = abrir_lectura(ruta);
    std::array<char, bytes_prefijo> contenido{};
    DWORD leidos = 0;
    if (!ReadFile(lector.get(), contenido.data(),
                  static_cast<DWORD>(contenido.size()), &leidos, nullptr)) {
        error_windows("ReadFile prefijo desbloqueado");
    }
    exigir(leidos == bytes_prefijo &&
            std::string_view(contenido.data(), leidos) == prefijo,
            "ReadFile no entrego exactamente el prefijo valido");
    char siguiente = 0;
    const BOOL resultado = ReadFile(lector.get(), &siguiente, 1, &leidos, nullptr);
    const DWORD error = GetLastError();
    exigir(!resultado && error == ERROR_LOCK_VIOLATION,
            "La cola no produjo ERROR_LOCK_VIOLATION (33): Win32=" +
            std::to_string(error));
    std::cout << "ReadFile: prefijo=4096, cola=ERROR_LOCK_VIOLATION (33)\n";
}

void observar_ifstream(const fs::path& ruta, const Datos& datos) {
    std::ifstream archivo(ruta, std::ios::binary);
    exigir(archivo.is_open(), "El control std::ifstream no pudo abrir el archivo");
    std::string recibido;
    char byte = 0;
    while (archivo.get(byte)) {
        recibido += byte;
        exigir(recibido.size() <= datos.prefijo.size() + datos.cola.size(),
                "std::ifstream devolvio mas bytes que el archivo");
    }
    exigir((datos.prefijo + datos.cola).compare(0, recibido.size(), recibido) == 0,
            "std::ifstream altero los bytes recibidos");
    std::cout << "Control std::ifstream: bytes=" << recibido.size()
              << " eof=" << archivo.eof() << " bad=" << archivo.bad() << '\n';
    if (recibido == datos.prefijo && archivo.eof() && !archivo.bad()) {
        std::cout << "Observado: el error de lectura se presenta como EOF "
                     "despues de 64 registros validos.\n";
    } else {
        // El fallo de la biblioteca antigua es evidencia, no un requisito para
        // versiones futuras de la STL que distingan correctamente el error.
        std::cout << "Esta STL presenta otro estado; la aprobacion depende de "
                     "ReadFile, ferror y el contrato de los clientes reales.\n";
    }
}

void comprobar_error_crt(const fs::path& ruta, const std::string& prefijo) {
    // El buffer vive hasta despues de fclose. La apertura permite compartir.
    std::array<char, bytes_prefijo> buffer{};
    std::unique_ptr<std::FILE, decltype(&std::fclose)> archivo(
        _wfsopen(ruta.c_str(), L"rb", _SH_DENYNO), &std::fclose);
    exigir(archivo != nullptr, "El control FILE no pudo abrir el archivo");
    exigir(std::setvbuf(archivo.get(), buffer.data(), _IOFBF, buffer.size()) == 0,
            "setvbuf fallo en el control FILE");
    std::array<char, bytes_prefijo> recibido{};
    const std::size_t cantidad = std::fread(
        recibido.data(), 1, recibido.size(), archivo.get());
    exigir(cantidad == bytes_prefijo && std::ferror(archivo.get()) == 0 &&
            std::string_view(recibido.data(), cantidad) == prefijo,
            "FILE no entrego el prefijo valido completo");
    char siguiente = 0;
    const std::size_t cola = std::fread(&siguiente, 1, 1, archivo.get());
    exigir(cola == 0 && std::ferror(archivo.get()) != 0 &&
            std::feof(archivo.get()) == 0,
            "FILE no distinguio el fallo de la cola mediante ferror");
    std::cout << "FILE: prefijo=4096, cola=ferror sin feof\n";
}

void comprobar_conservacion(const fs::path& ruta) {
    assetcheck::Lote destino;
    destino.admitidos.push_back({1, {"Testigo", "sonido", 19}});
    destino.rechazados.push_back({2, assetcheck::Resultado::duplicado, 1});
    assetcheck::ErrorGlobal error = assetcheck::ErrorGlobal::ninguno;
    exigir(!assetcheck::revisar_archivo(ruta, destino, error),
            "revisar_archivo acepto un prefijo incompleto");
    exigir(error == assetcheck::ErrorGlobal::lectura,
            "revisar_archivo no informo ErrorGlobal::lectura");
    exigir(destino.admitidos.size() == 1 && destino.rechazados.size() == 1,
            "El error de lectura cambio el tamano del lote anterior");
    const auto& admision = destino.admitidos.front();
    const auto& rechazo = destino.rechazados.front();
    exigir(admision.linea == 1 && admision.registro.id == "Testigo" &&
            admision.registro.tipo == "sonido" && admision.registro.tamano_bytes == 19 &&
            rechazo.linea == 2 && rechazo.causa == assetcheck::Resultado::duplicado &&
            rechazo.primera == 1,
            "El error de lectura modifico un campo del lote anterior");
}

LONGLONG tamano(HANDLE archivo) {
    LARGE_INTEGER bytes{};
    if (!GetFileSizeEx(archivo, &bytes)) {
        error_windows("GetFileSizeEx captura");
    }
    exigir(bytes.QuadPart >= 0 && bytes.QuadPart <= limite_captura,
            "La captura supera el limite de 4 MiB");
    return bytes.QuadPart;
}

std::string leer_captura(const fs::path& ruta) {
    Handle archivo = abrir_lectura(ruta);
    std::string contenido(static_cast<std::size_t>(tamano(archivo.get())), '\0');
    std::size_t recibidos = 0;
    while (recibidos < contenido.size()) {
        DWORD cantidad = 0;
        if (!ReadFile(archivo.get(), contenido.data() + recibidos,
                      static_cast<DWORD>(contenido.size() - recibidos),
                      &cantidad, nullptr)) {
            error_windows("ReadFile captura");
        }
        exigir(cantidad != 0, "La captura termino antes del tamano anunciado");
        recibidos += cantidad;
    }
    return contenido;
}

Handle crear_captura(const fs::path& ruta, SECURITY_ATTRIBUTES& seguridad) {
    // GetFileSizeEx necesita FILE_READ_ATTRIBUTES para vigilar el limite.
    Handle archivo(CreateFileW(ruta.c_str(), GENERIC_WRITE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ,
        &seguridad, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (archivo.get() == INVALID_HANDLE_VALUE) {
        error_windows("CreateFileW captura CREATE_NEW");
    }
    return archivo;
}

class AtributosProceso {
public:
    AtributosProceso() {
        SIZE_T bytes = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &bytes);
        exigir(bytes != 0, "No se obtuvo el tamano de los atributos del proceso");
        lista_ = static_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
            HeapAlloc(GetProcessHeap(), 0, bytes));
        if (!lista_) {
            throw std::bad_alloc();
        }
        if (!InitializeProcThreadAttributeList(lista_, 1, 0, &bytes)) {
            const DWORD error = GetLastError();
            HeapFree(GetProcessHeap(), 0, lista_);
            lista_ = nullptr;
            error_windows("InitializeProcThreadAttributeList", error);
        }
    }
    ~AtributosProceso() {
        DeleteProcThreadAttributeList(lista_);
        HeapFree(GetProcessHeap(), 0, lista_);
    }
    AtributosProceso(const AtributosProceso&) = delete;
    AtributosProceso& operator=(const AtributosProceso&) = delete;
    LPPROC_THREAD_ATTRIBUTE_LIST get() const noexcept { return lista_; }

private:
    LPPROC_THREAD_ATTRIBUTE_LIST lista_ = nullptr;
};

class ProcesoHijo {
public:
    explicit ProcesoHijo(HANDLE proceso) noexcept : proceso_(proceso) {}
    ~ProcesoHijo() {
        if (activo_) {
            // Solo el handle creado por esta prueba: nunca nombres ni PID ajenos.
            TerminateProcess(proceso_.get(), 124);
            WaitForSingleObject(proceso_.get(), 5000);
        }
    }
    ProcesoHijo(const ProcesoHijo&) = delete;
    ProcesoHijo& operator=(const ProcesoHijo&) = delete;

    DWORD esperar(HANDLE salida, HANDLE errores) {
        const ULONGLONG inicio = GetTickCount64();
        for (;;) {
            const DWORD estado = WaitForSingleObject(proceso_.get(), 100);
            if (estado == WAIT_FAILED) {
                error_windows("WaitForSingleObject cliente");
            }
            if (estado == WAIT_OBJECT_0) {
                activo_ = false;
            }
            // Tambien se limita durante la ejecucion, antes de cargar las capturas.
            tamano(salida);
            tamano(errores);
            if (estado == WAIT_OBJECT_0) {
                DWORD codigo = 0;
                if (!GetExitCodeProcess(proceso_.get(), &codigo)) {
                    error_windows("GetExitCodeProcess cliente");
                }
                return codigo;
            }
            exigir(estado == WAIT_TIMEOUT, "Estado de espera inesperado");
            exigir(GetTickCount64() - inicio < espera_cliente_ms,
                    "El cliente supero el limite de 30 segundos");
        }
    }

private:
    Handle proceso_;
    bool activo_ = true;
};

std::wstring citar_argumento(const std::wstring& argumento) {
    std::wstring citado = L"\"";
    std::size_t barras = 0;
    for (const wchar_t c : argumento) {
        if (c == L'\\') {
            ++barras;
        } else {
            citado.append(c == L'"' ? 2 * barras + 1 : barras, L'\\');
            citado += c;
            barras = 0;
        }
    }
    citado.append(2 * barras, L'\\');
    citado += L'"';
    return citado;
}

struct Ejecucion {
    DWORD codigo;
    std::string salida;
    std::string errores;
};

Ejecucion ejecutar(const fs::path& cliente, const fs::path& manifiesto,
                  const fs::path& directorio, const std::wstring& fase) {
    SECURITY_ATTRIBUTES seguridad{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    Handle entrada(CreateFileW(L"NUL", GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, &seguridad, OPEN_EXISTING, 0, nullptr));
    if (entrada.get() == INVALID_HANDLE_VALUE) {
        error_windows("CreateFileW NUL");
    }
    const fs::path ruta_salida = directorio / (fase + L"_stdout.bin");
    const fs::path ruta_errores = directorio / (fase + L"_stderr.bin");
    Handle salida = crear_captura(ruta_salida, seguridad);
    Handle errores = crear_captura(ruta_errores, seguridad);
    std::array<HANDLE, 3> heredados{entrada.get(), salida.get(), errores.get()};
    AtributosProceso atributos;
    if (!UpdateProcThreadAttribute(atributos.get(), 0,
            PROC_THREAD_ATTRIBUTE_HANDLE_LIST, heredados.data(),
            sizeof(heredados), nullptr, nullptr)) {
        error_windows("UpdateProcThreadAttribute HANDLE_LIST");
    }
    STARTUPINFOEXW inicio{};
    inicio.StartupInfo.cb = sizeof(inicio);
    inicio.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    inicio.StartupInfo.hStdInput = entrada.get();
    inicio.StartupInfo.hStdOutput = salida.get();
    inicio.StartupInfo.hStdError = errores.get();
    inicio.lpAttributeList = atributos.get();
    PROCESS_INFORMATION informacion{};
    // No usa shell. lpApplicationName identifica el ejecutable sin ambiguedad;
    // data() proporciona el buffer mutable que requiere CreateProcessW.
    std::wstring comando = citar_argumento(cliente.native()) + L" " +
        citar_argumento(manifiesto.native());
    if (!CreateProcessW(cliente.c_str(), comando.data(), nullptr, nullptr, TRUE,
            EXTENDED_STARTUPINFO_PRESENT | CREATE_NO_WINDOW, nullptr,
            directorio.c_str(), &inicio.StartupInfo, &informacion)) {
        error_windows("CreateProcessW cliente");
    }
    ProcesoHijo proceso(informacion.hProcess);
    Handle hilo(informacion.hThread);
    const DWORD codigo = proceso.esperar(salida.get(), errores.get());
    salida.cerrar();
    errores.cerrar();
    return {codigo, leer_captura(ruta_salida), leer_captura(ruta_errores)};
}

void comparar_bytes(const std::string& obtenido, const std::string& esperado,
                    const std::string& nombre) {
    if (obtenido == esperado) {
        return;
    }
    std::size_t distinto = 0;
    while (distinto < obtenido.size() && distinto < esperado.size() &&
           obtenido[distinto] == esperado[distinto]) {
        ++distinto;
    }
    throw std::runtime_error(nombre + ": diferencia en byte " +
        std::to_string(distinto) + ", recibidos=" + std::to_string(obtenido.size()) +
        ", esperados=" + std::to_string(esperado.size()));
}

void comprobar_ejecucion(const Ejecucion& resultado, DWORD codigo,
                        const std::string& salida, const std::string& errores,
                        const std::string& fase) {
    exigir(resultado.codigo == codigo,
            fase + ": codigo=" + std::to_string(resultado.codigo) +
            ", esperado=" + std::to_string(codigo));
    comparar_bytes(resultado.salida, salida, fase + " stdout");
    comparar_bytes(resultado.errores, errores, fase + " stderr");
}
} // namespace

int wmain(int argc, wchar_t* argv[]) {
    try {
        exigir(argc == 4,
                "Uso: lectura_windows <cliente.exe> <admisiones|tipos> <carpeta QA>");
        const fs::path cliente = fs::absolute(argv[1]);
        const std::wstring modo = argv[2];
        exigir(fs::is_regular_file(cliente), "No existe el ejecutable del cliente");
        exigir(modo == L"admisiones" || modo == L"tipos", "Modo de cliente desconocido");
        const fs::path directorio = crear_directorio_propio(fs::absolute(argv[3]));
        // Conserva exclusivamente este directorio propio para diagnosticar fallos.
        // No borra archivos: todos se crean con CREATE_NEW en el nuevo subarbol.
        std::cout << "Directorio QA: " << directorio.filename().string() << '\n';
        const fs::path manifiesto = directorio / L"manifiesto lectura.txt";
        const Datos datos = preparar_datos();
        escribir_nuevo(manifiesto, datos.prefijo + datos.cola);
        const std::string esperado = modo == L"admisiones" ? datos.admisiones :
            "TIPOS textura=64 malla=1 sonido=0 bytes_total=327\r\n"
            "RESUMEN lineas=65 aceptados=65 rechazados=0\r\n";

        comprobar_ejecucion(ejecutar(cliente, manifiesto, directorio, L"antes"),
                            0, esperado, "", "Antes del bloqueo");
        {
            BloqueoCola bloqueo(manifiesto, static_cast<DWORD>(datos.cola.size()));
            comprobar_lectura_nativa(manifiesto, datos.prefijo);
            observar_ifstream(manifiesto, datos);
            comprobar_error_crt(manifiesto, datos.prefijo);
            comprobar_ejecucion(ejecutar(cliente, manifiesto, directorio, L"bloqueado"),
                                2, "", "ERROR causa=LECTURA\r\n", "Cola bloqueada");
            comprobar_conservacion(manifiesto);
            bloqueo.desbloquear();
        }
        comprobar_ejecucion(ejecutar(cliente, manifiesto, directorio, L"despues"),
                            0, esperado, "", "Despues del desbloqueo");
        std::cout << "OK: LECTURA real, canales exactos y lote anterior conservado\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FALLO lectura Windows: " << error.what() << '\n';
        return 1;
    }
}
