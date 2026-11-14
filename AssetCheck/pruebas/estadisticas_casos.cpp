#include "assetcheck/estadisticas.hpp"

#include <iostream>
#include <string>

namespace {
int comprobaciones = 0;
int fallos = 0;

void comprobar(bool coincide, const char* nombre) {
    ++comprobaciones;
    std::cout << nombre << ": " << (coincide ? "OK" : "FALLO") << '\n';
    if (!coincide) {
        ++fallos;
    }
}
}

int main() {
    using namespace assetcheck;

    const Lote vacio{};
    const Estadisticas cero = contar_tipos(vacio);
    comprobar(cero.texturas == 0, "vacio_texturas");
    comprobar(cero.mallas == 0, "vacio_mallas");
    comprobar(cero.sonidos == 0, "vacio_sonidos");
    comprobar(cero.bytes_total == 0, "vacio_bytes");
    comprobar(salida_tipos(cero, vacio) ==
        "TIPOS textura=0 malla=0 sonido=0 bytes_total=0\n"
        "RESUMEN lineas=0 aceptados=0 rechazados=0\n",
        "vacio_texto_completo");

    Lote mezclado{};
    procesar_linea("A malla 2", 1, mezclado);
    procesar_linea("B sonido 7", 2, mezclado);
    procesar_linea("A sonido 9", 3, mezclado); // Repetido, no suma 9.
    procesar_linea("C textura 5", 4, mezclado);
    procesar_linea("D textura 0", 5, mezclado); // Tamano rechazado.
    procesar_linea("E imagen 2", 6, mezclado); // Tipo rechazado.
    procesar_linea("F sonido 30000", 7, mezclado);

    const Estadisticas datos = contar_tipos(mezclado);
    comprobar(datos.texturas == 1, "mezcla_texturas");
    comprobar(datos.mallas == 1, "mezcla_mallas");
    comprobar(datos.sonidos == 2, "mezcla_sonidos");
    comprobar(datos.bytes_total == 30014, "mezcla_bytes_sin_rechazos");
    comprobar(mezclado.admitidos.size() == 4, "mezcla_admisiones");
    comprobar(mezclado.rechazados.size() == 3, "mezcla_rechazos");
    comprobar(salida_tipos(datos, mezclado) ==
        "TIPOS textura=1 malla=1 sonido=2 bytes_total=30014\n"
        "RESUMEN lineas=7 aceptados=4 rechazados=3\n",
        "mezcla_texto_completo");
    comprobar(mezclado.rechazados.size() == 3
        && mezclado.rechazados[0].linea == 3
        && mezclado.rechazados[0].causa == Resultado::duplicado
        && mezclado.rechazados[0].primera == 1
        && mezclado.rechazados[1].causa == Resultado::tamano
        && mezclado.rechazados[2].causa == Resultado::tipo,
        "mezcla_causas_independientes");
    comprobar(datos.bytes_total == 2 + 7 + 5 + 30000,
        "mezcla_esperado_independiente");

    const Estadisticas repetido = contar_tipos(mezclado);
    comprobar(repetido.bytes_total == datos.bytes_total
        && repetido.texturas == datos.texturas
        && repetido.mallas == datos.mallas
        && repetido.sonidos == datos.sonidos,
        "consulta_repetida");

    Lote nuevo{};
    procesar_linea("A sonido 4", 1, nuevo);
    const Estadisticas distintos = contar_tipos(nuevo);
    comprobar(distintos.texturas == 0 && distintos.mallas == 0
        && distintos.sonidos == 1 && distintos.bytes_total == 4,
        "otra_coleccion_reinicia");
    comprobar(mezclado.admitidos.size() == 4
        && mezclado.rechazados.size() == 3
        && mezclado.admitidos[0].registro.tamano_bytes == 2,
        "primera_coleccion_intacta");
    comprobar(salida_tipos(distintos, nuevo) ==
        "TIPOS textura=0 malla=0 sonido=1 bytes_total=4\n"
        "RESUMEN lineas=1 aceptados=1 rechazados=0\n",
        "segunda_coleccion_salida");

    Lote extremo{};
    for (int i = 0; i < 100; ++i) {
        procesar_linea("Item" + std::to_string(i) + " textura 30000",
                       static_cast<std::size_t>(i + 1), extremo);
    }
    const Estadisticas maximo = contar_tipos(extremo);
    comprobar(extremo.admitidos.size() == 100 && extremo.rechazados.empty(),
        "cien_admisiones_independientes");
    comprobar(maximo.bytes_total == 3000000, "cien_tamanos_maximos");
    comprobar(maximo.texturas == 100 && maximo.mallas == 0
        && maximo.sonidos == 0, "cien_texturas");
    comprobar(salida_tipos(maximo, extremo) ==
        "TIPOS textura=100 malla=0 sonido=0 bytes_total=3000000\n"
        "RESUMEN lineas=100 aceptados=100 rechazados=0\n",
        "cien_salida_completa");

    std::cout << "Comparaciones=" << comprobaciones
              << " Fallos=" << fallos << '\n';
    return comprobaciones == 22 && fallos == 0 ? 0 : 1;
}
