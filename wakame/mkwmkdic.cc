#include <iostream>
#include <string>
using namespace std;

#include "matrix.h"
#include "word_dic.h"

int main(int argc, char** argv) {
  if(argc != 5) {
    cerr << "Usage: " << argv[0] << " <matrix.def> <word-dic.csv> <unk.csv> <ouput-dir>" << endl;
    return 1;
  }
  Wakame::Matrix::build(argv[1],(string(argv[4])+"/matrix.bin").c_str());
  Wakame::WordDic::build(argv[2],argv[3],argv[4]);
  return 0;
};
