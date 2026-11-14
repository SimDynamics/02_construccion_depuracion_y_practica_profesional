#include "assetcheck/manifiesto.hpp"
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using assetcheck::ErrorGlobal;
using assetcheck::Lote;
using assetcheck::Resultado;
namespace fs = std::filesystem;
int comprobaciones = 0;
int fallos = 0;

// SECCION 1: COMPARACION_COMPLETA
// Ningun esperado se obtiene llamando al lector o a los serializadores.
bool mismo_lote(const Lote& obtenido, const Lote& esperado) {
    if (obtenido.admitidos.size() != esperado.admitidos.size()
        || obtenido.rechazados.size() != esperado.rechazados.size()) {
        return false;
    }
    for (std::size_t i = 0; i < esperado.admitidos.size(); ++i) {
        const auto& a = obtenido.admitidos[i];
        const auto& b = esperado.admitidos[i];
        if (a.linea != b.linea || a.registro.id != b.registro.id
            || a.registro.tipo != b.registro.tipo
            || a.registro.tamano_bytes != b.registro.tamano_bytes) {
            return false;
        }
    }
    for (std::size_t i = 0; i < esperado.rechazados.size(); ++i) {
        const auto& a = obtenido.rechazados[i];
        const auto& b = esperado.rechazados[i];
        if (a.linea != b.linea || a.causa != b.causa || a.primera != b.primera) {
            return false;
        }
    }
    return true;
}

Lote semilla() {
    return Lote{{{1, {"Previo", "malla", 7}}},
                {{2, Resultado::tamano, 0}, {3, Resultado::duplicado, 1}}};
}

const char* nombre_error(ErrorGlobal error) {
    switch (error) {
        case ErrorGlobal::ninguno: return "NINGUNO";
        case ErrorGlobal::apertura: return "APERTURA";
        case ErrorGlobal::lectura: return "LECTURA";
        case ErrorGlobal::limite_lineas: return "LIMITE_LINEAS";
    }
    return "DESCONOCIDO";
}

const char* nombre_causa(Resultado causa) {
    switch (causa) {
        case Resultado::aceptado: return "ACEPTADO";
        case Resultado::formato: return "FORMATO";
        case Resultado::id: return "ID";
        case Resultado::tipo: return "TIPO";
        case Resultado::tamano: return "TAMANO";
        case Resultado::duplicado: return "DUPLICADO";
    }
    return "DESCONOCIDA";
}

void mostrar_lote(const Lote& lote) {
    std::cerr << "admitidos=" << lote.admitidos.size()
              << " rechazados=" << lote.rechazados.size() << '\n';
    for (const auto& a : lote.admitidos) {
        std::cerr << "A " << a.linea << ' ' << a.registro.id << ' '
                  << a.registro.tipo << ' ' << a.registro.tamano_bytes << '\n';
    }
    for (const auto& r : lote.rechazados) {
        std::cerr << "R " << r.linea << ' ' << nombre_causa(r.causa)
                  << ' ' << r.primera << '\n';
    }
}

// SECCION 2: EJECUTAR_CASO
bool comprobar(const std::string& nombre, bool correcto) {
    ++comprobaciones;
    if (!correcto) {
        ++fallos;
        std::cerr << "FALLO " << comprobaciones << ": " << nombre << '\n';
    }
    return correcto;
}

void comprobar_revision(const std::string& nombre,
                        bool retorno, ErrorGlobal error, const Lote& obtenido,
                        bool retorno_esperado, ErrorGlobal error_esperado,
                        const Lote& esperado) {
    const bool correcto = retorno == retorno_esperado && error == error_esperado
        && mismo_lote(obtenido, esperado);
    if (!comprobar(nombre, correcto)) {
        std::cerr << "Esperado: retorno=" << retorno_esperado
                  << " error=" << nombre_error(error_esperado) << '\n';
        mostrar_lote(esperado);
        std::cerr << "Obtenido: retorno=" << retorno
                  << " error=" << nombre_error(error) << '\n';
        mostrar_lote(obtenido);
    }
}

