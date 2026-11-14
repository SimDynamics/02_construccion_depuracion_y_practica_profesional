#pragma once
#include <string>

namespace assetcheck {
struct Registro {
    std::string id;
    std::string tipo;
    int tamano_bytes = 0;
};
enum class Resultado { aceptado, formato, id, tipo, tamano, duplicado };
// El rechazo debe conservar todo el registro anterior.
Resultado leer_registro(const std::string& linea, Registro& destino);
}
