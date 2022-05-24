/*
  convert_PolyLine_to_Bezier.cpp

  -source-charset:utf-8 -execution-charset:utf-8
  test with OpenCV3 (x64 vc15) FreeType (x64)
  link opencv_world3412.lib (dll) freetype.lib (dll)
*/

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
#include <cvUtils.hpp>
#include <cvFreeTypeFace.hpp>

#define BASEDIR "E:\\virtual2\\SlowShutter\\"
#define V_W 640
#define V_H 480
#define OUTDIR BASEDIR
#define IM_W 640
#define IM_H 480

#define BASECOUNT 40000
#define BREAKCOUNT 600 // when NOWAIT (600/24sec 1536KB) (save 10fps:60frms)
// #define NOWAIT // NOWAIT: <30fps, no NOWAIT: <17fps (depends on CPU)

#define FMTPNG "%scontours_font_%02d_%s.png"

using namespace std;
using namespace utf8string;
using namespace cvutils;
using namespace cvfreetypeface;

int font_outline_test(cv::Mat &dst,
  cvFTManager &ftm, int n, const string &facename)
{
  const string abN("aAbB");
  const string a2z("abcdefghijklmnopqrstuvwxyz");
  const string ivs("祗祇󠄁祇󠄀祇衹");
  cv::Scalar b(255, 0, 0); // BGR HSV(120, 255, 255)
  cv::Scalar g(0, 255, 0); // BGR HSV(60, 255, 255)
  cv::Scalar r(0, 0, 255); // BGR HSV(0, 255, 255) Hue(R, G, B)=(0, 60, 120)
  cv::Scalar w(255, 255, 255);
  cvFTFace &f2 = ftm.getFace(n + 2);
  cvFTFace &f1 = ftm.getFace(n + 1);
  cvFTFace &ff = ftm.getFace(n);
  cv::Mat grayROI(32, 280, CV_8UC1);
  grayROI.setTo(160); // gray
  ff.pUTF8(grayROI, facename, cv::Point(4, 0), 24, g); // test ff color on gray
  cv::cvtColor(grayROI, grayROI, cv::COLOR_GRAY2BGR);
  grayROI.copyTo(dst(cv::Rect(340, 8, 280, 32)));
  // some font size <= 16 used bitmap without FT_LOAD_NO_BITMAP
  ff.pUTF8(dst, abN, cv::Point(4, 4), 16, b, 1, cv::LINE_AA, false, g, 1);
  ff.pUTF8(dst, a2z, cv::Point(40, 8), 16, g, 1, cv::LINE_AA, false, b, 1);
  ff.pUTF8(dst, a2z, cv::Point(0, 24), 24, g, 1, cv::LINE_AA, false, b, -1);
  ff.pUTF8(dst, a2z, cv::Point(0, 48), 32, b, 1, cv::LINE_AA, false, r, 1);
  ff.pUTF8(dst, a2z, cv::Point(0, 80), 64, b, 1, cv::LINE_AA, false, r, -1);
  f2.pUTF8(dst, "4∀ΔVA∇Å", cv::Point(0, 216), 64, r, 1, 4, true); // blo
  f1.pUTF8(dst, "8∀ΔVA∇Å", cv::Point(0, 280), 64, r, 1, 8, true); // blo
  ff.pUTF8(dst, "Space X -=-!|#", cv::Point(0, 280), 64, r);
  f2.pUTF8(dst, "園遠淵円圓延", cv::Point(350, 172), 48, w);
  f1.pUTF8(dst, "園遠淵円圓延", cv::Point(350, 222), 48, w, 1, 1);
  f1.pUTF8(dst, "まみむめもなにぬねの", cv::Point(120, 332), 48, w);
  f2.pUTF8(dst, "國", cv::Point(8, 340), 96, w); // test contours
  ff.pUTF8(dst, ivs, cv::Point(120, 380), 64, w); // test IVS
  ff.pUTF8(dst, ivs, cv::Point(24, 450), 24, b, 1, cv::LINE_AA, false, g, 1);
  ff.pUTF8(dst, "鷗𠮟", cv::Point(500, 380), 64, w); // test Surrogate-pair
  cv::imwrite(format(FMTPNG, BASEDIR, n, facename.c_str()).c_str(), dst);
  return 0;
}

