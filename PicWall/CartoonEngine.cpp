//
//  CartoonEngine.cpp
//  Im2Cartoon
//
//  Created by Zhipeng Wu on 2/15/13.
//  Copyright (c) 2013 Zhipeng Wu. All rights reserved.
//

#include "CartoonEngine.h"

bool CartoonEngine::Convert2Cartoon(int iter_num,
                                    int d,
                                    double sigma_color,
                                    double sigma_space,
                                    double max_gradient,
                                    double min_edge_strength) {
  // Step 1: bilateral filter the original image.
  if(!Bilateral2(iter_num, d, sigma_color, sigma_space))
    return false;
  // Step 2: generate image abstraction by detecting edges.
  cv::Mat edge_map = ImageAbstraction(max_gradient, min_edge_strength);
  // Step 3: merge the results from step 1 and 2.
  for (int r = 0; r < rows_; ++r) {
    for (int c = 0; c < cols_; ++c) {
      cv::Vec3b value = cartoon_.at<cv::Vec3b>(r, c);
      value = static_cast<cv::Vec3b>(value * edge_map.at<float>(r, c));
      cartoon_.at<cv::Vec3b>(r, c) = value;
    }
  }
  return true;
}

cv::Mat CartoonEngine::ImageAbstraction(double max_gradient,
                                        double min_edge_strength) {
//  cv::Mat edge_map(rows_, cols_, CV_32FC1, cv::Scalar(0));
//  if ((image_.empty()) || (CV_8UC3 != image_.type()))
//    return edge_map;
//  // Get sobel gradients.
//  cv::Mat gray;
//  cv::cvtColor(image_, gray, CV_BGR2GRAY);
//  cv::Mat gx, gy;
//  cv::Sobel(gray, gx, CV_32FC1, 1, 0);
//  cv::Sobel(gray, gy, CV_32FC1, 0, 1);
//  for (int r = 0; r < rows_; ++r) {
//    for (int c = 0; c < cols_; ++c) {
//      float value = gx.at<float>(r, c) * gx.at<float>(r, c) +
//      gy.at<float>(r, c) * gy.at<float>(r, c);
//      if (value > max_gradient)
//        value = 1;
//      else
//        value = value / max_gradient;
//      if (value < min_edge_strength)
//        edge_map.at<float>(r, c) = 1;
//      else
//        edge_map.at<float>(r, c) = 1 - value;
//    }
//  }
//  return edge_map;
  CoherentLine cl(image_);
  return cl.fdog_edge();
}


bool CartoonEngine::Bilateral(int iter_num,
                              int d,
                              double sigma_color,
                              double sigma_space) {
  if ((image_.empty()) || (CV_8UC3 != image_.type()) || (iter_num < 1))
    return false;
  cv::Mat cartoon_img = image_;
  cv::Mat temp;
  // Step 1: resize the original image to save pprocessing time (if needed).
  cv::Size2i small_size(cols_, rows_);
  if ((rows_ > 500) || (cols_ > 500)) {
    if (rows_ > cols_) {
      small_size.height = 500;
      small_size.width =
      static_cast<int>(500 * (static_cast<float>(cols_) / rows_));
    } else {
      small_size.width = 500;
      small_size.height =
      static_cast<int>(500 * (static_cast<float>(rows_) / cols_));
    }
    cv::resize(cartoon_img, cartoon_img, small_size);
    temp.create(small_size, CV_8UC3);
  } else {
    temp.create(rows_, cols_, CV_8UC3);
  }
  // Step 2: do bilateral filtering for several times for cartoon rendition.
  for (int i = 0; i < iter_num; ++i) {
    cv::bilateralFilter(cartoon_img, temp, d, sigma_color, sigma_space);
    cv::bilateralFilter(temp, cartoon_img, d, sigma_color, sigma_space);
  }
  // Step 3: luminance quantization.
  // Step 4: revert the original image size.
  if (small_size.width != cols_) {
    cv::resize(cartoon_img, cartoon_img, cv::Size2i(cols_, rows_));
  }
  cartoon_img.copyTo(cartoon_);
  return true;
}

