// json_allocator.h
#ifndef JSON_ALLOCATOR_H
#define JSON_ALLOCATOR_H

#include <vector>

namespace json{
  // チャンク(メモリブロック)を保持する構造体
  struct chunk{
    chunk(unsigned init_size=0x8000) 
      :offset(0), max_chunk_size(init_size) 
    {
      chunks.push_back(new char[max_chunk_size]);
    }
    ~chunk() {
      for(unsigned i=0; i < chunks.size(); i++)
        delete [] chunks[i];
    }
    
    std::vector<char*> chunks;         // チャンクの配列
    unsigned           max_chunk_size; // 一番大きいチャンク(= chunks.back())のサイズ
    unsigned           offset;         // 未使用のメモリ開始位置
  };
    
  template <class T>
  class allocator {
  public:
    /**********/
    /* 型関連 */
    // 型定義
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;
  
    // アロケータをU型にバインドする
    template <class U>
    struct rebind { typedef allocator<U> other; };
  
    /******************/
    /* コンストラクタ */
    allocator(chunk& cnk) : cnk_(cnk) {}

    template <class U> allocator(const allocator<U>& src) 
      : cnk_(const_cast<chunk&>(src.get_chunk())) {}


    /*******************************************/
    /* STLのアロケータクラスに必要なメソッド群 */
    // メモリを割り当てる
    pointer allocate(unsigned num){
      if(cnk_.offset+sizeof(T)*num >= cnk_.max_chunk_size){
        // メモリが足りない場合は、chunkを増やす
        cnk_.max_chunk_size *= 2;
        cnk_.chunks.push_back(new char [cnk_.max_chunk_size]);
        cnk_.offset=0;
        
        return allocate(num); // もう一度allocate
      }
      
      // offsetから、sizeof(T)*num分だけ、メモリを割り当てる
      // XXX: ここでコンストラクタを呼び出す必要はない?
      pointer ptr = new (cnk_.chunks.back()+cnk_.offset) T[num];
      cnk_.offset += sizeof(T)*num;
      return ptr;      
    }
    
    pointer allocate() {
      if(cnk_.offset+sizeof(T) >= cnk_.max_chunk_size){
        // メモリが足りない場合は、chunkを増やす
        cnk_.max_chunk_size *= 2;
        cnk_.chunks.push_back(new char [cnk_.max_chunk_size]);
        cnk_.offset=0;
      }
      
      // offsetから、sizeof(T)分だけ、メモリを割り当てる
      unsigned offset = cnk_.offset;
      cnk_.offset += sizeof(T);
      return reinterpret_cast<pointer>(cnk_.chunks.back()+offset);
    }

    // 割当て済みの領域を初期化する
    void construct(pointer p, const T& value) {
      new( (void*)p ) T(value);
    }

    // メモリを解放する
    // ※ 何も行わない
    void deallocate(pointer p, unsigned num) {}

    // 初期化済みの領域を削除する
    void destroy(pointer p) { p->~T(); }


    /**************************/
    /* JSONに必要なメソッド群 */
    // 未使用メモリ開始位置(offset)を0にリセットする
    void reset() {
      cnk_.offset = 0;
      if(cnk_.chunks.size() > 1) {
        // チャンクが複数ある場合は、一番最後のもの以外はdeleteする
        char *tmp = cnk_.chunks.back();
        for(unsigned i=0; i < cnk_.chunks.size()-1; i++)
          delete [] cnk_.chunks[i];
        cnk_.chunks.clear();
        cnk_.chunks.push_back(tmp);
      }
    }
    
    unsigned max_size() const {
      // XXX:
      return 0xFFFFFFFF/sizeof(T);
    }

    // 引数のポインタ(ptr)が、あらかじめ確保しているメモリブロックの範囲内かどうかを判定する
    // 範囲外なら、trueを返す
    // ※ 上限のみをチェックしている
    bool out_of_range(char* ptr) {
      return cnk_.chunks.back()+cnk_.max_chunk_size <= ptr;
    }
    
    // 2番目のコンストラクタで使用
    const chunk& get_chunk() const { return cnk_; }    

  private:
    chunk& cnk_;
  };
}

#endif
