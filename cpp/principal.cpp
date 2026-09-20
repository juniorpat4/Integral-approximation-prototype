// principal.cpp
//
// Recibe los datos ya extraidos (desde datos_integral.json, generado por
// integracion.py) y es el punto donde se implementan T(n), M(n) y S(n).

#include <iostream>
#include "DatosIntegral.hpp"
using namespace std;
double T(int n, const DatosIntegral& datos, FuncionEvaluable& f){
	double suma = f(datos.a)+f(datos.b);
	double h = (datos.b-datos.a)/n;
	double x=datos.a;
	for(int i=1; i<n; i++){
		x=x+h;
		suma=suma+2*f(x);
	}
	return (h/2)*suma;
}
double M(int n, const DatosIntegral& datos, FuncionEvaluable& f){
	double sum1=0;
	double h = (datos.b-datos.a)/n;
	double x1=datos.a;
	for(int i=0; i<n; i++){
		double mid_point = x1 + h/2;
		sum1 = sum1 + f(mid_point);
		x1 = x1 + h;
	}
	return h*sum1;
}
double S(int n, const DatosIntegral& datos, FuncionEvaluable& f){
	if(n%2!=0) {
		cout<<"Aviso: n ajustado de " <<n<<"a"<<(n+1)<<"para poder aplicar Simpson (n debe ser par).\n";
		n =n+1;
	}
	return (1.0/3.0)*(T(n/2,datos,f)+2*M(n/2,datos,f));
}
void principal_funcion(const DatosIntegral& datos) {
    if(!datos.es_definida){
        cout<<"Integral indefinida: no hay [a, b] para aproximar.\n";
        return;
    }

    FuncionEvaluable f(datos.expresion_cpp);

    cout<<"f(x)       = "<<datos.expresion_str<<"\n";
    cout<<"[a, b]     = ["<<datos.a<<", "<<datos.b<<"]\n";
    if(datos.has_n)cout<<"n          = "<<datos.n<<"\n";
    if(datos.has_h)cout<<"h          = "<<datos.h<<"\n";
    if(datos.has_max_abs_f2){
        cout<<"max|f''|   ~= "<<datos.max_abs_f2<<"\n";
    }else{
        cout<<"max|f''|   : no acotada en [a,b] (sin cota de error valida)\n";
    }
    if(datos.has_max_abs_f4){
        cout<<"max|f''''| ~= "<<datos.max_abs_f4<<"\n";
    }else{
        cout<<"max|f''''| : no acotada en [a,b] (sin cota de error valida)\n";
    }

    if(!datos.has_n){
        cout<<"No se especifico 'n' en los datos; no se puede calcular T(n), M(n), S(n).\n";
        return;
    }

    double t = T(datos.n, datos, f);
    double m = M(datos.n, datos, f);
    double s = S(datos.n, datos, f);

    cout<<"T(n) ~= "<<t<<"\n";
    cout<<"M(n) ~= "<<m<<"\n";
    cout<<"S(n) ~= "<<s<<"\n";
}

int main(int argc, char** argv) {
    string ruta = (argc > 1) ? argv[1] : "../datos_integral.json";

    try {
        DatosIntegral datos = cargar_datos_json(ruta);
        principal_funcion(datos);
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
