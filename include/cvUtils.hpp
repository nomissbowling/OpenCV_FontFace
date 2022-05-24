/*
  cvUtils.hpp
*/

#ifndef __CVUTILS_HPP__
#define __CVUTILS_HPP__

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

#include <UTF8string.hpp>

namespace cvutils {

using namespace std;
using namespace utf8string;

static uint8_t COLS[][3] = { // RGB
  {240, 192, 32}, {32, 240, 192}, {192, 32, 240}, {255, 255, 255},
  {240, 32, 192}, {192, 240, 32}, {32, 192, 240}, {0, 0, 0}};

vector<cv::Scalar> createColors()
{
  vector<cv::Scalar> colors(_countof(COLS));
  int i = 0;
  for(auto it = colors.begin(); it != colors.end(); ++it, ++i)
    *it = cv::Scalar(COLS[i][2], COLS[i][1], COLS[i][0]); // BGR
  return colors;
}

template<class P> // cv::Point or cv::Point2d etc
vector<P> getMinMax(vector<P> &points) // Contour or Bounds etc
{
  vector<P> r(2);
  r[0].x = r[1].x = points[0].x;
  r[0].y = r[1].y = points[0].y;
  for(auto it = points.begin(); it != points.end(); ++it){
    if(r[0].x > it->x) r[0].x = it->x;
    if(r[0].y > it->y) r[0].y = it->y;
    if(r[1].x < it->x) r[1].x = it->x;
    if(r[1].y < it->y) r[1].y = it->y;
  }
  return r;
}

template<class P> // cv::Point or cv::Point2d etc
vector<P> getMinMaxOfContours(vector<vector<P> > &contours) // Contours2 or 2d
{
  vector<P> r(2);
  int i = 0;
  for(auto it = contours.begin(); it != contours.end(); ++it, ++i){
    vector<P> q = getMinMax(*it);
    if(!i) r = q;
    else{
#if 1
      q.insert(q.end(), r.begin(), r.end());
#else
      q.reserve(q.size() + r.size()); // or capacity()
      std::copy(r.begin(), r.end(), std::back_inserter(q));
#endif
      r = getMinMax(q);
    }
  }
  return r;
}

template<typename ... Args>
void putFmtTxt(cv::Mat &dst, cv::Point &pos, cv::Scalar &col,
  const std::string &fmt, Args ... args)
{
  const string &txt = format(fmt, args ...);
  const int th = 1;
  const double fs = 1.0;
  const int ff = cv::FONT_HERSHEY_PLAIN;
  // const cv::FontFace ff("timesnewroman.ttf"); // only new version
  cv::Scalar &dc = cv::Scalar(255, 255, 255) - col;
  cv::putText(dst, txt, pos, ff, fs, dc, th + 1, cv::LINE_AA);
  cv::putText(dst, txt, pos, ff, fs, col, th, cv::LINE_AA);
  // cv::addText(dst, txt, pos, "Consolas", fs, col, th, cv::LINE_AA); // Qt
}

size_t drawPolys(cv::Mat &dst, int mode, int bc, vector<cv::Scalar> &hcols,
  vector<vector<cv::Point> > &contours, vector<cv::Vec4i> &hierarchy)
{
  size_t ncontours = contours.size();
  if(!ncontours) return 0;
  size_t num = 0;
  for(auto it = contours.begin(); it != contours.end(); ++it, ++num){
    int h = hierarchy[num][3]; // parent
    cv::Scalar &hc = hcols[h < 0 ? 0 : (h % (hcols.size() - 2)) + 1]; // no bk
    if(bc & 0x01){
      auto mm = getMinMax(*it);
      fprintf(stdout, "min(%d %d) max(%d %d)\n",
        mm[0].x, mm[0].y, mm[1].x, mm[1].y);
      cv::Rect rct(mm[0], mm[1]);
      int th = 2; // thickness
      cv::rectangle(dst, rct, hc, th, cv::LINE_AA);
    }
    if(bc & 0x02){
#if 0 // only for convex polygon
      cv::fillConvexPoly(dst, &(*it)[0], (*it).size(), hc, cv::LINE_AA);
#else // draw each contour as ncontours==1
      const cv::Point *ppt[] = {&(*it)[0]};
      int npt[] = {(int)(*it).size()};
      size_t nsz = _countof(npt);
      int th = 1; // thickness
      bool c = true; // closed
      if(mode) cv::fillPoly(dst, ppt, npt, nsz, hc, cv::LINE_AA);
      else cv::polylines(dst, ppt, npt, nsz, c, hc, th, cv::LINE_AA);
#endif
    }
  }
  // repeat after draw all contours to over draw texts
  num = 0;
  for(auto it = contours.begin(); it != contours.end(); ++it, ++num){
    int h = hierarchy[num][3]; // parent
    cv::Scalar &hc = hcols[h < 0 ? 0 : (h % (hcols.size() - 2)) + 1]; // no bk
    string &p = h < 0 ? "p" : format(":%d", h);
    putFmtTxt(dst, (*it)[h < 0 ? 0 : it->size() / 2], hc, "%zd%s", num, &p[0]);
  }
  return ncontours;
}

size_t drawPolys(cv::Mat &dst, int mode,
  vector<vector<cv::Point> > &contours, vector<cv::Vec4i> &hierarchy,
  const cv::Scalar &col)
{
  size_t ncontours = contours.size();
  if(!ncontours) return 0;
  int c = -1; // contourIdx ( <0: all contours, =>0: index )
  int th = mode ? -1 : 2; // thickness ( <0: fill, >0: line )
  int d = 2; // maxLevel (depth)
  cv::drawContours(dst, contours, c, col, th, cv::LINE_AA, hierarchy, d);
  return ncontours;
}

size_t drawPolys(cv::Mat &dst, int mode,
  vector<vector<cv::Point> > &vpts, const cv::Scalar &col)
{
  size_t nvpts = vpts.size();
  if(!nvpts) return 0;
  vector<const cv::Point *> ppt(nvpts);
  vector<int> npt(nvpts);
  size_t num = 0;
  for(auto it = vpts.begin(); it != vpts.end(); ++it, ++num){
    ppt[num] = &(*it)[0];
    npt[num] = (int)(*it).size();
  }
  size_t nsz = npt.size();
  int th = 2; // thickness
  bool c = true; // closed
  if(mode) cv::fillPoly(dst, &ppt[0], &npt[0], nsz, col, cv::LINE_AA);
  else cv::polylines(dst, &ppt[0], &npt[0], nsz, c, col, th, cv::LINE_AA);
  return nvpts;
}

}

#endif // __CVUTILS_HPP__
