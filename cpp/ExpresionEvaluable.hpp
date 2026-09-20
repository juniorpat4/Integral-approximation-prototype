// ExpresionEvaluable.hpp
//
// Evaluador de expresiones matematicas en runtime, escrito a mano
// (reemplaza a exprtk.hpp, que es demasiado pesado para compilar con
// TDM-GCC 4.9.2). Soporta el subconjunto de sintaxis que genera
// sympy.ccode(): +, -, *, /, ^, pow(a,b), sin, cos, tan, asin, acos,
// atan, sinh, cosh, tanh, exp, log, log10, sqrt, abs, y la constante pi.
//
// Tambien maneja indeterminaciones (0/0, etc.) en singularidades removibles
// (p. ej. sin(x)/x en x=0) aproximando el limite numericamente.
//
// Uso:
//   FuncionEvaluable f("pow(x, 2) + sin(x)");
//   double y = f(1.5);

#pragma once

#include <cctype>
#include <cmath>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

class ExpresionEvaluable {
public:
    explicit ExpresionEvaluable(const std::string& texto) : texto_(texto), pos_(0) {
        raiz_ = parsear_expresion();
        saltar_espacios();
        if (pos_ != texto_.size()) {
            throw std::runtime_error("Caracter inesperado en la expresion cerca de: " +
                                      texto_.substr(pos_));
        }
    }

    // Evalua f(x). Si el resultado es una indeterminacion (0/0, etc. -> NaN
    // en aritmetica de punto flotante, p. ej. sin(x)/x en x=0), se asume que
    // es una singularidad REMOVIBLE (el limite existe aunque la formula no
    // este definida justo ahi) y se aproxima evaluando muy cerca del punto
    // por ambos lados y promediando -- sin necesitar calculo simbolico.
    double evaluar(double x) const {
        double y = raiz_(x);
        if (!std::isnan(y)) return y;

        const double eps = 1e-6;
        double y_izq = raiz_(x - eps);
        double y_der = raiz_(x + eps);
        bool izq_ok = !std::isnan(y_izq) && !std::isinf(y_izq);
        bool der_ok = !std::isnan(y_der) && !std::isinf(y_der);

        if (izq_ok && der_ok) return 0.5 * (y_izq + y_der);
        if (der_ok) return y_der;
        if (izq_ok) return y_izq;

        throw std::runtime_error("f(x) es indeterminada en x = " + std::to_string(x) +
                                  " y no se pudo aproximar su limite.");
    }

    double operator()(double x) const { return evaluar(x); }

private:
    using Nodo = std::function<double(double)>;

    std::string texto_;
    size_t pos_;
    Nodo raiz_;

    void saltar_espacios() {
        while (pos_ < texto_.size() && std::isspace(static_cast<unsigned char>(texto_[pos_]))) {
            ++pos_;
        }
    }

    char mirar() {
        saltar_espacios();
        return pos_ < texto_.size() ? texto_[pos_] : '\0';
    }

    bool consumir(char esperado) {
        if (mirar() == esperado) {
            ++pos_;
            return true;
        }
        return false;
    }

    // expr := termino (('+' | '-') termino)*
    Nodo parsear_expresion() {
        Nodo izquierda = parsear_termino();
        for (;;) {
            if (consumir('+')) {
                Nodo derecha = parsear_termino();
                izquierda = [izquierda, derecha](double x) { return izquierda(x) + derecha(x); };
            } else if (consumir('-')) {
                Nodo derecha = parsear_termino();
                izquierda = [izquierda, derecha](double x) { return izquierda(x) - derecha(x); };
            } else {
                break;
            }
        }
        return izquierda;
    }

    // termino := factor (('*' | '/') factor)*
    Nodo parsear_termino() {
        Nodo izquierda = parsear_factor();
        for (;;) {
            if (consumir('*')) {
                Nodo derecha = parsear_factor();
                izquierda = [izquierda, derecha](double x) { return izquierda(x) * derecha(x); };
            } else if (consumir('/')) {
                Nodo derecha = parsear_factor();
                izquierda = [izquierda, derecha](double x) { return izquierda(x) / derecha(x); };
            } else {
                break;
            }
        }
        return izquierda;
    }

    // factor := ('-' | '+')? potencia
    Nodo parsear_factor() {
        if (consumir('-')) {
            Nodo interior = parsear_factor();
            return [interior](double x) { return -interior(x); };
        }
        consumir('+');
        return parsear_potencia();
    }

    // potencia := primario ('^' factor)?   (asociativo a la derecha)
    Nodo parsear_potencia() {
        Nodo base = parsear_primario();
        if (consumir('^')) {
            Nodo exponente = parsear_factor();
            return [base, exponente](double x) { return std::pow(base(x), exponente(x)); };
        }
        return base;
    }

