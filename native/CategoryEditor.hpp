#pragma once
#include "Core.hpp"
namespace udm {
inline void saveCategoryProperties(Manager& manager,const std::string& original,const std::string& name,const std::string& extensions,const std::string& hosts,const std::string& folder,bool remember){
 Lock lock(manager.mutex);auto previous=manager.state["Settings"];
 try{manager.state["Settings"]["CategoryRememberLast"][original.empty()?name:original]=remember;manager.editCategory(original,name,extensions,hosts,folder);}
 catch(...){manager.state["Settings"].swap(previous);throw;}
}
}
