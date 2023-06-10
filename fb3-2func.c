/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* funciones de ayuda para la calculadora */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "fb3-2.h"

/* tabla de simbolos */
/* aplicar hash a un simbolo */
static unsigned symhash(char *sym){
    unsigned int hash = 0;
    unsigned c;
    
    while(c = *sym++) hash = hash*9 ^ c;
    
    return hash;
}

struct symbol *lookup(char *sym){
    struct symbol *sp = &symtab[symhash(sym)%NHASH];
    int scount = NHASH;
    
    while(--scount >= 0){
        if(sp->name && !strcmp(sp->name, sym)) 
            return sp;
        
        if(!sp->name){
            sp->name = strdup(sym);
            sp->value = 0;
            sp->func = NULL;
            sp->syms = NULL;
            return sp;
        }
        
        if(++sp >= symtab+NHASH)    /* prueba con la siguiente entrada */
            sp = symtab;
    }
    yyerror("la tabla de símbolos está agotada\n");
    abort();
}

/* creacion de nodos del arbol de sintaxis abstracta */

struct ast *newast(int nodetype, struct ast *l, struct ast *r){
    struct ast *a = malloc(sizeof(struct ast));
    
    if(!a){
        yyerror("no hay espacio");
        exit(0);
    }
    
    a->nodetype = nodetype;
    a->l = l;
    a->r = r;
    return a;
}

struct ast *newnum(double d){
    struct numval *a = malloc(sizeof(struct numval));
    
    if(!a){
        yyerror("no hay espacio");
        exit(0);
    }
    
    a->nodetype = 'K';
    a->number = d;
    return(struct ast *)a;
}

struct ast *newcmp(int cmptype, struct ast *l, struct ast *r){
    struct ast *a = malloc(sizeof(struct ast));
    
    if(!a){
        yyerror("no hay espacio");
        exit(0);
    }
    
    a->nodetype = '0' + cmptype;
    a->l = l;
    a->r = r;
    return a;
}

struct ast *newfunc(int functype, struct ast *l){
    struct fncall *a = malloc(sizeof(struct fncall));
    
    if(!a){
        yyerror("no hay espacio");
        exit(0);
    }
    
    a->nodetype = 'F';
    a->l = l;
    a->functype = functype;
    return (struct ast *)a;
}

struct ast *newcall(struct symbol *s, struct ast *l){
    struct ufncall *a = malloc(sizeof(struct ufncall));
    
    if(!a){
        yyerror("no hay espacio");
        exit(0);
    }
    
    a->nodetype = 'C';
    a->l = l;
    a->s = s;
    return (struct ast *)a;
}

struct ast *newref(struct symbol *s){
    struct symref *a = malloc(sizeof(struct symref));
    
    if(!a){
        yyerror("no hay espacio");
        exit(0);
    }
    
    a->nodetype = 'N';
    a->s = s;
    return (struct ast *)a;
}

struct ast *newasgn(struct symbol *s, struct ast *v){
    struct symasgn *a = malloc(sizeof(struct symasgn));
    
    if(!a){
        yyerror("no hay espacio");
        exit(0);
    }
    
    a->nodetype = '=';
    a->s = s;
    a->v = v;
    return (struct ast *)a;
}

struct ast *newflow(int nodetype, struct ast *cond, struct ast *tl, struct ast *el){
    struct flow *a = malloc(sizeof(struct flow));
    
    if(!a){
        yyerror("no hay espacio");
        exit(0);
    }
    
    a->nodetype = nodetype;
    a->cond = cond;
    a->tl = tl;
    a->el = el;
    return (struct ast *)a;
}

