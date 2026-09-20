"""
Prototipo: extraccion de datos para aproximar integrales
mediante la Regla del Trapecio y la Regla de Simpson (Sección 4.3).

Este módulo se encarga UNICAMENTE de:
  1. Parsear la función ingresada por el usuario (integral definida o indefinida).
  2. Calcular el ancho de cada subintervalo h = (b - a) / n.
  3. Calcular simbólicamente f''(x) y f''''(x) (necesarias para las cotas de
     error del Trapecio y de Simpson respectivamente) y estimar numéricamente
     su valor máximo absoluto en [a, b].
  4. Empaquetar todo en un objeto DatosIntegral y entregarlo a principal_funcion,
     donde se implementarán T(n), M(n) y S(n).

Dependencias: sympy, numpy, scipy
    pip install sympy numpy scipy
"""

import json
import re
from dataclasses import dataclass
from typing import Callable, Optional, Tuple

import numpy as np
import sympy as sp
from scipy.optimize import minimize_scalar


# Nombres de funciones en español -> su equivalente reconocido por sympy.
# Se ordenan las claves más largas primero para que "arcsen" no quede
# parcialmente reemplazado por la regla de "sen".
_FUNCIONES_ES_A_EN = {
    "arcsen": "asin",
    "arccos": "acos",
    "arctg": "atan",
    "arcctg": "acot",
    "senh": "sinh",
    "cosh": "cosh",
    "tgh": "tanh",
    "sen": "sin",
    "tg": "tan",
    "ctg": "cot",
    "cotg": "cot",
    "raiz": "sqrt",
    "ln": "log",
}


def _traducir_funciones_es(expresion_str: str) -> str:
    """Reemplaza nombres de funciones trigonométricas/etc. en español por
    los nombres en inglés que sympy reconoce (p. ej. sen(x) -> sin(x))."""
    resultado = expresion_str
    for nombre_es in sorted(_FUNCIONES_ES_A_EN, key=len, reverse=True):
        patron = r"\b" + re.escape(nombre_es) + r"\s*\("
        resultado = re.sub(patron, _FUNCIONES_ES_A_EN[nombre_es] + "(", resultado)
    return resultado


@dataclass
class DatosIntegral:
    expresion_str: str
    expresion: sp.Expr
    variable: sp.Symbol
    es_definida: bool
    a: Optional[float] = None
    b: Optional[float] = None
    n: Optional[int] = None
    h: Optional[float] = None
    f: Optional[Callable[[float], float]] = None
    f2: Optional[sp.Expr] = None
    f4: Optional[sp.Expr] = None
    max_abs_f2: Optional[float] = None
    max_abs_f4: Optional[float] = None


def parsear_funcion(expresion_str: str, variable_str: str = "x") -> Tuple[sp.Expr, sp.Symbol]:
    """Convierte el texto ingresado por el usuario en una expresión simbólica de sympy."""
    variable = sp.symbols(variable_str)
    expresion_traducida = _traducir_funciones_es(expresion_str)
    expresion = sp.sympify(expresion_traducida, locals={variable_str: variable})
    return expresion, variable


def calcular_ancho_subintervalo(a: float, b: float, n: int) -> float:
    """h = (b - a) / n, el ancho de cada subintervalo de igual longitud."""
    if n <= 0:
        raise ValueError("El número de subintervalos 'n' debe ser positivo.")
    return (b - a) / n


