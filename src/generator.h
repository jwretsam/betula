
#ifndef __GENERATOR_H
#define __GENERATOR_H

#include "syntax_tree.h"

// ----------------------

class fmt_string
{
public:

    std::string str;

    fmt_string() { };

    void erase(void) { str.erase(); };

    void print(const char *fmt_spec, ...);

    void coma(bool with_space=true);

    bool empty(void) const { return str.empty(); };
    unsigned int length(void) const { return str.length(); };
    const char* c_str(void) const { return str.c_str(); };
};


// ----------------------

class codegen;

// ----------------------



class gen_env
{
public:

    codegen *generator;

};


// ----------------------

class gen_src
{
public:

    gen_env env;
    bpl::syntax_tree &st;
    bpl::syntax_tree::out_module out;

    gen_src() = delete;

    gen_src(bpl::syntax_tree &a_st, const gen_env &a_env) : env(a_env), st(a_st)
    {
	st.get_out_module(out);
    };
};


// ----------------------

class codegen
{
protected:

    gen_src *gs = 0;

public:

    codegen() { };
    virtual ~codegen() { };

    virtual void set_source(gen_src *a_gs) { gs = a_gs; };

    virtual void process(void) throw(std::string) = 0;

};

// ----------------------



// ----------------------

void process(const gen_env &env);

#endif // __GENERATOR_H
