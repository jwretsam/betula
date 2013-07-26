#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "generator.h"

using namespace std;
using namespace bpl;

// ----------------------

#define TAB "   "

class codegen_cpp : public codegen
{
private:

    FILE *C = 0;
    FILE *H = 0;

public:

    codegen_cpp() { };

    virtual void process(void) throw(std::string);

private:

    void gen_enum(const decl_enum &z);
    void gen_message(const decl_message &z);

    bool std_type_def(const string &type, string &itype, string &initializer) const;

};

#define HP(FMT,ARGS...) fprintf(H, FMT, ## ARGS )
#define CP(FMT,ARGS...) fprintf(C, FMT, ## ARGS )


void codegen_cpp::process(void) throw(std::string)
{
    string name = gs->out.extract_name(gs->out.name);
    string header_filename = name + ".h";
    string code_filename = name + ".cpp";

    C = fopen(code_filename.c_str(), "w");
    if(!C)
	throw string(string("create file ") + code_filename);

    H = fopen(header_filename.c_str(), "w");
    if(!H)
	throw string(string("create file ") + header_filename);

    CP("#include \"%s\"\n\n", header_filename.c_str());

    string hdr_def = "__BETULA_" + name + "_H";
    HP("#ifndef %s\n#define %s\n\n", hdr_def.c_str(), hdr_def.c_str());

    
    // Include headers
//    HP("#include <string>\n");
//    HP("#include <vector>\n");
//    HP("#include <ostream>\n");
    HP("\n#include <betula.h>\n");
    HP("\n");
    for(auto &x : gs->out.import_list)
	HP("#include \"%s\"\n", string(gs->out.extract_name(x)+".h").c_str());

    HP("\n\n");
    // -------------
    
    for(auto package : gs->out.packages)
    {
	CP("#define CHECK(SIZE) do{ if((long)(SIZE) > __buffer_len) return false; }while(0)\n\n");
	CP("inline void __move_buffer(const char* &buffer, long size) { buffer += size; };\n");
	CP("#define MOVE(SIZE) __move_buffer(__buffer,(long)(SIZE))\n\n");
	CP("#define VGET(TYPE) betula::ntoh_##TYPE(*(betula::TYPE*)__buffer)\n");

    
	HP("namespace %s {\n\n", package->get_name().c_str());
	CP("namespace %s {\n\n", package->get_name().c_str());
	
	for(auto &t : package->get_decl())
	{
	    const decl_item *item = t.get();

	    switch(item->get_type())
	    {
		case decl_item::Enum:
		{
		    gen_enum( *(decl_enum*)item );
		    break;
		};

		case decl_item::Message:
		{
		    gen_message( *(decl_message*)item );
		    break;
		};

		default: throw string("Internal error: unexpexted item type");
	    };
	};
	
	HP("\n}; // namespace %s\n\n", package->get_name().c_str());
	CP("\n}; // namespace %s\n\n", package->get_name().c_str());
    };

    HP("#endif // %s\n", hdr_def.c_str());
    fclose(H);
    fclose(C);
};


void codegen_cpp::gen_enum(const decl_enum &z)
{
    HP("enum %s {\n", z.get_name().c_str());

    bool first = true;
    int last_val = 0;

    for(auto &x : z.get_items())
    {
	if(first)
	{
	    first = false;
	    last_val = x.value;
	    if(!x.value)
		HP(TAB "%s", x.name.c_str());
	    else
		HP(TAB "%s = %d", x.name.c_str(), x.value);
	    continue;
	};

	if( (x.value - last_val) == 1 )
	    HP(",\n" TAB "%s", x.name.c_str());
	else
	    HP(",\n" TAB "%s = %d", x.name.c_str(), x.value);
	
	last_val = x.value;
    };

    HP("\n};\n\n");
};

