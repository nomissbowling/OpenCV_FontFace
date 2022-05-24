/*
  UTF8string.hpp

  assume same size 'typedef unsigned int FT_UInt32;'
*/

#ifndef __UTF8STRING_HPP__
#define __UTF8STRING_HPP__

#define UNICODE
#define _UNICODE
#include <wchar.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <string>

#include <stdexcept>
#include <exception>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace utf8string {

using namespace std;

template<typename ... Args>
std::string format(const std::string &fmt, Args ... args)
{
  size_t len = std::snprintf(nullptr, 0, fmt.c_str(), args ...);
  std::vector<char> buf(len + 1);
  std::snprintf(&buf[0], len + 1, fmt.c_str(), args ...);
  return std::string(&buf[0], &buf[0] + len);
}

class UTF8string {
private:
public:
  UTF8string(){}
  virtual ~UTF8string(){}

  // only utf-8 f3a08480->ue0100 f0a0ae9f->u20b9f
  // pre: false *p += N, true keep *p
  static uint fetchUTF8(uchar **p, bool pre){
    uint c = '?'; // illegal (0, c0-c1, f5-ff, or [1,2,3] < 80)
    uchar *b = *p;
    if(b[0] >= 0x0f0 && b[0] <= 0x0f4){ // (f0-f4)->(10000-10ffff) 4b-start
      if(b[1] & 0x80 && b[2] & 0x80 && b[3] & 0x80){
        if(!pre) *p += 3; // +1 later
        c = ((b[0] << 18) & 0x1c0000) | ((b[1] << 12) & 0x3f000) |
            ((b[2] << 6) & 0x0fc0) | (b[3] & 0x3f);
      }
    }else if(b[0] >= 0x0e0){ // (e0-ef)->(800-ffff) 3b-start
      if(b[1] & 0x80 && b[2] & 0x80){
        if(!pre) *p += 2; // +1 later
        c = ((b[0] << 12) & 0x0f000) | ((b[1] << 6) & 0x0fc0) | (b[2] & 0x3f);
      }
    }else if(b[0] >= 0x0c2){ // (c2-df)->(80-7ff) 2b-start
      if(b[1] & 0x80){
        if(!pre) *p += 1; // +1 later
        c = ((b[0] << 6) & 0x07c0) | (b[1] & 0x3f);
      }
    }else if(b[0] > 0 && b[0] < 0x0c0){ // (00-7f, 80-bf)->(through) 1b
//      if(b[0] < 0x80){
        c = b[0];
//      }
    }
    if(!pre && b[0]) ++(*p);
    return c;
  }

  static int u8len(const string &u8s){
    int len = 0;
    for(uchar *b = (uchar *)&u8s[0]; *b; ++len){
      uint wc = fetchUTF8(&b, false); // fetch and increment b
      uint wvs = fetchUTF8(&b, true); // pre fetch IVS SVS etc
      if(wvs >= 0x0e0100 && wvs < 0x0e01f0)
        wvs = fetchUTF8(&b, false); // fetch and increment b
    }
    return len;
  }
};

}

#endif // __UTF8STRING_HPP__
