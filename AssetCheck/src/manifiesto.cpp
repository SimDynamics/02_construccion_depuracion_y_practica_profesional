#include "assetcheck/manifiesto.hpp"
#include <fstream>
#include <istream>
#include <locale>
#include <new>
#include <sstream>
#include <utility>

namespace assetcheck {
namespace {
const char* nombre_causa(Resultado resultado) {
    switch (resultado) {
        case Resultado::aceptado: return "ACEPTADO";
        case Resultado::formato: return "FORMATO";
        case Resultado::id: return "ID";
        case Resultado::tipo: return "TIPO";
        case Resultado::tamano: return "TAMANO";
        case Resultado::duplicado: return "DUPLICADO";
    }
    return "DESCONOCIDA";
}
} // namespace

bool revisar_flujo(std::istream& origen, Lote& destino, ErrorGlobal& error) {
    error = ErrorGlobal::ninguno;
    Lote candidato;
    std::string linea;
    std::size_t numero = 0;
    bool tiene_bytes = false;
    bool linea_truncada = false;

    const auto terminar_linea = [&]() -> bool {
        ++numero;
        if (numero > 100) {
            error = ErrorGlobal::limite_lineas;
            return false;
        }
        procesar_linea(linea, numero, candidato);
        linea.clear();
        tiene_bytes = false;
        linea_truncada = false;
        return true;
    };

    for (;;) {
        // El try cubre solo la extraccion del origen. No cubre las operaciones
        // sobre cadenas, vectores, el lote candidato ni el destino.
        std::char_traits<char>::int_type dato;
        try {
            dato = origen.get();
        } catch (const std::bad_alloc&) {
            throw;
        } catch (...) {
            if (origen.bad()) {
                error = ErrorGlobal::lectura;
                return false;
            }
            if (origen.eof()) {
                break;
            }
            throw;
        }

        if (dato == std::char_traits<char>::eof()) {
            if (origen.bad() || !origen.eof()) {
                error = ErrorGlobal::lectura;
                return false;
            }
            break;
        }
        const char c = static_cast<char>(dato);
        if (c == '\n') {
            // Si se descarto algun byte, el ultimo CR retenido podria no
            // ser el inmediatamente anterior a LF: en ese caso no se retira.
            if (!linea_truncada && !linea.empty() && linea.back() == '\r') {
                linea.pop_back();
            }
            if (!terminar_linea()) {
                return false;
            }
        } else {
            tiene_bytes = true;
            if (linea.size() < 81) {
                linea.push_back(c);
            } else {
                linea_truncada = true;
            }
        }
    }

    // Un CR final sin LF sigue siendo contenido de esta ultima linea.
    if (tiene_bytes && !terminar_linea()) {
        return false;
    }
    destino = std::move(candidato);
    return true;
}

bool revisar_archivo(const std::filesystem::path& ruta, Lote& destino,
                     ErrorGlobal& error) {
    std::ifstream archivo(ruta, std::ios::binary);
    if (!archivo.is_open()) {
        error = ErrorGlobal::apertura;
        return false;
    }
    return revisar_flujo(archivo, destino, error);
}

std::string salida_admisiones(const Lote& lote) {
    std::ostringstream salida;
    salida.imbue(std::locale::classic());
    for (const auto& admision : lote.admitidos) {
        salida << "ACEPTADO linea=" << admision.linea
               << " id=" << admision.registro.id
               << " tipo=" << admision.registro.tipo
               << " bytes=" << admision.registro.tamano_bytes << '\n';
    }
    salida << "RESUMEN lineas=" << lote.admitidos.size() + lote.rechazados.size()
           << " aceptados=" << lote.admitidos.size()
           << " rechazados=" << lote.rechazados.size() << '\n';
    return salida.str();
}

std::string salida_rechazos(const Lote& lote) {
    std::ostringstream salida;
    salida.imbue(std::locale::classic());
    for (const auto& rechazo : lote.rechazados) {
        salida << "RECHAZO linea=" << rechazo.linea
               << " causa=" << nombre_causa(rechazo.causa);
        if (rechazo.causa == Resultado::duplicado) {
            salida << " primera=" << rechazo.primera;
        }
        salida << '\n';
    }
    return salida.str();
}

std::string diagnostico_global(ErrorGlobal error) {
    switch (error) {
        case ErrorGlobal::ninguno: return "";
        case ErrorGlobal::apertura: return "ERROR causa=APERTURA\n";
        case ErrorGlobal::lectura: return "ERROR causa=LECTURA\n";
        case ErrorGlobal::limite_lineas: return "ERROR causa=LIMITE_LINEAS\n";
    }
    return "";
}

int codigo_terminacion(ErrorGlobal error, const Lote& lote) {
    if (error != ErrorGlobal::ninguno) {
        return 2;
    }
    return lote.rechazados.empty() ? 0 : 1;
}
} // namespace assetcheck
