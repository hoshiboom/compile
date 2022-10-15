%{
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <map>


struct entry{   
    char* name;
    double value;
};
std::map <char*,double> stable;
#ifndef YYSTYPE
#define YYSTYPE entry
#endif

char identfier[30];

extern int yyparse ( ) ;//语法分析入口，由bison自动生成
FILE* yyin ;//指定输入，可以指向文件或标准输入流等
void yyerror(const char* s ) ;
int yylex ( ) ;//由lex自动生成，返回终结符含义，由于没有使用lex故需要自己重写


%}


%token<value> INTEGER 
%token<name> ID
%token ADD SUB MUL DIV LP RP EQUAL
%type<value> expr term factor

%left ADD SUB
%left MUL DIV
%left RP
%right LP
%right UMINUS

%%

lines:line lines 
    | line
    ;

line: ID EQUAL expr ';' {stable[$1]=$3;}
    | expr ';'{printf("%f\n",$1);}
    ;

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
    | ID { $$=stable[$1]; }
    ;

%%
int yylex ()
 {

int inchar;
    while (1) {
        inchar = getchar ( ) ;
        if ( inchar == ' ' || inchar== '\t' || inchar == '\n'){}//识别空格和制表符
        
        else if(isdigit(inchar)){
            yylval.value = 0;
            while ( isdigit ( inchar )) {
            yylval.value = yylval.value * 10 + inchar - '0' ;
            inchar = getchar ( ) ;
            }
            ungetc( inchar , stdin ) ;//将最后一个不是数字的inchar推到标准输入流

            return INTEGER;
        }

        else if(isalpha(inchar)||(inchar=='_')){
            int i=0;
            while(isalpha(inchar)||(inchar=='_')||isdigit(inchar)){
                identfier[i] = inchar;
                inchar=getchar(); 
                i++;
            }
            identfier[i] = '\0';
            yylval.name=identfier;
            ungetc(inchar,stdin);
            return ID;   
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
