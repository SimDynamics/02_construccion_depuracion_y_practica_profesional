#include "assetcheck/estadisticas.hpp"
#include "assetcheck/manifiesto.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "ERROR causa=APERTURA\n";
        return 2;
    }

    assetcheck::Lote lote{};
    assetcheck::ErrorGlobal error = assetcheck::ErrorGlobal::ninguno;

    if (!assetcheck::revisar_archivo(argv[1], lote, error)) {
        std::cerr << assetcheck::diagnostico_global(error);
        return 2;
    }

    const assetcheck::Estadisticas datos = assetcheck::contar_tipos(lote);
    std::cout << assetcheck::salida_tipos(datos, lote);
    std::cerr << assetcheck::salida_rechazos(lote);

    return assetcheck::codigo_terminacion(error, lote);
}
