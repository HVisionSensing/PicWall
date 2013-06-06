//
//  CartoonEngine.h
//  Im2Cartoon
//
//  Created by Zhipeng Wu on 2/15/13.
//  Copyright (c) 2013 Zhipeng Wu. All rights reserved.
//

#ifndef __Im2Cartoon__CartoonEngine__
#define __Im2Cartoon__CartoonEngine__

#include "CoherentLine.h"
#include <string>
#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>
using std::cout;
using std::endl;
using std::string;
using std::vector;

class CartoonEngine {
public:
  // Constructors:
  explicit CartoonEngine(const string& file_path) {
    image_ = cv::imread(file_path, 1);
    if (!image_.empty()) {
      rows_ = image_.rows;
      cols_ = image_.cols;
    }
  }
  explicit CartoonEngine(const cv::Mat& image) {
    if (!image.empty()) {
      if (3 == image.channels()) {
        image.copyTo(image_);
      } else {
        cv::cvtColor(image, image_, CV_GRAY2BGR);
      }
      rows_ = image_.rows;
      cols_ = image_.cols;
    }
  }
  
  // Accessers:
  const cv::Mat& cartoon() const {
    return cartoon_;
  }
  const cv::Mat& painting() const {
    return painting_;
  }
  
  // Cartoon conversion:
  bool Convert2Cartoon(int iter_num,
                       int d,
                       double sigma_color,
                       double sigma_space,
                       double max_gradient,
                       double min_edge_strength);
  bool Convert2Cartoon() {
    return Convert2Cartoon(7, 6, 9, 7, 20000, 0.3);
  }
  // Oil painting convertion.
  bool Convert2Painting(int neighbor, int levels);
  bool Convert2Painting() {
    return Convert2Painting(2, 256);
  }
  
private:
  // A: do bilateral filter for R, G, and B channels.
  bool Bilateral(int iter_num,
                 int d,
                 double sigma_color,
                 double sigma_space);
  // B: convert image from BGR to Lab.
  // Only do bilateral filtering for L channel.
  // Finally reconstruct the image (Lab to BGR).
  bool Bilateral2(int iter_num,
                  int d,
                  double sigma_color,
                  double sigma_space);
  // Generate image abstraction by edge line drawing.
  cv::Mat ImageAbstraction(double max_gradient, double min_edge_strength);
  // Luminance quantization.
  bool LuminanceQuantization(cv::Mat* luminance, int levels);
  cv::Mat image_;      // Original input image.
  cv::Mat cartoon_;    // Converted cartoon image.
  cv::Mat painting_;   // COnverted oil painting image.
  int rows_;           // Original image row number.
  int cols_;           // Original image col number.
  
  // Disallow copy and assign.
  void operator= (const CartoonEngine&);
  CartoonEngine(const CartoonEngine&);
};

#endif /* defined(__Im2Cartoon__CartoonEngine__) */