def _maximizar_abs(func_num: Callable[[float], float], a: float, b: float,
                    muestras: int = 500) -> float:
    """
    Estima max_{x en [a,b]} |func_num(x)|.

    Estrategia (la derivada puede no ser convexa/cóncava, así que no basta con
    revisar los extremos): un barrido en malla fina localiza la región del
    máximo global y luego una minimización acotada (scipy) refina el valor
    exacto alrededor de ese candidato.
    """
    def _evaluar_seguro(x: float) -> float:
        # Algunas funciones (p. ej. sqrt(1-x**2)) tienen derivadas no acotadas
        # cerca de los extremos del intervalo (tangente vertical, división
        # entre cero, dominio inválido). En vez de dejar que la excepción
        # detenga el programa, se reporta como +inf: la cota de error
        # realmente no existe ahí, así que es la respuesta matemáticamente
        # honesta.
        try:
            with np.errstate(all="ignore"):
                valor = abs(func_num(float(x)))
        except (ZeroDivisionError, ValueError, OverflowError):
            return float("inf")
        if not np.isfinite(valor):
            return float("inf")
        return float(valor)

    xs = np.linspace(a, b, muestras)
    ys = np.array([_evaluar_seguro(x) for x in xs], dtype=float)

    idx_max = int(np.argmax(ys))
    x0 = xs[idx_max]

    if not np.isfinite(ys[idx_max]):
        return float("inf")

    paso = (b - a) / (muestras - 1) if muestras > 1 else (b - a) or 1e-6
    lo, hi = max(a, x0 - paso), min(b, x0 + paso)

    def neg_abs(x):
        valor = _evaluar_seguro(x)
        return -valor if np.isfinite(valor) else np.inf  # inf real ya se filtro arriba

    candidato = ys[idx_max]
    if hi > lo:
        resultado = minimize_scalar(neg_abs, bounds=(lo, hi), method="bounded")
        if resultado.success and np.isfinite(resultado.fun):
            candidato = max(candidato, -resultado.fun)

    return float(candidato)


def calcular_derivadas_y_maximos(expresion: sp.Expr, variable: sp.Symbol,
                                  a: float, b: float) -> dict:
    """
    f''  -> cota de error de la Regla del Trapecio.
    f'''' -> cota de error de la Regla de Simpson.
    """
    f2_sym = sp.diff(expresion, variable, 2)
    f4_sym = sp.diff(expresion, variable, 4)

    f2_num = sp.lambdify(variable, f2_sym, modules=["numpy"])
    f4_num = sp.lambdify(variable, f4_sym, modules=["numpy"])

    max_abs_f2 = _maximizar_abs(f2_num, a, b)
    max_abs_f4 = _maximizar_abs(f4_num, a, b)

    if not np.isfinite(max_abs_f2):
        print(f"Aviso: f''(x) = {f2_sym} no está acotada en [{a}, {b}] "
              "(probable singularidad/tangente vertical en un extremo). "
              "La cota de error del Trapecio no existe en este intervalo cerrado.")
    if not np.isfinite(max_abs_f4):
        print(f"Aviso: f''''(x) = {f4_sym} no está acotada en [{a}, {b}] "
              "(probable singularidad/tangente vertical en un extremo). "
              "La cota de error de Simpson no existe en este intervalo cerrado.")

    return {
        "f2": f2_sym,
        "f4": f4_sym,
        "max_abs_f2": max_abs_f2,
        "max_abs_f4": max_abs_f4,
    }


def extraer_datos_integral(expresion_str: str, es_definida: bool,
                            a: Optional[float] = None, b: Optional[float] = None,
                            n: Optional[int] = None, variable_str: str = "x") -> DatosIntegral:
    """
    Punto único de análisis: parsea la función, y si la integral es definida,
    calcula ancho de subintervalo y máximos de derivadas. Devuelve todo
    empaquetado en un DatosIntegral, listo para pasarse a principal_funcion.
    """
    expresion, variable = parsear_funcion(expresion_str, variable_str)
    f_num = sp.lambdify(variable, expresion, modules=["numpy"])

    datos = DatosIntegral(
        expresion_str=expresion_str,
        expresion=expresion,
        variable=variable,
        es_definida=es_definida,
        a=a,
        b=b,
        n=n,
        f=f_num,
    )

    if es_definida:
        if a is None or b is None:
            raise ValueError("Una integral definida requiere límites 'a' y 'b'.")

        if n:
            datos.h = calcular_ancho_subintervalo(a, b, n)

        derivadas = calcular_derivadas_y_maximos(expresion, variable, a, b)
        datos.f2 = derivadas["f2"]
        datos.f4 = derivadas["f4"]
        datos.max_abs_f2 = derivadas["max_abs_f2"]
        datos.max_abs_f4 = derivadas["max_abs_f4"]
    else:
        print("Nota: integral indefinida -> no se calculan h ni máximos de derivadas "
              "(se necesita un intervalo [a, b] para las aproximaciones numéricas).")

    return datos


