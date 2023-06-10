/* calculadora con AST */

%{
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "fb3-2.h"

extern int yylex();
int yyerrork;
%}

%union{
    struct ast *a;
    struct symbol *s;
    struct symlist *sl;
    int fn;
    int i;
}

/* declaración de tokens */ /*%token <fn> FUNC --- Cuando se coloca <> indicamos que estamos por definir un token que se encuentra dentro de union*/

%token <s> NAME
%token <fn> FUNC
%token EOL
%token IF THEN ELSE WHILE DO LET

/*Mi codigo*/
%token RACIONAL
%token <i> ENTERO
%token BOOLEAN

%nonassoc <fn> CMP
%right '='
%left '+' '-'
%left '*' '/'
%nonassoc '|' UMINUS

%type <a> numRacional
%type <a> valor

%type <a> exp stmt list explist
%type <sl> symlist

%start calclist
/*Lo que se coloca en %% %% es la gramatica*/

%%
calclist:   /* nada */
|           calclist stmt EOL       {printf("= %s\n",Mostrar(eval($2)));
                                     /* treefree($2); */
                                     printf("> ");
                                    }
|           calclist LET NAME '(' symlist ')' '=' list EOL
                                    {/* dodef($3,$5,$8); */
                                     printf("Definido %s\n> ",$3->name);}
|           calclist error EOL      {yyerrork; printf("> ");}
;

stmt:   IF exp THEN list            {$$ = newflow('I',$2,$4,NULL);}
|       IF exp THEN list ELSE list  {$$ = newflow('I',$2,$4,$6);}
|       WHILE exp DO list           {$$ = newflow('W',$2,$4,NULL);}
|       exp
;

list:   /* nada */                  {$$ = NULL;}
|       stmt ';' list               {if ($3 == NULL)
                                        $$ = $1;
                                     else
                                        $$ = newast('L',$1,$3);
                                    }
;

exp:    exp CMP exp                 {$$ = newcmp($2,$1,$3);}
|       exp '+' exp                 {$$ = newast('+',$1,$3);}
|       exp '-' exp                 {$$ = newast('-',$1,$3);}
|       exp '*' exp                 {$$ = newast('*',$1,$3);}
|       exp '/' exp                 {$$ = newast('/',$1,$3);}
|       '|' exp                     {$$ = newast('|',$2,NULL);}
|       '(' exp ')'                 {$$ = $2;}
|       '-' exp %prec UMINUS        {$$ = newast('M',$2,NULL);}
|       NAME                        {$$ = newref($1);}
|       NAME '=' exp                {$$ = newasgn($1,$3);}
|       FUNC '(' explist ')'        {$$ = newfunc($1,$3);}
|       NAME '(' explist ')'        {$$ = newcall($1,$3);}
|       ENTERO                      {$$ = newEntero($1);}
|       numRacional                 {$$ = $1;}
;

numRacional:    valor RACIONAL valor    {$$ = newRacional($1,$3);}
; 

valor: ENTERO                       {$$ = newEntero($1);}
|     '(' exp ')'                   {$$ = $2;} 
;

explist:    exp
|           exp ',' explist         {$$ = newast('L',$1,$3);}
;

symlist:    NAME                    {$$ = newsymlist($1,NULL);}
|           NAME ',' symlist        {$$ = newsymlist($1,$3);}
;


%%
/*
 */
 
void yyerror(char *s, ...){
    va_list ap;
    va_start(ap,s);

    fprintf(stderr, "%d: error: ", yylineno);
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
}

int main(){
    printf("> ");
    return yyparse();
}

