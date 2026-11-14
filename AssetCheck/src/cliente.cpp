#include "assetcheck/manifiesto.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "ERROR causa=APERTURA\n";
        return 2;
    }
    assetcheck::Lote informe;
    assetcheck::ErrorGlobal error = assetcheck::ErrorGlobal::ninguno;
    if (!assetcheck::revisar_archivo(argv[1], informe, error)) {
        std::cerr << assetcheck::diagnostico_global(error);
        return 2;
    }
    std::cout << assetcheck::salida_admisiones(informe);
    std::cerr << assetcheck::salida_rechazos(informe);
    return assetcheck::codigo_terminacion(error, informe);
}