/* liberar un arbol AST */
/* void treefree(struct ast *a){
    switch(a->nodetype){
        case '+':
        case '-':
        case '*':
        case '/':
        case '1': case '2': case '3': case '4': case '5': case '6':
        case 'L':
            treefree(a->r);
            
        case '|':
        case 'M': case 'C': case 'F':
            treefree(a->l);
            
        case 'K': case 'N':
            break;
            
        case '=':
            free(((struct symasgn*)a)->v);
            break;
            
        case 'I': case 'W':
            free(((struct flow *)a)->cond);
            if(((struct flow *)a)->tl)
                treefree(((struct flow *)a)->tl);
            if(((struct flow *)a)->el)
                treefree(((struct flow *)a)->el);
            break;
        default: 
            printf("error interno: libera un nodo incorrecto %c\n", a->nodetype);
    }
    free(a); */    /* siempre liberar el nodo actual */
//}

struct symlist *newsymlist(struct symbol *sym, struct symlist *next){
    struct symlist *sl = malloc(sizeof(struct symlist));
    
    if(!sl){
        yyerror("no hay espacio");
        exit(0);
    }
    
    sl->sym = sym;
    sl->next = next;
    return sl;
}

/* liberar una lista de simbolos */
void symlistfree(struct symlist *sl){
    struct symlist *nsl;
    
    while(sl){
        nsl = sl->next;
        free(sl);
        sl = nsl;
    }
}

static double callbuiltin(struct fncall *);
static double calluser(struct ufncall *);


/* static double callbuiltin(struct fncall *f){
    enum bifs functype = f->functype;
    double v = eval(f->l);
    
    switch(functype){
        case B_sqrt:
            return sqrt(v);
        case B_exp:
            return exp(v);
        case B_log:
            return log(v);
        case B_print:
            printf("= %4.4g\n", v);
            return v;
        default:
            yyerror("funcion incorporada desconocida %d", functype);
            return 0.0;
    }
} */

/* definir una funcion */
/* void dodef(struct symbol *name, struct symlist *syms, struct ast *func){
    if(name->syms) symlistfree(name->syms);
    if(name->func) treefree(name->func);
    name->syms = syms;
    name->func = func;
} */

/*Es para llamar funciones, trabaja con el eval viejo*/
static double calluser(struct ufncall *f){
    struct symbol *fn = f->s;   /* nombre de la funcion */
    struct symlist *sl;         /* argumentos formales */
    struct ast *args = f->l;    /* argumentos actuales */
    double *oldval, *newval;    /* valores de argumentos salvados */
    double v;
    int nargs;
    int i;
    
    if(!fn->func){
        yyerror("llamada a una funcion indefinida: %s", fn->name);
        return 0;
    }
    
    /* cuenta los argumentos */
    sl = fn->syms;
    for(nargs = 0; sl; sl = sl->next)
        nargs++;
    
    /* preparacion para salvarlos */
    oldval = (double *)malloc(nargs * sizeof(double));
    newval = (double *)malloc(nargs * sizeof(double));
    if(!oldval || !newval){
        yyerror("No hay lugar en %s", fn->name); 
        return 0.0;
    }
    
    /* evalua los argumentos */
    for(i=0; i<nargs; i++){
        if(!args){
            yyerror("faltan argumentos en la llamada a %s", fn->name);
            free(oldval);
            free(newval);
            return 0.0;
        }
        
        if(args->nodetype=='L'){    /* si es un nodo lista */
           /*  newval[i] = eval(args->l);
            args = args->r; */
        }else{                      /* si es el final de la lista */
            /* newval[i] = eval(args);
            args = NULL; */
        }
    }
    
    /* salva los viejos valores de los argumentos formales, 
     * asigna los nuevos nodos
     */
   /*  sl = fn->syms;
    for(i=0; i<nargs; i++){
        struct symbol* s = sl->sym;
        
        oldval[i] = s->value;
        s->value = newval[i];
        sl = sl->next;
    }
    
    free(newval); */
    
    /* evalua la funcion */
    /* v = eval(fn->func); */
    
    /* pone los valores reales de los args formales de nuevo */
   /*  sl = fn->syms;
    for(i=0; i<nargs; i++){
        struct symbol *s = sl->sym;
        
        s->value = oldval[i];
        sl = sl->next;
    }
    
    free(oldval);
    return v; */
}

