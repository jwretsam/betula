#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "syntax_tree.h"

using namespace std;

namespace bpl {

// ----------------------
set_string std_typenames;
set_string std_int_typenames;
set_string std_float_typenames;

parser_data::parser_data(syntax_tree &a_st) : st(a_st)
{
    typenames.insert("int8");
    typenames.insert("sint8");
    typenames.insert("uint8");

    typenames.insert("int16");
    typenames.insert("sint16");
    typenames.insert("uint16");

    typenames.insert("int32");
    typenames.insert("sint32");
    typenames.insert("uint32");

    typenames.insert("int64");
    typenames.insert("sint64");
    typenames.insert("uint64");

    std_int_typenames = typenames;

    typenames.insert("float");
    typenames.insert("double");

    std_float_typenames.insert("float");
    std_float_typenames.insert("double");


    typenames.insert("string");

    std_typenames = typenames;
};


bool parser_data::add_typename(const std::string & id)
{
    string nm = full_id_name(id);
    if(is_typename(nm))
	return false;

    typenames.insert(nm);
//printf("   ADD: %s\n", nm.c_str());
    return true;
};

std::string parser_data::full_id_name(const std::string &name) const
{
    if(std_typenames.find(name) != std_typenames.end())
	return name;

    if(strstr(name.c_str(), "::"))
	return name;

    string s = string(package_name + "::" + name);
//printf("   FULLN: %s\n", s.c_str());
    return s;
};


std::string parser_data::extract_filename(const std::string &filepath) const
{
    const char *p = strrchr(filepath.c_str(), '/');
    return  p ? string(p+1) : filepath;
};


void parser_data::push_current_file(std::string filename)
{
    if(!current_file.empty())
	stack_files.push(current_file);

    string s = extract_filename(filename);
    st.add_import(s);
    current_file = s;
    st.set_current_file(current_file);
    last_files.insert(s);
};

std::string parser_data::pop_current_file(void)
{
    current_file.erase();
    if(!stack_files.empty())
    {
	current_file = stack_files.top();
	stack_files.pop();
	st.set_current_file(current_file);
    };

    return current_file;
};

bool parser_data::is_last_file(const std::string &filename) const
{
    return last_files.find(extract_filename(filename)) != last_files.end();
};

// ----------------------

bool decl_enum::add_item(const std::string &a_name, int value)
{
    for(auto x: items)
	if(a_name == x.name)
	    return false;

    items.push_back(item(a_name, value));
    top_val = value + 1;
    return true;
};

bool decl_enum::get_item(const std::string &a_name, int &value) const
{
    for(auto x: items)
	if(a_name == x.name)
	{
	    value = x.value;
	    return true;
	};

    return false;
};

void decl_enum::build_meta(void)
{
    max_val = INT_MIN;
    min_val = INT_MAX;

    for(auto &x : items)
    {
	if(x.value > max_val)
	    max_val = x.value;
	
	if(x.value < min_val)
	    min_val = x.value;
    };


    if(min_val >= 0 && max_val <= UCHAR_MAX)
    {
	is_unsigned = true;
	b_size = 1;
	s_type = "uint8";
    }
    else
    if(min_val >= SCHAR_MIN && max_val <= SCHAR_MAX)
    {
	is_unsigned = false;
	b_size = 1;
	s_type = "int8";
    }
    else
    if(min_val >= 0 && max_val <= USHRT_MAX)
    {
	is_unsigned = true;
	b_size = 2;
	s_type = "uint16";
    }
    else
    if(min_val >= SHRT_MIN && max_val <= SHRT_MAX)
    {
	is_unsigned = false;
	b_size = 2;
	s_type = "int16";
    }
    else
    {
	is_unsigned = false;
	b_size = 4;
	s_type = "int32";
    };
};


// ----------------------

bool decl_message::field::set_range(int r_min, int r_max)
{
    range[0] = r_min;
    range[1] = r_max;

    if(!is_single())
    {
	if(range[0] < NO_RANGE || range[1] < NO_RANGE)
	    return false;

	if(range[0] != -1 && range[1] != -1)
	    if(range[0] > range[1])
		return false;
    };
    
    return true;
};

void decl_message::field::add_init_value(int64_t val)
{
    decl_message::init_val v;
    v.type = decl_message::init_val::Int;
    v.i = val;
    init_list.push_back(v);
};

void decl_message::field::add_init_value(double val)
{
    decl_message::init_val v;
    v.type = decl_message::init_val::Float;
    v.f = val;
    init_list.push_back(v);
};

void decl_message::field::add_init_value(const std::string &val)
{
    decl_message::init_val v;
    v.type = decl_message::init_val::String;
    v.s = val;
    init_list.push_back(v);
};

void decl_message::field::add_init_value_id(const std::string &val)
{
    decl_message::init_val v;
    v.type = decl_message::init_val::Id;
    v.s = val;
    init_list.push_back(v);
};


const decl_message::field* decl_message::find_field(const std::string &a_name) const
{
    for(auto &f: fields)
	if(f.get_name() == a_name)
	    return &f;
    return 0;
};

decl_message::field* decl_message::add_field(const std::string &a_type, const std::string &a_name, bool a_required)
{
    if(find_field(a_name))
	return 0;

    fields.push_back(field(a_type, a_name, a_required));
    return &fields[fields.size()-1];
};

void decl_message::get_field_ref(decl_message::field_ref &r) const
{
    r.required.clear();
    r.optional.clear();

    for(auto &f : fields)
    {
	const field *p = &f;

	if(p->is_required())
	    r.required.push_back(p);
	else
	    r.optional.push_back(p);
    };
};
// ----------------------

decl_item* decl_package::find(const std::string &a_name) const
{
    for(auto &x : decl)
	if(x->get_name() == a_name)
	    return x.get();

    return 0;
};


decl_enum* decl_package::add_enum(const std::string &a_name)
{
    if(find(a_name))
	return 0;

    decl.push_back(std::unique_ptr<decl_item>(new decl_enum(a_name)));
    return (decl_enum*) decl[decl.size()-1].get();
};

decl_message* decl_package::add_message(const std::string &a_name)
{
    if(find(a_name))
	return 0;

    decl.push_back(std::unique_ptr<decl_item>(new decl_message(a_name)));
    return (decl_message*) decl[decl.size()-1].get();
};
    
    
bool decl_package::find_enum_item(const std::string &a_name, int *value) const
{
    int val;

    for(auto &x : decl)
	if(x->get_type() == decl_item::Enum)
	    if( ((decl_enum*)x.get())->get_item(a_name, val))
	    {
		if(value)
		    *value = val;
		return true;
	    };
    return false;
};

// ----------------------

void file_item::add_import_file(const std::string &filename)
{
    for(auto &x : import_list)
	if(filename == x)
	    return;
    import_list.push_back(filename);
};

// ----------------------

void syntax_tree::set_current_file(const std::string &filename)
{
    current_file = filename;
    if(main_file.empty())
	main_file = current_file;

};

decl_package* syntax_tree::add_package(const std::string &name)
{
    for(auto &x : packages)
	if(name == x->get_name())
	    return 0;

    packages.push_back(std::unique_ptr<decl_item>(new decl_package(name, current_file)));
    return (decl_package*)packages[packages.size()-1].get();
};

void syntax_tree::add_import(const std::string &filename)
{
    file_item &f = files[current_file];
    f.add_import_file(filename);
};

bool syntax_tree::type_exists(const std::string &a_type) const
{
    if(std_typenames.find(a_type) != std_typenames.end())
	return true;

    string p, n;
    split_name(a_type, p, n);

    decl_package *c_package = find_package(p);
/*    for(auto &x : packages)
	if(x->get_name() == p)
	{
	    c_package = (decl_package*) x.get();
	    break;
	};*/

    if(!c_package)
	return false;


    return (c_package->find(n) != 0);
};


decl_item* syntax_tree::find_typename_item(const std::string &a_type) const
{
    if(std_typenames.find(a_type) != std_typenames.end())
	return 0;

    string p, n;
    split_name(a_type, p, n);

    decl_package *c_package = find_package(p);
/*    for(auto &x : packages)
	if(x->get_name() == p)
	{
	    c_package = (decl_package*) x.get();
	    break;
	};*/

    if(!c_package)
	return 0;


    return c_package->find(n);
};


decl_package* syntax_tree::find_package(const std::string &name) const
{
    decl_package *c_package = 0;
    for(auto &x : packages)
	if(x->get_name() == name)
	{
	    c_package = (decl_package*) x.get();
	    break;
	};
    return c_package;
};

void syntax_tree::split_name(const std::string &src_name, std::string &package, std::string &name) const
{
    package.erase();
    name.erase();
    const char *p = src_name.c_str();
    const char *q = strstr(p, "::");
    if(q)
    {
	package.append(p, q-p);
	name = q+2;
    }
    else
    {
	package = ((decl_package*)(packages[packages.size()-1].get()))->get_name();
	name = src_name;
    };
};


bool syntax_tree::is_enum_initializer(const std::string &type, const std::string &init_value) const
{
    string e_package, e_name;
    split_name(type, e_package, e_name);

    string v_package, v_name;
    split_name(init_value, v_package, v_name);

    if(e_package != v_package)
	return false;

    decl_package *c_package = 0;
    for(auto &x : packages)
	if(x->get_name() == e_package)
	{
	    c_package = (decl_package*) x.get();
	    break;
	};

    if(!c_package)
	return false;

    decl_enum *e = (decl_enum*) c_package->find(e_name);
    if(!e || e->get_type() != decl_item::Enum)
	return false;

    int val;
    return e->get_item(v_name, val);
};


void syntax_tree::get_out_module(syntax_tree::out_module &m)
{
    m.packages.clear();

    m.name = main_file;
    m.import_list = files[main_file].get_import_list();

    for(auto &x : packages)
    {
	decl_package *p = (decl_package*) x.get();
	if(p->get_filename() == main_file)
	    m.packages.push_back(p);
    };
};

// ----------------------

std::string syntax_tree::out_module::extract_name(string &filename)
{
    string s;
    const char *p = filename.c_str();
    const char *q = strstr(p, ".bpl");

    if(q)
	s.append(p, q-p);
    else
	s = filename;
    return s;
};


// ----------------------


}; // namespace bpl
