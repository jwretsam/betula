
#ifndef __SYNTAX_TREE_H
#define __SYNTAX_TREE_H

#include <sys/types.h>

#include <memory>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <stack>

namespace bpl {

#define Z (*((bpl::parser_data*)lp))

// ----------------------

typedef std::vector<std::string> v_string;

// ----------------------

class decl_item
{
public:

    enum Type { Enum, Message, Package };

protected:

    std::string name;

    Type type = Enum;

public:

    decl_item() = delete;

    decl_item(const std::string &a_name, Type a_type) : name(a_name), type(a_type) { };
    virtual ~decl_item() { };

    virtual void build_meta(void) { };


    const std::string& get_name(void) const { return name; };
    Type get_type(void) const { return type; };
};

typedef std::unique_ptr<decl_item> p_decl_item;
typedef std::vector<p_decl_item> vp_decl_item;


// ----------------------

class decl_enum : public decl_item
{
public:

    struct item
    {
	std::string name;
	int value;

	item() = delete;	
	item(const std::string &a_name, int a_value) : name(a_name), value(a_value) { };
    };

    typedef std::vector<item> v_item;

private:
    v_item items;

    int top_val = 0;

public:

    int min_val = 0;
    int max_val = 0;
    int b_size = 0;
    bool is_unsigned = false;
    std::string s_type;

    decl_enum(const std::string &a_name) : decl_item(a_name, decl_item::Enum) { };

    const v_item& get_items(void) const { return items; };

    bool add_item(const std::string &a_name, int value);
    bool add_item(const std::string &a_name) { return add_item(a_name, top_val); };

    bool get_item(const std::string &a_name, int &value) const;

    virtual void build_meta(void);

};


// ----------------------

class decl_message : public decl_item
{
public:

    enum { NO_RANGE = -1 };

    struct init_val
    {
	enum Type { Int, Float, String, Id };
	
	Type type;
	int64_t i;
	double f;
	std::string s;
    };

    typedef std::vector<init_val> v_init_list;

    class field
    {
    private:
    
	std::string vtype;
	std::string name;
	bool required = true;
    
	int range[2] = { 1, 1 };

	v_init_list init_list;


    public:

	decl_item *link = 0;
    
	field(const std::string &a_type, const std::string &a_name, bool a_required) : vtype(a_type), name(a_name), required(a_required) { };

	const std::string& get_vtype(void) const { return vtype; };
	const std::string& get_name(void) const { return name; };

	bool set_range(int r_min, int r_max);
	int get_range_min(void) const { return range[0]; };
	int get_range_max(void) const { return range[0]; };

	bool is_single(void) const { return (range[0] == range[1]) && (range[0] == 1); };

	bool is_required(void) const { return required; };
	
	void add_init_value(int64_t val);
	void add_init_value(double val);
	void add_init_value(const std::string &val);
	void add_init_value_id(const std::string &val);
	
	const v_init_list& get_init_list(void) const { return init_list; };
	
    };

    typedef std::vector<field> v_field;

    typedef std::vector<const field*> vp_field;
    
    struct field_ref
    {
	vp_field required;
	vp_field optional;
    };
    // ------------------

    // ------------------

private:

    v_field fields;

public:



    decl_message(const std::string &a_name) : decl_item(a_name, decl_item::Message) { };

    const v_field& get_fields(void) const { return fields; };

    void get_field_ref(field_ref &r) const;

    const field* find_field(const std::string &a_name) const;
    
    field* add_field(const std::string &a_type, const std::string &a_name, bool a_required);
};


// ----------------------
class decl_package : public decl_item
{
public:


private:

    std::string filename;
    
    vp_decl_item decl;


public:

    decl_package(const std::string &a_name, const std::string &a_filename) : decl_item(a_name, decl_item::Package), filename(a_filename) { };

    const std::string& get_filename(void) const { return filename; };

    const vp_decl_item& get_decl(void) const { return decl; };

    decl_item* find(const std::string &a_name) const;

    decl_enum* add_enum(const std::string &a_name);
    decl_message* add_message(const std::string &a_name);

    bool find_enum_item(const std::string &a_name, int *value=0) const;

};

// ----------------------

class file_item
{
private:

    std::string name;
    v_string import_list;

public:

    file_item() { };

    const v_string& get_import_list(void) const { return import_list; };

    void add_import_file(const std::string &filename);
};

// ----------------------



// ----------------------

class syntax_tree
{
public:

    typedef std::map<std::string, file_item> m_files;

    struct out_module
    {
	std::vector<decl_package*> packages;
	std::string name;
	v_string import_list;

	static std::string extract_name(std::string &filename);
    };

private:

    m_files files;

    vp_decl_item packages;

    std::string current_file;
    std::string main_file;

public:

    syntax_tree(const std::string &filename) { set_current_file(filename); };

    void set_current_file(const std::string &filename);

    const vp_decl_item& get_packages(void) const { return packages; };

    decl_package* add_package(const std::string &name);

    decl_package* find_package(const std::string &name) const;

    void add_import(const std::string &filename);

    bool type_exists(const std::string &a_type) const;
    decl_item* find_typename_item(const std::string &a_type) const;

    bool is_enum_initializer(const std::string &type, const std::string &init_value) const;

    void split_name(const std::string &src_name, std::string &package, std::string &name) const;

    void get_out_module(out_module &m);
};


// ----------------------

typedef std::set<std::string> set_string;

extern set_string std_typenames;
extern set_string std_int_typenames;
extern set_string std_float_typenames;


// ----------------------


class parser_data
{
private:


    set_string typenames;

    std::string current_file = "STDIN";
    std::set<std::string> last_files;
    std::stack<std::string> stack_files;


public:
    syntax_tree &st;

    std::string package_name;
    decl_package *package = 0;

    std::string t_include_file;
    
    decl_enum *t_enum = 0;
    decl_message *t_message = 0;
    decl_message::field *t_field = 0;
    
    bool t_is_required = true;

    parser_data() = delete;
    parser_data(syntax_tree &a_st);

    void push_current_file(std::string filename);
    std::string pop_current_file(void);
    const std::string& get_current_file(void) const { return current_file; };
    bool is_last_file(const std::string &filename) const;

    bool is_typename(const std::string & id) const { return typenames.find(full_id_name(id)) != typenames.end(); };
    bool add_typename(const std::string & id);

    std::string full_id_name(const std::string &name) const;

private:

    std::string extract_filename(const std::string &filepath) const;

};

// ----------------------





// ----------------------



// ----------------------



}; // namespace bpl

#endif
