#ifndef WAKAME_MATRIX_H
#define WAKAME_MATRIX_H

#include <vector>
#include "../src/util/read_line.h" // XXX:
#include "../src/doar/mmap_t.h"    // XXX:

namespace Wakame {
  class Matrix {
    typedef short COST; // XXX:
    
  public:
    static bool build(const char* matrix_file, const char* output_file) {
      ReadLine rl(matrix_file);
      if(!rl)
	return false;
      
      // 一行目はサイズ
      const char* line = rl.read();
      unsigned left_num  = atoi(line);
      unsigned right_num = atoi(strchr(line,' '));

      std::vector<COST> matrix(left_num*right_num);      
      
      // 二行目以降はデータ
      // TODO: validation
      for(unsigned i=0; i < left_num; i++)
	for(unsigned j=0; j < right_num; j++) {
	  COST cost = atoi(strrchr(rl.read(),' '));
	  matrix[i*left_num+j] = cost;
	}
      
      FILE* f;
      f = fopen(output_file,"wb");
      fwrite(&left_num, sizeof(unsigned), 1, f);
      fwrite(&right_num, sizeof(unsigned), 1, f);
      fwrite(matrix.data(), sizeof(COST), matrix.size(), f);
      fclose(f);      
      return true;
    }

    Matrix(const char* matrix_bin) 
      : mm(matrix_bin), 
	left_size (static_cast<unsigned*>(mm.ptr)[0]),
	right_size(static_cast<unsigned*>(mm.ptr)[1]),
	matrix(reinterpret_cast<COST*>(static_cast<unsigned*>(mm.ptr)+2))
    {
    }

    COST link_cost(unsigned left_id, unsigned right_id) const {
      return matrix[left_id*left_size+right_id];
    }
  private:
    const mmap_t mm;
    const unsigned left_size;
    const unsigned right_size;
    const COST* matrix;
  };
}

#endif
