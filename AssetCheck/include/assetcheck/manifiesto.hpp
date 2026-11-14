#pragma once
#include "assetcheck/lote.hpp"
#include <filesystem>
#include <iosfwd>
#include <string>
namespace assetcheck {
enum class ErrorGlobal { ninguno, apertura, lectura, limite_lineas };
bool revisar_flujo(std::istream& origen, Lote& destino,
                   ErrorGlobal& error);
bool revisar_archivo(const std::filesystem::path& ruta, Lote& destino,
                    ErrorGlobal& error);
std::string salida_admisiones(const Lote& lote);
std::string salida_rechazos(const Lote& lote);
std::string diagnostico_global(ErrorGlobal error);
int codigo_terminacion(ErrorGlobal error, const Lote& lote);
}
