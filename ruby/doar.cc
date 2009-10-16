#include <ruby.h>
#include "../src/doar/searcher.h"
#include <string>

namespace {
  class Collect {
  public:
    Collect(const Doar::Searcher& sh)
      : srch(sh), ary(rb_ary_new()) {}
    
    void operator()(const char* key, unsigned offset, unsigned id) const {
      VALUE obj=rb_ary_new2(2);
      rb_ary_store(obj,0,INT2FIX(id));
      rb_ary_store(obj,1,INT2FIX(offset));
      
      rb_ary_push(ary, obj);
    }
    
    VALUE getArray() const { return ary; }
  private:
    const Doar::Searcher& srch;
    VALUE ary;
  };

  // common prefix search yield
  void cps_yield(const char*key, unsigned offset, unsigned id) {
    rb_yield_values(2,INT2FIX(id),INT2FIX(offset));
  }

  class EachAllKey {
  public:
    EachAllKey(const Doar::Searcher& sh)
      : srch(sh), len(0) {}

    void operator() (char c, Doar::Node parent) const {
      buf += c;
      if(parent.is_leaf()) {
	buf += srch.tail_ptr(parent); // XXX: tail_ptr
	rb_yield(rb_str_new2(buf.c_str()));
      } else {
	len++;
	srch.children(parent,*this);
	len--;
      }
      // XXX: std::stringでは小さくresizeしても確保したメモリが解放されないと仮定した処理
      //    : この仮定が満たされない場合でも、動作はするが、効率は悪くなる
      buf.resize(len);
    }
  private:
    const Doar::Searcher& srch;
    mutable std::string buf;
    mutable unsigned len;
  };
}

extern "C" {
  VALUE srch_new(VALUE klass, VALUE doar_data_file_path);
  VALUE srch_delete(Doar::Searcher *ptr);
  VALUE srch_bracket(VALUE self, VALUE key);
  VALUE srch_member(VALUE self, VALUE key);
  VALUE srch_common_prefix_search(VALUE self, VALUE key);
  VALUE srch_each_common_prefix(VALUE self, VALUE key);
  VALUE srch_each(VALUE self);
  VALUE srch_size(VALUE self);

  void Init_doar(void) {
    VALUE mdl = rb_define_module("Doar");
    
    /* TODO: 後で
    VALUE cTrie = rb_define_class_under(mdl,"Trie",rb_cObject);
    rb_define_singleton_method(cTrie,"new",(VALUE (*)(...))trie_new,0);
    rb_define_method(cTrie, "insert", (VALUE (*)(...))trie_insert, 1);
    rb_define_method(cTrie, "[]", (VALUE (*)(...))trie_bracket, 1);
    rb_define_method(cTrie, "member?", (VALUE (*)(...))trie_member, 1);
    rb_define_method(cTrie, "common_prefix_search", (VALUE (*)(...))trie_common_prefix_search, 1);
    rb_define_method(cTrie, "each_common_prefix", (VALUE (*)(...))trie_each_common_prefix, 1);
    rb_define_method(cTrie, "save", (VALUE (*)(...))trie_save,1);
    rb_define_method(cTrie, "load", (VALUE (*)(...))trie_load,1);
    rb_define_method(cTrie, "to_a", (VALUE (*)(...))trie_to_a, 0);
    */
    
    VALUE cSrch = rb_define_class_under(mdl,"Searcher",rb_cObject);
    rb_define_singleton_method(cSrch,"new",(VALUE (*)(...))srch_new,1);
    rb_define_method(cSrch, "[]", (VALUE (*)(...))srch_bracket, 1);
    rb_define_method(cSrch, "member?", (VALUE (*)(...))srch_member, 1);
    rb_define_method(cSrch, "common_prefix_search", (VALUE (*)(...))srch_common_prefix_search, 1);
    rb_define_method(cSrch, "each_common_prefix", (VALUE (*)(...))srch_each_common_prefix, 1);    
    rb_define_method(cSrch, "each", (VALUE (*)(...))srch_each, 0);
    rb_define_method(cSrch, "size", (VALUE (*)(...))srch_size, 0);
  }
  
  VALUE srch_new(VALUE klass, VALUE doar_data_file_path){
    Doar::Searcher* ptr;
    VALUE obj = Data_Make_Struct(klass,Doar::Searcher,NULL,srch_delete,ptr);
    new ((void*)ptr) Doar::Searcher(StringValuePtr(doar_data_file_path));
    return obj;
  }
  
  VALUE srch_delete(Doar::Searcher *ptr){
    ptr->~Searcher();
    ruby_xfree(ptr);
  }

  VALUE srch_bracket(VALUE self, VALUE key) {
    Doar::Searcher* ptr;
    Data_Get_Struct(self, Doar::Searcher, ptr);

    Doar::Node n = ptr->search(StringValuePtr(key));
    return n.valid() ? INT2FIX(n.id()) : Qnil;
  }
  
  VALUE srch_member(VALUE self, VALUE key) {
    Doar::Searcher* ptr;
    Data_Get_Struct(self, Doar::Searcher, ptr);  
    
    Doar::Node n = ptr->search(StringValuePtr(key));
    return n.valid() ? Qtrue : Qfalse;
  }

  VALUE srch_common_prefix_search(VALUE self, VALUE key) {
    Doar::Searcher* ptr;
    Data_Get_Struct(self, Doar::Searcher, ptr); 

    Collect fn(*ptr);
    ptr->common_prefix_search(StringValuePtr(key), fn);
    return fn.getArray();
  }

  VALUE srch_each_common_prefix(VALUE self, VALUE key) {
    Doar::Searcher* ptr;
    Data_Get_Struct(self, Doar::Searcher, ptr); 

    ptr->common_prefix_search(StringValuePtr(key), cps_yield);
    return Qnil;
  }

  VALUE srch_each(VALUE self) {
    Doar::Searcher* ptr;
    Data_Get_Struct(self, Doar::Searcher, ptr); 
    ptr->children(ptr->root_node(), EachAllKey(*ptr));
    return Qnil;
  }

  VALUE srch_size(VALUE self) {
    Doar::Searcher* ptr;
    Data_Get_Struct(self, Doar::Searcher, ptr); 
    return INT2FIX(ptr->size());
  }
}