void comprobar_texto(const std::string& nombre, const std::string& obtenido,
                     const std::string& esperado) {
    if (!comprobar(nombre, obtenido == esperado)) {
        std::cerr << "Esperado (" << esperado.size() << " bytes):\n" << esperado
                  << "\nObtenido (" << obtenido.size() << " bytes):\n" << obtenido << '\n';
    }
}

void comprobar_codigo(const std::string& nombre, int obtenido, int esperado) {
    if (!comprobar(nombre, obtenido == esperado)) {
        std::cerr << "Esperado=" << esperado << " obtenido=" << obtenido << '\n';
    }
}

void probar_flujo(const std::string& nombre, const std::string& bytes,
                  bool retorno_esperado, ErrorGlobal error_esperado,
                  const Lote& esperado,
                  std::ios::iostate mascara = std::ios::goodbit) {
    Lote destino = semilla();
    ErrorGlobal error = ErrorGlobal::apertura;
    try {
        std::istringstream origen(bytes);
        origen.exceptions(mascara); // El flujo nuevo todavia esta sano.
        const bool retorno = assetcheck::revisar_flujo(origen, destino, error);
        comprobar_revision(nombre, retorno, error, destino,
                           retorno_esperado, error_esperado, esperado);
    } catch (const std::exception& e) {
        comprobar(nombre, false);
        std::cerr << "Esperado: retorno=" << retorno_esperado
                  << " error=" << nombre_error(error_esperado)
                  << "\nObtenido: excepcion inesperada: " << e.what() << '\n';
        std::cerr << "Lote esperado:\n";
        mostrar_lote(esperado);
        std::cerr << "Lote conservado al lanzar:\n";
        mostrar_lote(destino);
    }
}

// SECCION 3: ORIGEN_QUE_FALLA
// stringbuf entrega primero todo el prefijo. Al pedir el caracter siguiente,
// underflow lanza: ese final es un fallo real del origen, no EOF normal.
class OrigenQueFalla : public std::stringbuf {
    bool memoria_;
    bool activado_ = false;
protected:
    int_type underflow() override {
        activado_ = true;
        if (memoria_) throw std::bad_alloc{};
        throw std::runtime_error("fallo de lectura provocado");
    }
public:
    explicit OrigenQueFalla(const std::string& prefijo, bool memoria = false)
        : std::stringbuf(prefijo, std::ios::in), memoria_(memoria) {}
    bool activado() const { return activado_; }
};

// SECCION 4: CAMBIO_REGIONAL
// Una region artificial separa cada cifra. Asi se observa el cambio incluso
// en numeros de linea entre 10 y 100, dentro del dominio del manifiesto.
class SeparadorRegional : public std::numpunct<char> {
protected:
    char do_thousands_sep() const override { return '_'; }
    std::string do_grouping() const override {
        return "\1"; // Byte de valor 1: grupos de una cifra; no el texto "1".
    }
};

void probar_region(const Lote& informe, const std::string& admisiones,
                   const std::string& rechazos) {
    const std::locale anterior = std::locale();
    try {
        // locale se hace cargo de la faceta creada con new: no se hace delete.
        std::locale::global(std::locale(std::locale::classic(), new SeparadorRegional));
        std::ostringstream control;
        control << 30000;
        comprobar_texto("control que hace observable la region", control.str(), "3_0_0_0_0");
        comprobar_texto("admisiones independientes de region",
                        assetcheck::salida_admisiones(informe), admisiones);
        comprobar_texto("rechazos independientes de region",
                        assetcheck::salida_rechazos(informe), rechazos);
    } catch (const std::exception& e) {
        comprobar("configuracion regional sin excepciones", false);
        std::cerr << "Esperado: completar las comparaciones regionales\n"
                  << "Obtenido: " << e.what() << '\n';
    }
    std::locale::global(anterior); // Tambien se restaura tras el catch anterior.
}

