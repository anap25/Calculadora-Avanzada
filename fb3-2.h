/*
 * Declaraciones para una calculadora con funciones
 */
#include "fb3-2.tab.h"
/* interfaz para el lexer */
extern int yylineno;
void yyerror(char *s, ...);

/* tabla de simbolos */
struct symbol{
    char *name;
    struct ast *value; /*para almacenar los valores que le asigno en variables*/
    struct ast *func;
    struct symlist *syms;
};

/* tabla de simbolos simple de tamaño fijo */
#define NHASH 9997
struct symbol symtab[NHASH];

struct symbol *lookup(char*); /*Lookup, lo que hace es buscar una direccion de memoria en ram(Tabla de simbolos) para poder almacenar el parametro que se le pasa por parametro*/

/* lista de simbolos, para una lista de argumentos */
struct symlist{
    struct symbol *sym;
    struct symlist *next;
};

struct symlist *newsymlist(struct symbol *sym, struct symlist *next);
void symlistfree(struct symlist *sl);

/* tipos de nodos
 * + - * / |
 * 0-7 ops de comparacion, codificados en bits 04 igual, 02 menor, 01 mayor
 * M menos unario
 * L expresion o lista de sentencias
 * I sentencia IF
 * W sentencia WHILE
 * N referencia a un simbolo
 * = asignacion
 * S lista de simbolos
 * F llamada a una funcion incorporada
 * C llamada a una funcion de usuario
 */

enum bifs{              /* funciones incorporadas */
    B_sqrt = 1,
    B_exp,
    B_log,
    B_print
};

/* nodos en el arbol de sintaxis abstracta */
/* todos tienen un nodetype inicial en comun */

struct ast{
    int nodetype;
    struct ast *l;
    struct ast *r;
};

struct fncall{
    int nodetype;
    struct ast *l;
    enum bifs functype;
};

struct ufncall{         /* funciones de usuario */
    int nodetype;
    struct ast *l;
    struct symbol *s;
};

struct flow{
    int nodetype;       /* tipo I o W */
    struct ast *cond;   /* condicion */
    struct ast *tl;     /* rama "then" o lista "do" */
    struct ast *el;     /* rama opcional "else" */
};

struct numval{
    int nodetype;       /* tipo K */
    double number;
};

struct symref{
    int nodetype;       /* tipo N */
    struct symbol *s;
};

struct symasgn{
    int nodetype;       /* tipo = */
    struct symbol *s;
    struct ast *v;      /* valor */
};

/*Mi codigo*/

struct ent{
   int nodetype; //estructura de un token ENTERO
   int valor; 
};

struct rac{
    int nodetype; //estructura de un token RACIONAL
    struct ast *numerador;
    struct ast *denominador;
};

/* construccion del AST */
struct ast *newast(int nodetype, struct ast *l, struct ast *r);
struct ast *newcmp(int cmptype, struct ast *l, struct ast *r);
struct ast *newfunc(int functype, struct ast *l);
struct ast *newcall(struct symbol *s, struct ast *l);
struct ast *newref(struct symbol *s);
struct ast *newasgn(struct symbol *s, struct ast *v);
struct ast *newnum(double d);
struct ast *newflow(int nodetype, struct ast *cond, struct ast *tl, struct ast *tr);


/* evaluar un AST */
struct ast *eval(struct ast *a);

/*Estructuras definidas para numero racional*/
struct ast *newEntero(int valor);
struct ast *newRacional(struct ast *numerador, struct ast *denominador);

/*OPERACIONES*/

//Para simplificar las fracciones.
struct ast *mcd(struct ast *a, struct ast *b);
struct ast *simplificar(struct ast *numRacional);

struct ast *suma(struct ast *num1, struct ast *num2);
struct ast *sumaRacional(struct ast *num1, struct ast *num2);

struct ast *resta(struct ast *num1, struct ast *num2);
struct ast *restaRacional(struct ast *num1, struct ast *num2);

struct ast *multiplicacion(struct ast *num1, struct ast *num2);
struct ast *multRacional(struct ast *num1, struct ast *num2);

struct ast *division(struct ast *num1, struct ast *num2);
struct ast *divRacional(struct ast *num1, struct ast *num2);

struct ast *setSigno(struct ast *a);

/*COMPARACIONES*/
struct ast *comparaciones(struct ast *a);
double getNumero(struct ast *numero);


/*Funciones para mostrar*/
char *concatenar(const char *s1, const char *s2); 
char *numToString(struct ast *a);
char *Mostrar(struct ast *a);







/* definir una funcion */
/* void dodef(struct symbol *name, struct symlist *syms, struct ast *stmts); */


//FUNCIONES DEFINIDAS

/* evaluar un AST */
//double eval(struct ast *);
/* borrar y liberar un AST */
/* void treefree(struct ast *); */

