%{
#include <stdio.h>
#include <stdlib.h>
#ifndef YYSTYPE
#define YYSTYPE double
#endif


extern int yyparse ( ) ;//语法分析入口，由bison自动生成
FILE* yyin ;//指定输入，可以指向文件或标准输入流等
void yyerror(const char* s ) ;
int yylex ( ) ;//由lex自动生成，返回终结符含义，由于没有使用没有使用lex故需要自己重写

%}


%token INTEGER 
%token ADD SUB MUL DEV LP RP
%left ADD SUB
%left MUL DEV
%left RP
%right LP
%right UMINUS

%start lines

%%

lines:line lines 
    | line
    ;

line: expr '\n' { printf("%f\n", $2);}

expr: expr ADD term { $$ = $1 + $3 ; }  
    | expr SUB term { $$ = $1 - $3 ; } 
    | SUB expr %prec UMINUS { $$ = -$2 ; } 
    ;

term: term MUL factor { $$ = $1 * $3 ; }
    | term DEV factor { $$ = $1 / $3 ; }

factor:LP expr RP { $$ = $2 ; }
    | INTEGER { $$ = $1 ; }
    ;

%%
int yylex ()
 {

int inchar;
    while (1) {
        inchar = getchar ( ) ;
        if ( inchar == ' ' || inchar== '\t' || inchar == '\n'){}//识别空格和制表符
        
        else if ( isdigit( inchar )) {
            yylval = 0;
            while ( isdigit ( inchar )) {
            yylval = yylval * 10 + inchar - '0' ;
            inchar = getchar ( ) ;
            }
            ungetc( inchar , stdin ) ;//将inchar推到标准输入流以便接下来赋给yyin

            return INTEGER;
        }

        else if ( inchar=='+')return ADD;
        else if ( inchar=='-')return SUB;
        else if ( inchar=='*')return MUL;
        else if ( incahr=='/')return DEV;
        else if ( inchar == '(')return LP;
        else if ( inchar == ')')return RP;
        else return inchar ;
    }
}



void yyerror(const char* s) {
    fprintf (stderr , "Parse error : %s\n",s );
    exit (1);
}



int main(void) {
    yyin = stdin ;
    do {
        yyparse();
    } while (! feof (yyin));
    return 0;
}
