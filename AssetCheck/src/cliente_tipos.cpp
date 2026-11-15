// src/cliente_tipos.cpp (referencia del autor)
#include "assetcheck/estadisticas.hpp"
#include "assetcheck/manifiesto.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "ERROR causa=APERTURA\n";
        return 2;
    }

    assetcheck::Lote lote;
    assetcheck::ErrorGlobal error = assetcheck::ErrorGlobal::ninguno;

    if (!assetcheck::revisar_archivo(argv[1], lote, error)) {
        std::cerr << assetcheck::diagnostico_global(error);
        return 2;
    }

    std::cout << assetcheck::salida_tipos(
        assetcheck::contar_tipos(lote), lote);
    std::cerr << assetcheck::salida_rechazos(lote);
    return assetcheck::codigo_terminacion(error, lote);
}
