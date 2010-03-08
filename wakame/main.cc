#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

#include "word_dic.h"
#include "matrix.h"
#include "unknown.h"
#include "../src/util/gettime.h"

#include "allocator.h"

typedef vector<Wakame::MorphemeNodes> NodesArray;

void set_mincost_node(Wakame::MorphemeNode& n, const Wakame::MorphemeNodes& prevs, const Wakame::Matrix& mt) {
  const Wakame::MorphemeNodes::Node* p = prevs.front();

  n.cost = p->value.cost+mt.link_cost(p->value.word.rgt_id, n.word.lft_id);
  n.prev = &p->value;
  
  for(p=p->next; p; p=p->next) {
    const Wakame::MorphemeNode& prev = p->value;

    int cost = prev.cost + mt.link_cost(prev.word.rgt_id, n.word.lft_id);
    if(cost < n.cost) {
      n.cost = cost;
      n.prev = &prev;
    }
  }
  //cerr << "## " << n.prev << endl;
  n.cost += n.word.cost;
}

// XXX: for dev
void ppp(const Wakame::MorphemeNodes::Node *p) {
  cerr << "#################" << endl;
  for(; p; p = p->next) {
    cerr << "# " << &p->value << " @" << p->value.cost << endl;
  }
  cerr << endl;
}
bool node_cost_less(const Wakame::MorphemeNode *a, const Wakame::MorphemeNode *b) {
  return a->cost < b->cost;
}

int main(int argc, char** argv) {
  const Wakame::WordDic wdic(argv[1]);
  const Wakame::Matrix  mt((string(argv[1])+"/matrix.bin").c_str());

  ReadLine rl(argv[2]);
  const char* line;

  Wakame::Unknown unk("dat");

  while((line=rl.read())) {
    Wakame::MorphemeNodes::alloc.reset();

    unsigned len = strlen(line);

    //cout << "LEN: " << len << endl;
    if(len==0) // XXX:
      continue;

    NodesArray nsary(len+1);

    nsary[0].push_front(Wakame::BOS_EOS_WORD,0);
    for(size_t i=0; i < nsary.size(); i++) { // i < nsary.size()-1でも大丈夫? (2010/03/09)
      
      if(nsary[i].empty()==false) {
	const Wakame::MorphemeNodes& prevs = nsary[i];
	//ppp(prevs.front());

	Wakame::MorphemeNodes rlt;
	wdic.search(line+i,rlt);
	unk.search(line+i,wdic,rlt);

	const Wakame::MorphemeNodes::Node *p=rlt.front();
	for(; p; p = p->next) {
	  const Wakame::MorphemeNode& n = p->value;
	  
	  nsary[i+n.length].push_front(n.word,n.length);
	  Wakame::MorphemeNode& cur = nsary[i+n.length].front()->value;
	  set_mincost_node(cur,prevs,mt);
	}
      }
    }
    
    // EOS
    Wakame::MorphemeNode eos(Wakame::BOS_EOS_WORD,0);

    // XXX: check for dev (due to absence of unknown word handling)
    if(nsary.back().empty()==false) {
      set_mincost_node(eos,nsary.back(),mt);
    
      const Wakame::MorphemeNode* cur = eos.prev;
      std::vector<const Wakame::MorphemeNode*> ms;
      
      for(; cur; cur = cur->prev)
	if(cur->length != 0) // XXX: checks for bos
	  ms.push_back(cur);
      std::reverse(ms.begin(),ms.end());
      
      size_t start=0;
      for(size_t i=0; i < ms.size(); i++) {
	fwrite(line+start,sizeof(char),ms[i]->length,stdout);
	putchar('\t');
	puts(wdic.get_info(ms[i]->word));
	
	start += ms[i]->length;
      }
    } else {
      puts("# FAILURE");
    }
    puts("EOS");
  }
  return 0;
}
