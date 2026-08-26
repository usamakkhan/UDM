#pragma once
#include "Core.hpp"
namespace udm {
struct Field {unsigned id=0,wire=0;unsigned long long number=0;Bytes data;};
struct Proto {
 std::vector<Field> fields;
 static Proto parse(const Bytes&);
 Bytes encode()const;
 bool has(unsigned)const;
 unsigned long long number(unsigned,unsigned long long fallback=0)const;
 Bytes data(unsigned)const;
 std::string text(unsigned)const;
 Proto& remove(std::initializer_list<unsigned>);
 Proto& add(unsigned,const Bytes&);
 Proto& set(unsigned,const Bytes&);
 Proto& set(unsigned,const std::string&);
 Proto& set(unsigned,unsigned long long);
 Proto& floating(unsigned,float);
 std::vector<unsigned long long> numbers(unsigned)const;
};
unsigned umpInteger(const Bytes&);
bool sameFormat(const Bytes&,const Bytes&);
Bytes formatIdentity(const Json&);
}
