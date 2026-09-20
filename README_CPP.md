# Puente Python -> C++

## Flujo

1. Corres `python integracion.py`, ingresas la función y los límites.
2. Se genera `datos_integral.json` en la raíz del proyecto.
3. El ejecutable de C++ (`cpp/principal.cpp`) lee ese JSON y desde ahí
   implementas T(n), M(n) y S(n).

## Paso 1: instalar un compilador de C++

En esta máquina no hay ninguno todavía (`cl`, `g++` y `clang++` no se
encontraron). La opción más simple en Windows es **MinGW-w64** vía MSYS2:

1. Instala MSYS2: https://www.msys2.org/ (descarga el instalador y ejecútalo).
2. Abre "MSYS2 MinGW64" desde el menú inicio y corre:
   ```bash
   pacman -S mingw-w64-x86_64-gcc
   ```
3. Agrega `C:\msys64\mingw64\bin` a tu variable de entorno `PATH` (o abre
   siempre la terminal "MSYS2 MinGW64").
4. Verifica en PowerShell:
   ```powershell
   g++ --version
   ```

Alternativa: instalar "Visual Studio Build Tools" con el workload "Desktop
development with C++" y usar `cl.exe` desde la "Developer PowerShell for VS".

## Paso 2: descargar la única dependencia externa (header único)

`ExpresionEvaluable.hpp` (el evaluador de f(x)) ya está escrito a mano en
este proyecto, sin dependencias. Solo falta `json.hpp`:

- `json.hpp` — desde https://github.com/nlohmann/json/releases
  (el archivo `single_include/nlohmann/json.hpp` de la última release,
  renómbralo a `json.hpp`) — colócalo dentro de la carpeta `cpp/`.

Al final, `cpp/` debe verse así:
```
cpp/
  DatosIntegral.hpp
  ExpresionEvaluable.hpp
  principal.cpp
  json.hpp        <- descargado
```

## Paso 3: compilar

Desde la carpeta `cpp/`, con GCC moderno (7+):
```bash
g++ -std=c++17 -O2 principal.cpp -o principal.exe
```

Con TDM-GCC 4.9.2 (el que trae Dev-C++ clásico), usa en su lugar:
```bash
g++ -std=gnu++11 -O2 principal.cpp -o principal.exe
```
(o configura ese mismo flag en Dev-C++: Herramientas → Opciones del
compilador → "Añadir los siguientes comandos al llamar al compilador").

Con Visual Studio (`cl.exe`) sería:
```powershell
cl /std:c++17 /EHsc /O2 principal.cpp /Fe:principal.exe
```

## Paso 4: ejecutar

```bash
./principal.exe ../datos_integral.json
```

(el JSON lo genera `python integracion.py`, se guarda en la raíz del
proyecto, un nivel arriba de `cpp/`).

## Por qué así

- Los números (`a`, `b`, `n`, `h`, `max_abs_f2`, `max_abs_f4`) se serializan
  directo a JSON: no hay ambigüedad.
- La función `f(x)` no es un número, así que además del texto original
  (`expresion_str`) se exporta una versión en sintaxis tipo C
  (`expresion_cpp`, generada con `sympy.ccode`) que `ExpresionEvaluable`
  compila y evalúa en cualquier punto — así el programa en C++ sigue
  siendo genérico (cualquier función que el usuario escriba), no uno
  hardcodeado para un solo ejercicio.
- `f''(x)` y `f''''(x)` también se exportan como texto (`f2_cpp`, `f4_cpp`)
  por si en C++ necesitas evaluarlas en puntos específicos además del
  máximo (`max_abs_f2`, `max_abs_f4`) ya calculado en Python.

## Si prefieres evitar el parser de expresiones por completo

Alternativa más simple pero menos general: exporta solo los números
(sin `expresion_cpp`) y escribe `f(x)` a mano como una función C++ para
cada ejercicio puntual, ej.:

```cpp
double f(double x) { return std::sin(x) * x * x; }
```

Pierdes la posibilidad de que el usuario ingrese cualquier función desde
C++, pero te ahorras incluso `ExpresionEvaluable.hpp`.