// SECCION 5: ARCHIVOS_PROPIOS
fs::path reservar_directorio(const fs::path& padre) {
    if (!fs::is_directory(padre)) {
        throw std::runtime_error("El directorio padre de las pruebas no existe.");
    }
    for (std::size_t numero = 1; numero != 0; ++numero) {
        const fs::path candidato = padre / ("manifiesto_casos_" + std::to_string(numero));
        if (fs::create_directory(candidato)) {
            return candidato; // La reserva fue nueva: no se reutiliza otra carpeta.
        }
        // Si ya era un directorio, create_directory devuelve false y se prueba otro.
        // Cualquier otro error del sistema se propaga; no se borra nada para resolverlo.
    }
    throw std::runtime_error("No quedan numeros de directorio disponibles.");
}

void escribir_binario(const fs::path& ruta, const std::string& bytes) {
    if (fs::exists(ruta)) {
        throw std::runtime_error("El archivo de prueba ya existe: " + ruta.string());
    }
    std::ofstream archivo(ruta, std::ios::binary);
    if (!archivo.is_open()) {
        throw std::runtime_error("No se pudo crear el archivo de prueba: " + ruta.string());
    }
    archivo.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if (!archivo) {
        throw std::runtime_error("Fallo al escribir el archivo de prueba: " + ruta.string());
    }
    archivo.close();
    if (!archivo) {
        throw std::runtime_error("Fallo al cerrar el archivo de prueba: " + ruta.string());
    }
}

