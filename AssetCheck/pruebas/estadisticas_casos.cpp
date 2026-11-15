// pruebas/estadisticas_casos.cpp
#include "assetcheck/estadisticas.hpp"

#include <iostream>
#include <string>

namespace {

int comparaciones = 0;
int fallos = 0;

// SECCION 1: comparar y mostrar el resultado de cada comprobacion.
bool comprobar(bool coincide, const std::string& nombre) {
    ++comparaciones;
    if (!coincide) {
        ++fallos;
        std::cerr << "FALLO " << nombre << '\n';
    }
    return coincide;
}

int terminar() {
    std::cout << "Comparaciones=" << comparaciones
              << " Fallos=" << fallos << '\n';
    return comparaciones == 40 && fallos == 0 ? 0 : 1;
}

// SECCION 2: verificar el lote preparado, antes de contar sus tipos.
bool mismo_lote(const assetcheck::Lote& obtenido,
                const assetcheck::Lote& esperado) {
    if (obtenido.admitidos.size() != esperado.admitidos.size()
        || obtenido.rechazados.size() != esperado.rechazados.size()) {
        return false;
    }

    for (std::size_t i = 0; i < esperado.admitidos.size(); ++i) {
        const auto& a = obtenido.admitidos[i];
        const auto& e = esperado.admitidos[i];
        if (a.linea != e.linea || a.registro.id != e.registro.id
            || a.registro.tipo != e.registro.tipo
            || a.registro.tamano_bytes != e.registro.tamano_bytes) {
            return false;
        }
    }

    for (std::size_t i = 0; i < esperado.rechazados.size(); ++i) {
        const auto& a = obtenido.rechazados[i];
        const auto& e = esperado.rechazados[i];
        if (a.linea != e.linea || a.causa != e.causa
            || a.primera != e.primera) {
            return false;
        }
    }
    return true;
}

// SECCION 3: cuatro comparaciones separadas y un diagnostico de sus valores.
void comprobar_estadisticas(const assetcheck::Estadisticas& obtenido,
                            const assetcheck::Estadisticas& esperado,
                            const std::string& nombre) {
    comprobar(obtenido.texturas == esperado.texturas, nombre + "/texturas");
    comprobar(obtenido.mallas == esperado.mallas, nombre + "/mallas");
    comprobar(obtenido.sonidos == esperado.sonidos, nombre + "/sonidos");
    comprobar(obtenido.bytes_total == esperado.bytes_total, nombre + "/bytes");

    if (obtenido.texturas != esperado.texturas
        || obtenido.mallas != esperado.mallas
        || obtenido.sonidos != esperado.sonidos
        || obtenido.bytes_total != esperado.bytes_total) {
        std::cerr << nombre << " esperado: " << esperado.texturas << ' '
                  << esperado.mallas << ' ' << esperado.sonidos << ' '
                  << esperado.bytes_total << '\n';
        std::cerr << nombre << " obtenido: " << obtenido.texturas << ' '
                  << obtenido.mallas << ' ' << obtenido.sonidos << ' '
                  << obtenido.bytes_total << '\n';
    }
}

void comprobar_salida(const std::string& obtenido,
                      const std::string& esperado,
                      const std::string& nombre) {
    if (!comprobar(obtenido == esperado, nombre)) {
        std::cerr << "Esperado:\n" << esperado
                  << "Obtenido:\n" << obtenido;
    }
}

} // namespace