/*Crear las estructuras */
struct ast *newEntero(int valor){
     struct ent *num_entero = malloc(sizeof(struct ent));
    
    if(!num_entero){
        yyerror("No hay espacio para una estructura de entero");
        exit(0);
    } 
    
    num_entero->nodetype = ENTERO;
    num_entero->valor = valor;
    return (struct ast *)num_entero;
    
}
struct ast *newRacional(struct ast *numerador, struct ast *denominador){
    struct rac *racional = malloc(sizeof(struct rac));

    if(!racional){
        yyerror("No hay espacio disponible para una estructura de racional");
        exit(0);
    }
    racional->nodetype = RACIONAL;
    racional->numerador = numerador;
    
    if(denominador->nodetype == ENTERO){
        if(((struct ent *)denominador)->valor != 0){
            
            racional->denominador = denominador;
        }else{
            racional = NULL;
        }
    }else{
        racional->denominador = denominador;
    }
    

    return (struct ast *)racional;
}

/*FUNCIONES PARA TRABAJAR CON LAS OPERACIONES*/

/*Para trabajar con negativos*/
struct ast *setSigno(struct ast *a){
    struct ast *resultado = NULL;

    /* printf("Dentro de setSigno\n"); */
    if (a->nodetype == ENTERO){
       /*  printf("Resultado Dentro del if: %i\n",((struct ent *)a)->valor); */
        ((struct ent *)a)->valor = (-1)*(((struct ent *)a)->valor);
    
    }else{
        /* printf("EL numero racional que llega: \n"); */
        struct ast * numerador = ((struct rac *)a)->numerador;
        struct ast * denominador = ((struct rac *)a)->denominador;

        /* printf("RACIONAL numerador: %i",((struct ent *)numerador)->valor);
        printf("RACIONAL denominador: %i",((struct ent *)denominador)->valor); */

        (((struct ent *)(((struct rac *)a)->numerador))->valor) = (-1)*(((struct ent *)(((struct rac *)a)->numerador))->valor);

       /*  printf("Despues del cambio de signo.\n");
        printf("Resultado RACIONAL numerador: %i",((struct ent *)((struct rac *)a)->numerador)->valor);
        printf("Resultado RACIONAL denominador: %i",((struct ent *)((struct rac *)a)->denominador)->valor);
 */
    }
    return a;
}


struct ast *mcd(struct ast *a, struct ast *b){
    struct ent *valorB = NULL;

   /*  printf("Dentro del mcd \n");
    printf("Valor que viene con a %i\n",a->l);
    printf("------------------------ \n");
    printf("Valor que viene con b %i\n",((struct ent *)b)->valor); */

    valorB = ((struct ent *)b);
   /*  printf("ValorB %i \n",valorB->valor); */
    
    if(valorB->valor == 0){
       /*  printf("Dentro del if \n");
        printf("Valor retornado de a %i",a->l); */
        return a;
    }else{
       /*  printf("Dentro del else \n");
        printf("Valor nuevo que le paso a B: %i\n", (((struct ent *)a)->valor % ((struct ent *)b)->valor));
        printf("Valor de b: %i\n",b->l); */
        return mcd(b , newEntero((((struct ent *)a)->valor % ((struct ent *)b)->valor)));
    }

}

struct ast *simplificar(struct ast *numRacional){
    
    struct ast *resultado = NULL;
    struct ast *NumMCD = NULL;
    struct ast *numNuevo = NULL;
    struct ast *denNuevo = NULL;

    struct ast *num = ((struct rac *)numRacional)->numerador;
    struct ast *den = ((struct rac *)numRacional)->denominador;

    NumMCD = mcd(num,den);
    int valMCD = ((struct ent *)NumMCD)->valor;

    /*Lo hago para que me muestre bien los numeros con signo negativo*/
    //printf("Dentro del simplificar, valor MCD %i\n",valMCD);
    if (valMCD < 0){
        valMCD = valMCD * (-1);
    }