def _expresion_a_texto_cpp(expresion: sp.Expr) -> str:
    """
    Convierte una expresión sympy a un texto evaluable por un parser de
    expresiones en C++ (p. ej. exprtk), que usa sintaxis de tipo C:
    pow(x, 2), sin(x), sqrt(x), abs(x), pi, etc.
    """
    texto = sp.ccode(expresion)
    texto = texto.replace("fabs(", "abs(")
    texto = texto.replace("M_PI", "pi")
    return texto


def exportar_datos_json(datos: DatosIntegral, ruta: str) -> None:
    """
    Serializa DatosIntegral a un archivo JSON que un programa en C++ pueda
    leer para continuar con T(n), M(n) y S(n).

    Se incluye 'expresion_cpp' (y las de las derivadas) para que C++ pueda
    evaluar f(x), f''(x) y f''''(x) con un parser de expresiones en tiempo
    de ejecución (p. ej. exprtk), sin tener que re-implementar sympy en C++.
    """
    payload = {
        "expresion_str": datos.expresion_str,
        "expresion_cpp": _expresion_a_texto_cpp(datos.expresion),
        "variable": str(datos.variable),
        "es_definida": datos.es_definida,
        "a": datos.a,
        "b": datos.b,
        "n": datos.n,
        "h": datos.h,
        "f2_cpp": _expresion_a_texto_cpp(datos.f2) if datos.f2 is not None else None,
        "f4_cpp": _expresion_a_texto_cpp(datos.f4) if datos.f4 is not None else None,
        # inf no es un numero JSON valido (nlohmann::json lo rechazaria al leerlo
        # en C++), asi que se exporta como null cuando la derivada no esta acotada.
        "max_abs_f2": datos.max_abs_f2 if datos.max_abs_f2 is not None and np.isfinite(datos.max_abs_f2) else None,
        "max_abs_f4": datos.max_abs_f4 if datos.max_abs_f4 is not None and np.isfinite(datos.max_abs_f4) else None,
    }
    with open(ruta, "w", encoding="utf-8") as archivo:
        json.dump(payload, archivo, indent=2, ensure_ascii=False)
    print(f"\nDatos exportados a: {ruta}")


def principal_funcion(datos: DatosIntegral) -> DatosIntegral:
    """
    Punto de entrada para el resto del programa (Trapecio, Simpson, T(n),
    M(n), S(n), gráficas, etc.). Recibe ya listos:

        datos.f            -> f(x) evaluable numéricamente
        datos.a, datos.b   -> límites de integración
        datos.n, datos.h   -> número de subintervalos y su ancho
        datos.f2, datos.f4 -> f''(x) y f''''(x) simbólicas
        datos.max_abs_f2   -> max|f''(x)|  en [a, b]  (cota de error Trapecio)
        datos.max_abs_f4   -> max|f''''(x)| en [a, b] (cota de error Simpson)

    La implementación de las reglas de aproximación queda pendiente.
    """
    # TODO: implementar aquí T(n), M(n) y S(n) usando los datos extraídos.
    return datos


def _entrada_interactiva() -> DatosIntegral:
    print("=== Extracción de datos para aproximación de integrales ===")
    expresion_str = input("Ingrese la función f(x): ").strip()
    respuesta = input("¿Es una integral definida? (s/n): ").strip().lower()
    es_definida = respuesta in ("s", "si", "sí", "y", "yes")

    a = b = n = None
    if es_definida:
        a = float(input("Límite inferior a: ").strip())
        b = float(input("Límite superior b: ").strip())
        n_str = input("Número de subintervalos n (Enter para omitir): ").strip()
        if n_str:
            n = int(n_str)

    datos = extraer_datos_integral(expresion_str, es_definida, a, b, n)

    print("\n--- Datos extraídos ---")
    print(f"f(x)        = {datos.expresion}")
    print(f"Definida    = {datos.es_definida}")
    if datos.es_definida:
        print(f"[a, b]      = [{datos.a}, {datos.b}]")
        if datos.n:
            print(f"n           = {datos.n}")
            print(f"h           = {datos.h}")
        print(f"f''(x)      = {datos.f2}")
        print(f"f''''(x)    = {datos.f4}")
        print(f"max|f''|    ≈ {datos.max_abs_f2}")
        print(f"max|f''''|  ≈ {datos.max_abs_f4}")

    exportar_datos_json(datos, "datos_integral.json")
    return principal_funcion(datos)


if __name__ == "__main__":
    _entrada_interactiva()