void codegen_cpp::gen_message(const decl_message &z)
{
    static const char *tabs[] =
    {
	TAB,
	TAB TAB,
	TAB TAB TAB,
	TAB TAB TAB TAB,
	TAB TAB TAB TAB TAB
    };


    fmt_string fs_decl, fs_constr;

    string current_package, message_name;
    gs->st.split_name(z.get_name(), current_package, message_name);


    HP("class %s : public betula::message_base {\n", z.get_name().c_str());

    decl_message::field_ref r;
    z.get_field_ref(r);
    unsigned int opt_count = r.optional.size();
    unsigned int opt_size = (opt_count >> 3) + ((opt_count & 7) != 0);

    if(opt_size)
    {
	HP("private:\n\n");

	HP(TAB "enum {\n");
	bool first = true;
	
	for(auto pf : r.optional)
	{
	    string s = "i_" + pf->get_name();
	    if(first)
	    {
		first = false;
		HP(TAB TAB "%s", s.c_str());
	    }
	    else
		HP(",\n" TAB TAB "%s", s.c_str());
	};
	HP("\n" TAB "};\n\n");

	if(opt_size > 1)
	{
	    HP(TAB "bool i_has(int index) const { return (optional[index>>3] & (1 << (index & 7)) ) != 0; };\n");
	    HP(TAB "void i_set(int index) { optional[index>>3] |= 1 << (index & 7); };\n");
	    HP(TAB "void i_clr(int index) { optional[index>>3] &= ~(1 << (index & 7)); };\n\n");

	    HP(TAB "betula::uint8 optional[%u];\n\n", opt_size);
	    fs_constr.print(TAB);
	    for(unsigned int i=0; i<opt_size; i++)
		fs_constr.print("optional[%u] = ", i);
	    fs_constr.print("0;\n");
	    
	}
	else
	{
	    HP(TAB "bool i_has(int index) const { return (optional & (1 << index) ) != 0; };\n");
	    HP(TAB "void i_set(int index) { optional |= 1 << index; };\n");
	    HP(TAB "void i_clr(int index) { optional &= ~(1 << index); };\n\n");

	    HP(TAB "betula::uint8 optional;\n\n");
	    fs_decl.print("optional(0)");
	};
    };



    HP("public:\n\n");

    string itype;
    string initializer;

    for(auto &f : z.get_fields())
    {
	if(std_type_def(f.get_vtype(), itype, initializer))
	{
	    if(f.is_single())
	    {
		if(!initializer.empty())
		{
		    HP(TAB "%s %s;", itype.c_str(), f.get_name().c_str());
		    fs_decl.coma();
		    fs_decl.print("%s(%s)", f.get_name().c_str(), initializer.c_str());
		}
		else
		    HP(TAB "%s %s;", itype.c_str(), f.get_name().c_str());
		if(f.is_required())
		    HP("\n");
		else
		    HP("  // optional\n");
		continue;
	    };
	}
	else
	    itype = f.get_vtype();
	
	if(f.is_single())
	{
	    if(f.link && f.link->get_type() == decl_item::Enum)
	    {
		string p_name, c_name;
		gs->st.split_name(f.get_vtype(), p_name, c_name);
		string package_prefix;
		if(p_name != current_package)
		    package_prefix = p_name + "::";
		string init_enum_item = package_prefix + ((decl_enum*)f.link)->get_items()[0].name;
		
		HP(TAB "%s %s;", itype.c_str(), f.get_name().c_str());
		fs_decl.coma();
		fs_decl.print("%s(%s)", f.get_name().c_str(), init_enum_item.c_str());
	    }
	    else
		HP(TAB "%s %s;", itype.c_str(), f.get_name().c_str());
	}
	else
	    HP(TAB "std::vector<%s> %s;", itype.c_str(), f.get_name().c_str());

	if(f.is_required())
	    HP("\n");
	else
	    HP("  // optional\n");
    };


    // Constructor
    HP("\n\n" TAB "%s();\n\n", z.get_name().c_str());
    CP("\n%s::%s()", z.get_name().c_str(), z.get_name().c_str());
    if(!fs_decl.empty())
	CP(" : %s", fs_decl.c_str());

    CP("\n{\n");
    if(!fs_constr.empty())
	CP(TAB "%s", fs_constr.c_str());
    CP("};\n\n");


    // Optional (functions)
    for(auto pf : r.optional)
    {
	string s = "i_" + pf->get_name();

	HP(TAB "bool has_%s(void) const { return i_has(%s); };\n", pf->get_name().c_str(), s.c_str());
	HP(TAB "void set_%s(void) { i_set(%s); };\n", pf->get_name().c_str(), s.c_str());
	HP(TAB "void clear_%s(void) { i_clr(%s); };\n", pf->get_name().c_str(), s.c_str());
	HP("\n");
    };


    // Serialize
    
    HP(TAB "virtual void serialize(std::string &__out_buf) const;\n");
    CP("void %s::serialize(std::string &__out_buf) const\n{\n", z.get_name().c_str());

    CP(TAB "betula::u_std_types __u;\n\n");
    
    if(opt_size)
    {
	if(opt_size > 1)
	    CP(TAB "__out_buf.append((const char*)optional, %u);\n\n", opt_size);
	else
	    CP(TAB "__out_buf.append((const char*)&optional, 1);\n\n");
    };

    for(auto &f : z.get_fields())
    {
	const string &fs_type = f.get_vtype();
	const char* f_type = f.get_vtype().c_str();
	const char* f_name = f.get_name().c_str();
	CP(TAB "// %s\n", f_name);

	bool is_optional = !f.is_required();
	
	int tab_index = 0;
	const char *tab = tabs[tab_index];

	if(is_optional)
	{
	    tab = tabs[++tab_index];
	    CP(TAB "if( has_%s() ) {\n", f_name);
	};
	
	if(f.is_single())
	{
	    if(std_typenames.find(f.get_vtype()) != std_typenames.end())
	    {
		if(f.get_vtype() == "string")
		{
		    CP("%s__u.uint32_f = betula::hton_uint32(%s.length());\n", tab, f_name);
		    CP("%s__out_buf.append(__u.c, 4);\n", tab);
		    CP("%s__out_buf.append(%s);\n", tab, f_name);
		}
		else
		{
		    if(fs_type[fs_type.length()-1] == '8')
			CP("%s__out_buf.append((const char*)&%s, 1);\n", tab, f_name);
		    else
		    {
			CP("%s__u.%s_f = betula::hton_%s(%s);\n", tab, f_type, f_type, f_name);
			CP("%s__out_buf.append(__u.c, sizeof(betula::%s));\n", tab, f_type);
		    };
		};
	    }
	    else
	    {
		if(f.link && f.link->get_type() == decl_item::Enum)
		{
		    const decl_enum &e = *(decl_enum*) f.link;
		    if(e.b_size > 1)
			CP("%s__u.%s_f = betula::hton_%s(%s);", tab, e.s_type.c_str(), e.s_type.c_str(), f_name);
		    else
			CP("%s__u.%s_f = %s;", tab, e.s_type.c_str(), f_name);
		    CP("\t//enum %s\n", f_type);
		    CP("%s__out_buf.append(__u.c, sizeof(betula::%s));\n", tab, e.s_type.c_str());
		}
		else
		{
		    CP("%s%s.serialize(__out_buf);\n", tab, f_name);
		};
	    };
	}
	else
	{   // array
	    CP("%s__u.uint32_f = betula::hton_uint32(%s.size());\n", tab, f_name);
	    CP("%s__out_buf.append(__u.c, 4);\n", tab);
	    
	    if(std_typenames.find(f.get_vtype()) != std_typenames.end())
	    {

		if(f.get_vtype() == "string")
		{
		    CP("%sfor(std::vector<std::string>::const_iterator __x=%s.begin(); __x!=%s.end(); ++__x) {\n", tab, f_name, f_name);
		    tab = tabs[++tab_index];
		    CP("%s__u.uint32_f = betula::hton_uint32(__x->length());\n", tab);
		    CP("%s__out_buf.append(__u.c, 4);\n", tab);
		    CP("%s__out_buf.append(*__x);\n", tab);
		    tab = tabs[--tab_index];
		    CP("%s};\n", tab);
		}
		else
		{
		    if(fs_type[fs_type.length()-1] == '8')
			CP("%s__out_buf.append((const char*)%s.data(), %s.size());\n", tab, f_name, f_name);
		    else
		    {
			CP("#ifdef BETULA_NETWORK_ORDER\n");
			CP("%s__out_buf.append((const char*)%s.data(), %s.size()*sizeof(betula::%s));\n", tab, f_name, f_name, f_type);
			CP("#else\n");
			CP("%sfor(std::vector<betula::%s>::const_iterator __x=%s.begin(); __x!=%s.end(); ++__x) {\n", tab, f_type, f_name, f_name);
			tab = tabs[++tab_index];
			CP("%s__u.%s_f = betula::hton_%s(*__x);\n", tab, f_type, f_type);
			CP("%s__out_buf.append(__u.c, sizeof(betula::%s));\n", tab, f_type);
			tab = tabs[--tab_index];
			CP("%s};\n", tab);
			CP("#endif\n");
		    };
		};
	    }
	    else
	    {
		if(f.link && f.link->get_type() == decl_item::Enum)
		{
		    const decl_enum &e = *(decl_enum*) f.link;
		    CP("%sfor(std::vector<%s>::const_iterator __x=%s.begin(); __x!=%s.end(); ++__x) {\n", tab, f_type, f_name, f_name);
		    tab = tabs[++tab_index];
		    
		    if(e.b_size > 1)
			CP("%s__u.%s_f = betula::hton_%s(*__x);", tab, e.s_type.c_str(), e.s_type.c_str());
		    else
			CP("%s__u.%s_f = *__x;", tab, e.s_type.c_str());
		    CP("\t//enum %s\n", f_type);
		    CP("%s__out_buf.append(__u.c, sizeof(betula::%s));\n", tab, e.s_type.c_str());
		    
		    tab = tabs[--tab_index];
		    CP("%s};\n", tab);
		}
		else
		{
		    CP("%sfor(std::vector<%s>::const_iterator __x=%s.begin(); __x!=%s.end(); ++__x)\n", tab, f_type, f_name, f_name);
		    tab = tabs[++tab_index];
		    CP("%s__x->serialize(__out_buf);\n", tab+1);
		    tab = tabs[--tab_index];
		};
	    };
	};
	
	if(is_optional)
	    CP(TAB "};\n");
	CP("\n");
    };

    CP("};\n\n"); // Serialize



    // Unserialize
    
    HP(TAB "virtual bool unserialize(const char* &__buffer, long &__buffer_len);\n");
    CP("bool %s::unserialize(const char* &__buffer, long &__buffer_len)\n{\n", z.get_name().c_str());

    CP(TAB "betula::u_std_types __u;\n\n");
    bool def_vsize = false;

    if(opt_size)
    {
	CP(TAB "CHECK(%u);\n", opt_size);
	if(opt_size > 1)
	{
	    CP(TAB "memcpy(optional, __buffer, %u);\n", opt_size);
	    CP(TAB "MOVE(%u);\n\n", opt_size);
	}
	else
	{
	    CP(TAB "optional = (betula::uint8) *__buffer++;\n");
	    CP(TAB "__buffer_len--;\n\n");
	};
    };


    for(auto &f : z.get_fields())
    {
	const char* f_type = f.get_vtype().c_str();
	const char* f_name = f.get_name().c_str();

	bool is_optional = !f.is_required();
	
	int tab_index = 0;
	const char *tab = tabs[tab_index];

	if(!f.is_single() && !def_vsize)
	{
	    CP(TAB "unsigned long __vsize;\n\n");
	    def_vsize = true;
	};

	CP(TAB "// %s\n", f_name);
//	CP("#ifdef BETULA_DEBUG\n");
//	CP(TAB "std::cout << \"US: %s::%s\" << std::endl;\n", z.get_name().c_str(), f_name);
//	CP("#endif\n");

	if(is_optional)
	{
	    tab = tabs[++tab_index];
	    CP(TAB "if( has_%s() ) {\n", f_name);
	};

	if(f.is_single())
	{
	    if(std_typenames.find(f.get_vtype()) != std_typenames.end())
	    {
		if(f.get_vtype() == "string")
		{
		    CP("%sCHECK(4);\n", tab);
		    CP("%s__u.uint32_f = VGET(uint32);\n", tab);
		    CP("%sMOVE(4);\n", tab);
		    CP("%sCHECK(__u.uint32_f);\n", tab);
		    CP("%s%s.append(__buffer, __u.uint32_f);\n", tab, f_name);
		    CP("%sMOVE(__u.uint32_f);\n", tab);
		}
		else
		{
		    CP("%sCHECK(sizeof(betula::%s));\n", tab, f_type);
		    CP("%s%s = VGET(%s);\n", tab, f_name, f_type);
		    CP("%sMOVE(sizeof(betula::%s));\n", tab, f_type);
		};
	    }
	    else
	    {
		if(f.link && f.link->get_type() == decl_item::Enum)
		{
		    const decl_enum &e = *(decl_enum*) f.link;
		    CP("%sCHECK(sizeof(betula::%s));\n", tab, e.s_type.c_str());
		    CP("%s%s = (%s) VGET(%s);", tab, f_name, f_type, e.s_type.c_str());
		    CP("\t//enum %s\n", f_type);
		    CP("%sMOVE(sizeof(betula::%s));\n", tab, e.s_type.c_str());
		    
		}
		else
		{
		    CP("%sif(!%s.unserialize(__buffer, __buffer_len))\n", tab, f_name);
		    CP("%sreturn false;\n", tabs[tab_index+1]);
		};
	    };
	}
	else
	{   // array
	    CP("%sCHECK(4);\n", tab);
	    CP("%s__vsize = VGET(uint32);\n", tab);
	    CP("%sMOVE(4);\n", tab);
	    CP("%s%s.reserve(__vsize);\n", tab, f_name);
	    
	    if(std_typenames.find(f.get_vtype()) != std_typenames.end())
	    {
		if(f.get_vtype() == "string")
		{
		    CP("%swhile(__vsize--) {\n", tab);
		    tab = tabs[++tab_index];
		    		    
		    CP("%sCHECK(4);\n", tab);
		    CP("%s__u.uint32_f = VGET(uint32);\n", tab);
		    CP("%sMOVE(4);\n", tab);
		    CP("%sCHECK(__u.uint32_f);\n", tab);
		    CP("%s%s.push_back(std::string());\n", tab, f_name);
		    CP("%s%s.back().append(__buffer, __u.uint32_f);\n", tab, f_name);
		    CP("%sMOVE(__u.uint32_f);\n", tab);

		    tab = tabs[--tab_index];
		    CP("%s};\n", tab);
		}
		else
		{
		    CP("%sdo {\n", tab);
		    tab = tabs[++tab_index];
		    CP("%sunsigned int __data_size = __vsize*sizeof(betula::%s);\n", tab, f_type);
		    CP("%sCHECK(__data_size);\n", tab);

		    CP("%swhile(__vsize--) {\n", tab);
		    tab = tabs[++tab_index];
			CP("%s%s.push_back(VGET(%s));\n", tab, f_name, f_type);
			CP("%s__buffer += sizeof(betula::%s);\n", tab, f_type);
		    tab = tabs[--tab_index];
		    CP("%s};\n", tab);

		    CP("%s__buffer_len -= __data_size;\n", tab);
		    tab = tabs[--tab_index];
		    CP("%s}while(0);\n", tab);

		};
	    }
	    else
	    {
		if(f.link && f.link->get_type() == decl_item::Enum)
		{
		    const decl_enum &e = *(decl_enum*) f.link;
		    CP("%sdo {\n", tab);
		    tab = tabs[++tab_index];
		    CP("%sunsigned int __data_size = __vsize*sizeof(betula::%s);\n", tab, e.s_type.c_str());

		    CP("%sCHECK(__data_size);\n", tab);

			CP("%swhile(__vsize--) {\n", tab);
			tab = tabs[++tab_index];
			    CP("%s%s.push_back((%s) VGET(%s));", tab, f_name, f_type, e.s_type.c_str());
			    CP("\t//enum %s\n", f_type);
			    CP("%s__buffer += sizeof(betula::%s);\n", tab, e.s_type.c_str());
			tab = tabs[--tab_index];
			CP("%s};\n", tab);

		    CP("%s__buffer_len -= __data_size;\n", tab);
		    tab = tabs[--tab_index];
		    CP("%s}while(0);\n", tab);

		    
		}
		else
		{
		    CP("%swhile(__vsize--) {\n", tab);
		    tab = tabs[++tab_index];
			CP("%s%s.push_back(%s());\n", tab, f_name, f_type);
			CP("%sif(!%s.back().unserialize(__buffer, __buffer_len))\n", tab, f_name);
			CP("%sreturn false;\n", tabs[tab_index+1]);
		    tab = tabs[--tab_index];
		    CP("%s};\n", tab);
		};
	    };
	};



	if(is_optional)
	    CP(TAB "};\n");

	CP("\n");

    };

    CP(TAB "return true;\n");
    CP("};\n\n\n"); // Unserialize



    // DebugPrint

    HP(TAB "virtual void debug_print(std::ostream &__dest, int __level=0) const;\n");
    CP("void %s::debug_print(std::ostream &__dest, int __level) const\n{\n", z.get_name().c_str());

    CP(TAB "betula::print_tab(__dest, __level++) << \"message %s\" << std::endl;\n", z.get_name().c_str());

    for(auto &f : z.get_fields())
    {
	const string &fs_type = f.get_vtype();
	const char* f_type = f.get_vtype().c_str();
	const char* f_name = f.get_name().c_str();

	bool is_optional = !f.is_required();
	
	int tab_index = 0;
	const char *tab = tabs[tab_index];

	CP(TAB "// %s\n", f_name);

	if(is_optional)
	{
	    tab = tabs[++tab_index];
	    CP(TAB "if( has_%s() ) {\n", f_name);
	};

	if(std_typenames.find(f.get_vtype()) != std_typenames.end())
	{
	    if(f.is_single())
	    {
		if(f.get_vtype().back() == '8')
		{
		    const char *type_mod = (f_type[0]=='u') ? "unsigned int" : "int";
		    CP("%sbetula::print_fname(betula::print_tab(__dest, __level), \"%s\") << (%s)%s << std::endl;\n", tab, f_name, type_mod, f_name);
		}
		else
		    CP("%sbetula::print_fname(betula::print_tab(__dest, __level), \"%s\") << %s << std::endl;\n", tab, f_name, f_name);
	    }
	    else
	    {
		CP("%sbetula::print_fname(betula::print_tab(__dest, __level), \"%s\");\n", tab, f_name);
		
		string tp = (fs_type == "string") ? string("std::string") : string("betula::" + fs_type);
		CP("%sfor(std::vector<%s>::const_iterator __x=%s.begin(); __x!=%s.end(); ++__x)\n", tab, tp.c_str(), f_name, f_name);
		if(f.get_vtype().back() == '8')
		{
		    const char *type_mod = (f_type[0]=='u') ? "unsigned int" : "int";
		    CP(TAB "%s__dest << ' ' << (%s)*__x;\n", tab, type_mod);
		}
		else
		    CP(TAB "%s__dest << ' ' << *__x;\n", tab);
		    
		CP("%s__dest << std::endl;\n", tab);
	    };
	}
	else
	{
	    if(f.link && f.link->get_type() == decl_item::Enum)
	    {
		const decl_enum &e = *(decl_enum*) f.link;
		CP("%sbetula::print_fname(betula::print_tab(__dest, __level), \"ENUM %s\");\n", tab, f_name);
		const char* nm = f_name;
		if(!f.is_single())
		{
		    nm = "*__x";
		    CP("%sfor(std::vector<%s>::const_iterator __x=%s.begin(); __x!=%s.end(); ++__x)\n", tab, f_type, f_name, f_name);
		    tab = tabs[++tab_index];
		};

		string p_name, c_name;
		gs->st.split_name(f.get_vtype(), p_name, c_name);
		string package_prefix;
		if(p_name != current_package)
		    package_prefix = p_name + "::";
		
		CP("%sswitch((betula::int32)%s) {\n", tab, nm);
		tab = tabs[++tab_index];
		for(auto &t : e.get_items())
		{
		    string xt = package_prefix + t.name;
		    CP("%scase %s: __dest << ' ' << \"%s\"; break;\n", tab, xt.c_str(), xt.c_str());
		};
		CP("%sdefault: __dest << ' ' << \"WRONG(\" << (betula::int32)%s << \")\";\n", tab, nm);
		tab = tabs[--tab_index];
		CP("%s};\n", tab);

		if(!f.is_single())
		    tab = tabs[--tab_index];

		CP("%s__dest << std::endl;\n", tab);
	    }
	    else
	    {
		CP("%sbetula::print_fname(betula::print_tab(__dest, __level), \"MESSAGE %s\");\n", tab, f_name);
		CP("%s__dest << std::endl;\n", tab);
		const char *nm = f_name;
		if(!f.is_single())
		{
		    nm = "(*__x)";
		    CP("%sfor(std::vector<%s>::const_iterator __x=%s.begin(); __x!=%s.end(); ++__x)\n", tab, f_type, f_name, f_name);
		    tab = tabs[++tab_index];
		};
		CP("%s%s.debug_print(__dest, __level+1);\n", tab, nm);
		
		if(!f.is_single())
		    tab = tabs[--tab_index];
		
	    };
	};


	if(is_optional)
	    CP(TAB "};\n");
	CP("\n");
	
    };

    CP("};\n\n"); // DebugPrint




    HP("};\n\n"); // Message (header)
};


bool codegen_cpp::std_type_def(const string &type, string &itype, string &initializer) const
{
    if(std_typenames.find(type) == std_typenames.end())
	return false;

    initializer.erase();

    if(type == "string")
    {
	itype = "std::string";
    }
    else
    if(std_float_typenames.find(type) != std_float_typenames.end())
    {
	itype = type;
	initializer = "0.0";
    }
    else
    {
	itype = "betula::" + type;
	initializer = "0";
    };

    return true;
};



// ----------------------
// ----------------------

codegen* get_codegen_cpp(void)
{
    return new codegen_cpp;
};