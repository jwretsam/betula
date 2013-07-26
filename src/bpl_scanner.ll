
%{
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
using namespace std;

#include "bpl_grammar.hh"
#include "syntax_tree.h"

#define YY_DECL int yylex (YYSTYPE * yylval_param,YYLTYPE * yylloc_param, int *lp )

int yycolumn = 1;

#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno; \
    yylloc->first_column = yycolumn; yylloc->last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;


void yyerror(YYLTYPE *yylloc, int *lp, const char *s, ...);
#define Z_ERROR(FMT,ARGS...) yyerror(yylloc, lp, FMT, ## ARGS)


%}

%option yylineno
%option noyywrap
%option outfile="bpl_scanner.cc"
%option bison-bridge bison-locations

%x COMMENT
%x INCL

ID	[A-Za-z_][A-Za-z0-9_]*
DECIMAL ([[:digit:]]{-}[0])[[:digit:]]*
HEX	0[xX][[:xdigit:]]+ 
OCTAL	0[01234567]*
NFLOAT  [[:digit:]]+\.[[:digit:]]*


%% 

package		{ return PACKAGE; }
enum		{ return ENUM; }
message		{ return MESSAGE; }
required	{ return REQUIRED; }
optional	{ return OPTIONAL; }
import		{ BEGIN(INCL); return IMPORT; }




{ID}::{ID}	{
		    yylval->str = new std::string(yytext);
		    return Z.is_typename(*yylval->str) ? TYPENAME : ID;
		}


{ID}		{
		    yylval->str = new std::string(yytext);
		    return Z.is_typename(*yylval->str) ? TYPENAME : IDN;
		}

{DECIMAL}	{ yylval->num = atoi(yytext); return NUMBER; };

{OCTAL}		{
		    yylval->num = 0;
		    for(const char *p=&yytext[2]; *p; p++)
		    {
			unsigned char ch = (unsigned char) *p;
			yylval->num <<= 3;
			yylval->num |= ch-'0';
		    };
		    return NUMBER;
		}



{HEX}		{
		    yylval->num = 0;
		    for(const char *p=&yytext[2]; *p; p++)
		    {
			unsigned char ch = (unsigned char) *p;
			yylval->num <<= 4;
			yylval->num |= (ch < 'A') ? ch-'0' : ( (ch<'a') ? ch-'A' : ch-'a');
		    };
		    return NUMBER;
		}

{NFLOAT}	{ yylval->fnum = atof(yytext); return FLOAT; }


[\[\];={},]	{ return yytext[0];}

(\/\/)+[^\n]*
"/*"	{ BEGIN(COMMENT); }

[\n]		{ yycolumn = 1; }

\"(\\.|[^\\"])*\" { yylval->str = new std::string(yytext); return STRING_LITERAL; }

[ \t\v\f]	{ }
.		{ printf("Lex error\n"); exit(-1); }


<<EOF>>		{
		    Z.pop_current_file();
		    yycolumn = 1;
    		    yypop_buffer_state();
		    if ( !YY_CURRENT_BUFFER )
    	            {
    	        	yyterminate();
    	            }
    	        }

<INCL>{
\"(\\.|[^\\"])*\" { yylval->str = new std::string(yytext); return STRING_LITERAL; }
[ \t]		{ }
[;]		{
		    std::string fnm;
		    fnm.append(Z.t_include_file.c_str()+1, Z.t_include_file.length()-2);
		    if(Z.is_last_file(fnm))
			yyerror(yylloc, lp, "error: duplicate import %s", fnm.c_str());
		    
		    yyin = fopen( fnm.c_str(), "r" );
		    if( !yyin )
			yyerror(yylloc, lp, "error: open file %s", fnm.c_str());
		    yypush_buffer_state(yy_create_buffer( yyin, YY_BUF_SIZE ));
		    Z.push_current_file(fnm);
		    
		    BEGIN(INITIAL);
		}
}


<COMMENT>{
"*/"      BEGIN(0);
[^*\n]+   { yycolumn = 1; }
"*"[^/]   ;
\n        { yycolumn = 1; }
}

%%


