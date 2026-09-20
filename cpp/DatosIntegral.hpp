// DatosIntegral.hpp
//
// Espejo en C++ del DatosIntegral de Python (integracion.py).
// Carga el JSON exportado por exportar_datos_json(...) y deja todo listo
// para que principal_funcion() implemente T(n), M(n) y S(n).
//
// Dependencias (ver README_CPP.md para descargarlos):
//   - json.hpp               (nlohmann/json, parseo de datos_integral.json)
//   - ExpresionEvaluable.hpp (evaluador de expresiones propio, sin dependencias externas)
// Ambos deben colocarse en esta misma carpeta (cpp/).

#pragma once

#include <fstream>
#include <stdexcept>
#include <string>

#include "json.hpp"
#include "ExpresionEvaluable.hpp"

// Nota: sin std::optional (no disponible en GCC < 7 / TDM-GCC 4.9.2).
// En su lugar, cada valor "opcional" trae su propio flag has_*.
struct DatosIntegral {
    std::string expresion_str;   // texto original ingresado por el usuario
    std::string expresion_cpp;   // misma función, en sintaxis evaluable (pow, sin, sqrt, ...)
    std::string variable;        // nombre de la variable, normalmente "x"
    bool es_definida = false;

    double a = 0.0;
    double b = 0.0;
    int n = 0;
    double h = 0.0;
    bool has_n = false;
    bool has_h = false;

    std::string f2_cpp;   // f''(x)   -> cota de error del Trapecio
    std::string f4_cpp;   // f''''(x) -> cota de error de Simpson
    double max_abs_f2 = 0.0;
    double max_abs_f4 = 0.0;
    // false si Python no pudo acotar la derivada en [a,b] (p. ej. tangente
    // vertical en un extremo, como sqrt(1-x^2) en x=+-1): en ese caso NO
    // existe cota de error valida y max_abs_f2/f4 no deben usarse.
    bool has_max_abs_f2 = false;
    bool has_max_abs_f4 = false;
};

// FuncionEvaluable = ExpresionEvaluable (ver alias al final de
// ExpresionEvaluable.hpp): envuelve un texto de función
// (p. ej. "pow(x, 2) + sin(x)") como algo evaluable en cualquier x.

inline DatosIntegral cargar_datos_json(const std::string& ruta) {
    std::ifstream archivo(ruta);
    if (!archivo.is_open()) {
        throw std::runtime_error("No se pudo abrir: " + ruta);
    }

    nlohmann::json j;
    archivo >> j;

    DatosIntegral datos;
    datos.expresion_str = j.at("expresion_str").get<std::string>();
    datos.expresion_cpp = j.at("expresion_cpp").get<std::string>();
    datos.variable = j.at("variable").get<std::string>();
    datos.es_definida = j.at("es_definida").get<bool>();

    if (!j.at("a").is_null()) datos.a = j.at("a").get<double>();
    if (!j.at("b").is_null()) datos.b = j.at("b").get<double>();
    if (!j.at("n").is_null()) { datos.n = j.at("n").get<int>(); datos.has_n = true; }
    if (!j.at("h").is_null()) { datos.h = j.at("h").get<double>(); datos.has_h = true; }

    if (!j.at("f2_cpp").is_null()) datos.f2_cpp = j.at("f2_cpp").get<std::string>();
    if (!j.at("f4_cpp").is_null()) datos.f4_cpp = j.at("f4_cpp").get<std::string>();
    if (!j.at("max_abs_f2").is_null()) {
        datos.max_abs_f2 = j.at("max_abs_f2").get<double>();
        datos.has_max_abs_f2 = true;
    }
    if (!j.at("max_abs_f4").is_null()) {
        datos.max_abs_f4 = j.at("max_abs_f4").get<double>();
        datos.has_max_abs_f4 = true;
    }

    return datos;
}
