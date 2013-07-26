
#include <stdio.h>

#include "generator.h"

codegen* get_codegen_cpp(void);

int main(int argc, char **argv)
{
    codegen *generator = get_codegen_cpp();

    gen_env env;
    env.generator = generator;

    process(env);

    return 0;
};
