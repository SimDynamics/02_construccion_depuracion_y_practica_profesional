#include "assetcheck/registro.hpp"

#include <locale>
#include <sstream>

namespace assetcheck {
namespace {
bool letra_ascii(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
bool digito_ascii(char c) {
    return c >= '0' && c <= '9';
}
bool id_valido(const std::string& id) {
    if (id.empty() || id.size() > 12 || !letra_ascii(id[0]))
        return false;
    for (char c : id)
        if (!letra_ascii(c) && !digito_ascii(c) && c != '_')
            return false;
    return true;
}
bool tamano_valido(const std::string& texto, int& destino)
{
    if (texto.empty() || texto.size() > 5) {
        return false;
    }
    int candidato = 0;
    for (char c : texto) {
        if (c < '0' || c > '9') {
            return false;
        }
        candidato = candidato * 10 + (c - '0');
    }
    if (candidato < 1 || candidato > 30000) {
        return false;
    }
    destino = candidato;
    return true;
}
} // namespace

Resultado leer_registro(const std::string& linea, Registro& destino)
{
    if (linea.size() > 80) return Resultado::formato;
    for (unsigned char ch : linea) {
        if (ch != '\t' && (ch < 32 || ch > 126))
            return Resultado::formato;
    }

    std::istringstream entrada(linea);
    entrada.imbue(std::locale::classic());
    std::string id, tipo, tamano, extra;
    if (!(entrada >> id >> tipo >> tamano) || (entrada >> extra))
        return Resultado::formato;

    if (!id_valido(id)) return Resultado::id;
    if (tipo != "textura" && tipo != "malla" && tipo != "sonido")
        return Resultado::tipo;

    int cantidad = 0;
    if (!tamano_valido(tamano, cantidad)) return Resultado::tamano;

    Registro candidato{id, tipo, cantidad};
    destino = candidato;
    return Resultado::aceptado;
}
} // namespace assetcheck
