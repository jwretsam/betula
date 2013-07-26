%{

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <string>

#include <sys/types.h>

#include "bpl_grammar.hh"
#include "syntax_tree.h"


/*#define YYPARSE_PARAM scanner */
/* #define YYLEX_PARAM lp */

#define Q(FMT,ARGS...) do{ printf(FMT "\n", ## ARGS); fflush(stdout); }while(0)

#define YY_DECL int yylex (YYSTYPE * yylval_param,YYLTYPE * yylloc_param, int *lp )

void yyerror(YYLTYPE *yylloc, int *lp, const char *s, ...);

int yylex (YYSTYPE * yylval_param,YYLTYPE * yylloc_param, int *lp );

%}

%error-verbose
%locations
%define api.pure

%lex-param   {int *lp}
%parse-param {int *lp}

%union {
    int64_t num;
    double fnum;
    std::string* str;
}


%token PACKAGE ENUM MESSAGE REQUIRED OPTIONAL IMPORT
%token <str> ID IDN TYPENAME STRING_LITERAL
%token <num> NUMBER
%token <fnum> FLOAT

%type <num> exp_int
%type <str> id


%%

start
	: /* empty */
	| unit_list
	;

unit_list
	: unit_def
	| unit_list unit_def
	;

unit_def
	: IMPORT STRING_LITERAL
			{
			    Z.t_include_file = *$2;
			    Q("Import %s", $2->c_str());
			    delete $2;
			}
	| package_def
	;


package_def
	: PACKAGE IDN
		    {
			Z.package_name = *$2;
			Z.package = Z.st.add_package(Z.package_name);
			if(!Z.package)
			    yyerror(&yylloc, lp, "error: duplicate package %s", Z.package_name.c_str());
			Q("------\nPackage %s", $2->c_str());
			delete $2;
		    }
		    '{' decl_list '}' { Z.package_name.erase(); Z.package = 0;} ';'
	;


decl_list
	: decl_item
	| decl_list decl_item
	;

decl_item
	: enum_decl
	| message_decl
	;



enum_decl
	: ENUM IDN  {
//			std::string nm = Z.full_id_name(*$2);
//			if(!Z.add_typename(nm))
//			    yyerror(&yylloc, lp, "error: enum %s already exists", nm.c_str());
			
			Z.add_typename(Z.full_id_name(*$2));
			Z.t_enum = Z.package->add_enum(*$2);
			Q("Decl enum %s", $2->c_str());
			if(!Z.t_enum)
			    yyerror(&yylloc, lp, "error: typename %s already exists", $2->c_str());
			delete $2;
		    }
		    '{' enum_items_list '}' ';' { Z.t_enum->build_meta(); }
	;


enum_items_list
	: enum_item
	| enum_items_list ',' enum_item
	;

enum_item
	: IDN '=' exp_int	{
				    Q("Enum item: %s = %lld", $1->c_str(), $3);
				    if(!Z.t_enum->add_item(*$1, $3))
					yyerror(&yylloc, lp, "error: enum item %s already exists", $1->c_str());
				    delete $1;
				}
	| IDN			{
				    Q("Enum item: %s ---", $1->c_str());
				    if(!Z.t_enum->add_item(*$1))
					yyerror(&yylloc, lp, "error: enum item %s already exists", $1->c_str());
				    delete $1;
				}
	;


message_decl
	: MESSAGE IDN	{
			    Q("Message %s", $2->c_str());
//			    std::string nm = Z.full_id_name(*$2);
//			    if(!Z.add_typename(nm))
//				yyerror(&yylloc, lp, "error: message %s already exists", nm.c_str());
				
			    Z.add_typename(Z.full_id_name(*$2));
			    Z.t_message = Z.package->add_message(*$2); 

			    if(!Z.t_message)
				yyerror(&yylloc, lp, "error: typename %s already exists", $2->c_str());
			    Z.t_is_required = true;
			    delete $2;

			}
			'{' field_list '}' ';' { Z.t_message->build_meta(); }
	;


id	
	: ID
	| IDN
	;

field_list
	: field_decl
	| field_list field_decl
	;

field_decl
	: field_class field_def
	| field_def
	;

field_class
	: REQUIRED	{ Q("Required"); Z.t_is_required = true; }
	| OPTIONAL	{ Q("Optional"); Z.t_is_required = false; }
	;

field_def
	: field_desc '[' array_def ']' '=' '{' initialize_list '}' ';'
	| field_desc '[' array_def ']' ';'
	| field_desc '=' initialize_item ';'
	| field_desc ';'
	;

field_desc
	: TYPENAME id
			{
			    if(!Z.st.type_exists(*$1))
				yyerror(&yylloc, lp, "error: undefined typename %s", $1->c_str());
			    
			    Z.t_field = Z.t_message->add_field(*$1, *$2, Z.t_is_required);
			    if(!Z.t_field)
				yyerror(&yylloc, lp, "error: duplicate field '%s' in message %s", $2->c_str(), Z.t_message->get_name().c_str());
			    
			    Z.t_field->link = Z.st.find_typename_item(*$1);
			    Q("Field %s %s", $1->c_str(), $2->c_str());
			    delete $1;
			    delete $2;
			}
	;

array_def
	: /* empty */		{ Q("Range FULL"); Z.t_field->set_range(bpl::decl_message::NO_RANGE, bpl::decl_message::NO_RANGE); };
	| exp_int ',' exp_int	{
				    if(!Z.t_field->set_range($1, $3))
					yyerror(&yylloc, lp, "error: incorrect range");
				    Q("Range %lld - %lld", $1, $3);
				}
	| exp_int ',' 		{
				    if(!Z.t_field->set_range($1, bpl::decl_message::NO_RANGE))
					yyerror(&yylloc, lp, "error: incorrect range");
				    Q("Range %lld - FULL", $1);
				}
	| exp_int  		{
				    if(!Z.t_field->set_range(1, $1))
					yyerror(&yylloc, lp, "error: incorrect range");
				    Q("Range 1 - %lld", $1);
				}
	;


initialize_list
	: initialize_item
	| initialize_list ',' initialize_item
	;

initialize_item
	: NUMBER		{
				    Z.t_field->add_init_value($1);
				    Q("   Init item %lld", $1);
				}
	| FLOAT			{
				    Z.t_field->add_init_value($1);
				    Q("   Init item %lf", $1);
				}
	| STRING_LITERAL	{
				    std::string s;
				    s.append($1->c_str()+1, $1->length()-2);
				    Z.t_field->add_init_value(s);
				    Q("   Init item %s", $1->c_str());
				    delete $1;
				}
	| id			{
				    if(!Z.st.is_enum_initializer(Z.t_field->get_vtype(), *$1))
					yyerror(&yylloc, lp, "error: wrong initialize value");
				    Z.t_field->add_init_value_id(*$1);
				    Q("   Init item id=%s", $1->c_str());
//				    delete $1;
				    
				}
	;

exp_int
	: NUMBER
	;


%%

void yyerror(YYLTYPE *yylloc, int *lp, const char *s, ...) {

    va_list ap;
    va_start(ap, s);
  
    fprintf(stderr, "%s:%d:%d: ", 
	Z.get_current_file().c_str(),
	yylloc->first_line, yylloc->first_column
    );

    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");

	// might as well halt now:
	    exit(-1);
};