    if(valMCD != 1){
       /*  printf("Dentro de simplificar, dentro del if: \n");
        printf("Valores para obtener el num: %i\n",((struct ent *)num)->valor);
        printf("Valores para obtener el numMCD: %i\n",((struct ent *)NumMCD)->valor); */
        numNuevo = division(num,NumMCD);
        /* printf("Valor del nuevo numerador: %i\n",((struct ent *)numNuevo)->valor); */
        denNuevo = division(den,NumMCD);
        /* printf("Valor del nuevo denominador: %i\n",((struct ent *)denNuevo)->valor); */
        
        //Si el denominador es 1, me queda un numero entero, cuyo valor es el numerador.
        if (((struct ent *)denNuevo)->valor == 1){
            resultado = newEntero(((struct ent *)numNuevo)->valor);
        }else{
            resultado = newRacional(numNuevo,denNuevo);
        }
    }else{
        if (((struct ent *)den)->valor == 1){
            resultado = newEntero(((struct ent *)num)->valor);
        }else{
            resultado = numRacional;
        }
    }

    return resultado;
}

struct ast *suma(struct ast *num1, struct ast *num2){
    struct ast *resultado = NULL;

    //printf("Dentro de suma\n");
    if(num1 != NULL && num2 != NULL){
        /* printf("Dentro del if de suma\n"); */
        /*Identifico que tipo de numeros estoy sumando*/

       /*  printf("Nodetype num1: %i\n",num1->nodetype);
        printf("Nodetype num2: %i\n",num2->nodetype); */
        if (num1->nodetype == ENTERO && num2->nodetype == ENTERO){

            int x = ((struct ent *)num1)->valor;
            int y = ((struct ent *)num2)->valor;
            
            /* printf("Valor de x %i\n",x);
            printf("Valor de y %i\n",y); */
           
            resultado = newEntero(x+y);

        }else{ 
            if(num1->nodetype == RACIONAL && num2->nodetype == RACIONAL){
                
                resultado = sumaRacional(num1,num2);

            }else{
                /*Uno de los numeros es Entero y el otro Racional*/
                if(num1->nodetype == RACIONAL){
                    resultado = sumaRacional(num1,newRacional(num2,newEntero(1)));
                }else{
                    resultado = sumaRacional(newRacional(num1,newEntero(1)),num2);
                }
            }
        }   

        
    }
    return resultado;
}

struct ast* sumaRacional(struct ast *num1, struct ast *num2){
    struct ast *auxNumerador1 = ((struct rac *)num1)->numerador;
    struct ast *auxDenominador1 = ((struct rac *)num1)->denominador;
    int denominador;

    struct ast *auxNumerador2 = ((struct rac *)num2)->numerador;
    struct ast *auxDenominador2 = ((struct rac *)num2)->denominador;

    int numerador1 = ((struct ent *)auxNumerador1)->valor;
    int numerador2 = ((struct ent *)auxNumerador2)->valor;
    int den1 = ((struct ent *)auxDenominador1)->valor;
    int den2 = ((struct ent *)auxDenominador2)->valor;
    
    /* printf("Dentro de sumaRacional.\n");
    printf("Valor de den1 %i\n",den1);
    printf("Valor de den2 %i\n",den2); */

    if(den1 == den2){
        denominador = den1;
    }else{
        denominador = den1 * den2;
    }

    /* printf("Valor de denominador %i\n",denominador); */
    /* int denominador = den1 * den2; */
    
    //int numerador = den1 * numerador2 + den2 * numerador1;
    
    int numerador = (denominador/den1)*numerador1 + (denominador/den2)*numerador2;
    /* printf("Valor del numerador %i\n",numerador); */

    struct ast *resultado = newRacional(newEntero(numerador),newEntero(denominador));

