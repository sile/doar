#ifndef WAKAME_UNKNOWN_H
#define WAKAME_UNKNOWN_H

#include "morpheme_node.h"
#include "char_category.h"
#include "word_dic.h"
#include <string>

namespace Wakame {
  class Unknown {
  public:
    Unknown(const char* dir) 
      : tbl((std::string(dir)+"/char-category.map").c_str())
    {

    }
    void search(const char* text, const WordDic& wd, MorphemeNodes& result) const {
      const char* cur=text;
      const CharCategory& cc = const_cast<CharCategory&>(tbl.category(utf8_to_ucs2(cur)));
      
      //cout << "id:"<<cc.id<<" inv:"<<cc.invoke<<" gr:"<<cc.group<<" len:"<<cc.length <<endl;
      //cout << text << endl;
      if(text[0]=='\0')
	return; // XXX:

      // 
      if(!result.empty() && !cc.invoke)
	return;
      
      //cout << "@@@@@@@@@@@@@@@" << endl;
      //
      bool b=true;
      // TODO: NULLチェックはなくしたい
      // XXX: この方法だと、例えばlength=2,group=0の文字が、textの末尾にある場合に、NULLチェックにより追加されなくなっていしまうのでは?(2010/03/08)
      for(unsigned i=0; *cur!='\0' && i < cc.length; i++) {
	wd.get_from_id(cc.id,cur-text,result);
	//cout << "@" << cur << endl;
	unsigned cid = tbl.category_id(utf8_to_ucs2(cur));
	if(cc.id != cid){
	  b=false;
	  break;
	}
      }

      // 
      if(cc.group && b && *cur!='\0') {
	while(*cur!='\0') {
	  //cout << "#" << endl;
	  const char* last=cur;
	  unsigned cid = tbl.category_id(utf8_to_ucs2(cur));
	  if(cc.id != cid){
	    //cout << "@@" << last << endl;
	    wd.get_from_id(cc.id,last-text,result);
	    return;
	  }
	}
	//cout << text << endl;
	//cout << "?" << cur-text << endl;
	wd.get_from_id(cc.id,cur-text,result);	
      }
    }

  private:
    const CharCategoryTable tbl;
  };
}

#endif
