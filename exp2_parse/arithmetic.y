%{
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifndef YYSTYPE
#define YYSTYPE double
#endif


extern int yyparse ( ) ;//语法分析入口，由bison自动生成
FILE* yyin ;//指定输入，可以指向文件或标准输入流等
void yyerror(const char* s ) ;
int yylex ( ) ;//由lex自动生成，返回终结符含义，由于没有使用没有使用lex故需要自己重写

%}


%token INTEGER 
%token ADD SUB MUL DIV LP RP
%left ADD SUB
%left MUL DIV
%left RP
%right LP
%right UMINUS

%%

lines:line lines 
    | line
    ;

line: expr ';' { printf("%f\n", $1);}

expr: term
    | expr ADD term { $$ = $1 + $3 ; }  
    | expr SUB term { $$ = $1 - $3 ; } 
    | SUB expr %prec UMINUS { $$ = -$2 ; } 
    ;

term: factor
    | term MUL factor { $$ = $1 * $3 ; }
    | term DIV factor { $$ = $1 / $3 ; }
    ;

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
            ungetc( inchar , stdin ) ;//将最后一个不是数字的inchar推到标准输入流

            return INTEGER;
        }

        else if ( inchar=='+')return ADD;
        else if ( inchar=='-')return SUB;
        else if ( inchar=='*')return MUL;
        else if ( inchar=='/')return DIV;
        else if ( inchar=='(')return LP;
        else if ( inchar==')')return RP;
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