   /* printf("Valor de resultado numerador: %i\n",((struct ent *)(((struct rac *)resultado)->numerador))->valor);
    printf("Valor de resultado denominador: %i\n",((struct ent *)(((struct rac *)resultado)->denominador))->valor);
     */
    /* struct ast *simplificado = simplificar(resultado);

    printf("Valor de resultado numerador: %i\n",((struct ent *)(((struct rac *)simplificado)->numerador))->valor);
    printf("Valor de resultado denominador: %i\n",((struct ent *)(((struct rac *)simplificado)->denominador))->valor); */
    
    
    return simplificar(resultado); 
    /* return resultado; */

}

struct ast *resta(struct ast *num1, struct ast *num2){
    struct ast *resultado = NULL;

    if(num1 != NULL && num2 != NULL){
        //printf("DENTRO DE LA FUNC RESTA.\n");
        if (num1->nodetype == ENTERO && num2->nodetype == ENTERO){
            int x = ((struct ent *)num1)->valor;
            int y = ((struct ent *)num2)->valor;

            resultado = newEntero(x-y);
        }else{
            if(num1->nodetype == RACIONAL && num2->nodetype == RACIONAL){
                resultado = restaRacional(num1,num2);
            }else{
                /*Uno de los numeros es Entero y el otro Racional*/
                if(num1->nodetype == RACIONAL){
                    resultado = restaRacional(num1,newRacional(num2,newEntero(1)));
                }else{
                    resultado = restaRacional(newRacional(num1,newEntero(1)),num2);
                }
            }
        }
        
       /*  printf("Resultado en resta: %i\n",((struct ent *)resultado)->valor); */
    }
    
    return resultado;
}

struct ast *restaRacional(struct ast *num1, struct ast *num2){
    int denominador;

    struct ast *auxNumerador1 = ((struct rac *)num1)->numerador;
    struct ast *auxDenominador1 = ((struct rac *)num1)->denominador;

    struct ast *auxNumerador2 = ((struct rac *)num2)->numerador;
    struct ast *auxDenominador2 = ((struct rac *)num2)->denominador;

    int numerador1 = ((struct ent *)auxNumerador1)->valor;
    int numerador2 = ((struct ent *)auxNumerador2)->valor;
    int den1 = ((struct ent *)auxDenominador1)->valor;
    int den2 = ((struct ent *)auxDenominador2)->valor;
    
     if(den1 == den2){
        denominador = den1;
    }else{
        denominador = den1 * den2;
    }
    //int denominador = den1 * den2; 
    //int numerador = den1 * numerador2 - den2 * numerador1;
    int numerador = (denominador/den1)*numerador1 - (denominador/den2)*numerador2;

    struct ast *resultado = newRacional(newEntero(numerador),newEntero(denominador));
    /* printf("Dentro de la resta racional numerador: %i\n",numerador);
    printf("Dentro de la resta racional denominador: %i\n",denominador); */
    
    return simplificar(resultado); 
    /* return resultado; */
}

struct ast *multiplicacion(struct ast *num1, struct ast *num2){
    struct ast *resultado = NULL;

    if(num1 != NULL && num2 != NULL){
        if (num1->nodetype == ENTERO && num2->nodetype == ENTERO){
            int x = ((struct ent *)num1)->valor;
            int y = ((struct ent *)num2)->valor;

            resultado = newEntero(x*y);
        }else{
            if(num1->nodetype == RACIONAL && num2->nodetype == RACIONAL){
                resultado = multRacional(num1,num2);
            }else{
                /*Uno de los numeros es Entero y el otro Racional*/
                if(num1->nodetype == RACIONAL){
                    resultado = multRacional(num1,newRacional(num2,newEntero(1)));
                }else{
                    resultado = multRacional(newRacional(num1,newEntero(1)),num2);
                }
            }
        }
    }
    
    
    return resultado;
}
struct ast *multRacional(struct ast *num1, struct ast *num2){

    struct ast *auxNumerador1 = ((struct rac *)num1)->numerador;
    struct ast *auxDenominador1 = ((struct rac *)num1)->denominador;

