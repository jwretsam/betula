
#include "betula.h"

namespace betula {


std::ostream& print_tab(std::ostream &out, int level)
{
   while(level-- > 0)
      out << "   ";
   return out;
};


std::ostream& print_fname(std::ostream &out, std::string field_name)
{     
   out << field_name;
   int x = 8 - (int)field_name.length();
   while(x-- > 0)
      out << ' ';
   out << ": ";
   return out;
};









}; // namespace betula
