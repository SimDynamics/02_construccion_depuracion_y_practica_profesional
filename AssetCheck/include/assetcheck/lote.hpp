#pragma once
// lote.hpp, junto a registro.hpp (la biblioteca anterior)
#include "assetcheck/registro.hpp"
#include <cstddef>
#include <string>
#include <vector>
namespace assetcheck {
struct Admision { std::size_t linea; Registro registro; };
struct Rechazo {
    std::size_t linea;
    Resultado causa;
    std::size_t primera = 0;
};
struct Lote {
    std::vector<Admision> admitidos;
    std::vector<Rechazo> rechazados;
};
void procesar_linea(const std::string& texto,
                   std::size_t numero, Lote& lote);
}
