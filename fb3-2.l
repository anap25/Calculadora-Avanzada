/* reconocimiento de tokens para la calculadora */
%option noyywrap nodefault yylineno

%{
#include "fb3-2.h"

%}

/* exponente flotante */
EXP     ([Ee][-+]?[0-9]+)


/*Numeros enteros*/

DIGITO [0-9]
NUM_ENTERO {DIGITO}*

%%
 /* operadores de caracter simple */
"+" |
"-" |
"*" |
"=" |
"|" |
"," |
";" |
"(" |
")"         {return yytext[0];}

 /*Sintaxis para reconocer un numero racional*/
"/" {return RACIONAL;}

 /* operadores de comparación, todos como el token CMP */
 /*Bison genera el archivo fb3-2.tab.h a partir del .y */
 /*Con el yylval es lo que usa flex para comunicarse con bison con los tokens que nos genera bison en el archivo fb3-2.tab.h*/
">"         {yylval.fn = 1; return CMP;}
"<"         {yylval.fn = 2; return CMP;}
"<>"        {yylval.fn = 3; return CMP;}
"=="        {yylval.fn = 4; return CMP;}
">="        {yylval.fn = 5; return CMP;}
"<="        {yylval.fn = 6; return CMP;}

 /* palabras clave */ /*Está como patron accion, a partir de la palabra que me aparece, es la accion que se va a realizar.*/
"if"        {return IF;} 
"then"      {return THEN;}
"else"      {return ELSE;}
"while"     {return WHILE;}
"do"        {return DO;}
"let"       {return LET;}

 /* funciones incluidas */

"exp"       {yylval.fn = B_exp; return FUNC;}
"print"     {yylval.fn = B_print; return FUNC;}

 /* Variables*/
[a-zA-Z][a-zA-Z0-9]*        {yylval.s = lookup(yytext); return NAME;}

{NUM_ENTERO}                {yylval.i = atoi(yytext); return ENTERO;} /*Reconozco el patrón*/

 /* "//".* */

[ \t]

\\\n                        {printf("c> ");}  /* ignora continuación de línea */
\n                          {return EOL;}

.                           {yyerror("Caracter misterioso %c\n", *yytext);}

%%

