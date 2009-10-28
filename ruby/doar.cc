#include <ruby.h>
#include <doar/searcher.h>
#include <string>

namespace {
  class Collect {
  public:
    Collect(const Doar::Searcher& sh)
      : srch(sh), ary(rb_ary_new()) {}
    
    void operator()(const char* key, unsigned offset, unsigned id) const {
      VALUE obj=rb_ary_new2(2);
      rb_ary_store(obj,0,INT2FIX(offset));
      rb_ary_store(obj,1,INT2FIX(id));
      
      rb_ary_push(ary, obj);
    }
    
    VALUE getArray() const { return ary; }
  private:
    const Doar::Searcher& srch;
    VALUE ary;
  };

  // Common Prefix Search yield
  void cps_yield(const char*key, unsigned offset, unsigned id) {
    rb_yield_values(2,INT2FIX(offset),INT2FIX(id));
  }

  class YieldAllKey {
  public:
    YieldAllKey(const Doar::Searcher& sh) 
      : srch(sh), len(0) {}
    YieldAllKey(const Doar::Searcher& sh, const std::string& base) 
      : srch(sh), buf(base), len(base.size()) {}

    void operator() (char c, Doar::Node node, const char* tail) const {
      buf += c;
      if(node.is_leaf()) {
	buf += tail;
	rb_yield_values(2, rb_str_new2(buf.c_str()), INT2FIX(node.id()));
      } else {
	len++;
	srch.each_child(node,*this);
	len--;
      }

      buf.resize(len);
    }
  private:
    const Doar::Searcher& srch;
    mutable std::string buf;
    mutable unsigned len;
  };
}

extern "C" {
  VALUE srch_new(VALUE klass, VALUE doar_index_file_path);
  VALUE srch_delete(Doar::Searcher *ptr);
  VALUE srch_bracket(VALUE self, VALUE key);
  VALUE srch_key(VALUE self, VALUE key);
  VALUE srch_common_prefix_search(VALUE self, VALUE key);
  VALUE srch_each_common_prefix(VALUE self, VALUE key);
  VALUE srch_each(int argc, VALUE *argv, VALUE self);
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
    rb_define_method(cSrch, "key?", (VALUE (*)(...))srch_key, 1);
    rb_define_method(cSrch, "common_prefix_search", (VALUE (*)(...))srch_common_prefix_search, 1);
    rb_define_method(cSrch, "each_common_prefix", (VALUE (*)(...))srch_each_common_prefix, 1);    
    rb_define_method(cSrch, "each", (VALUE (*)(...))srch_each, -1);
    rb_define_method(cSrch, "size", (VALUE (*)(...))srch_size, 0);
  }
  
  VALUE srch_new(VALUE klass, VALUE doar_index_file_path){
    Doar::Searcher* ptr;
    VALUE obj = Data_Make_Struct(klass,Doar::Searcher,NULL,srch_delete,ptr);
    const char* path=StringValuePtr(doar_index_file_path);
    new ((void*)ptr) Doar::Searcher(path);

    switch(ptr->status) {
    case Doar::Status::OPEN_FILE_FAILED:
      rb_raise(rb_eArgError,"Can't open file: %s", path);
    case Doar::Status::INVALID_FILE_FORMAT:
      rb_raise(rb_eArgError,"This file isn't Doar index file: %s", path);
    case Doar::Status::FILE_IS_CORRUPTED:
      rb_raise(rb_eArgError,"This index file is corrupted: %s", path);
    }
    
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
    return n ? INT2FIX(n.id()) : Qnil;
  }
  
  VALUE srch_key(VALUE self, VALUE key) {
    Doar::Searcher* ptr;
    Data_Get_Struct(self, Doar::Searcher, ptr);  
    
    Doar::Node n = ptr->search(StringValuePtr(key));
    return n ? Qtrue : Qfalse;
  }

  VALUE srch_common_prefix_search(VALUE self, VALUE key) {
    Doar::Searcher* ptr;
    Data_Get_Struct(self, Doar::Searcher, ptr); 

    Collect fn(*ptr);
    ptr->each_common_prefix(StringValuePtr(key), fn);
    return fn.getArray();
  }

  VALUE srch_each_common_prefix(VALUE self, VALUE key) {
    Doar::Searcher* ptr;
    Data_Get_Struct(self, Doar::Searcher, ptr); 

    ptr->each_common_prefix(StringValuePtr(key), cps_yield);
    return Qnil;
  }

  VALUE srch_each(int argc, VALUE *argv, VALUE self){
    VALUE root_str;
    VALUE omit_base;
    rb_scan_args(argc,argv,"02",&root_str,&omit_base);
    
    Doar::Searcher* ptr;
    Data_Get_Struct(self, Doar::Searcher, ptr); 
    if(root_str==Qnil) {
      ptr->each_child(ptr->root_node(), YieldAllKey(*ptr));
    } else {
      Doar::Node root=ptr->root_node();
      const char* base = StringValuePtr(root_str);
      ptr->search(base,root);
      ptr->each_child(root, YieldAllKey(*ptr, omit_base==Qtrue?"":base));
    }
    return Qnil;
  }

  VALUE srch_size(VALUE self) {
    Doar::Searcher* ptr;
    Data_Get_Struct(self, Doar::Searcher, ptr); 
    return INT2FIX(ptr->size());
  }
}
