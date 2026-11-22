#include "assetcheck/registro.hpp"

#include <iostream>
#include <string>
#include <vector>

using assetcheck::Registro;
using assetcheck::Resultado;
using assetcheck::leer_registro;

bool iguales(const Registro& a, const Registro& b)
{
    return a.id == b.id && a.tipo == b.tipo &&
           a.tamano_bytes == b.tamano_bytes;
}

struct Caso {
    std::string nombre;
    std::string linea;
    Resultado esperado;
    Registro registro_esperado;
};

int main()
{
    const Registro anterior{"Anterior", "sonido", 87};
    const std::vector<Caso> pruebas{
        {"normal", "Roca malla 240", Resultado::aceptado, {"Roca", "malla", 240}},
        {"tabuladores", "\tBrisa\tsonido\t00009\t", Resultado::aceptado, {"Brisa", "sonido", 9}},
        {"minimos", "A textura 1", Resultado::aceptado, {"A", "textura", 1}},
        {"maximos", "Abc123456789 malla 30000", Resultado::aceptado, {"Abc123456789", "malla", 30000}},
        {"minusculas", "roca malla 240", Resultado::aceptado, {"roca", "malla", 240}},
        {"80 bytes", "Roca malla 240" + std::string(66, ' '), Resultado::aceptado, {"Roca", "malla", 240}},
        {"81 bytes", "Roca malla 240" + std::string(67, ' '), Resultado::formato, anterior},
        {"cuarto campo", "Roca malla 240 extra", Resultado::formato, anterior},
        {"falta tamano", "Roca malla", Resultado::formato, anterior},
        {"vacia", "", Resultado::formato, anterior},
        {"solo separadores", " \t ", Resultado::formato, anterior},
        {"NUL interno", std::string("Roca\0 malla 240", 15), Resultado::formato, anterior},
        {"LF interno", "Roca\n malla 240", Resultado::formato, anterior},
        {"CR interno", "Roca\r malla 240", Resultado::formato, anterior},
        {"DEL", std::string(1, static_cast<char>(127)) + " malla 4", Resultado::formato, anterior},
        {"UTF8", "\xC3\x91 malla 4", Resultado::formato, anterior},
        // Dos o mas defectos en una linea: devolver la primera causa
        // segun el orden FORMATO, ID, TIPO, TAMANO, sin tocar destino.
        {"precedencia formato con campo extra", "7Roca imagen 0 extra", Resultado::formato, anterior},
        {"precedencia formato con campo ausente", "7Roca imagen", Resultado::formato, anterior},
        {"precedencia ID sobre tipo y tamano", "_Roca imagen 0", Resultado::id, anterior},
        {"precedencia tipo sobre tamano", "Roca imagen 0", Resultado::tipo, anterior},
        {"CR aislado final", "Roca malla 2\r", Resultado::formato, anterior},
        {"ID antes de tipo", "_A material 0", Resultado::id, anterior},
        {"ID empieza con cifra", "7Roca malla 2", Resultado::id, anterior},
        {"ID de 13", "Abc1234567890 malla 2", Resultado::id, anterior},
        {"guion en ID", "Ro-ca malla 2", Resultado::id, anterior},
        {"tipo antes de tamano", "Roca Malla 0", Resultado::tipo, anterior},
        {"cero", "Roca malla 0", Resultado::tamano, anterior},
        {"signo", "Roca malla +12", Resultado::tamano, anterior},
        {"sufijo", "Roca malla 12x", Resultado::tamano, anterior},
        {"seis digitos", "Roca malla 000001", Resultado::tamano, anterior},
        {"sobre limite", "Roca malla 30001", Resultado::tamano, anterior},
        {"cinco nueves", "Roca malla 99999", Resultado::tamano, anterior}
    };

    int casos = 0;
    int fallos = 0;
    for (const Caso& caso : pruebas) {
        ++casos;
        Registro destino = anterior;
        const Resultado obtenido = leer_registro(caso.linea, destino);
        if (obtenido != caso.esperado || !iguales(destino, caso.registro_esperado)) {
            ++fallos;
            std::cerr << "Fallo: " << caso.nombre << '\n';
        }
    }

    // Estas tres llamadas comparten destino y forman un solo caso de continuacion.
    ++casos;
    Registro destino = anterior;
    const Resultado r1 = leer_registro("Puerta malla 8", destino);
    const bool primero = r1 == Resultado::aceptado &&
                         iguales(destino, Registro{"Puerta", "malla", 8});
    const Resultado r2 = leer_registro("Puerta sonido +9", destino);
    const bool segundo = r2 == Resultado::tamano &&
                         iguales(destino, Registro{"Puerta", "malla", 8});
    const Resultado r3 = leer_registro("Puerta sonido 9", destino);
    const bool tercero = r3 == Resultado::aceptado &&
                         iguales(destino, Registro{"Puerta", "sonido", 9});
    if (!primero || !segundo || !tercero) {
        ++fallos;
        std::cerr << "Fallo: continuacion\n";
    }

    std::cout << "Casos=" << casos << " Fallos=" << fallos << '\n';
    return fallos == 0 ? 0 : 1;
}