    struct ast *auxNumerador2 = ((struct rac *)num2)->numerador;
    struct ast *auxDenominador2 = ((struct rac *)num2)->denominador;

    int numerador1 = ((struct ent *)auxNumerador1)->valor;
    int numerador2 = ((struct ent *)auxNumerador2)->valor;
    int den1 = ((struct ent *)auxDenominador1)->valor;
    int den2 = ((struct ent *)auxDenominador2)->valor;

    int numerador = numerador1 * numerador2;
    int denominador = den1 * den2;

    struct ast *resultado = newRacional(newEntero(numerador),newEntero(denominador));

    return simplificar(resultado); 

    /* return resultado; */

}

struct ast *division(struct ast *num1, struct ast *num2){
    struct ast *resultado = NULL;

   /*  printf("Estoy en division\n"); */
    if (num1 != NULL && num2 !=NULL){
        /* printf("Dentro del if\n"); */
        if (num1->nodetype == ENTERO && num2->nodetype == ENTERO){
           /*  printf("Dentro del segundo if\n"); */
            int x = ((struct ent *)num1)->valor;
            int y = ((struct ent *)num2)->valor;

            /* printf("Valor de x: %i",x);
            printf("Valor de y: %i",y); */
           
            if(y != 0){
                /* printf("Dentro del if y != 0\n");
                printf("Resultado de x/y != 0 %i\n",x/y);
                printf("Resultado de x%y %i\n",x%y); */
                if (x%y != 0){
                   /*  printf("Valor de x: %i\n",x);
                    printf("Valor de y: %i\n",y); */
                    /* printf("Estoy dentro del if en DIVISION"); */
                    resultado = newRacional(num1,num2);
                }else{
                    resultado = newEntero(x/y);
                }
            }
            /* printf("Despues del if en division\n"); */
            
            /*Si es una division de enteros, lo vuelvo racional y lo simplifico*/
        /*  printf("Dentro de DIVISION");
            printf("Numero racional.");
            struct ast *numRacional = newRacional(num1,num2);

            printf("Numerador. %i",((struct ent *)(((struct rac *)numRacional)->numerador))->valor);
            printf("Denominador. %i",((struct ent *)(((struct rac *)numRacional)->denominador))->valor);
            resultado = simplificar(numRacional);
            printf("Lo que me da en resultado de division \n");
            printf("Resultado nodeType",resultado->nodetype); */
        }else{
            if(num1->nodetype == RACIONAL && num2->nodetype == RACIONAL){
                resultado = divRacional(num1,num2);
            }else{
                /*Uno de los numeros es Entero y el otro Racional*/
                if(num1->nodetype == RACIONAL){
                    resultado = divRacional(num1,newRacional(num2,newEntero(1)));
                }else{
                    resultado = divRacional(newRacional(num1,newEntero(1)),num2);
                }
            }
        }
    
    }
    
    return resultado;
}


struct ast *divRacional(struct ast *num1, struct ast *num2){
    struct ast *auxNumerador1 = ((struct rac *)num1)->numerador;
    struct ast *auxDenominador1 = ((struct rac *)num1)->denominador;

    struct ast *auxNumerador2 = ((struct rac *)num2)->numerador;
    struct ast *auxDenominador2 = ((struct rac *)num2)->denominador;

    int numerador1 = ((struct ent *)auxNumerador1)->valor;
    int den1 = ((struct ent *)auxDenominador1)->valor;
    
    int numerador2 = ((struct ent *)auxNumerador2)->valor;
    int den2 = ((struct ent *)auxDenominador2)->valor;

    int numerador = numerador1 * den2;
    int denominador = den1 * numerador2;

    struct ast *resultado = newRacional(newEntero(numerador),newEntero(denominador));

    return simplificar(resultado); 

    /* return resultado; */
}

