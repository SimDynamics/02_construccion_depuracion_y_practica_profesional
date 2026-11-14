#include "assetcheck/lote.hpp"
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace {
using assetcheck::Lote;
using assetcheck::Registro;
using assetcheck::Resultado;

bool mismo_registro(const Registro& obtenido, const Registro& esperado) {
    return obtenido.id == esperado.id
        && obtenido.tipo == esperado.tipo
        && obtenido.tamano_bytes == esperado.tamano_bytes;
}

bool mismo_lote(const Lote& obtenido, const Lote& esperado) {
    // La igualdad de tamanos detecta tanto elementos faltantes como sobrantes.
    if (obtenido.admitidos.size() != esperado.admitidos.size()
        || obtenido.rechazados.size() != esperado.rechazados.size()) {
        return false;
    }
    // Se comparan posiciones, no conjuntos: tambien importa el orden.
    for (std::size_t i = 0; i < esperado.admitidos.size(); ++i) {
        if (obtenido.admitidos[i].linea != esperado.admitidos[i].linea
            || !mismo_registro(obtenido.admitidos[i].registro,
                               esperado.admitidos[i].registro)) {
            return false;
        }
    }
    for (std::size_t i = 0; i < esperado.rechazados.size(); ++i) {
        if (obtenido.rechazados[i].linea != esperado.rechazados[i].linea
            || obtenido.rechazados[i].causa != esperado.rechazados[i].causa
            || obtenido.rechazados[i].primera != esperado.rechazados[i].primera) {
            return false;
        }
    }
    return true;
}

const char* nombre(Resultado resultado) {
    switch (resultado) {
        case Resultado::aceptado: return "ACEPTADO";
        case Resultado::formato: return "FORMATO";
        case Resultado::id: return "ID";
        case Resultado::tipo: return "TIPO";
        case Resultado::tamano: return "TAMANO";
        case Resultado::duplicado: return "DUPLICADO";
    }
    return "RESULTADO_DESCONOCIDO";
}

void mostrar_lote(const Lote& lote) {
    std::cerr << "Admitidos=" << lote.admitidos.size()
              << " Rechazados=" << lote.rechazados.size() << '\n';
    for (const auto& admision : lote.admitidos) {
        std::cerr << "A " << admision.linea << ' ' << admision.registro.id
                  << ' ' << admision.registro.tipo << ' '
                  << admision.registro.tamano_bytes << '\n';
    }
    for (const auto& rechazo : lote.rechazados) {
        std::cerr << "R " << rechazo.linea << ' ' << nombre(rechazo.causa)
                  << ' ' << rechazo.primera << '\n';
    }
}

struct Caso {
    const char* nombre;
    std::vector<std::string> lineas;
    Lote esperado;
};
} // namespace

