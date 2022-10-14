%{
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#ifndef YYSTYPE
#define YYSTYPE char*
#endif

extern int yyparse ( ) ;//语法分析入口，由bison自动生成
FILE* yyin ;//指定输入，可以指向文件或标准输入流等
void yyerror(const char* s ) ;
int yylex ( ) ;//由lex自动生成，返回终结符含义，由于没有使用没有使用lex故需要自己重写

%}


%token INTEGER ID
%token ADD SUB MUL DIV LP RP
%left ADD SUB
%left MUL DIV
%right UMINUS

%start lines

%%

lines:line lines 
    | line
    ;

line: expr ';' { printf("%s\n", $1);}

expr: expr ADD expr {$$ = (char *)malloc(50*sizeof(char)); strcpy($$,$1); strcat($$,$3); strcat($$,"+ "); }  
    | expr SUB expr {$$ = (char *)malloc(50*sizeof(char)); strcpy($$,$1); strcat($$,$3); strcat($$,"- "); } 
    | SUB expr %prec UMINUS {$$ = (char *)malloc(50*sizeof(char)); strcpy($$,"-"); strcat($$,$2); } 
    | expr MUL expr {$$ = (char *)malloc(50*sizeof(char)); strcpy($$,$1); strcat($$,$3); strcat($$,"* "); }
    | expr DIV expr {$$ = (char *)malloc(50*sizeof(char)); strcpy($$,$1); strcat($$,$3); strcat($$,"/ "); }
    | LP expr RP    { $$ = (char *)malloc(50*sizeof(char)); strcpy($$,$2); }
    | INTEGER       { $$ = (char *)malloc(50*sizeof(char)); strcpy($$,$1); strcat($$," "); }
    | ID            { $$ = (char *)malloc(50*sizeof(char)); strcpy($$,$1); strcat($$," "); }
    ;



%%
int yylex ()
 {

    char inchar;
    while (1) {
        inchar = getchar ( ) ;
        if ( inchar == ' ' || inchar== '\t' || inchar == '\n'){}//识别空格和制表符
        
        else if ( isdigit( inchar )) {
            int i=0;
            while ( isdigit ( inchar )) {
                yylval[i]=inchar ;
                inchar = getchar ( ) ;
                i++;
            }
            yylval[i]='\0';
            ungetc( inchar , stdin ) ;//????????????????????将inchar推到标准输入流以便接下来赋给yyin

            return INTEGER;
        }

        else if(('a'<=inchar&&inchar<='z')||('A'<=inchar&&'Z'>=inchar)||(inchar=='_')){
            int i=0;
            while(('a'<=inchar&&inchar<='z')||('A'<=inchar&&'Z'>=inchar)||(inchar=='_')||isdigit(inchar)){
                yylval[i] = inchar;
                inchar=getchar(); 
                i++;
            }
            yylval[i] = '\0';
            ungetc(inchar,stdin);
            return ID;   
        }

        else if ( inchar=='+')return ADD;
        else if ( inchar=='-')return SUB;
        else if ( inchar=='*')return MUL;
        else if ( inchar=='/')return DIV;
        else if ( inchar =='(')return LP;
        else if ( inchar ==')')return RP;
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