double getNumero(struct ast *numero){
    double aux ;

    if(numero->nodetype == ENTERO){
        aux = ((struct ent *)numero)->valor;
    }else{
        struct ast *numerador = ((struct rac *)numero)->numerador;
        struct ast *denominador = ((struct rac *)numero)->denominador;

        int valNum = ((struct ent *)numerador)->valor;
        int valDen = ((struct ent *)denominador)->valor;

        aux = (double)(valNum/valDen);
    }

    return aux;
}


struct ast *comparaciones(struct ast *a){
    struct ast *resultado = NULL;

    if(a != NULL){
        struct ast *num1, *num2;

        /*En caso de tener expresiones, las evaluo.*/
        num1 = eval(a->l);
        num2 = eval(a->r);

        if (num1 != NULL && num2 != NULL){
            double x,y;
            int resp;

            x = getNumero(num1);
            y = getNumero(num2);


            switch (a->nodetype){
                case '1':
                    resp = x > y ? 1 : 0; /*Si es verd da un 1 sino un 0 (TERNARIOS)*/
                break;
                case '2':
                    resp = x < y ? 1 : 0; 
                break;
                case '3':
                   /*  printf("Dentro de case 3\n");
                    printf("Valor de x %i",x);
                    printf("Valor de y %i",y); */

                    resp = x != y ? 1 : 0;
 
                    /* printf("Valor de resp %i",resp); */
                break;
                case '4':
                    resp = x == y ? 1 : 0;
                break;
                case '5': 
                    resp = x >= y ? 1 : 0;
                break;
                default:
                    resp = x <= y ? 1 : 0;
            }

            resultado = newast(BOOLEAN, newEntero(resp), NULL);
        }
    }
    
    return resultado;
    
}


char *concatenar(const char *s1, const char *s2) {
    const size_t len1 = strlen(s1); /* size_t  es una variante del sizeOf, lo uso pq necesito saber cuantos caracteres va a tener la cadena*/
    const size_t len2 = strlen(s2);
    
    char *result = malloc(len1+len2+1); /* malloc, reserva memoria */
    
    if (result != NULL){
        memcpy(result, s1, len1); /* en result compio lo que tengo en s1 y su tamaño */
        memcpy(result+len1, s2, len2+1); //+1 para el fin de cadena '\0'
        
        return result;
    } else {
        yyerror("no hay espacio - CONCATENAR.");
        exit(0);
    }
}


char *numToString(struct ast *a){ /*Recorro el arbol(valores que ingresan por teclado) para pasarlos a un string*/
	char *resul = "";
    char elem[20];
    int residual;
   /*  printf("numToString: %d",a->nodetype); */
    /*spintf(vable, formato del numero, numeros a convertir en cadena)*/
    /* int sprintf(char *str, const char *format, [arg1, arg2, ... ]); */
    
    if(a->nodetype == RACIONAL) {
		if (a->l != NULL){

           /*  printf("Valor de a en RACIONAL - numToString (a->l != NULL) : %d",a->nodetype); */
			resul = concatenar(resul, numToString(a->l));
            
			//printf("RACIONAL L: %s\n", resul);
			}

            resul = concatenar(resul,"/");

			if(a->r != NULL){
				resul = concatenar(resul, numToString(a->r));
				//printf("RACIONAL R: %s\n", resul);
			}
	      
		} else {
			if(a->nodetype == ENTERO){
                /* residual = sprintf(resul,"%d",((struct ent *)a)->valor); */
				residual = snprintf(elem, sizeof(elem),"%d", ((struct ent *)a)->valor); /* pasa flotantes a string */
               /*  printf("Valor de a en ENTERO - numToString: %d",a->nodetype);
                printf("Valor de resul: %s",resul);
                printf("Valor en estructura: %d",((struct ent *)a)->valor); */
                resul = concatenar(resul, elem);

			} else {
			    if (a->l != NULL){
					resul = concatenar(resul, numToString(a->l)); 
			    }
			    if (a->r != NULL){
					resul = concatenar(resul, ",");
					resul = concatenar(resul, numToString(a->r)); 	
			    }
	        }
	    }
	

	return resul;
}
char *Mostrar(struct ast *a){
    char *cadena = "";

   /*  printf("mostrar : %d",a->nodetype); */

    if(!a){
        return "ERROR.";
    }
        switch(a->nodetype){
            case ENTERO:
                cadena = concatenar(cadena,numToString(a));
            break;
            
            case RACIONAL:
                cadena = concatenar(cadena,numToString(a));
            break;

            case BOOLEAN:
                if(((struct ent *)a->l)->valor == 1){
                    cadena = "TRUE";
                }else{
                    cadena = "FALSE";
                }
            break;
           
            default: 
                printf("No se reconoce tipo en MOSTRAR %d\n",a->nodetype);
        }
        return cadena;
    
}
struct ast *eval(struct ast *a){
    struct ast *resultado = NULL;
    struct ast *num;
    struct ast *den;


