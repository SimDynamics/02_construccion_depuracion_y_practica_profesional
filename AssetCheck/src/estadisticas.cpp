#include "assetcheck/estadisticas.hpp"

#include <locale>
#include <sstream>

namespace assetcheck {

Estadisticas contar_tipos(const Lote& lote) {
    Estadisticas total{};
    for (const auto& admision : lote.admitidos) {
        const Registro& registro = admision.registro;
        if (registro.tipo == "textura") {
            ++total.texturas;
        } else if (registro.tipo == "malla") {
            ++total.mallas;
        } else if (registro.tipo == "sonido") {
            ++total.sonidos;
        }
        total.bytes_total += registro.tamano_bytes;
    }
    return total;
}

std::string salida_tipos(const Estadisticas& datos, const Lote& lote) {
    std::ostringstream salida;
    salida.imbue(std::locale::classic());
    salida << "TIPOS textura=" << datos.texturas
           << " malla=" << datos.mallas
           << " sonido=" << datos.sonidos
           << " bytes_total=" << datos.bytes_total << '\n';
    salida << "RESUMEN lineas=" << lote.admitidos.size() + lote.rechazados.size()
           << " aceptados=" << lote.admitidos.size()
           << " rechazados=" << lote.rechazados.size() << '\n';
    return salida.str();
}

} // namespace assetcheck
