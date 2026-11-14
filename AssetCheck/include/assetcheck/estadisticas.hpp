// include/assetcheck/estadisticas.hpp
#pragma once
#include "assetcheck/lote.hpp"

#include <cstddef>
#include <string>

namespace assetcheck {

struct Estadisticas {
    std::size_t texturas = 0,
                mallas = 0,
                sonidos = 0;
    int bytes_total = 0;
};

Estadisticas contar_tipos(const Lote& lote);
std::string salida_tipos(const Estadisticas& datos, const Lote& lote);

}