// SECCION 4: casos y expectativas independientes de contar_tipos.
int main() {
    using namespace assetcheck;

    // Caso 1: lote vacio, cuatro ceros y dos filas completas.
    const Lote vacio{};
    const Estadisticas cero = contar_tipos(vacio);
    comprobar_estadisticas(cero, {0, 0, 0, 0}, "vacio");
    comprobar_salida(salida_tipos(cero, vacio),
        "TIPOS textura=0 malla=0 sonido=0 bytes_total=0\n"
        "RESUMEN lineas=0 aceptados=0 rechazados=0\n", "vacio/salida");

    // Caso 2: seis admisiones asimetricas y un duplicado que no suma 9.
    Lote mezcla{};
    procesar_linea("A malla 2", 1, mezcla);
    procesar_linea("B sonido 7", 2, mezcla);
    procesar_linea("A sonido 9", 3, mezcla);
    procesar_linea("C textura 5", 4, mezcla);
    procesar_linea("D malla 3", 5, mezcla);
    procesar_linea("E sonido 4", 6, mezcla);
    procesar_linea("F sonido 6", 7, mezcla);
    const Lote mezcla_esperada{
        {{1, {"A", "malla", 2}}, {2, {"B", "sonido", 7}},
         {4, {"C", "textura", 5}}, {5, {"D", "malla", 3}},
         {6, {"E", "sonido", 4}}, {7, {"F", "sonido", 6}}},
        {{3, Resultado::duplicado, 1}}
    };
    if (!comprobar(mismo_lote(mezcla, mezcla_esperada), "preparacion/mezcla")) {
        return terminar();
    }
    const Estadisticas datos = contar_tipos(mezcla);
    comprobar_estadisticas(datos, {1, 2, 3, 27}, "mezcla");
    comprobar_salida(salida_tipos(datos, mezcla),
        "TIPOS textura=1 malla=2 sonido=3 bytes_total=27\n"
        "RESUMEN lineas=7 aceptados=6 rechazados=1\n", "mezcla/salida");

    // Caso 3: un rechazo no reserva el ID para una aparicion valida posterior.
    Lote invalido_valido{};
    procesar_linea("Loma textura 0", 1, invalido_valido);
    procesar_linea("Loma textura 11", 2, invalido_valido);
    procesar_linea("_Eco sonido 2", 3, invalido_valido);
    procesar_linea("Eco sonido 2", 4, invalido_valido);
    procesar_linea("Plano imagen 3", 5, invalido_valido);
    procesar_linea("Sobran sonido 2 extra", 6, invalido_valido);
    const Lote invalido_valido_esperado{
        {{2, {"Loma", "textura", 11}}, {4, {"Eco", "sonido", 2}}},
        {{1, Resultado::tamano, 0}, {3, Resultado::id, 0},
         {5, Resultado::tipo, 0}, {6, Resultado::formato, 0}}
    };
    if (!comprobar(mismo_lote(invalido_valido, invalido_valido_esperado),
                   "preparacion/invalido_valido")) {
        return terminar();
    }
    comprobar_estadisticas(contar_tipos(invalido_valido),
                            {1, 0, 1, 13}, "invalido_valido");

    // Caso 4: A y a son distintos; cada uno conserva su primer origen.
    Lote mayusculas{};
    procesar_linea("A textura 3", 1, mayusculas);
    procesar_linea("a malla 5", 2, mayusculas);
    procesar_linea("A sonido 9", 3, mayusculas);
    procesar_linea("a sonido 8", 4, mayusculas);
    const Lote mayusculas_esperadas{
        {{1, {"A", "textura", 3}}, {2, {"a", "malla", 5}}},
        {{3, Resultado::duplicado, 1}, {4, Resultado::duplicado, 2}}
    };
    if (!comprobar(mismo_lote(mayusculas, mayusculas_esperadas),
                   "preparacion/mayusculas")) {
        return terminar();
    }
    comprobar_estadisticas(contar_tipos(mayusculas), {1, 1, 0, 8}, "mayusculas");

    // Caso 5: hay tres lineas revisadas, pero ninguna admision que contar.
    Lote rechazados{};
    procesar_linea("A malla 0", 1, rechazados);
    procesar_linea("_B textura 5", 2, rechazados);
    procesar_linea("C imagen 7", 3, rechazados);
    const Lote rechazos_esperados{
        {},
        {{1, Resultado::tamano, 0}, {2, Resultado::id, 0},
         {3, Resultado::tipo, 0}}
    };
    if (!comprobar(mismo_lote(rechazados, rechazos_esperados),
                   "preparacion/rechazados")) {
        return terminar();
    }
    comprobar_estadisticas(contar_tipos(rechazados), {0, 0, 0, 0}, "rechazados");

    // Caso 6: cien IDs diferentes, cada uno con el tamano maximo permitido.
    Lote maximo{};
    Lote maximo_esperado{};
    for (int i = 0; i < 100; ++i) {
        const std::string id = "Item" + std::to_string(i);
        const std::size_t linea = static_cast<std::size_t>(i + 1);
        procesar_linea(id + " textura 30000", linea, maximo);
        maximo_esperado.admitidos.push_back({linea, {id, "textura", 30000}});
    }
    if (!comprobar(mismo_lote(maximo, maximo_esperado), "preparacion/maximo")) {
        return terminar();
    }
    const Estadisticas limite = contar_tipos(maximo);
    comprobar_estadisticas(limite, {100, 0, 0, 3000000}, "maximo");
    comprobar_salida(salida_tipos(limite, maximo),
        "TIPOS textura=100 malla=0 sonido=0 bytes_total=3000000\n"
        "RESUMEN lineas=100 aceptados=100 rechazados=0\n", "maximo/salida");

    // Casos 7 y 8: cada consulta reinicia sus cuatro campos.
    comprobar_estadisticas(contar_tipos(mezcla), {1, 2, 3, 27}, "mezcla_repetida");
    comprobar_estadisticas(contar_tipos(vacio), {0, 0, 0, 0}, "vacio_tras_mezcla");
    return terminar();
}
