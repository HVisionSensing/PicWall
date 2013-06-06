//
//  Collage.h
//  image-browser
//
//  Created by Zhipeng Wu on 10/2/12.
//
//

#ifndef __image_browser__Collage__
#define __image_browser__Collage__

#include "MangaEngine.h"
#include "CartoonEngine.h"
#include "SketchEngine.h"
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <time.h>
#define random(x) (rand() % x)
#define MAX_ITER_NUM 100      // Max number of aspect ratio adjustment.
#define MAX_TREE_GENE_NUM 10000  // Max number of tree re-generation.

class FloatRect {
public:
  FloatRect () {
    x_ = 0;
    y_ = 0;
    width_ = 0;
    height_ = 0;
  }
  float x_;
  float y_;
  float width_;
  float height_;
};

class TreeNode {
public:
  TreeNode() {
    child_type_ = 'N';
    split_type_ = 'N';
    is_leaf_ = true;
    alpha_ = 0;
    alpha_expect_ = 0;
    position_ = FloatRect();
    left_child_ = NULL;
    right_child_ = NULL;
    parent_ = NULL;
    img_path_ = "";
  }
  char child_type_;      // Is this node left child "l" or right child "r".
  char split_type_;      // If this node is a inner node, we set 'v' or 'h', which indicate
  // vertical cut or horizontal cut.
  bool is_leaf_;         // Is this node a leaf node or a inner node.
  float alpha_expect_;   // If this node is a leaf, we set expected aspect ratio of this node.
  float alpha_;          // If this node is a leaf, we set actual aspect ratio of this node.
  FloatRect position_;    // The position of the node on canvas.
  TreeNode* left_child_;
  TreeNode* right_child_;
  TreeNode* parent_;
  std::string img_path_;
};

class AlphaUnit {
public:
  int image_ind_;          // The related image index.
  float alpha_;            // Aspect ratio value.
  float alpha_recip_;      // Reciprocal sapect ratio value.
  std::string image_path_; // The related image path.
};

// Collage with pre-defined aspect ratio
class CollageAdvanced {
public:
  // Constructors.
  // We need to let the user decide the canvas height.
  // Since the aspect ratio will be calculate by our program, we can compute
  // canvas width accordingly.
  CollageAdvanced(const std::vector<std::string> input_image_list);
  ~CollageAdvanced() {
    ReleaseTree(tree_root_);
    image_alpha_vec_.clear();
  }
  
  // Create collage.
  // width is the canvas width. It is fixed. We can let the canvas height to be changed.
  // If we use CreateCollage, the generated collage may have strange aspect ratio such as
  // too big or too small, which seems to be difficult to be shown. We let the user to
  // input their expected aspect ratio and fast adjust to make the result aspect ratio
  // close to the user defined one.
  // The thresh here controls the closeness between the result aspect ratio and the expect
  // aspect ratio. e.g. expect_alpha is 1, thresh is 2. The result aspect ratio is around
  // [1 / 2, 1 * 2] = [0.5, 2].
  // We also define MAX_ITER_NUM = 100,
  // If max iteration number is reached and we cannot find a good result aspect ratio,
  // this function returns -1.
  int CreateCollage(int width,
                    float expected_alpha,
                    float thresh);
  
  /****************************************************************************/
  //newlly added:
  
  // canvas_size: user-specified canvas size.
  // border_size: the size for white border.
  // threshold: the threshold to stop collage generation.
  // manga_mode: if it is true, the split-type for root node is set to 'h'.
  // style: reading style. ('u': left-to-right; 'j': right-to_left).
  bool CreateCollage(const cv::Size2i canvas_size,
                     const int border_size,
                     const float threshold,
                     const bool manga_mode,
                     const char style);
  // Function overloading:
  bool CreateCollage (const cv::Size2i canvas_size,
                      const int border_size) {
    float threshold = 1.1;
    const bool manga_mode = false;
    const char style = 'u';
    return CreateCollage(canvas_size, border_size, threshold, manga_mode, style);
  }
  bool CreateCollage (const cv::Size2i canvas_size) {
    int border_size = 6;
    return CreateCollage(canvas_size, border_size);
  }
  bool CreateCollage () {
    // A4 paper (portrait) at 150dpi, width = 1240 pixel, height = 1754 pixel.
    cv::Size2i canvas_size(1240, 1754);
    return CreateCollage(canvas_size);
  }
  