// SECCION 6: MAIN_Y_CASOS
void ejecutar_casos(const fs::path& trabajo) {
    const Lote previo = semilla();
    const Lote vacio{};
    const Lote uno{{{1, {"Sol", "sonido", 2}}}, {}};
    const Lote dos{{{1, {"Roca", "malla", 240}}, {2, {"Sol", "sonido", 2}}}, {}};
    const Lote rechazo_y_siguiente{{{2, {"Sol", "sonido", 2}}}, {{1, Resultado::formato, 0}}};
    const Lote blanco{{}, {{1, Resultado::formato, 0}}};
    const std::string b80 = "Roca malla 240" + std::string(66, ' ');
    const std::string siguiente = "Sol sonido 2\n";

    // 1-6: los limites locales no deben consumir ni renumerar la segunda linea.
    probar_flujo("80 mas CRLF y siguiente", b80 + "\r\n" + siguiente, true, ErrorGlobal::ninguno, dos);
    probar_flujo("81 mas LF y siguiente", b80 + " \n" + siguiente, true, ErrorGlobal::ninguno, rechazo_y_siguiente);
    probar_flujo("80 mas CRCRLF y siguiente", b80 + "\r\r\n" + siguiente, true, ErrorGlobal::ninguno, rechazo_y_siguiente);
    probar_flujo("80 mas CRXLF y siguiente", b80 + "\rX\n" + siguiente, true, ErrorGlobal::ninguno, rechazo_y_siguiente);
    probar_flujo("80 mas LF y siguiente", b80 + "\n" + siguiente, true, ErrorGlobal::ninguno, dos);
    probar_flujo("81 mas CRLF y siguiente", b80 + " \r\n" + siguiente, true, ErrorGlobal::ninguno, rechazo_y_siguiente);

    // 7-12: vacio, terminadores y estado completo de un lote mezclado.
    probar_flujo("vacio reemplaza previo", "", true, ErrorGlobal::ninguno, vacio);
    probar_flujo("ultima sin LF reemplaza previo", "Sol sonido 2", true, ErrorGlobal::ninguno, uno);
    probar_flujo("solo LF", "\n", true, ErrorGlobal::ninguno, blanco);
    probar_flujo("solo CRLF", "\r\n", true, ErrorGlobal::ninguno, blanco);
    probar_flujo("solo CR", "\r", true, ErrorGlobal::ninguno, blanco);
    const Lote mezcla{{{2, {"Pieza", "malla", 3}}, {3, {"Caja", "textura", 12}}},
                     {{1, Resultado::tamano, 0}, {4, Resultado::duplicado, 2}}};
    probar_flujo("mezcla conserva primera admision y origen",
        "Pieza malla 0\nPieza malla 3\nCaja textura 00012\nPieza sonido 9",
        true, ErrorGlobal::ninguno, mezcla);

    // 13-17: separadores, bytes invalidos y continuidad de la segunda linea.
    probar_flujo("TAB permitido entre campos", "Roca\tmalla\t240\r\nSol sonido 2",
        true, ErrorGlobal::ninguno, dos);
    const Lote tres{{{1, {"Roca", "malla", 240}}, {2, {"Sol", "sonido", 2}},
                     {3, {"Caja", "textura", 12}}}, {}};
    probar_flujo("terminadores mezclados y ultima sin LF",
        "Roca malla 240\r\nSol sonido 2\nCaja textura 12",
        true, ErrorGlobal::ninguno, tres);
    const Lote vacia_intermedia{{{1, {"Roca", "malla", 240}}, {3, {"Sol", "sonido", 2}}},
                               {{2, Resultado::formato, 0}}};
    probar_flujo("linea vacia entre dos validas", "Roca malla 240\n\nSol sonido 2",
        true, ErrorGlobal::ninguno, vacia_intermedia);
    probar_flujo("NUL y siguiente valida",
        "Roca malla 240" + std::string(1, '\0') + "\n" + siguiente,
        true, ErrorGlobal::ninguno, rechazo_y_siguiente);
    probar_flujo("byte 127 y siguiente valida",
        "Roca malla 240" + std::string(1, static_cast<char>(127)) + "\n" + siguiente,
        true, ErrorGlobal::ninguno, rechazo_y_siguiente);

    // 18-20: el esperado de las cien causas se construye del contrato,
    // no consultando el lector. La linea 101 debe conservar toda la semilla.
    std::string cien;
    Lote cien_rechazos;
    for (std::size_t i = 1; i <= 100; ++i) {
        cien += "A malla 0\n";
        cien_rechazos.rechazados.push_back({i, Resultado::tamano, 0});
    }
    probar_flujo("100 rechazos completan", cien, true, ErrorGlobal::ninguno, cien_rechazos);
    probar_flujo("101 con LF conserva previo", cien + "A malla 0\n", false, ErrorGlobal::limite_lineas, previo);
    probar_flujo("101 sin LF conserva previo", cien + "Sol sonido 2", false, ErrorGlobal::limite_lineas, previo);

    // 21-30: EOF normal con cinco mascaras; cada ensayo crea un flujo sano nuevo.
    const std::vector<std::pair<const char*, std::ios::iostate>> mascaras{
        {"fail", std::ios::failbit}, {"eof", std::ios::eofbit},
        {"bad", std::ios::badbit}, {"fail y eof", std::ios::failbit | std::ios::eofbit},
        {"fail eof y bad", std::ios::failbit | std::ios::eofbit | std::ios::badbit}
    };
    for (const auto& m : mascaras) {
        probar_flujo(std::string("EOF vacio mascara ") + m.first, "", true, ErrorGlobal::ninguno, vacio, m.second);
        probar_flujo(std::string("EOF ultima mascara ") + m.first, "Sol sonido 2", true, ErrorGlobal::ninguno, uno, m.second);
    }

    // 31-32: dos admisiones, un rechazo y una cola preceden al fallo real.
    const std::string prefijo = "Roca malla 240\nSol sonido 2\nErr malla 0\nMi";
    for (const auto mascara : {std::ios::goodbit, std::ios::badbit}) {
        OrigenQueFalla buffer(prefijo);
        std::istream origen(&buffer); // buffer vive hasta despues de esta lectura.
        origen.exceptions(mascara);
        Lote destino = previo;
        ErrorGlobal error = ErrorGlobal::ninguno;
        try {
            const bool retorno = assetcheck::revisar_flujo(origen, destino, error);
            const bool correcto = !retorno && error == ErrorGlobal::lectura
                && mismo_lote(destino, previo) && buffer.activado() && origen.bad();
            if (!comprobar(mascara == std::ios::goodbit ? "fallo absorbido por get" : "fallo relanzado por get", correcto)) {
                std::cerr << "Esperado: retorno=0 error=LECTURA underflow=1 bad=1\n";
                mostrar_lote(previo);
                std::cerr << "Obtenido: retorno=" << retorno << " error=" << nombre_error(error)
                          << " underflow=" << buffer.activado() << " bad=" << origen.bad() << '\n';
                mostrar_lote(destino);
            }
        } catch (const std::exception& e) {
            comprobar("fallo real debe retornar false/LECTURA", false);
            std::cerr << "Obtenido: excepcion inesperada: " << e.what() << '\n';
        }
    }

    // 33: con badbit en la mascara, la bad_alloc original alcanza la biblioteca.
    {
        OrigenQueFalla buffer(prefijo, true);
        std::istream origen(&buffer);
        origen.exceptions(std::ios::badbit);
        Lote destino = previo;
        ErrorGlobal error = ErrorGlobal::ninguno;
        bool propago = false;
        try { assetcheck::revisar_flujo(origen, destino, error); }
        catch (const std::bad_alloc&) { propago = true; }
        catch (...) {}
        if (!comprobar("bad_alloc que alcanza la biblioteca se propaga",
                       propago && buffer.activado() && mismo_lote(destino, previo))) {
            std::cerr << "Esperado: bad_alloc y lote anterior intacto; obtenido propago=" << propago << '\n';
            mostrar_lote(destino);
        }
    }

    // 34: sin excepciones activadas, fail sin EOF es un retorno false/LECTURA.
    {
        std::istringstream origen("Sol sonido 2\n");
        origen.setstate(std::ios::failbit);
        Lote destino = previo;
        ErrorGlobal error = ErrorGlobal::ninguno;
        const bool retorno = assetcheck::revisar_flujo(origen, destino, error);
        comprobar_revision("fail sin EOF retorna LECTURA", retorno, error, destino,
                           false, ErrorGlobal::lectura, previo);
    }

    // 35: la misma marca con mascara fail hace lanzar al get, sin bad ni eof.
    {
        std::istringstream origen("Sol sonido 2\n");
        origen.exceptions(std::ios::failbit);
        try { origen.setstate(std::ios::failbit); }
        catch (const std::ios_base::failure&) {} // La marca queda activada.
        Lote destino = previo;
        ErrorGlobal error = ErrorGlobal::ninguno;
        bool propago = false;
        try { assetcheck::revisar_flujo(origen, destino, error); }
        catch (const std::ios_base::failure&) { propago = true; }
        catch (...) {}
        if (!comprobar("excepcion sin bad ni EOF se propaga",
                       propago && !origen.bad() && !origen.eof() && mismo_lote(destino, previo))) {
            std::cerr << "Esperado: excepcion y lote anterior intacto; obtenido propago=" << propago << '\n';
            mostrar_lote(destino);
        }
    }

    // 36-37: los archivos son propios de este directorio nuevo.
    const fs::path binario = trabajo / "binario.manifiesto";
    escribir_binario(binario, b80 + "\r\n" + std::string(1, '\x1A') + "\nSol sonido 2");
    {
        Lote destino = previo;
        ErrorGlobal error = ErrorGlobal::lectura;
        const bool retorno = assetcheck::revisar_archivo(binario, destino, error);
        const Lote esperado{{{1, {"Roca", "malla", 240}}, {3, {"Sol", "sonido", 2}}},
                            {{2, Resultado::formato, 0}}};
        comprobar_revision("archivo binario conserva CTRLZ y sigue", retorno, error, destino,
                           true, ErrorGlobal::ninguno, esperado);
    }
    {
        Lote destino = previo;
        ErrorGlobal error = ErrorGlobal::ninguno;
        const bool retorno = assetcheck::revisar_archivo(trabajo / "no_existe.manifiesto", destino, error);
        comprobar_revision("apertura fallida conserva previo", retorno, error, destino,
                           false, ErrorGlobal::apertura, previo);
    }

    // 38-51: todas las causas, cadenas enteras y codigos se fijan por separado.
    const Lote informe{{{1, {"A", "malla", 30000}}, {6, {"B", "sonido", 2}}},
        {{2, Resultado::formato, 0}, {3, Resultado::id, 0}, {4, Resultado::tipo, 0},
         {5, Resultado::tamano, 0}, {7, Resultado::duplicado, 1},
         {8, Resultado::formato, 0}, {9, Resultado::tipo, 0}, {10, Resultado::tamano, 0},
         {11, Resultado::id, 0}, {12, Resultado::duplicado, 6}}};
    const std::string admisiones =
        "ACEPTADO linea=1 id=A tipo=malla bytes=30000\n"
        "ACEPTADO linea=6 id=B tipo=sonido bytes=2\n"
        "RESUMEN lineas=12 aceptados=2 rechazados=10\n";
    const std::string rechazos =
        "RECHAZO linea=2 causa=FORMATO\nRECHAZO linea=3 causa=ID\n"
        "RECHAZO linea=4 causa=TIPO\nRECHAZO linea=5 causa=TAMANO\n"
        "RECHAZO linea=7 causa=DUPLICADO primera=1\nRECHAZO linea=8 causa=FORMATO\n"
        "RECHAZO linea=9 causa=TIPO\nRECHAZO linea=10 causa=TAMANO\n"
        "RECHAZO linea=11 causa=ID\nRECHAZO linea=12 causa=DUPLICADO primera=6\n";
    comprobar_texto("cadena admisiones exacta", assetcheck::salida_admisiones(informe), admisiones);
    comprobar_texto("cadena rechazos exacta", assetcheck::salida_rechazos(informe), rechazos);
    comprobar_texto("cadena vacio tiene resumen", assetcheck::salida_admisiones(vacio), "RESUMEN lineas=0 aceptados=0 rechazados=0\n");
    comprobar_texto("cadena sin rechazos vacia", assetcheck::salida_rechazos(vacio), "");
    comprobar_texto("diagnostico ninguno", assetcheck::diagnostico_global(ErrorGlobal::ninguno), "");
    comprobar_texto("diagnostico apertura", assetcheck::diagnostico_global(ErrorGlobal::apertura), "ERROR causa=APERTURA\n");
    comprobar_texto("diagnostico lectura", assetcheck::diagnostico_global(ErrorGlobal::lectura), "ERROR causa=LECTURA\n");
    comprobar_texto("diagnostico limite", assetcheck::diagnostico_global(ErrorGlobal::limite_lineas), "ERROR causa=LIMITE_LINEAS\n");
    comprobar_codigo("codigo vacio", assetcheck::codigo_terminacion(ErrorGlobal::ninguno, vacio), 0);
    comprobar_codigo("codigo admision sin rechazo", assetcheck::codigo_terminacion(ErrorGlobal::ninguno, uno), 0);
    comprobar_codigo("codigo revision con rechazo", assetcheck::codigo_terminacion(ErrorGlobal::ninguno, informe), 1);
    for (const auto causa : {ErrorGlobal::apertura, ErrorGlobal::lectura, ErrorGlobal::limite_lineas}) {
        comprobar_codigo(std::string("codigo global ") + nombre_error(causa),
                          assetcheck::codigo_terminacion(causa, previo), 2);
    }

    // 52-54: region observable y cadenas de ambos canales aun en formato clasico.
    probar_region(informe, admisiones, rechazos);
}
} // namespace

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Uso: comprobar_manifiesto [directorio_padre_de_pruebas]\n";
        return 2;
    }
    try {
        const fs::path padre = argc == 2 ? fs::absolute(argv[1]) : fs::current_path();
        const fs::path trabajo = reservar_directorio(padre);
        std::cout << "DirectorioPruebas=" << trabajo.string() << '\n';
        ejecutar_casos(trabajo);
        std::cout << "Comprobaciones=" << comprobaciones << " Fallos=" << fallos << '\n';
        return fallos == 0 ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "No se pudo completar el comprobador: " << e.what() << '\n';
        return 2;
    }
}
