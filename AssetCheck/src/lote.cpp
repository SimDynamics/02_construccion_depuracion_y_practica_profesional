// src/lote.cpp
#include "assetcheck/lote.hpp"
namespace assetcheck {
void procesar_linea(const std::string& texto,
                    std::size_t numero, Lote& lote) {
    Registro candidata;
    const Resultado local = leer_registro(texto, candidata);
    if (local != Resultado::aceptado) {
        lote.rechazados.push_back({numero, local, 0});
        return;
    }
    for (const auto& anterior : lote.admitidos) {
        if (anterior.registro.id == candidata.id) {
            lote.rechazados.push_back(
                {numero, Resultado::duplicado, anterior.linea});
            return;
        }
    }
    lote.admitidos.push_back({numero, candidata});
}
}
