/*
  cvFreeTypeFace.hpp
*/

#ifndef __CVFREETYPEFACE_HPP__
#define __CVFREETYPEFACE_HPP__

#define UNICODE
#define _UNICODE
#include <wchar.h>

// use CV_PI instead of M_PI
// #define _USE_MATH_DEFINES
#include <opencv2/opencv.hpp>

// #include <opencv2/imgproc.hpp> // cv::FONT *, cv::LINE *, cv::FILLED

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

#include <freetype/freetype.h>
#include <freetype/ftoutln.h>

#include <UTF8string.hpp>
#include <cvUtils.hpp>

namespace cvfreetypeface {

using namespace std;
using namespace utf8string;
using namespace cvutils;

class cvFTOutline {
private:
  int parts; // Bezier parts Conic Cube convert to Line
  int scaleAA; // for anti aliasing DPI etc
  FT_Outline_Funcs fncs;
  FT_Vector prev;
  vector<cv::Point> pts;
  vector<vector<cv::Point> > contours;

  inline static double i2d(int a){ return (double)(a << 6); }
  inline static int d2i(double a){ return (int)a >> 6; }
  inline static cv::Point d2i(cv::Point2d &p){
    return cv::Point(d2i(p.x), d2i(p.y));
  }

  static int moveTo(const FT_Vector *to, void *self){
    cvFTOutline *p = (cvFTOutline *)self;
    if(p->pts.size()){
      p->contours.push_back(p->pts); // copied
      p->pts.clear();
    }
    if(!to) return 1;
    p->pts.push_back(d2i(cv::Point2d(to->x, to->y)));
    p->prev = *to;
    return 0;
  }

  static int lineTo(const FT_Vector *to, void *self){
    if(!to) return 1;
    cvFTOutline *p = (cvFTOutline *)self;
    p->pts.push_back(d2i(cv::Point2d(to->x, to->y)));
    p->prev = *to;
    return 0;
  }

  static int conicTo(const FT_Vector *cp, const FT_Vector *to, void *self){
    if(!cp || !to) return 1;
    cvFTOutline *p = (cvFTOutline *)self;
    for(int i = 0; i <= p->parts; ++i){ // Bezier to Line
      double r = (double)i / p->parts;
      double s = 1.0 - r;
      double q[] = {s * s, 2.0 * s * r, r * r};
      p->pts.push_back(d2i(cv::Point2d(
        (p->prev.x) * q[0] + cp->x * q[1] + to->x * q[2],
        (p->prev.y) * q[0] + cp->y * q[1] + to->y * q[2])));
    }
    p->prev = *to;
    return 0;
  }

  static int cubicTo(const FT_Vector *cp1, const FT_Vector *cp2,
    const FT_Vector *to, void *self){
    if(!cp1 || !cp2 || !to) return 1;
    cvFTOutline *p = (cvFTOutline *)self;
    for(int i = 0; i <= p->parts; ++i){ // Bezier to Line
      double r = (double)i / p->parts;
      double s = 1.0 - r;
      double q[] = {s * s * s, 3.0 * s * s * r, 3.0 * s * r * r, r * r * r};
      p->pts.push_back(d2i(cv::Point2d(
        (p->prev.x) * q[0] + cp1->x * q[1] + cp2->x * q[2] + to->x * q[3],
        (p->prev.y) * q[0] + cp1->y * q[1] + cp2->y * q[2] + to->y * q[3])));
    }
    p->prev = *to;
    return 0;
  }

public:
  cvFTOutline(int nparts=16, int nscaleAA=1024)
  : parts(nparts), scaleAA(nscaleAA) {
    fncs.shift = 0;
    fncs.move_to = cvFTOutline::moveTo;
    fncs.line_to = cvFTOutline::lineTo;
    fncs.cubic_to = cvFTOutline::cubicTo;
    fncs.conic_to = cvFTOutline::conicTo;
  }

  virtual ~cvFTOutline(){}

  cv::Point glyphToPath(FT_GlyphSlot slot, int x, int y){
    FT_Outline outline = slot->outline;
    FT_Matrix m = {(int)i2d(scaleAA), 0, 0, (int)-i2d(scaleAA)};
    FT_Outline_Transform(&outline, &m);
    FT_Outline_Translate(&outline, (FT_Pos)i2d(x), (FT_Pos)i2d(y));
    FT_Outline_Decompose(&outline, &fncs, (void*)this);
    moveTo(NULL, (void*)this);
    return d2i(cv::Point2d(slot->advance.x, slot->advance.y));
  }

  int flush(cv::Mat &im, cv::Scalar &color, int thickness, int linetype){
#if 0
    for(auto &pts : contours){
      if(pts.size()){
        const cv::Point *ptsLst[] = {&(pts[0])};
        int npt[] = {(int)pts.size()};
        cv::polylines(im, ptsLst, npt, _countof(npt), false,
          color, thickness, linetype, 0);
        pts.clear();
      }
    }
#else
    if(contours.size()){
#if 0
      drawPolys(im, 1, contours, color);
#else
      cv::drawContours(im, contours, -1, color, -1, cv::LINE_AA,
        vector<cv::Vec4i>(), 2);
#endif
      contours.clear();
    }
#endif
    return 0;
  }
};

class cvFTFace {
private:
  FT_Face ftface;
  bool validface;

public:
  cvFTFace(FT_Library ftlib, const char *facename){
#if 0
    FT_New_Memory_Face(ftlib, memfont_ttf, memfont_ttf_len, 0, &ftface);
#else
    validface = !FT_New_Face(ftlib, facename, 0, &ftface) ? true : false;
#if 0
    fprintf(stderr, "[%s] %d\n", facename, validface);
#endif
#endif
  }

