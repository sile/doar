#include <iostream>
#include <fstream>
#include "../doar/builder.h"
#include "../doar/double_array.h"
#include "../util/gettime.h"

// TODO: help message
int main(int argc, char** argv) {
  if(argc < 2) {
    std::cerr <<"Usage: mkdoar "
	      <<" [--extend=[source-index-file]] [--conc-static] [--help #no implemented]"
	      <<" <output-index-file> [input-words-file]" << std::endl;
    return 1;
  }
  
  int beg_i=1;
  bool conc_static=false;
  const char* source_index_file=NULL;
  
  // handle options
  while(strncmp("--",argv[beg_i],2)==0) {
    if (strcmp("conc-static",argv[beg_i]+2)==0)
      conc_static=true;
    else if(strcmp ("extend",argv[beg_i]+2)==0)
      source_index_file=reinterpret_cast<const char*>(-1);
    else if(strncmp("extend=",argv[beg_i]+2,strlen("extend="))==0)
      source_index_file = argv[beg_i]+2+strlen("extend=");
    
    beg_i++;
  }

  if(argc <= beg_i) {
    std::cerr << "Output data file must be specified." << std::endl;
    return 1;
  }

  // input and output file
  const char* output_file = argv[beg_i++];
  const char* input_file = beg_i < argc ? argv[beg_i] : NULL;
  if(source_index_file==reinterpret_cast<const char*>(-1)) 
    source_index_file=output_file;


  // construct and save
  double beg_t = 0;
  if(conc_static) {
    if(source_index_file)
      std::cerr << "WARN: " << "--extend option ignored." << std::endl;

    if(input_file==NULL) {
      std::cerr << "Input words file must be specified." << std::endl;
      return 1;
    }
    
    beg_t = gettime();
    Doar::Builder bld;
    std::cerr << "=== Construct from " << input_file << " ===" << std::endl;
    if(bld.build(input_file)!=0) {
      std::cerr << "Construct failed: Input file is unsorted or has duplicated lines." << std::endl;
      return 1;
    }
    std::cerr << "Key set size: " << bld.size() << std::endl;
    std::cerr << "Elapsed: " << gettime()-beg_t << " sec" << std::endl << std::endl;
    
    beg_t = gettime();
    std::cerr << "=== Save to " << output_file << " ===" << std::endl;
    if(bld.save(output_file)==false) {
      std::cerr << "Save failed" << std::endl;
      return 1;
    }
    std::cerr << "Elapsed: " << gettime()-beg_t << " sec" << std::endl << std::endl;    
  } else {
    Doar::DoubleArray trie;
    if(source_index_file) {
      beg_t = gettime();
      std::cerr << "=== Load from " << source_index_file << " ===" << std::endl;
      if(trie.load(source_index_file)!=Doar::Status::OK) {
	std::cerr << "Load failed" << std::endl;
	return 1;
      }
      std::cerr << "Key set size: " << trie.size() << std::endl;
      std::cerr << "Elapsed: " << gettime()-beg_t << " sec" << std::endl << std::endl;
    }

    std::istream* in = input_file ? new std::ifstream(input_file) : &std::cin;
    std::string word;
    std::cerr << "=== Construct from " << (input_file ? input_file : "standard input") << " ===" << std::endl;
    beg_t = gettime();
    while(getline(*in,word))
      trie.insert(word.c_str());
    std::cerr << "Key set size: " << trie.size() << std::endl;
    std::cerr << "Elapsed: " << gettime()-beg_t << " sec" << std::endl << std::endl;
      
    beg_t = gettime();
    std::cerr << "=== Save to " << output_file << " ===" << std::endl;
    if(trie.save(output_file)==false) {
      std::cerr << "Save failed" << std::endl;
      return 1;
    }
    std::cerr << "Elapsed: " << gettime()-beg_t << " sec" << std::endl << std::endl;        

    if(input_file)
      delete in;
  }
  std::cerr << "DONE" << std::endl;
  return 0;
}
