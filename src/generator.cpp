#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "generator.h"


int yyparse(int *lp);
// ----------------------

void fmt_string::print(const char *fmt_spec, ...)
{
    char buf[16*1024];

    va_list arglist;
    va_start (arglist, fmt_spec);
    int len = vsprintf(buf, fmt_spec, arglist);
    va_end(arglist);
    str.append(buf, len);
};

void fmt_string::coma(bool with_space)
{
    if(!str.empty())
    {
	str += with_space ? ", " : ",";
    };
};

// ----------------------

void process(const gen_env &env)
{

    bpl::syntax_tree st("STDIN");
    bpl::parser_data pd(st);
    yyparse((int*)&pd);

    gen_src src(st, env);
    
    env.generator->set_source(&src);

    try
    {
	env.generator->process();
	delete env.generator;
    }
    catch(std::string error)
    {
	fprintf(stderr, "error: %s\n", error.c_str());
	exit(-1);
    };
};


// ----------------------


