//
//  SketchEngine.cpp
//  Im2Sketch
//
//  Created by Zhipeng Wu on 2/16/13.
//  Copyright (c) 2013 Zhipeng Wu. All rights reserved.
//

#include "SketchEngine.h"

bool SketchEngine::Convert2Sketch(int hardness, int directions, float strength) {
  if (image_.empty() || gray_image_.empty() || tonal_sample_.empty())
    return false;
  // structure ranges at [0, 1]
  cv::Mat structure = StrokeStructure(hardness, directions, strength);
  // tone ranges at [0, 1];
  cv::Mat tone = ToneMapping();
  // texture ranges at [0, 1];
  cv::Mat texture = TextureRendering(tone);
  // pencil ranges at [0, 1];
  cv::Mat pencil = structure.mul(texture);
  //  cv::imshow("structure", structure);
  //  cv::waitKey();
  //  cv::imshow("tone", tone);
  //  cv::waitKey();
  //  cv::imshow("texture", texture);
  //  cv::waitKey();
  //  cv::imshow("pencil", pencil);
  //  cv::waitKey();
  pencil = pencil * 255;
  pencil.convertTo(pencil_sketch_, CV_8UC1);
  //  cv::imshow("pencil_sketch_", pencil_sketch_);
  //  cv::waitKey();
  
  // Create color pencil.
  cv::Mat lab_img;
  cv::cvtColor(image_, lab_img, CV_BGR2Lab);
  vector<cv::Mat> channels;
  cv::split(lab_img, channels);
  channels[0] = pencil_sketch_;
  cv::merge(channels, lab_img);
  cv::cvtColor(lab_img, color_sketch_, CV_Lab2BGR);
  //  cv::imshow("color_sketch_", color_sketch_);
  //  cv::waitKey();
  return true;
}

// Generate pencil stroke structure.
cv::Mat SketchEngine::StrokeStructure(int hardness, int directions, float strength) {
  vector<cv::Mat> masks;
  vector<cv::Mat> Gs;
  vector<cv::Mat> Cs;
  
  // Step 1: get gradients.
  cv::Mat gx, gy, gradient;
  cv::Sobel(gray_image_, gx, CV_32FC1, 1, 0);
  cv::Sobel(gray_image_, gy, CV_32FC1, 0, 1);
  cv::magnitude(gx, gy, gradient);
  //  cv::imshow("gradient", gradient / 200);
  //  cv::waitKey();
  // Step 2: filtering image by using directional masks.
  int mask_size;
  if (rows_ > cols_)
    mask_size = cols_ / 30;
  else
    mask_size = rows_ / 30;
  for (int i = 0; i < directions; ++i) {
    float angle = static_cast<float>(i) * 180 / directions;
    cv::Mat mask = GenerateMask(mask_size, angle, hardness);
    cv::Mat G;
    cv::Mat C(rows_, cols_, CV_32FC1, cv::Scalar(0));
    cv::filter2D(gradient, G, -1, mask);
    masks.push_back(mask);
    Gs.push_back(G);
    Cs.push_back(C);
  }
  // Step 3: set the Cs.
  for (int r = 0; r < rows_; ++r) {
    for (int c =0; c < cols_; ++c) {
      int index = 0;
      float max_value = 0;
      for (int i = 0; i < directions; ++i) {
        if (Gs[i].at<float>(r, c) > max_value) {
          max_value = Gs[i].at<float>(r, c);
          index = i;
        }
      }
      Cs[index].at<float>(r, c) = gradient.at<float>(r, c);
    }
  }
  // Step 4: Line shaping.
  cv::Mat structure(rows_, cols_, CV_32FC1, cv::Scalar(0));
  for (int i = 0; i < directions; ++i) {
    cv::filter2D(Cs[i], Cs[i], -1, masks[i]);
    structure += Cs[i];
  }
  // Step 5: normalize structure to range [0, 1].
  cv::normalize(structure, structure, 0, 1, cv::NORM_MINMAX);
  structure = (cv::Mat::ones(rows_, cols_, CV_32FC1) - structure * strength);
  //  cv::imshow("structure", structure);
  //  cv::waitKey();
  return structure;
}

// Generate directional masks.
cv::Mat SketchEngine::GenerateMask(int mask_size, float angle, int hardness) {
  assert(mask_size > 1);
  int midline = mask_size / 2;
  int size = midline * 2 - 1;
  cv::Mat mask(size, size, CV_32FC1, cv::Scalar(0));
  for (int i = 0; i < size; ++i) {
    mask.at<float>(midline, i) = 255;
    for (int j = 0; j < hardness - 1; ++ j) {
      mask.at<float>(midline + j, i) = 255;
      mask.at<float>(midline - j, i) = 255;
    }
  }
  // Rotate according to angle.
  cv::Point2i src_center(midline, midline);
  cv::Mat rot_mat = cv::getRotationMatrix2D(src_center, angle, 1.0);
  cv::warpAffine(mask, mask, rot_mat, mask.size());
  return mask;
}