    if(!a){
        /* yyerror("error interno, evaluación nula"); */
        return resultado;
    }

    switch(a->nodetype){
        case ENTERO:
            resultado = a;
            /* printf("Valor de a en eval - ENTERO: %d",a->nodetype); */
        break;

        case RACIONAL:
            if((a->l)->nodetype == ENTERO && (a->r)->nodetype == ENTERO){
                resultado = simplificar(a);
            }else{
               /*  printf("Estoy en else - eval- racional\n"); */
                resultado = division(eval(a->l),eval(a->r));
            }
            /* printf("Dentro del eval, case RACIONAL: \n"); */
            /* num = ((struct  rac *)a)->numerador;
            den = ((struct  rac *)a)->denominador;

            int valNum = ((struct ent *)num)->valor;
            int valDen = ((struct ent *)den)->valor; */
            
            //if(valNum < valDen){
                /* printf("Dentro del if");
                printf("Valor de numerador: %i\n",valNum);
                printf("Valor de denominador: %i\n",valDen); */
            /* struct ast *val = mcd(num,den);
            int valorMCD = ((struct ent *)val)->valor;
                
            if(valorMCD != 1){ */
                   /*  printf("valor de simplificar: %i\n",((struct ent *)(((struct rac *)simplificar(a))->numerador))->valor);
                    printf("valor de simplificar: %i\n",((struct ent *)(((struct rac *)simplificar(a))->denominador))->valor); */
                   /*  printf("Resultado: %i",((struct ent *)(((struct rac *)resultado)->numerador))->valor);
                    printf("Resultado: %i",((struct ent *)(((struct rac *)resultado)->denominador))->valor); */
                   // printf("Resultado, numerador: %i\n",resultado->l);
                /* }else{
                    resultado = a;
                } */
                
            //}
        break;

        case '+':
            /* printf("Estoy en suma - eval\n"); */
            resultado = suma(eval(a->l),eval(a->r)); 
        break;

        case '-':
           /*  printf("ESTOY EN RESTA\n."); */
            resultado = resta(eval(a->l),eval(a->r));
        break;

        case '*':
            resultado = multiplicacion(eval(a->l),eval(a->r));
        break;
        case 'M': 
           /*  printf("Estoy en M \n"); */
            resultado = setSigno(eval(a->l)); 
        break;
        /* asignacion */
        case '=': 
            resultado = ((struct symasgn *)a)->s->value = eval(((struct symasgn *)a)->v);
            //resultado = a;
        break;
        /* referencia */
        case 'N': 
            resultado = ((struct symref *)a)->s->value;
            //resultado = newast('N',((struct symref *)a)->s,NULL);
            //resultado = a;
            break;
       
    /* comparaciones */
        case '1': case '2': case '3': case '4': case '5': case '6':
            resultado = comparaciones(a); 
        break;
        
        /* case '/':
            resultado = division(eval(a->l),eval(a->r));  
        break; */
        
        
        default: 
            printf("ERROR INTERNO EVAL: nodo incorrecto %d\n", a->nodetype);
    }

    return resultado;

}