bool CartoonEngine::Bilateral2(int iter_num,
                               int d,
                               double sigma_color,
                               double sigma_space) {
  if ((image_.empty()) || (CV_8UC3 != image_.type()) || (iter_num < 1))
    return false;
  
  // Step 1: convert color, only do filtering on the luminance channel.
  cv::Mat bgr_img = image_;
  cv::Mat lab_img;
  cv::cvtColor(bgr_img, lab_img, CV_BGR2Lab);
  std::vector<cv::Mat> lab_channels;
  cv::split(lab_img, lab_channels);
  cv::Mat temp;
  // Step 1: resize the original image to save pprocessing time (if needed).
  cv::Size2i small_size(cols_, rows_);
  if ((rows_ > 500) || (cols_ > 500)) {
    if (rows_ > cols_) {
      small_size.height = 500;
      small_size.width =
      static_cast<int>(500 * (static_cast<float>(cols_) / rows_));
    } else {
      small_size.width = 500;
      small_size.height =
      static_cast<int>(500 * (static_cast<float>(rows_) / cols_));
    }
    cv::resize(lab_channels[0], lab_channels[0], small_size);
    temp.create(small_size, CV_8UC1);
  } else {
    temp.create(rows_, cols_, CV_8UC1);
  }
  // Step 2: do bilateral filtering for several times for cartoon rendition.
  for (int i = 0; i < iter_num; ++i) {
    cv::bilateralFilter(lab_channels[0], temp, d, sigma_color, sigma_space);
    cv::bilateralFilter(temp, lab_channels[0], d, sigma_color, sigma_space);
  }
  // Step 3: luminance quantization.
  if(!LuminanceQuantization(&lab_channels[0], 8))
    return false;
  // Step 4: revert the original image size.
  if (small_size.width != cols_) {
    cv::resize(lab_channels[0], lab_channels[0], cv::Size2i(cols_, rows_));
  }
  cv::merge(lab_channels, lab_img);
  cv::cvtColor(lab_img, bgr_img, CV_Lab2BGR);
  bgr_img.copyTo(cartoon_);
  return true;
}

bool CartoonEngine::LuminanceQuantization(cv::Mat* luminance, int levels){
  if ((NULL == luminance) ||luminance->empty() ||
      (CV_8UC1 != luminance->type()))
    return false;
  int diff = static_cast<int>(static_cast<float>(100) / levels);
  for (int r = 0; r < luminance->rows; ++r) {
    for (int c = 0; c < luminance->cols; ++c) {
      uchar value = luminance->at<uchar>(r, c);
      value = static_cast<int>(value / diff * diff);
      luminance->at<uchar>(r, c) = value;
    }
  }
  return true;
}

bool CartoonEngine::Convert2Painting(int neighbor, int levels) {
  if ((image_.empty()) || (CV_8UC3 != image_.type()))
    return false;
  
  // Resize image for fast processing.
  cv::Size2i small_size(cols_, rows_);
  cv::Mat img = image_;
  if ((rows_ > 300) || (cols_ > 300)) {
    if (rows_ > cols_) {
      small_size.height = 300;
      small_size.width =
      static_cast<int>(300 * (static_cast<float>(cols_) / rows_));
    } else {
      small_size.width = 300;
      small_size.height =
      static_cast<int>(300 * (static_cast<float>(rows_) / cols_));
    }
    cv::resize(img, img, small_size);
  }
  painting_.create(small_size.height, small_size.width, CV_8UC3);
  vector<int> hist(levels, 0);
  vector<int> b_sum(levels, 0);
  vector<int> g_sum(levels, 0);
  vector<int> r_sum(levels, 0);
  
  for (int r = 0; r < small_size.height; ++r) {
    for (int c = 0; c < small_size.width; ++c) {
      for (int i = 0; i < levels; ++i) {
        hist[i] = 0;
        b_sum[i] = g_sum[i] = r_sum[i] = 0;
      }
      int r_start = std::max(0, r - neighbor);
      int r_end = std::min(small_size.height, r + neighbor);
      int c_start = std::max(0, c - neighbor);
      int c_end = std::min(small_size.width, c + neighbor);
      for (int rr = r_start; rr < r_end; ++rr) {
        for (int cc = c_start; cc < c_end; ++cc) {
          cv::Vec3b pixel = img.at<cv::Vec3b>(rr, cc);
          int ind = static_cast<int>((pixel[0] + pixel[1] + pixel[2]) / 3) * (levels - 1) / 255;
          ++hist[ind];
          b_sum[ind] += static_cast<int>(pixel[0]);
          g_sum[ind] += static_cast<int>(pixel[1]);
          r_sum[ind] += static_cast<int>(pixel[2]);
        }
      }
      
      int new_ind = 0;
      int cur_max = hist[0];
      for (int l = 1; l < levels; ++l) {
        if (hist[l] > hist[new_ind]) {
          new_ind = l;
          cur_max = hist[l];
        }
      }
      
      painting_.at<cv::Vec3b>(r, c)[0] =
      static_cast<uchar>(b_sum[new_ind] / cur_max);
      painting_.at<cv::Vec3b>(r, c)[1] =
      static_cast<uchar>(g_sum[new_ind] / cur_max);
      painting_.at<cv::Vec3b>(r, c)[2] =
      static_cast<uchar>(r_sum[new_ind] / cur_max);
    }
  }
  if (small_size.width != cols_) {
    cv::resize(painting_, painting_, cv::Size2i(cols_, rows_));
  }
  return true;
}