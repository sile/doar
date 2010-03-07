#ifndef WAKAME_CHAR_CATEGORY_H
#define WAKAME_CHAR_CATEGORY_H

#include "../src/doar/mmap_t.h"    // XXX:

namespace Wakame {
  struct CharCategory {
    unsigned id;
    bool     invoke;
    bool     group;
    unsigned length;  // XXX: フィールドの順番は開発用
  };

  class CharCategoryTable {
  public:
    CharCategoryTable(const char* tbl_path) 
      : m_tbl(tbl_path), tbl(static_cast<const unsigned char*>(m_tbl.ptr)){
      // XXX:
      cc_dev[0] = (CharCategory){0,0,1,0}; // DEFAULT
      cc_dev[1] = (CharCategory){1,0,1,0}; // SPACE
      cc_dev[2] = (CharCategory){2,0,0,2}; // KANJI
      cc_dev[3] = (CharCategory){3,1,1,0}; // SYMBOL
      cc_dev[4] = (CharCategory){4,1,1,0}; // NUMERIC
      cc_dev[5] = (CharCategory){5,1,1,0}; // ALPHA
      cc_dev[6] = (CharCategory){6,0,1,2}; // HIRAGANA
      cc_dev[7] = (CharCategory){7,1,1,2}; // KATAKANA
      cc_dev[8] = (CharCategory){8,1,1,0}; // KANJINUMERIC
      cc_dev[9] = (CharCategory){9,1,1,0}; // GREEK
      cc_dev[10]= (CharCategory){10,1,1,0};// CYRILLIC
    }

    // TODO: 複数対応
    unsigned char category_id(unsigned ucs2_char_code) const {
      return tbl[ucs2_char_code];
    }

    const CharCategory& category(unsigned ucs2_char_code) const {
      // TODO:
      return cc_dev[category_id(ucs2_char_code)];
    }

  private:
    const mmap_t m_tbl;
    const unsigned char* tbl;
    
    // XXX: for develop
    CharCategory cc_dev[11];
  };
  

  // TODO: util
  unsigned utf8_to_ucs2(const char*& s) {
    if     (!(*s&0x80)) return *s++;                                                  // 0xxxxxxx
    else if(!(*s&0x40)) return s++,  0;      // NOTE: detect invalid utf8 format      // 10xxxxxx
    else if(!(*s&0x20)) return s+=2, ((s[-2]&0x1F)<<6)|(s[-1]&0x3F);                  // 110xxxxx
    else if(!(*s&0x10)) return s+=3, ((s[-3]&0xF)<<12)|((s[-2]&0x3F)<<6)|(s[-1]&0x3F);// 1110xxxx
    for(s++; *s&0x80; s++); // NOTE: ucs2の範囲外(and)は、default値を返す
    return 0;
  }
}

#endif