  virtual ~cvFTFace(){
    if(validface) FT_Done_Face(ftface);
  }

  cv::Point pUTF8(cv::InputOutputArray dst, const string &u8s,
    cv::Point &pos, int fontScale, cv::Scalar &col,
    int thickness=1, int linetype=cv::LINE_AA, bool bottomLeftOrigin=false,
    cv::Scalar &gradTo=cv::Scalar(255, 255, 255), int gradToDirec=0){
    cv::Point org(pos);
    if(!validface || u8s.empty()) return org - pos;
    if(dst.depth() != CV_8U && linetype == cv::LINE_AA) linetype = cv::LINE_8;
    vector<cv::Scalar> colors; // not use colors{col}
    colors.push_back(col);
    if(gradToDirec){
      cv::Mat dots(1, 180, CV_8UC3);
      for(int i = 0; i < 3; ++i){
        dots.at<cv::Vec3b>(0, 0)[i] = col[i];
        dots.at<cv::Vec3b>(0, 1)[i] = gradTo[i];
      }
      cv::cvtColor(dots, dots, cv::COLOR_BGR2HSV);
      cv::Scalar hsv = dots.at<cv::Vec3b>(0, 0);
      cv::Scalar hsvGradTo = dots.at<cv::Vec3b>(0, 1);
      if(hsv[0] > hsvGradTo[0]){
        if(gradToDirec > 0) hsv[0] -= 180; // cw
      }else{
        if(gradToDirec < 0) hsvGradTo[0] -= 180; // ccw
      }
      for(int th = 1; th < 180; ++th){ // Hue 0-179
        cv::Vec3b &v = dots.at<cv::Vec3b>(0, th);
        double r = (double)th / (180 - 1);
        int h = (1.0 - r) * hsv[0] + r * hsvGradTo[0];
        v[0] = (uchar)(h + (h < 0 ? 180 : 0)) % 180;
        v[1] = (uchar)((1.0 - r) * hsv[1] + r * hsvGradTo[1]);
        v[2] = (uchar)((1.0 - r) * hsv[2] + r * hsvGradTo[2]);
      }
      cv::cvtColor(dots, dots, cv::COLOR_HSV2BGR);
      for(int th = 1; th < 180; ++th)
        colors.push_back(cv::Scalar(dots.at<cv::Vec3b>(0, th)));
#if 0
      cv::Mat tmp;
      cv::resize(dots, tmp, cv::Size(360, 4), 0, 0, cv::INTER_LANCZOS4);
      tmp.copyTo(dst.getMat()(cv::Rect(24, 475, 360, 4))); // drift 472-476
#endif
    }
#if 0 // 0-179 [-1] <= 179 (when len << size) (sometime doesnot contain gradTo)
    double gd = (double)colors.size() / UTF8string::u8len(u8s);
#else // 0-179 [-1] == 179 (like as numpy.linspace) (everytime contains gradTo)
    double gd = (double)(colors.size() - 1) / (UTF8string::u8len(u8s) - 1);
#endif
    FT_Set_Pixel_Sizes(ftface, fontScale, fontScale);
    int height = fontScale;
    if(bottomLeftOrigin) org.y -= height;
    int i = 0;
    for(uchar *b = (uchar *)&u8s[0]; *b; ++i){
      cv::Scalar &c = colors[(int)(i * gd) % colors.size()];
      FT_UInt32 wc = UTF8string::fetchUTF8(&b, false); // fetch and increment b
#if 0
      FT_Load_Char(ftface, wc, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP);
#else
      FT_Int gid = 0;
      FT_UInt32 wvs = UTF8string::fetchUTF8(&b, true); // pre fetch IVS SVS etc
      if(wvs >= 0x0e0100 && wvs < 0x0e01f0){
        wvs = UTF8string::fetchUTF8(&b, false); // fetch and increment b
        gid = FT_Face_GetCharVariantIndex(ftface, wc, wvs);
#if 0
        fprintf(stderr, "VS %08x: %08x: %08x\n", wvs, wc, gid);
#endif
      }
      if(!gid) gid = FT_Get_Char_Index(ftface, wc);
#if 0
      if(!gid) fprintf(stderr, "glyph id: %08x: %08x\n", wc, gid);
#endif
      FT_Load_Glyph(ftface, gid, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP);
#endif
      cvFTOutline ol;
      org += ol.glyphToPath(ftface->glyph, org.x, org.y + height);
      ol.flush(dst.getMat(), c, thickness, linetype);
    }
    if(bottomLeftOrigin) org.y += height;
    return org - pos;
  }
};

class cvFTManager {
private:
  FT_Library ftlib;
  vector<cvFTFace *> vft;
public:
  cvFTManager(){
    FT_Init_FreeType(&ftlib);
  }

  virtual ~cvFTManager(){
    for(auto &ff : vft){ delete ff; ff = NULL; }
    FT_Done_FreeType(ftlib);
  }

  size_t addFace(const string &facepath){
    vft.push_back(new cvFTFace(ftlib, facepath.c_str()));
    return vft.size();
  }

  cvFTFace &getFace(int n){
    return *vft[n % vft.size()];
  }
};

}

#endif // __CVFREETYPEFACE_HPP__