  // Create a B5 size (728, 1028) manga.
  bool B5Manga(const char style) {
    const cv::Size2i manga_size(728, 1028);
    const int border_size = 10;
    const float threshold = 1.1;
    bool manga_mode = true;
    return CreateCollage(manga_size, border_size, threshold, manga_mode, style);
  }
  // 1. Japanese reading order (right-left, top-bottom)
  bool B5JPManga() {
    return B5Manga('j');
  }
  // 2. Us reading order (left-right, top-bottom)
  bool B5USManga() {
    return B5Manga('u');
  }
  
  // Collage output:
  // 'type' refers to the kind of non-photorealistic features we provide.
  // 'type = 'p': Output as a photo collage.
  // 'type = 'm': Output as a manga collage.
  // 'type = 'e': Output as a pencil sketch collage.
  // 'type = 'o': Output as a color pencil sketch collage.
  // 'type = 'c': Output as a cartoon collage.
  // 'type = 'i': Output as a oil painting collage.
  cv::Mat OutputCollage(const char type, bool accurate);
  cv::Mat OutputCollage(const char type) {
    return OutputCollage(type, true);
  }
  
  // B. Output as a html file.
  bool OutputHtml(const char type, const std::string output_html_path);
  /****************************************************************************/
  
  // Output collage into a single image.
  cv::Mat OutputCollageImage() const;
  // Output collage into a html page.
  bool OutputCollageHtml (const std::string output_html_path);
  
  // Accessors:
  int image_num() const {
    return image_num_;
  }
  int canvas_height() const {
    return canvas_height_;
  }
  int canvas_width() const {
    return canvas_width_;
  }
  float canvas_alpha() const {
    return canvas_alpha_;
  }
  
private:
  // Recursively calculate aspect ratio for all the inner nodes.
  // The return value is the aspect ratio for the node.
  float CalculateAlpha(TreeNode* node);
  // Top-down Calculate the image positions in the colage.
  bool CalculatePositions(TreeNode* node);
  // newlly added:
  bool CalculatePositions(const char style, TreeNode* node) ;
  // Clean and release the binary_tree.
  void ReleaseTree(TreeNode* node);
  // Guided binary tree generation.
  void GenerateTree(float expect_alpha);
  // Divide-and-conquer tree generation.
  TreeNode* GuidedTree(TreeNode* parent,
                       char child_type,
                       float expect_alpha,
                       int image_num,
                       std::vector<AlphaUnit>& alpha_array,
                       float root_alpha);
  // Find the best-match aspect ratio image in the given array.
  // alpha_array is the array storing aspect ratios.
  // find_img_alpha is the best-match alpha value.
  // After finding the best-match one, the AlphaUnit is removed from alpha_array,
  // which means that we have dispatched one image with a tree leaf.
  bool FindOneImage(float expect_alpha,
                    std::vector<AlphaUnit>& alpha_array,
                    float& find_img_alpha,
                    std::string& find_img_path);
  // Find the best fit aspect ratio (two images) in the given array.
  // find_split_type returns 'h' or 'v'.
  // If it is 'h', the parent node is horizontally split, and 'v' for vertically
  // split. After finding the two images, the corresponding AlphaUnits are
  // removed, which means we have dispatched two images.
  bool FindTwoImages(float expect_alpha,
                     std::vector<AlphaUnit>& alpha_array,
                     char& find_split_type,
                     float& find_img_alpha_1,
                     std::string& find_img_path_1,
                     float& find_img_alpha_2,
                     std::string& find_img_path_2);
  // Top-down adjust aspect ratio for the final collage.
  bool AdjustAlpha(TreeNode* node, float thresh);
  
  // Vector containing input images' aspect ratios.
  std::vector<AlphaUnit> image_alpha_vec_;
  // Vector containing leaf nodes of the tree.
  std::vector<TreeNode*> tree_leaves_;
  // Number of images in the collage. (number of leaf nodes in the tree)
  int image_num_;
  // Full balanced binary for collage generation.
  TreeNode* tree_root_;
  // Canvas height, this is decided by the user.
  int canvas_height_;
  // Canvas aspect ratio, return by CalculateAspectRatio ().
  float canvas_alpha_;
  // Canvas width, this is computed according to canvas_aspect_ratio_.
  int canvas_width_;
  
};

#endif /* defined(__image_browser__Collage__) */
