//
//  SketchEngine.h
//  Im2Sketch
//
//  Created by Zhipeng Wu on 2/16/13.
//  Copyright (c) 2013 Zhipeng Wu. All rights reserved.
//
//  Reference:
//  "Combining Sketch and Tone for Pencil Drawing Production"
//  Cewu Lu, Li Xu, Jiaya Jia
//  International Symposium on Non-Photorealistic Animation and Rendering
//  (NPAR 2012), June, 2012

#ifndef __Im2Sketch__SketchEngine__
#define __Im2Sketch__SketchEngine__

#include <string>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
using std::vector;
using std::cout;
using std::endl;
using std::string;

class SketchEngine {
public:
  // Constructors:
  explicit SketchEngine(const string& file_path, const string& tonal_path) {
    image_ = cv::imread(file_path, 1);
    tonal_sample_ = cv::imread(tonal_path, 0);
    if ((!image_.empty()) && (!tonal_sample_.empty())) {
      rows_ = image_.rows;
      cols_ = image_.cols;
      cv::cvtColor(image_, gray_image_, CV_BGR2GRAY);
      gray_image_.convertTo(gray_image_, CV_32FC1);
      tonal_sample_.convertTo(tonal_sample_, CV_32FC1);
    }
  }
  explicit SketchEngine(const cv::Mat& image, const cv::Mat& tonal) {
    if ((!image.empty()) && (!tonal.empty())) {
      if (3 == image.channels()) {
        image.copyTo(image_);
      } else {
        cv::cvtColor(image, image_, CV_GRAY2BGR);
      }
      if (3 == tonal.channels()) {
        cv::cvtColor(tonal, tonal_sample_, CV_BGR2GRAY);
      } else {
        tonal.copyTo(tonal_sample_);
      }
      rows_ = image_.rows;
      cols_ = image_.cols;
      cv::cvtColor(image_, gray_image_, CV_BGR2GRAY);
      gray_image_.convertTo(gray_image_, CV_32FC1);
      tonal_sample_.convertTo(tonal_sample_, CV_32FC1);
    }
  }
  
  // Accessers:
  const cv::Mat& pencil_sketch() const {
    return pencil_sketch_;
  }
  const cv::Mat& color_sketch() const {
    return color_sketch_;
  }
  
  // Sketch conversion:
  bool Convert2Sketch(int hardness, int directions, float strength);
  bool Convert2Sketch() {
    return Convert2Sketch(1, 8, 0.9);
  }
  
private:
  // Member functions.
  // Generate pencil stroke structure.
  cv::Mat StrokeStructure(int hardness, int directions, float strength);
  // Map the gray-scale image to pencil-sketch-like tone.
  cv::Mat ToneMapping();
  // Histogram specification by using group mapping.
  bool HistSpecification(const cv::Mat& c_target, cv::Mat* tone);
  // Pencil drawing texture rendering by using tonal sample image.
  cv::Mat TextureRendering(cv::Mat& tone);
  // Generate directional masks.
  cv::Mat GenerateMask(int mask_size, float angle, int hardness);
  cv::Mat image_;            // Original input image.
  cv::Mat gray_image_;       // Grau-scale image of the original input.
  cv::Mat tonal_sample_;     // The tonal sample used for texture rendering.
  cv::Mat pencil_sketch_;    // Single color (grayscale) sketch.
  cv::Mat color_sketch_;     // Colored sketch.
  int rows_;                 // Original image row number.
  int cols_;                 // Original image col number.
  
  // Disallow copy and assign.
  void operator= (const SketchEngine&);
  SketchEngine(const SketchEngine&);
};


#endif /* defined(__Im2Sketch__SketchEngine__) */