    // primario := numero | 'x' | 'pi' | identificador '(' expr (',' expr)* ')' | '(' expr ')'
    Nodo parsear_primario() {
        saltar_espacios();
        if (pos_ >= texto_.size()) {
            throw std::runtime_error("Fin de expresion inesperado.");
        }

        char c = texto_[pos_];

        if (c == '(') {
            ++pos_;
            Nodo interior = parsear_expresion();
            if (!consumir(')')) throw std::runtime_error("Falta ')' en la expresion.");
            return interior;
        }

        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            return parsear_numero();
        }

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            return parsear_identificador();
        }

        throw std::runtime_error(std::string("Caracter inesperado: '") + c + "'");
    }

    Nodo parsear_numero() {
        size_t inicio = pos_;
        while (pos_ < texto_.size() &&
               (std::isdigit(static_cast<unsigned char>(texto_[pos_])) || texto_[pos_] == '.')) {
            ++pos_;
        }
        // notacion cientifica: 1e-05, 2.5E+3
        if (pos_ < texto_.size() && (texto_[pos_] == 'e' || texto_[pos_] == 'E')) {
            size_t marca = pos_;
            ++pos_;
            if (pos_ < texto_.size() && (texto_[pos_] == '+' || texto_[pos_] == '-')) ++pos_;
            if (pos_ < texto_.size() && std::isdigit(static_cast<unsigned char>(texto_[pos_]))) {
                while (pos_ < texto_.size() && std::isdigit(static_cast<unsigned char>(texto_[pos_]))) {
                    ++pos_;
                }
            } else {
                pos_ = marca;  // no era notacion cientifica valida
            }
        }
        double valor = std::stod(texto_.substr(inicio, pos_ - inicio));
        return [valor](double) { return valor; };
    }

    Nodo parsear_identificador() {
        size_t inicio = pos_;
        while (pos_ < texto_.size() &&
               (std::isalnum(static_cast<unsigned char>(texto_[pos_])) || texto_[pos_] == '_')) {
            ++pos_;
        }
        std::string nombre = texto_.substr(inicio, pos_ - inicio);

        if (mirar() == '(') {
            ++pos_;  // consumir '('
            std::vector<Nodo> args;
            args.push_back(parsear_expresion());
            while (consumir(',')) {
                args.push_back(parsear_expresion());
            }
            if (!consumir(')')) throw std::runtime_error("Falta ')' tras los argumentos de " + nombre);
            return construir_funcion(nombre, args);
        }

        if (nombre == "x" || nombre == "X") {
            return [](double x) { return x; };
        }
        if (nombre == "pi") {
            return [](double) { return M_PI; };
        }
        if (nombre == "e") {
            return [](double) { return M_E; };
        }

        throw std::runtime_error("Identificador desconocido: " + nombre);
    }

    Nodo construir_funcion(const std::string& nombre, const std::vector<Nodo>& args) {
        auto necesita = [&](size_t n) {
            if (args.size() != n) {
                throw std::runtime_error(nombre + " esperaba " + std::to_string(n) +
                                          " argumento(s), recibio " + std::to_string(args.size()));
            }
        };

        if (nombre == "pow") { necesita(2); Nodo a = args[0], b = args[1];
            return [a, b](double x) { return std::pow(a(x), b(x)); }; }
        if (nombre == "sin") { necesita(1); Nodo a = args[0]; return [a](double x) { return std::sin(a(x)); }; }
        if (nombre == "cos") { necesita(1); Nodo a = args[0]; return [a](double x) { return std::cos(a(x)); }; }
        if (nombre == "tan") { necesita(1); Nodo a = args[0]; return [a](double x) { return std::tan(a(x)); }; }
        if (nombre == "asin") { necesita(1); Nodo a = args[0]; return [a](double x) { return std::asin(a(x)); }; }
        if (nombre == "acos") { necesita(1); Nodo a = args[0]; return [a](double x) { return std::acos(a(x)); }; }
        if (nombre == "atan") { necesita(1); Nodo a = args[0]; return [a](double x) { return std::atan(a(x)); }; }
        if (nombre == "sinh") { necesita(1); Nodo a = args[0]; return [a](double x) { return std::sinh(a(x)); }; }
        if (nombre == "cosh") { necesita(1); Nodo a = args[0]; return [a](double x) { return std::cosh(a(x)); }; }
        if (nombre == "tanh") { necesita(1); Nodo a = args[0]; return [a](double x) { return std::tanh(a(x)); }; }
        if (nombre == "exp") { necesita(1); Nodo a = args[0]; return [a](double x) { return std::exp(a(x)); }; }
        if (nombre == "log" || nombre == "ln") { necesita(1); Nodo a = args[0]; return [a](double x) { return std::log(a(x)); }; }
        if (nombre == "log10") { necesita(1); Nodo a = args[0]; return [a](double x) { return std::log10(a(x)); }; }
        if (nombre == "sqrt") { necesita(1); Nodo a = args[0]; return [a](double x) { return std::sqrt(a(x)); }; }
        if (nombre == "abs" || nombre == "fabs") { necesita(1); Nodo a = args[0]; return [a](double x) { return std::fabs(a(x)); }; }
        if (nombre == "cbrt") { necesita(1); Nodo a = args[0]; return [a](double x) { return std::cbrt(a(x)); }; }

        throw std::runtime_error("Funcion no soportada: " + nombre);
    }
};

// Alias con el nombre que ya usa DatosIntegral.hpp / principal.cpp.
using FuncionEvaluable = ExpresionEvaluable;