int main(int ac, char **av)
{
  fprintf(stdout, "sizeof(size_t): %zu\n", sizeof(size_t));

  vector<cv::Scalar> colors = createColors();
  cvFTManager ftm;
  char *basedir[] = {".\\", "C:\\Windows\\Fonts\\"};
  char *fs[] = {"mikaP.ttf", "migu-1m-regular.ttf", "ipaexg.ttf", "ipaexm.ttf",
    "DFLgs9.ttc", "migmix-1m-regular.ttf", "msgothic.ttc", "msmincho.ttc"};
  for(int n = 0; n < _countof(fs); ++n)
    ftm.addFace(format("%s%s", basedir[n ? 1 : 0], fs[n]));
  cv::Mat dst(IM_H, IM_W, CV_8UC3);
  dst.setTo(cv::Scalar(128, 192, 32)); // BGR
  vector<cv::Point> trO{{-20 + 40, 50}, {-20 + 70, 30}, {-20 + 80, 60}}; // R
  vector<cv::Point> trR{{40, 50}, {70, 30}, {80, 60}}; // R
  vector<vector<cv::Point> > vtrX{trO, trO, trR}; // O-O-R OK like line+fill

  vector<string> wn({"Src", "Gray", "Diff", "KpPk", "Mask", "Dst"});
  for(vector<string>::iterator i = wn.begin(); i != wn.end(); ++i)
    cv::namedWindow(*i, CV_WINDOW_AUTOSIZE | CV_WINDOW_FREERATIO);
  int cam_id = 0; // 1; // 0; // may be 'ManyCam Virtual Webcam'
  int width = IM_W, height = IM_H, fourcc;
  double fps = 27.0; // 30.0;
#if 1
  cv::VideoCapture cap(cv::CAP_DSHOW + cam_id); // cap(FN);
#else
/*
  see also test_OpenCV3.py
 path ../FFmpeg/ffmpeg-2022-02-17-git-2812508086-essentials_build/bin/ff....exe
 client first
  # ffplay -probesize 32 -sync ext udp://127.0.0.1:11111
  convert_PolyLine_to_Bezier (self)
 server second
  ffmpeg -i _map.mp4 -f mpegts udp://127.0.0.1:11111
*/
  char *udp = "udp://127.0.0.1:11111";
  fprintf(stdout, "open [%s]\n", udp);
  cv::VideoCapture cap(udp); // ffmpeg OpenCV H.264
  if(!cap.isOpened()) cap.open(udp);
#endif
  if(!cap.isOpened()) return 1;
  fprintf(stdout, "width: %d, height %d\n",
    (int)cap.get(cv::CAP_PROP_FRAME_WIDTH),
    (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT));
  // save to .mp4 and convert .mp4 by ShinkuSuperLite
  fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v'); // OK .p4 (mp4v)
  // fourcc = cv::VideoWriter::fourcc('X', 'V', 'I', 'D'); // OK must save .avi
  // fourcc = cv::VideoWriter::fourcc('X', '2', '6', '4'); // OK .avi need .dll
  // fourcc = cv::VideoWriter::fourcc('H', '2', '6', '4'); // OK .avi need .dll
  // fourcc = 0x00000020; // fallback tag
  bool col = true;
  cv::VideoWriter wr(OUTDIR"_p2b.mp4", fourcc, fps, cv::Size(IM_W, IM_H), col);
  cv::Mat frm(V_H, V_W, CV_8UC3);
  cv::Mat gry(IM_H, IM_W, CV_8UC1); // to CV_8UC3
  cv::Mat img(IM_H, IM_W, CV_8UC3);
  int cnt = BASECOUNT;
  long tick0 = cvGetTickCount();
  while(cap.read(frm)){
    long tick = cvGetTickCount() - tick0;
    double sec = (tick / cvGetTickFrequency()) / 1000000.0; // usec to sec
    int fcnt = cnt - BASECOUNT + 1;
    double ffps = fcnt / sec; // not same as fps of the cv::VideoWriter

    if(!(cnt % 30)){
      dst.setTo(cv::Scalar(128, 192, 32)); // BGR
      drawPolys(dst, 1, vtrX, colors[0] / 2);
      int n = (cnt / 30) % _countof(fs);
#if 1
      fprintf(stderr, "%d [%s]\n", n, fs[n]);
#endif
      font_outline_test(dst, ftm, n, fs[n]);
    }

    // now putFmtTxt does not erase previous drawing (as fast as possible)
    putFmtTxt(dst, cv::Point{20, 460}, colors[0], "FPS:%06.2f", ffps);
    putFmtTxt(dst, cv::Point{120, 460}, colors[1], "Time:%07.2f", sec);
    putFmtTxt(dst, cv::Point{240, 460}, colors[2], "Frame: %08d", fcnt);

    cv::GaussianBlur(frm, frm, cv::Size(3, 3), 0);
    cv::Mat tmp;
    cv::resize(frm, tmp, cv::Size(IM_W, IM_H), 0, 0, cv::INTER_LANCZOS4);
    if(cnt == BASECOUNT) tmp.copyTo(img);
    cv::cvtColor(tmp, gry, cv::COLOR_BGR2GRAY); // CV_8UC1
    cv::cvtColor(gry, gry, cv::COLOR_GRAY2BGR); // CV_8UC3

    dst.copyTo(img);

    cv::imshow("Src", tmp);
    cv::imshow("Gray", gry);
    cv::imshow("Diff", dst);
    cv::imshow("Dst", img);
    wr << img;
    ++cnt;
    int k = cv::waitKey(1); // 1ms > 15ms ! on Windows
    if(k == 'q' || k == '\x1b') break;
  }
  wr.release();
  cap.release();
  cv::destroyAllWindows();
  fprintf(stdout, "frames: %d\n", cnt);
  return 0;
}