int main() {
    // Cada fila declara sus entradas y su estado final esperado sin consultar
    // procesar_linea ni leer_registro para calcular la expectativa.
    const std::vector<Caso> casos = {
        // 1. Sin llamadas, ambas colecciones deben quedar vacias.
        {"lote vacio", {}, Lote{{}, {}}},
        // 2-4. Se conservan los tres campos y la linea de una unica admision.
        {"una malla", {"A malla 1"},
            Lote{{{1, {"A", "malla", 1}}}, {}}},
        {"una textura y ceros iniciales", {"Caja textura 00012"},
            Lote{{{1, {"Caja", "textura", 12}}}, {}}},
        {"un sonido de tamano maximo", {"Voz sonido 30000"},
            Lote{{{1, {"Voz", "sonido", 30000}}}, {}}},
        // 5-6. IDs distintos deben admitirse en los dos sentidos de orden.
        {"distintos A despues B", {"A malla 3", "B sonido 7"},
            Lote{{{1, {"A", "malla", 3}}, {2, {"B", "sonido", 7}}}, {}}},
        {"distintos B despues A", {"B sonido 7", "A malla 3"},
            Lote{{{1, {"B", "sonido", 7}}, {2, {"A", "malla", 3}}}, {}}},
        // 7. El orden de admision no es el orden alfabetico.
        {"tres admisiones en orden de llegada",
            {"Z sonido 9", "A malla 2", "M textura 6"},
            Lote{{{1, {"Z", "sonido", 9}}, {2, {"A", "malla", 2}},
                  {3, {"M", "textura", 6}}}, {}}},
        // 8-11. El ID decide el duplicado incluso al variar los otros campos.
        {"repeticion inmediata identica", {"Caja malla 3", "Caja malla 3"},
            Lote{{{1, {"Caja", "malla", 3}}}, {{2, Resultado::duplicado, 1}}}},
        {"duplicado con otro tipo", {"Caja malla 3", "Caja sonido 3"},
            Lote{{{1, {"Caja", "malla", 3}}}, {{2, Resultado::duplicado, 1}}}},
        {"duplicado con otro tamano", {"Caja malla 3", "Caja malla 7"},
            Lote{{{1, {"Caja", "malla", 3}}}, {{2, Resultado::duplicado, 1}}}},
        {"duplicado con otro tipo y tamano", {"Caja malla 3", "Caja sonido 7"},
            Lote{{{1, {"Caja", "malla", 3}}}, {{2, Resultado::duplicado, 1}}}},
        // 12-13. Buscar solo el ultimo elemento no basta.
        {"duplicado de la primera tras otra admision",
            {"Caja malla 3", "Sol textura 4", "Caja sonido 7"},
            Lote{{{1, {"Caja", "malla", 3}}, {2, {"Sol", "textura", 4}}},
                 {{3, Resultado::duplicado, 1}}}},
        {"duplicado de una admision intermedia",
            {"Norte textura 2", "Este malla 3", "Sur sonido 4", "Este sonido 8"},
            Lote{{{1, {"Norte", "textura", 2}}, {2, {"Este", "malla", 3}},
                  {3, {"Sur", "sonido", 4}}}, {{4, Resultado::duplicado, 2}}}},
        // 14. Los rechazos repetidos conservan el primer origen y su orden.
        {"dos duplicados del mismo origen",
            {"Caja textura 12", "Caja malla 4", "Caja sonido 9"},
            Lote{{{1, {"Caja", "textura", 12}}},
                 {{2, Resultado::duplicado, 1}, {3, Resultado::duplicado, 1}}}},
        // 15-17. Rechazar localmente no reserva el ID para una linea posterior.
        {"tamano invalido antes de admision", {"Voz sonido 0", "Voz sonido 6"},
            Lote{{{2, {"Voz", "sonido", 6}}}, {{1, Resultado::tamano, 0}}}},
        {"tipo invalido antes de admision", {"X texture 2", "X textura 1"},
            Lote{{{2, {"X", "textura", 1}}}, {{1, Resultado::tipo, 0}}}},
        {"formato invalido antes de admision", {"Caja malla 3 extra", "Caja malla 3"},
            Lote{{{2, {"Caja", "malla", 3}}}, {{1, Resultado::formato, 0}}}},
        // 18-19. La comparacion distingue mayusculas; cada ID conserva su origen.
        {"Caja y caja son distintos", {"Caja malla 3", "caja sonido 5"},
            Lote{{{1, {"Caja", "malla", 3}}, {2, {"caja", "sonido", 5}}}, {}}},
        {"duplicados de ambas variantes",
            {"Caja malla 3", "caja sonido 5", "Caja textura 8", "caja malla 9"},
            Lote{{{1, {"Caja", "malla", 3}}, {2, {"caja", "sonido", 5}}},
                 {{3, Resultado::duplicado, 1}, {4, Resultado::duplicado, 2}}}},
        // 20-23. El rechazo local tiene prioridad sobre una coincidencia de ID.
        {"TIPO antes de DUPLICADO", {"Caja malla 3", "Caja material 7"},
            Lote{{{1, {"Caja", "malla", 3}}}, {{2, Resultado::tipo, 0}}}},
        {"TAMANO antes de DUPLICADO", {"Caja malla 3", "Caja sonido 0"},
            Lote{{{1, {"Caja", "malla", 3}}}, {{2, Resultado::tamano, 0}}}},
        {"FORMATO antes de DUPLICADO", {"Caja malla 3", "Caja sonido 7 extra"},
            Lote{{{1, {"Caja", "malla", 3}}}, {{2, Resultado::formato, 0}}}},
        {"TIPO antes de TAMANO y DUPLICADO", {"Caja malla 3", "Caja material 0"},
            Lote{{{1, {"Caja", "malla", 3}}}, {{2, Resultado::tipo, 0}}}},
        // 24. Un rechazo intermedio no borra ni sustituye la admision anterior.
        {"origen conservado tras rechazo local",
            {"Voz sonido 0", "Voz sonido 6", "Voz material 0", "Voz malla 9"},
            Lote{{{2, {"Voz", "sonido", 6}}},
                 {{1, Resultado::tamano, 0}, {3, Resultado::tipo, 0},
                  {4, Resultado::duplicado, 2}}}},
        // 25. Ejemplo de la pagina 9, con todos los campos comparados.
        {"Pieza invalida valida intermedia y duplicada",
            {"Pieza malla 0", "Pieza malla 3", "Caja textura 00012", "Pieza sonido 9"},
            Lote{{{2, {"Pieza", "malla", 3}}, {3, {"Caja", "textura", 12}}},
                 {{1, Resultado::tamano, 0}, {4, Resultado::duplicado, 2}}}},
        // 26. Traza completa de la pagina 5: no se renumeran las admisiones.
        {"traza Puerta y Caja",
            {"Puerta malla 0", "Puerta malla 3", "Caja textura 00012",
             "Puerta sonido 7", "puerta sonido 5", "Caja malla 4"},
            Lote{{{2, {"Puerta", "malla", 3}}, {3, {"Caja", "textura", 12}},
                  {5, {"puerta", "sonido", 5}}},
                 {{1, Resultado::tamano, 0}, {4, Resultado::duplicado, 2},
                  {6, Resultado::duplicado, 3}}}},
        // 27. Aqui el llamador proporciona todos los numeros, incluso el blanco.
        {"linea vacia entre admision y duplicado",
            {"X textura 1", "", "X textura 8"},
            Lote{{{1, {"X", "textura", 1}}},
                 {{2, Resultado::formato, 0}, {3, Resultado::duplicado, 1}}}},
        // 28. Se conserva la primera causa local, no la ultima encontrada.
        {"ID antes de TIPO y TAMANO", {"9X material 0", "X malla 2"},
            Lote{{{2, {"X", "malla", 2}}}, {{1, Resultado::id, 0}}}},
        // 29. Un prefijo comun no convierte dos identificadores en iguales.
        {"A y AA son distintos", {"A malla 2", "AA malla 2"},
            Lote{{{1, {"A", "malla", 2}}, {2, {"AA", "malla", 2}}}, {}}},
        // 30-31. Dos lotes nuevos sucesivos pueden admitir el mismo ID.
        {"primer lote con Reinicio", {"Reinicio malla 0", "Reinicio malla 3"},
            Lote{{{2, {"Reinicio", "malla", 3}}}, {{1, Resultado::tamano, 0}}}},
        {"segundo lote nuevo con Reinicio", {"Reinicio sonido 7"},
            Lote{{{1, {"Reinicio", "sonido", 7}}}, {}}}
    };

    int comprobaciones = 0;
    int fallos = 0;
    for (const Caso& caso : casos) {
        Lote obtenido; // Estado nuevo para cada ensayo completo.
        for (std::size_t i = 0; i < caso.lineas.size(); ++i) {
            assetcheck::procesar_linea(caso.lineas[i], i + 1, obtenido);
        }
        ++comprobaciones;
        if (!mismo_lote(obtenido, caso.esperado)) {
            ++fallos;
            std::cerr << "Comprobacion " << comprobaciones << ": " << caso.nombre
                      << "\nEsperado:\n";
            mostrar_lote(caso.esperado);
            std::cerr << "Obtenido:\n";
            mostrar_lote(obtenido);
        }
    }
    std::cout << "Comprobaciones=" << comprobaciones << " Fallos=" << fallos << '\n';
    return fallos == 0 ? 0 : 1;
}