// Map the gray-scale image to pencil-sketch-like tone.
cv::Mat SketchEngine::ToneMapping() {
  cv::Mat tone(rows_, cols_, CV_32FC1, cv::Scalar(0));
  if (gray_image_.empty())
    return tone;
  
  // Prepare tartget cumulative hisotgram.
  cv::Mat c_target(256, 1, CV_32FC1);
  cv::Mat h_bright(256, 1, CV_32FC1);
  cv::Mat h_dark(256, 1, CV_32FC1);
  cv::Mat h_mid(256, 1, CV_32FC1);
  float sum_bright = 0;
  float sum_dark = 0;
  float sum_mid = 0;
  float sum = 0;
  for (int i = 0; i < 256; ++i) {
    // bright tone.
    h_bright.at<float>(i) = expf(static_cast<float>(i - 255) / 9);
    sum_bright += h_bright.at<float>(i);
    // dark tone.
    h_dark.at<float>(i) = expf(static_cast<float>((i - 90) * (i - 90)) / -242);
    sum_dark += h_dark.at<float>(i);
    // middle tone
    if ((i <= 225) && (i >= 105))
      h_mid.at<float>(i) = 1;
    else
      h_mid.at<float>(i) = 0;
    sum_mid += h_mid.at<float>(i);
  }
  
  float bright, dark, mid;
  for (int i = 0; i < 256; ++i) {
    bright = h_bright.at<float>(i) / sum_bright;
    dark = h_dark.at<float>(i) / sum_dark;
    mid = h_mid.at<float>(i) / sum_mid;
    sum += (52 * bright + 37 * mid + 11 * dark) / 100;
    c_target.at<float>(i) = sum;
  }
  
  // Tone mapping by using histogram specification.
  HistSpecification(c_target, &tone);
  //  cv::imshow("tone", tone);
  //  cv::waitKey();
  return tone;
}

// Histogram specification by using group mapping.
bool SketchEngine::HistSpecification(const cv::Mat& c_target, cv::Mat* tone) {
  if (c_target.empty() || gray_image_.empty())
    return false;
  // The mapping of pixel values from original image to target image.
  cv::Mat hist_map(1, 256, CV_32FC1);
  // Histogram for original image. (CV_32FC1)
  cv::Mat h_ori;
  float range[] = {0, 255};
  const float* ranges = {range};
  int histSize = 256;
  cv::calcHist(&gray_image_, 1, 0, cv::Mat(), h_ori, 1, &histSize, &ranges, true, false);
  // Cumulative histogram for original image.
  cv::Mat c_ori(256, 1, CV_32FC1);
  float sum = 0;
  for (int i = 0; i < 256; ++i) {
    sum += h_ori.at<float>(i);
    c_ori.at<float>(i) = sum;
  }
  for (int i = 0; i < 256; ++i) {
    c_ori.at<float>(i) /= sum;
  }
  
  // Distance matrix.
  cv::Mat dist(256, 256, CV_32FC1);
  for (int y = 0; y < 256; ++y) {
    for (int x = 0; x < 256; ++x) {
      dist.at<float>(x, y) = fabs(c_ori.at<float>(y) - c_target.at<float>(x));
    }
  }
  // Construct mapping index.
  int last_start_y = 0, last_end_y = 0, start_y = 0, end_y = 0;
  float min_dist = 0;
  for (int x = 0; x < 256; ++x) {
    min_dist = dist.at<float>(x, 0);
    for (int y = 0; y < 256; ++y) {
      if (min_dist >= dist.at<float>(x, y)) {
        end_y = y;
        min_dist = dist.at<float>(x, y);
      }
    }
    if ((start_y != last_start_y) || (end_y != last_end_y)) {
      for (int i = start_y; i <= end_y; ++i) {
        hist_map.at<float>(i) = static_cast<float>(x);
      }
      last_start_y = start_y;
      last_end_y = end_y;
      start_y = last_end_y + 1;
    }
  }
  
  // Image pixel value mapping.
  for (int r = 0; r < rows_; ++r) {
    for (int c = 0; c < cols_; ++c) {
      float gray = gray_image_.at<float>(r, c);
      float new_gray = hist_map.at<float>(static_cast<int>(gray)) / 255.0;
      tone->at<float>(r, c) = new_gray;
    }
  }
  return true;
}

// Pencil drawing texture rendering by using tonal sample image.
cv::Mat SketchEngine::TextureRendering(cv::Mat& tone) {
  cv::Mat texture(rows_, cols_, CV_32FC1, cv::Scalar(0));
  if (tone.empty())
    return texture;
  
  // Get a large tonal sample, by naive tiling.
  int ny = ceil(static_cast<float>(rows_) / tonal_sample_.rows);
  int nx = ceil(static_cast<float>(cols_) / tonal_sample_.cols);
  if ((ny > 1) || (nx > 1))
    cv::repeat(tonal_sample_, ny, nx, tonal_sample_);
  cv::resize(tonal_sample_, tonal_sample_, cv::Size2i(cols_, rows_));
  tonal_sample_ = tonal_sample_ / 255;
  
  cv::Mat Beta(rows_, cols_, CV_32FC1, cv::Scalar(1));
  for (int c = 0; c < cols_; ++c) {
    Beta.at<float>(0, c) =
    logf(tone.at<float>(0, c) + 0.000001) / logf(tonal_sample_.at<float>(0, c) + 0.000001);
  }
  for (int r = 0; r < rows_; ++r) {
    Beta.at<float>(r, 0) =
    logf(tone.at<float>(r, 0) + 0.000001) / logf(tonal_sample_.at<float>(r, 0) + 0.000001);
  }
  for (int r = 1; r < rows_; ++r) {
    for (int c = 1; c < cols_; ++c) {
      float t = logf(tonal_sample_.at<float>(r, c) + 0.000001);
      float v = (t * logf(tone.at<float>(r, c) + 0.000001) +
                 0.2 * (Beta.at<float>(r - 1, c) + Beta.at<float>(r, c - 1))) /
      (t * t + 0.4);
      Beta.at<float>(r, c) = v;
    }
  }
  for (int r = 0; r < rows_; ++r) {
    for (int c = 0; c < cols_; ++c) {
      texture.at<float>(r, c) = powf(tonal_sample_.at<float>(r, c), Beta.at<float>(r, c));
    }
  }
  
  return texture;
}