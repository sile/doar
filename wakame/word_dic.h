#ifndef WAKAME_WORD_DIC_H
#define WAKAME_WORD_DIC_H

#include <string>
#include <vector>
#include <fstream>

#include <doar/searcher.h>
#include <doar/double_array.h>
#include "../src/util/read_line.h" // XXX:
#include "../src/doar/mmap_t.h"    // XXX:

#include "word.h"
#include "morpheme_node.h"

namespace Wakame {
  class WordDic {
  public:
    // XXX: for dev unk_file
    static bool build(const char* word_file, const char* unk_file, const char* output_dir) {
      ReadLine rl(word_file);
      if(!rl)
	return false;

      ReadLine unk_rl(unk_file);
      if(!unk_rl)
	return false;

      // data output stream
      std::string odir(output_dir);
      std::ofstream dout((odir+"/word.info").c_str(),std::ios::binary);
      if(!dout)
	return false;
      
      Doar::DoubleArray da;
      std::vector<Words> ws_ary;
      
      const char* line;
      std::string s;
      unsigned data_index = 0;
      
      // XXX: for dev
      //     未知語用: 検索用のキーを追加しないところ以外は普通の単語と同様
      // XXX: この方法だと、単語辞書に'KANJI'などのエントリがあった場合に、衝突が起こるのでは?(2010//03/08)
      while((line=unk_rl.read())) {
	if(ws_ary.size()%500==0)
	  std::cout << ws_ary.size() << std::endl;
	
	// TODO: assert
	s = line;
	std::size_t p1 = s.find(',');
	std::size_t p2 = s.find(',',p1+1);
	std::size_t p3 = s.find(',',p2+1);
	std::size_t p4 = s.find(',',p3+1);
	dout << s.substr(p4+1) << '\0';   // TODO: zip?
	
	Word w(atoi(s.substr(p1+1,p2).c_str()),
	       atoi(s.substr(p2+1,p3).c_str()),
	       atoi(s.substr(p3+1,p4).c_str()),
	       data_index);

	data_index += s.size()-(p4+1) + 1;

	// XXX: has assumption
	// XXX: for dev
	std::string unk_key = "";
	unk_key += static_cast<char>(1);
	unk_key += static_cast<char>(1);
	unk_key += s.substr(0,p1);
	if(da.insert(unk_key.c_str())) {
	  ws_ary.resize(ws_ary.size()+1);
	  ws_ary.back().push_back(w);
	} else {
	  ws_ary[da.search(unk_key.c_str()).id()].push_back(w);
	}
      }      


      while((line=rl.read())) {
	if(ws_ary.size()%500==0)
	  std::cout << ws_ary.size() << std::endl;
	
	// TODO: assert
	s = line;
	std::size_t p1 = s.find(',');
	std::size_t p2 = s.find(',',p1+1);
	std::size_t p3 = s.find(',',p2+1);
	std::size_t p4 = s.find(',',p3+1);
	dout << s.substr(p4+1) << '\0';   // TODO: zip?
	
	Word w(atoi(s.substr(p1+1,p2).c_str()),
	       atoi(s.substr(p2+1,p3).c_str()),
	       atoi(s.substr(p3+1,p4).c_str()),
	       data_index);

	data_index += s.size()-(p4+1) + 1;

	
	// XXX: has assumption
	if(da.insert(s.substr(0,p1).c_str())) {
	  ws_ary.resize(ws_ary.size()+1);
	  ws_ary.back().push_back(w);
	} else {
	  ws_ary[da.search(s.substr(0,p1).c_str()).id()].push_back(w);
	}
      }

      da.save((odir+"/word.idx").c_str());
      
      //
      std::vector<unsigned> idx_map;
      unsigned index=0;
      for(std::size_t i=0; i < ws_ary.size(); i++) {
	idx_map.push_back(index);
	index+=ws_ary[i].size();
      }
      idx_map.push_back(index);

      {
	FILE* f;
	f = fopen((odir+"/idx.map").c_str(),"wb");
	fwrite(idx_map.data(), sizeof(unsigned), idx_map.size(), f);
	fclose(f);
      }
      {
	FILE* f;
	f = fopen((odir+"/word.dat").c_str(),"wb");
	for(std::size_t i=0; i < ws_ary.size(); i++)
	  fwrite(ws_ary[i].data(),sizeof(Word),ws_ary[i].size(),f);
	fclose(f);
      }
	     
      return true;
    }

    struct collect {
      collect(MorphemeNodes& mns, const WordDic& wd)
	: mns(mns), wd(wd) {}
      void operator()(const char* key, unsigned offset, unsigned id) const {
	const Word* cur = wd.words+wd.indices[id];
	const Word* end = wd.words+wd.indices[id+1];
	for(; cur != end; cur++)
	  mns.push_front(*cur,offset);
      }
      MorphemeNodes& mns;
      const WordDic& wd;
    };

    void search(const char* text, MorphemeNodes& result) const {
      collect cl(result,*this);
      da.each_common_prefix(text,cl);
    }

    // TODO: rename and 共通化
    void get_from_id(unsigned word_id, unsigned word_length, MorphemeNodes& result) const {
	const Word* cur = words+indices[word_id];
	const Word* end = words+indices[word_id+1];
	for(; cur != end; cur++)
	  result.push_front(*cur,word_length);
    }
    
    // XXX: for dev
    const char* get_info(const Word& w) const {
      return info+w.index_of_info;
    }
    
    WordDic(const char* data_dir) 
      : da((std::string(data_dir)+"/word.idx").c_str()),
	m_im((std::string(data_dir)+"/idx.map").c_str()),
	m_wd((std::string(data_dir)+"/word.dat").c_str()),
	m_wi((std::string(data_dir)+"/word.info").c_str()),	
	indices(static_cast<unsigned*>(m_im.ptr)),
	words(static_cast<Word*>(m_wd.ptr)),
	info(static_cast<const char*>(m_wi.ptr))
    {
    }
    
  private:
    const Doar::Searcher da;

    const mmap_t m_im; // idx.map
    const mmap_t m_wd; // word.dat
    const mmap_t m_wi; // word.info
    const unsigned* indices;
    const Word*    words;
    const char* info;
  };
}

#endif
