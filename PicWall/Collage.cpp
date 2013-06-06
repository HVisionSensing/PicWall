//
//  Collage.cpp
//  image-browser
//
//  Created by Zhipeng Wu on 10/2/12.
//
//

#include "Collage.h"
#include <math.h>
#include <fstream>
#include <iostream>
#include <sys/stat.h>


bool less_than(AlphaUnit m, AlphaUnit n) {
  return m.alpha_ < n.alpha_;
}
CollageAdvanced::CollageAdvanced(std::vector<std::string> input_image_list) {
  for (int i = 0; i < input_image_list.size(); ++i) {
    std::string img_path = input_image_list[i];
    cv::Mat img = cv::imread(img_path.c_str());
    AlphaUnit new_unit;
    new_unit.image_ind_ = i;
    new_unit.alpha_ = static_cast<float>(img.cols) / img.rows;
    new_unit.alpha_recip_ = static_cast<float>(img.rows) / img.cols;
    new_unit.image_path_ = img_path;
    image_alpha_vec_.push_back(new_unit);
  }
  canvas_width_ = -1;
  canvas_alpha_ = -1;
  canvas_height_ = -1;
  image_num_ = static_cast<int>(input_image_list.size());
  srand(static_cast<unsigned>(time(0)));
  tree_root_ = new TreeNode();
}

int CollageAdvanced::CreateCollage(int width,
                                   float expect_alpha,
                                   float thresh
                                   ) {
  assert(width > 0);
  assert(thresh > 1);
  assert(expect_alpha > 0);
  canvas_width_ = width;
  tree_root_->alpha_expect_ = expect_alpha;
  float lower_bound = expect_alpha / thresh;
  float upper_bound = expect_alpha * thresh;
  int total_iter_counter = 1;
  int iter_counter = 1;
  int tree_gene_counter = 1;
  // Step 1: Sort the image_alpha_ vector fot generate guided binary tree.
  std::sort(image_alpha_vec_.begin(), image_alpha_vec_.end(), less_than);
  // Step 2: Generate a guided binary tree by using divide-and-conquer.
  GenerateTree(expect_alpha);
  // Step 3: Calculate the actual aspect ratio for the generated collage.
  canvas_alpha_ = CalculateAlpha(tree_root_);
  
  while ((canvas_alpha_ < lower_bound) || (canvas_alpha_ > upper_bound)) {
    // Call the following function to adjust the aspect ratio from top to down.
    
    /*************************************************************************/
    tree_root_->alpha_expect_ = expect_alpha;
    bool changed = false;
    changed = AdjustAlpha(tree_root_, thresh);
    // Calculate actual aspect ratio again.
    canvas_alpha_ = CalculateAlpha(tree_root_);
    ++iter_counter;
    ++total_iter_counter;
    if ((iter_counter > MAX_ITER_NUM) || (!changed)) {
      std::cout << "********************************************" << std::endl;
      if (changed) {
        std::cout << "max iteration number reached..." << std::endl;
      } else {
        std::cout << "tree structure unchanged after iteration: "
        << iter_counter << std::endl;
      }
      std::cout << "********************************************" << std::endl;
      // We should generate binary tree again
      iter_counter = 1;
      ++total_iter_counter;
      /*************************************************************************/
      
      GenerateTree(expect_alpha);
      canvas_alpha_ = CalculateAlpha(tree_root_);
      ++tree_gene_counter;
      if (tree_gene_counter > MAX_TREE_GENE_NUM) {
        std::cout << "-------------------------------------------------------";
        std::cout << std::endl;
        std::cout << "WE HAVE DONE OUR BEST, BUT COLAAGE GENERATION FAILED...";
        std::cout << std::endl;
        std::cout << "-------------------------------------------------------";
        std::cout << std::endl;
        return -1;
      }
    }
  }
  canvas_height_ = static_cast<int>(canvas_width_ / canvas_alpha_);
  tree_root_->position_.x_ = 0;
  tree_root_->position_.y_ = 0;
  tree_root_->position_.height_ = canvas_height_;
  tree_root_->position_.width_ = canvas_width_;
  if (tree_root_->left_child_)
    CalculatePositions(tree_root_->left_child_);
  if (tree_root_->right_child_)
    CalculatePositions(tree_root_->right_child_);
  return 1;
}

// After calling CreateCollage() and FastAdjust(), call this function to save result
// collage to a image file specified by out_put_image_path.
cv::Mat CollageAdvanced::OutputCollageImage() const {
  // Traverse tree_leaves_ vector. Resize tile image and paste it on the canvas.
  assert(canvas_alpha_ != -1);
  assert(canvas_width_ != -1);
  cv::Mat canvas(cv::Size(canvas_width_, canvas_height_),
                 CV_8UC3,
                 cv::Scalar(0, 0, 0));
  for (int i = 0; i < image_num_; ++i) {
    //    cv::imshow("", canvas);
    //    cv::waitKey();
    FloatRect pos = tree_leaves_[i]->position_;
    cv::Rect pos_cv(pos.x_, pos.y_, pos.width_, pos.height_);
    cv::Mat roi(canvas, pos_cv);
    cv::Mat resized_img(pos_cv.height, pos_cv.width, CV_8UC3);
    cv::Mat image = cv::imread(tree_leaves_[i]->img_path_.c_str());
    assert(image.type() == CV_8UC3);
    cv::resize(image, resized_img, resized_img.size());
    resized_img.copyTo(roi);
  }
  return canvas;
}

// After calling CreateCollage(), call this function to save result
// collage to a html file specified by out_put_html_path.
bool CollageAdvanced::OutputCollageHtml(const std::string output_html_path) {
  assert(canvas_alpha_ != -1);
  assert(canvas_width_ != -1);
  std::ofstream output_html(output_html_path.c_str());
  if (!output_html) {
    std::cout << "Error: OutputCollageHtml" << std::endl;
  }
  
  output_html << "<!DOCTYPE html>\n";
  output_html << "<html>\n";
  output_html << "<script src=\"/Users/wu/Softwares/prettyPhoto_uncompressed_3.1.5/js/jquery-1.6.1.min.js\" type=\"text/javascript\" charset=\"utf-8\"></script> <link rel=\"stylesheet\" href=\"/Users/wu/Softwares/prettyPhoto_uncompressed_3.1.5/css/prettyPhoto.css\" type=\"text/css\" media=\"screen\" charset=\"utf-8\" /> <script src=\"/Users/wu/Softwares/prettyPhoto_uncompressed_3.1.5/js/jquery.prettyPhoto.js\" type=\"text/javascript\" charset=\"utf-8\"></script>\n";
  output_html << "<style type=\"text/css\">\n";
  output_html << "body {background-image:url(/Users/WU/Projects/2012Collage/JMM_Demo/bg_resize.png);  background-position: top; background-repeat:repeat-x; background-attachment:fixed}\n";
  output_html << "</style>\n";
  output_html << "\t<body>\n";
  output_html << "<script type=\"text/javascript\" charset=\"utf-8\"> $(document).ready(function(){$(\"a[rel^='prettyPhoto']\").prettyPhoto();});</script>";
  output_html << "\t\t<div style=\"margin:20px auto; width:70%; position:relative;\">\n";
  for (int i = 0; i < image_num_; ++i) {
    output_html << "\t\t\t<a href=\"";
    output_html << tree_leaves_[i]->img_path_;
    output_html << "\" rel=\"prettyPhoto[pp_gal]\">\n";
    output_html << "\t\t\t\t<img src=\"";
    output_html << tree_leaves_[i]->img_path_;
    output_html << "\" style=\"position:absolute; width:";
    output_html << tree_leaves_[i]->position_.width_ - 1;
    output_html << "px; height:";
    output_html << tree_leaves_[i]->position_.height_ - 1;
    output_html << "px; left:";
    output_html << tree_leaves_[i]->position_.x_ - 1;
    output_html << "px; top:";
    output_html << tree_leaves_[i]->position_.y_ - 1;
    output_html << "px;\">\n";
    output_html << "\t\t\t</a>\n";
  }
  output_html << "\t\t</div>\n";
  //  output_html << "\t\t<div style=\"text-align:center\">\n";
  //  output_html << "\t\t<p style=\"position:relative; top:";
  //  output_html << canvas_height() << "px\">\n";
  //  output_html << "\t\t\t<a href=\"   \"><-Back</a>\n";
  //  output_html << "\t\t\t&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp";
  //  output_html << "\t\t\t<a href=\"   \">Next-></a>\n</p>\n";
  //  output_html << "\t\t</div>\n";
  output_html << "\t</body>\n";
  output_html << "</html>";
  output_html.close();
  return true;
}


// Private member functions:
// Recursively calculate aspect ratio for all the inner nodes.
// The return value is the aspect ratio for the node.
float CollageAdvanced::CalculateAlpha(TreeNode* node) {
  if (!node->is_leaf_) {
    float left_alpha = CalculateAlpha(node->left_child_);
    float right_alpha = CalculateAlpha(node->right_child_);
    if (node->split_type_ == 'v') {
      node->alpha_ = left_alpha + right_alpha;
      return node->alpha_;
    } else if (node->split_type_ == 'h') {
      node->alpha_ = (left_alpha * right_alpha) / (left_alpha + right_alpha);
      return node->alpha_;
    } else {
      std::cout << "Error: CalculateAlpha" << std::endl;
      return -1;
    }
  } else {
    // This is a leaf node, just return the image's aspect ratio.
    return node->alpha_;
  }
}

// Top-down Calculate the image positions in the colage.
bool CollageAdvanced::CalculatePositions(TreeNode* node) {
  // Step 1: calculate height & width.
  if (node->parent_->split_type_ == 'v') {
    // Vertical cut, height unchanged.
    node->position_.height_ = node->parent_->position_.height_;
    if (node->child_type_ == 'l') {
      node->position_.width_ = node->position_.height_ * node->alpha_;
    } else if (node->child_type_ == 'r') {
      node->position_.width_ = node->parent_->position_.width_ -
      node->parent_->left_child_->position_.width_;
    } else {
      std::cout << "Error: CalculatePositions step 0" << std::endl;
      return false;
    }
  } else if (node->parent_->split_type_ == 'h') {
    // Horizontal cut, width unchanged.
    node->position_.width_ = node->parent_->position_.width_;
    if (node->child_type_ == 'l') {
      node->position_.height_ = node->position_.width_ / node->alpha_;
    } else if (node->child_type_ == 'r') {
      node->position_.height_ = node->parent_->position_.height_ -
      node->parent_->left_child_->position_.height_;
    }
  } else {
    std::cout << "Error: CalculatePositions step 1" << std::endl;
    return false;
  }
  
  // Step 2: calculate x & y.
  if (node->child_type_ == 'l') {
    // If it is left child, use its parent's x & y.
    node->position_.x_ = node->parent_->position_.x_;
    node->position_.y_ = node->parent_->position_.y_;
  } else if (node->child_type_ == 'r') {
    if (node->parent_->split_type_ == 'v') {
      // y (row) unchanged, x (colmn) changed.
      node->position_.y_ = node->parent_->position_.y_;
      node->position_.x_ = node->parent_->position_.x_ +
      node->parent_->position_.width_ -
      node->position_.width_;
    } else if (node->parent_->split_type_ == 'h') {
      // x (column) unchanged, y (row) changed.
      node->position_.x_ = node->parent_->position_.x_;
      node->position_.y_ = node->parent_->position_.y_ +
      node->parent_->position_.height_ -
      node->position_.height_;
    } else {
      std::cout << "Error: CalculatePositions step 2 - 1" << std::endl;
    }
  } else {
    std::cout << "Error: CalculatePositions step 2 - 2" << std::endl;
    return false;
  }
  
  // Calculation for children.
  if (node->left_child_) {
    bool success = CalculatePositions(node->left_child_);
    if (!success) return false;
  }
  if (node->right_child_) {
    bool success = CalculatePositions(node->right_child_);
    if (!success) return false;
  }
  return true;
}


// Release the memory for binary tree.
void CollageAdvanced::ReleaseTree(TreeNode* node) {
  if (node == NULL) return;
  if (node->left_child_) ReleaseTree(node->left_child_);
  if (node->right_child_) ReleaseTree(node->right_child_);
  delete node;
}

void CollageAdvanced::GenerateTree(float expect_alpha) {
  if (tree_root_) ReleaseTree(tree_root_);
  tree_leaves_.clear();
  // Copy image_alpha_vec_ for local computation.
  std::vector<AlphaUnit> local_alpha;
  for (int i = 0; i < image_alpha_vec_.size(); ++i) {
    local_alpha.push_back(image_alpha_vec_[i]);
  }
  
  // Generate a new tree by using divide-and-conquer.
  tree_root_ = GuidedTree(tree_root_, 'N', expect_alpha,
                          image_num_, local_alpha, expect_alpha);
  // After guided tree generation, all the images have been dispatched to leaves.
  assert(local_alpha.size() == 0);
  return;
}

// Divide-and-conquer tree generation.
TreeNode* CollageAdvanced::GuidedTree(TreeNode* parent,
                                      char child_type,
                                      float expect_alpha,
                                      int img_num,
                                      std::vector<AlphaUnit>& alpha_array,
                                      float root_alpha) {
  if (alpha_array.size() == 0) {
    std::cout << "Error: GuidedTree 0" << std::endl;
    return NULL;
  }
  
  // Create a new TreeNode.
  TreeNode* node = new TreeNode();
  node->parent_ = parent;
  node->child_type_ = child_type;
  
  if (img_num == 1) {
    // Set the new node.
    node->is_leaf_ = true;
    // Find the best fit aspect ratio.
    bool success = FindOneImage(expect_alpha,
                                alpha_array,
                                node->alpha_,
                                node->img_path_);
    if (!success) {
      std::cout << "Error: GuidedTree 1" << std::endl;
      return NULL;
    }
    tree_leaves_.push_back(node);
  } else if (img_num == 2) {
    // Set the new node.
    node->is_leaf_ = false;
    TreeNode* l_child = new TreeNode();
    l_child->child_type_ = 'l';
    l_child->parent_ = node;
    l_child->is_leaf_ = true;
    node->left_child_ = l_child;
    
    TreeNode* r_child = new TreeNode();
    r_child->child_type_ = 'r';
    r_child->parent_ = node;
    r_child->is_leaf_ = true;
    node->right_child_ = r_child;
    // Find the best fit aspect ratio with two nodes.
    // As well as the split type for node.
    bool success = FindTwoImages(expect_alpha,
                                 alpha_array,
                                 node->split_type_,
                                 l_child->alpha_,
                                 l_child->img_path_,
                                 r_child->alpha_,
                                 r_child->img_path_);
    if (!success) {
      std::cout << "Error: GuidedTree 2" << std::endl;
      return NULL;
    }
    tree_leaves_.push_back(l_child);
    tree_leaves_.push_back(r_child);
  } else {
    node->is_leaf_ = false;
    float new_exp_alpha = 0;
    // Random split type.
    int v_h = random(2);
    if (expect_alpha > root_alpha * 2) v_h = 1;
    if (expect_alpha < root_alpha / 2) v_h = 0;
    if (v_h == 1) {
      node->split_type_ = 'v';
      new_exp_alpha = expect_alpha / 2;
    } else {
      node->split_type_ = 'h';
      new_exp_alpha = expect_alpha * 2;
    }
    int new_img_num_1 = static_cast<int>(img_num / 2);
    int new_img_num_2 = img_num - new_img_num_1;
    if (new_img_num_1 > 0) {
      TreeNode* l_child = new TreeNode();
      l_child = GuidedTree(node, 'l', new_exp_alpha,
                           new_img_num_1, alpha_array, root_alpha);
      node->left_child_ = l_child;
    }
    if (new_img_num_2 > 0) {
      TreeNode* r_child = new TreeNode();
      r_child = GuidedTree(node, 'r', new_exp_alpha,
                           new_img_num_2, alpha_array, root_alpha);
      node->right_child_ = r_child;
    }
  }
  return node;
}

// Find the best-match aspect ratio image in the given array.
// alpha_array is the array storing aspect ratios.
// find_img_alpha is the best-match alpha value.
// After finding the best-match one, the AlphaUnit is removed from alpha_array,
// which means that we have dispatched one image with a tree leaf.
bool CollageAdvanced::FindOneImage(float expect_alpha,
                                   std::vector<AlphaUnit>& alpha_array,
                                   float& find_img_alpha,
                                   std::string & find_img_path) {
  if (alpha_array.size() == 0) return false;
  // Since alpha_array has already been sorted, we use binary search to find
  // the best-match result.
  int finder = -1;
  int min_ind = 0;
  int mid_ind = -1;
  int max_ind = static_cast<int>(alpha_array.size()) - 1;
  while (min_ind + 1 < max_ind) {
    mid_ind = (min_ind + max_ind) / 2;
    if (alpha_array[mid_ind].alpha_ == expect_alpha) {
      finder = mid_ind;
      break;
    } else if (alpha_array[mid_ind].alpha_ > expect_alpha) {
      max_ind = mid_ind - 1;
    } else {
      min_ind = mid_ind + 1;
    }
  }
  if (finder == -1) {
    if (fabs(alpha_array[max_ind].alpha_ - expect_alpha) >
        fabs(alpha_array[min_ind].alpha_ - expect_alpha)) finder = min_ind;
    else finder = max_ind;
  }
  
  // Dispatch image to leaf node.
  find_img_alpha = alpha_array[finder].alpha_;
  find_img_path = alpha_array[finder].image_path_;
  // Remove the find result from alpha_array.
  //  std::cout<< alpha_array[finder].image_ind_ << std::endl;
  alpha_array.erase(alpha_array.begin() + finder);
  return true;
}

// Find the best fit aspect ratio (two images) in the given array.
// find_split_type returns 'h' or 'v'.
// If it is 'h', the parent node is horizontally split, and 'v' for vertically
// split. After finding the two images, the corresponding AlphaUnits are
// removed, which means we have dispatched two images.
bool CollageAdvanced::FindTwoImages(float expect_alpha,
                                    std::vector<AlphaUnit>& alpha_array,
                                    char& find_split_type,
                                    float& find_img_alpha_1,
                                    std::string& find_img_path_1,
                                    float& find_img_alpha_2,
                                    std::string& find_img_path_2) {
  if ((alpha_array.size() == 0) || (alpha_array.size() == 1)) return false;
  // There are two situations:
  // [1]: parent node is vertival cut.
  int i = 0;
  int j = static_cast<int>(alpha_array.size()) - 1;
  int best_v_i = i;
  int best_v_j = j;
  float min_v_diff = fabs(alpha_array[best_v_i].alpha_ +
                          alpha_array[best_v_j].alpha_ -
                          expect_alpha);
  while (i < j) {
    if (alpha_array[i].alpha_ + alpha_array[j].alpha_ > expect_alpha) {
      float diff = fabs(alpha_array[i].alpha_ +
                        alpha_array[j].alpha_ -
                        expect_alpha);
      if (diff < min_v_diff) {
        min_v_diff = diff;
        best_v_i = i;
        best_v_j = j;
      }
      --j;
    } else if (alpha_array[i].alpha_ + alpha_array[j].alpha_ < expect_alpha) {
      float diff = fabs(alpha_array[i].alpha_ +
                        alpha_array[j].alpha_ -
                        expect_alpha);
      if (diff < min_v_diff) {
        min_v_diff = diff;
        best_v_i = i;
        best_v_j = j;
      }
      ++i;
    } else {
      best_v_i = i;
      best_v_j = j;
      min_v_diff = 0;
      break;
    }
  }
  // [2]: parent node is horizontal cut;
  float expect_alpha_recip = 1 / expect_alpha;
  i = 0;
  j = static_cast<int>(alpha_array.size()) - 1;
  int best_h_i = i;
  int best_h_j = j;
  float min_h_diff = fabs(alpha_array[best_h_i].alpha_recip_ +
                          alpha_array[best_h_j].alpha_recip_ -
                          expect_alpha_recip);
  while (i < j) {
    if (alpha_array[i].alpha_recip_ + alpha_array[j].alpha_recip_ >
        expect_alpha_recip) {
      float diff = fabs(alpha_array[i].alpha_recip_ +
                        alpha_array[j].alpha_recip_ -
                        expect_alpha_recip);
      if (diff < min_h_diff) {
        min_h_diff = diff;
        best_h_i = i;
        best_h_j = j;
      }
      ++i;
    } else if (alpha_array[i].alpha_recip_  + alpha_array[j].alpha_recip_ <
               expect_alpha_recip) {
      float diff = fabs(alpha_array[i].alpha_recip_ +
                        alpha_array[j].alpha_recip_ -
                        expect_alpha_recip);
      if (diff < min_h_diff) {
        min_h_diff = diff;
        best_h_i = i;
        best_h_j = j;
      }
      --j;
    } else {
      best_h_i = i;
      best_h_j = j;
      min_h_diff = 0;
      break;
    }
  }
  
  // Find the best-match from the above two situations.
  float real_alpha_v = alpha_array[best_v_i].alpha_ + alpha_array[best_v_j].alpha_;
  float real_alpha_h = (alpha_array[best_h_i].alpha_ * alpha_array[best_h_j].alpha_) /
  (alpha_array[best_h_i].alpha_ + alpha_array[best_h_j].alpha_);
  
  float ratio_diff_v = -1;
  float ratio_diff_h = -1;
  if (real_alpha_v > expect_alpha) {
    ratio_diff_v = real_alpha_v / expect_alpha;
  } else {
    ratio_diff_v = expect_alpha / real_alpha_v;
  }
  if (real_alpha_h > expect_alpha) {
    ratio_diff_h = real_alpha_h / expect_alpha;
  } else {
    ratio_diff_h = expect_alpha / real_alpha_h;
  }
  
  assert(best_h_i < best_h_j);
  assert(best_v_i < best_v_j);
  
  if (ratio_diff_v <= ratio_diff_h) {
    find_split_type = 'v';
    find_img_path_1 = alpha_array[best_v_i].image_path_;
    find_img_alpha_1 = alpha_array[best_v_i].alpha_;
    find_img_alpha_2 = alpha_array[best_v_j].alpha_;
    find_img_path_2 = alpha_array[best_v_j].image_path_;
    
    //    std::cout << alpha_array[best_v_i].image_ind_
    //    << ":" << alpha_array[best_v_j].image_ind_ << std::endl;
    
    alpha_array.erase(alpha_array.begin() + best_v_j);
    alpha_array.erase(alpha_array.begin() + best_v_i);
  } else {
    find_split_type = 'h';
    find_img_path_1 = alpha_array[best_h_i].image_path_;
    find_img_alpha_1 = alpha_array[best_h_i].alpha_;
    find_img_alpha_2 = alpha_array[best_h_j].alpha_;
    find_img_path_2 = alpha_array[best_h_j].image_path_;
    //    std::cout << alpha_array[best_h_i].image_ind_
    //    << ":" << alpha_array[best_h_j].image_ind_ << std::endl;
    
    alpha_array.erase(alpha_array.begin() + best_h_j);
    alpha_array.erase(alpha_array.begin() + best_h_i);
  }
  return true;
}

bool CollageAdvanced::AdjustAlpha(TreeNode *node, float thresh) {
  assert(thresh > 1);
  if (node->is_leaf_) return false;
  if (node == NULL) return false;
  
  bool changed = false;
  
  float thresh_2 = 1 + (thresh - 1) / 2;
  
  if (node->alpha_ > node->alpha_expect_ * thresh_2) {
    // Too big actual aspect ratio.
    if (node->split_type_ == 'v') changed = true;
    node->split_type_ = 'h';
    node->left_child_->alpha_expect_ = node->alpha_expect_ * 2;
    node->right_child_->alpha_expect_ = node->alpha_expect_ * 2;
  } else if (node->alpha_ < node->alpha_expect_ / thresh_2 ) {
    // Too small actual aspect ratio.
    if (node->split_type_ == 'h') changed = true;
    node->split_type_ = 'v';
    node->left_child_->alpha_expect_ = node->alpha_expect_ / 2;
    node->right_child_->alpha_expect_ = node->alpha_expect_ / 2;
  } else {
    // Aspect ratio is okay.
    if (node->split_type_ == 'h') {
      node->left_child_->alpha_expect_ = node->alpha_expect_ * 2;
      node->right_child_->alpha_expect_ = node->alpha_expect_ * 2;
    } else if (node->split_type_ == 'v') {
      node->left_child_->alpha_expect_ = node->alpha_expect_ * 2;
      node->right_child_->alpha_expect_ = node->alpha_expect_ * 2;
    } else {
      std::cout << "Error: AdjustAlpha" << std::endl;
      return false;
    }
  }
  bool changed_l = AdjustAlpha(node->left_child_, thresh);
  bool changed_r = AdjustAlpha(node->right_child_, thresh);
  return changed||changed_l||changed_r;
}

/*****************************************************************************/
// newlly added:

bool CollageAdvanced::CreateCollage (const cv::Size2i canvas_size,
                                     const int border_size,
                                     const float threshold,
                                     const bool manga_mode,
                                     const char style) {
  if ((image_num_ <= 0) || canvas_size.width <= 0 || canvas_size.height <= 0) {
    std::cout << "error in CreateCollage..." << std::endl;
    return false;
  }
  srand(static_cast<unsigned>(time(0)));
  // Define the manga content area.
  float expect_alpha = static_cast<float>(canvas_size.width) /
  canvas_size.height;
  
  canvas_width_ = canvas_size.width;
  tree_root_->alpha_expect_ = expect_alpha;
  float lower_bound = expect_alpha / threshold;
  float upper_bound = expect_alpha * threshold;
  int total_iter_counter = 1;
  int iter_counter = 1;
  int tree_gene_counter = 1;
  // Step 1: Sort the image_alpha_ vector fot generate guided binary tree.
  std::sort(image_alpha_vec_.begin(), image_alpha_vec_.end(), less_than);
  // Step 2: Generate a guided binary tree by using divide-and-conquer.
  GenerateTree(expect_alpha);
  // Step 3: Calculate the actual aspect ratio for the generated collage.
  canvas_alpha_ = CalculateAlpha(tree_root_);
  
  while ((canvas_alpha_ < lower_bound) || (canvas_alpha_ > upper_bound)) {
    // Call the following function to adjust the aspect ratio from top to down.
    
    /*************************************************************************/
    tree_root_->alpha_expect_ = expect_alpha;
    bool changed = false;
    changed = AdjustAlpha(tree_root_, threshold);
    // Calculate actual aspect ratio again.
    canvas_alpha_ = CalculateAlpha(tree_root_);
    ++iter_counter;
    ++total_iter_counter;
    if ((iter_counter > MAX_ITER_NUM) || (!changed)) {
      std::cout << "********************************************" << std::endl;
      if (changed) {
        std::cout << "max iteration number reached..." << std::endl;
      } else {
        std::cout << "tree structure unchanged after iteration: "
        << iter_counter << std::endl;
      }
      std::cout << "********************************************" << std::endl;
      // We should generate binary tree again
      iter_counter = 1;
      ++total_iter_counter;
      /*************************************************************************/
      
      GenerateTree(expect_alpha);
      canvas_alpha_ = CalculateAlpha(tree_root_);
      ++tree_gene_counter;
      if (tree_gene_counter > MAX_TREE_GENE_NUM) {
        std::cout << "-------------------------------------------------------";
        std::cout << std::endl;
        std::cout << "WE HAVE DONE OUR BEST, BUT COLAAGE GENERATION FAILED...";
        std::cout << std::endl;
        std::cout << "-------------------------------------------------------";
        std::cout << std::endl;
        return false;
      }
    }
  }
  canvas_height_ = static_cast<int>(canvas_width_ / canvas_alpha_);
  tree_root_->position_.x_ = 0;
  tree_root_->position_.y_ = 0;
  tree_root_->position_.height_ = canvas_height_;
  tree_root_->position_.width_ = canvas_width_;
  if (tree_root_->left_child_)
    CalculatePositions(style, tree_root_->left_child_);
  if (tree_root_->right_child_)
    CalculatePositions(style, tree_root_->right_child_);
  return true;
}

// Top-down Calculate the image positions in the colage.
bool CollageAdvanced::CalculatePositions(const char style, TreeNode* node) {
  // Step 1: calculate height & width.
  if (node->parent_->split_type_ == 'v') {
    // Vertical cut, height unchanged.
    node->position_.height_ = node->parent_->position_.height_;
    if (node->child_type_ == 'l') {
      node->position_.width_ = node->position_.height_ * node->alpha_;
      if ('u' == style) {
        node->position_.x_ = node->parent_->position_.x_;
        node->position_.y_ = node->parent_->position_.y_;
      } else if ('j' == style) {
        node->position_.y_ = node->parent_->position_.y_;
        node->position_.x_ = node->parent_->position_.x_ +
        node->parent_->position_.width_ -
        node->position_.width_;
      } else {
        std::cout << "error: CalculatePositions style not supported..." << std::endl;
        return false;
      }
    } else if (node->child_type_ == 'r') {
      node->position_.width_ = node->parent_->position_.width_ -
      node->parent_->left_child_->position_.width_;
      if ('j' == style) {
        node->position_.x_ = node->parent_->position_.x_;
        node->position_.y_ = node->parent_->position_.y_;
      } else if ('u' == style) {
        node->position_.y_ = node->parent_->position_.y_;
        node->position_.x_ = node->parent_->position_.x_ +
        node->parent_->position_.width_ -
        node->position_.width_;
      } else {
        std::cout << "error: CalculatePositions style not supported..." << std::endl;
        return false;
      }
      
    } else {
      std::cout << "error: CalculatePositions V" << std::endl;
      return false;
    }
  } else if (node->parent_->split_type_ == 'h') {
    // Horizontal cut, width unchanged.
    node->position_.width_ = node->parent_->position_.width_;
    if (node->child_type_ == 'l') {
      node->position_.height_ = node->position_.width_ / node->alpha_;
      // If it is left child, use its parent's x & y.
      node->position_.x_ = node->parent_->position_.x_;
      node->position_.y_ = node->parent_->position_.y_;
    } else if (node->child_type_ == 'r') {
      node->position_.height_ = node->parent_->position_.height_ -
      node->parent_->left_child_->position_.height_;
      // x (column) unchanged, y (row) changed.
      node->position_.x_ = node->parent_->position_.x_;
      node->position_.y_ = node->parent_->position_.y_ +
      node->parent_->position_.height_ -
      node->position_.height_;
    } else {
      std::cout << "error: CalculatePositions H" << std::endl;
      return false;
    }
  } else {
    std::cout << "error: CalculatePositions undefiend..." << std::endl;
  }
  
  // Calculation for children.
  if (node->left_child_) {
    bool success = CalculatePositions(style, node->left_child_);
    if (!success) return false;
  }
  if (node->right_child_) {
    bool success = CalculatePositions(style, node->right_child_);
    if (!success) return false;
  }
  return true;
}

cv::Mat CollageAdvanced::OutputCollage(const char type, bool accurate) {
  cv::Mat canvas(cv::Size(canvas_width_, canvas_height_),
                 CV_8UC3, cv::Scalar(0, 0, 0));
  if ((-1 == canvas_alpha_) || (-1 == canvas_width_) || (-1 == canvas_height_)) {
    std::cout << "error: OutputCollage..." << std::endl;
    return canvas;
  }
  // Traverse tree_leaves_ vector. Resize tile image and paste it on the canvas.
  assert(canvas_alpha_ != -1);
  assert(canvas_width_ != -1);

  for (int i = 0; i < image_num_; ++i) {
    FloatRect pos = tree_leaves_[i]->position_;
    cv::Rect pos_cv(pos.x_, pos.y_, pos.width_, pos.height_);
    cv::Mat roi(canvas, pos_cv);
    cv::Mat resized_img(pos_cv.height, pos_cv.width, CV_8UC3);
    cv::Mat image = cv::imread(tree_leaves_[i]->img_path_.c_str(), 1);
    assert(image.type() == CV_8UC3);
    switch (type) {
      case 'p': {
        // Create a photo collage.
        cv::resize(image, resized_img, resized_img.size());
        break;
      }
      case 'm': {
        // Create a manga collage.
        std::auto_ptr<MangaEngine>
        manga_engine(new MangaEngine(image));
        manga_engine->Convert2Manga();
        cv::Mat manga_img = manga_engine->manga() * 255;
        // manga_img is CV_32FC1 type, we have to convert it to CV_8UC1.
        manga_img.convertTo(manga_img, CV_8UC1);
        // we then convert it to CV_8UC3 (gray -> color).
        cv::cvtColor(manga_img, manga_img, CV_GRAY2BGR);
        cv::resize(manga_img, resized_img, resized_img.size());
        break;
      }
      case 'c': {
        // Create a cartoon manga.
        std::auto_ptr<CartoonEngine>
        cartoon_engine(new CartoonEngine(image));
        cartoon_engine->Convert2Cartoon();
        cv::Mat cartoon_img = cartoon_engine->cartoon();
        cv::resize(cartoon_img, resized_img, resized_img.size());
        break;
      }
      case 'e': {
        // Create a pencil sketch collage.
        string tonal_path = "/Users/WU/Dropbox/reserch/VCIP2013/Image_morphing/"
        "code/Matlab/E_Pencil/TT3.jpg";
        cv::Mat tonal = cv::imread(tonal_path, 0);
        std::auto_ptr<SketchEngine>
        sketch_engine(new SketchEngine(image, tonal));
        sketch_engine->Convert2Sketch();
        cv::Mat pencil_img = sketch_engine->pencil_sketch();
        cv::cvtColor(pencil_img, pencil_img, CV_GRAY2BGR);
        cv::resize(pencil_img, resized_img, resized_img.size());
        break;
      }
      case 'o': {
        // Create a color pencil sketch collage.
        std::string tonal_path = "/Users/WU/Dropbox/reserch/VCIP2013/Image_morphing/"
        "code/Matlab/E_Pencil/TT3.jpg";
        cv::Mat tonal = cv::imread(tonal_path, 0);
        std::auto_ptr<SketchEngine>
        sketch_engine(new SketchEngine(image, tonal));
        sketch_engine->Convert2Sketch();
        cv::Mat color_pencil_img = sketch_engine->color_sketch();
        cv::resize(color_pencil_img, resized_img, resized_img.size());
        break;
      }
      case 'i': {
        // Create an oil painting collage.
        std::auto_ptr<CartoonEngine>
        painting_engine(new CartoonEngine(image));
        painting_engine->Convert2Painting();
        cv::Mat painting_img = painting_engine->painting();
        cv::resize(painting_img, resized_img, resized_img.size());
        break;
      }
      default: {
        std::cout << "error in OutputCollage.. " << type << " not supported..." << std::endl;
        return canvas;
      }
    }

    resized_img.copyTo(roi);
  }
  return canvas;
}

bool CollageAdvanced::OutputHtml(const char type, const std::string output_html_path) {
  assert(canvas_alpha_ != -1);
  assert(canvas_width_ != -1);  
  std::ofstream output_html(output_html_path.c_str());
  if (!output_html) {
    std::cout << "error: OutputHtmlManga" << std::endl;
    return false;
  }  
  
  output_html << "<!DOCTYPE html>\n";
  output_html << "<html>\n";
  output_html << "<script src=\"/Users/wu/Dropbox/reserch/APSIPA2013/demo/PicWall_1.03/prettyPhoto/js/jquery-1.6.1.min.js\" type=\"text/javascript\" charset=\"utf-8\"></script> <link rel=\"stylesheet\" href=\"/Users/wu/Dropbox/reserch/APSIPA2013/demo/PicWall_1.03/prettyPhoto/css/prettyPhoto.css\" type=\"text/css\" media=\"screen\" charset=\"utf-8\" /> <script src=\"/Users/wu/Dropbox/reserch/APSIPA2013/demo/PicWall_1.03/prettyPhoto/js/jquery.prettyPhoto.js\" type=\"text/javascript\" charset=\"utf-8\"></script>\n";
  output_html << "<style type=\"text/css\">\n";
  output_html << "body {background-image:url(/Users/WU/Dropbox/reserch/MidTerm/FriendWall/data/others/bg_resize.png);  background-position: top; background-repeat:repeat-x; background-attachment:fixed}\n";
  output_html << "</style>\n";
  output_html << "\t<body>\n";
  output_html << "<script type=\"text/javascript\" charset=\"utf-8\"> $(document).ready(function(){$(\"a[rel^='prettyPhoto']\").prettyPhoto();});</script>";
  output_html << "\t\t<div style=\"margin:20px auto; width:60%; position:relative;\">\n";
  if (type != 'p') {
    std::string temp_path = output_html_path.substr(0, output_html_path.rfind('.')) + "/";
    mkdir(temp_path.c_str(), S_IRWXU);
    char buff[255];
    std::string save_path = temp_path;
    for (int i = 0; i < image_num_; ++i) {
      // *****************Load image*****************
      cv::Mat image = cv::imread(tree_leaves_[i]->img_path_.c_str(), 1);
      // *********Non-photorealistic rendering*******
      cv::Mat img;
      std::sprintf(buff, "%d", i);
      save_path = temp_path + buff;
      switch (type) {
        case 'p': {
          // Photo collage.
          image.copyTo(img);
          save_path += "_photo.jpg";
          break;
        }
        case 'm': {
          // Manga collage.
          std::auto_ptr<MangaEngine> manga_engine(new MangaEngine(image));
          manga_engine->Convert2Manga();
          img = manga_engine->manga() * 255;
          // manga_img is CV_32FC1 type, we have to convert it to CV_8UC3.
          img.convertTo(img, CV_8UC1);
          cv::cvtColor(img, img, CV_GRAY2BGR);
          save_path += "_manga.jpg";
          break;
        }
        case 'e': {
          // Pencil sketch collage.
          string tonal_path = "/Users/WU/Dropbox/reserch/VCIP2013/Image_morphing/code/Matlab/E_Pencil/t2.jpg";
          cv::Mat tonal = cv::imread(tonal_path, 0);
          std::auto_ptr<SketchEngine>
          sketch_engine(new SketchEngine(image, tonal));
          sketch_engine->Convert2Sketch();
          img = sketch_engine->pencil_sketch();
          cv::cvtColor(img, img, CV_GRAY2BGR);
          save_path += "_pencil.jpg";
          break;
        }
        case 'o': {
          // Color pencil sketch.
          string tonal_path = "/Users/WU/Dropbox/reserch/VCIP2013/Image_morphing/code/Matlab/E_Pencil/t2.jpg";
          cv::Mat tonal = cv::imread(tonal_path, 0);
          std::auto_ptr<SketchEngine>
          sketch_engine(new SketchEngine(image, tonal));
          sketch_engine->Convert2Sketch();
          img = sketch_engine->color_sketch();
          save_path += "_color_pencil.jpg";
          break;
        }
        case 'c': {
          // Cartoon collage.
          std::auto_ptr<CartoonEngine>
          cartoon_engine(new CartoonEngine(image));
          cartoon_engine->Convert2Cartoon();
          img = cartoon_engine->cartoon();
          save_path += "_cartoon.jpg";
          break;
        }
        case 'i': {
          // Oil painting collage.
          std::auto_ptr<CartoonEngine>
          painting_engine(new CartoonEngine(image));
          painting_engine->Convert2Painting();
          img = painting_engine->painting();
          save_path += "_painting.jpg";
          break;
        }
        default: {
          break;
        }
      }
      // ****************Add border******************
      int border = std::max(img.rows, img.cols) / 30;
      int new_height = img.rows - border * 2;
      int new_width = img.cols - border * 2;
      cv::Rect content_rect(border, border, new_width, new_height);
      cv::Mat new_img(img.rows, img.cols, CV_8UC3, cv::Scalar::all(255));
      cv::Mat roi(new_img, content_rect);
      cv::resize(img, img, cv::Size2i(new_width, new_height));
      img.copyTo(roi);
      cv::rectangle(new_img, content_rect, cv::Scalar::all(0), border / 5);
      // ****************Save image******************
      cv::imwrite(save_path, new_img);
      // ***************Print Html*******************
      output_html << "\t\t\t<a href=\"";
      output_html << save_path;
      output_html << "\" rel=\"prettyPhoto[pp_gal]\">\n";
      output_html << "\t\t\t\t<img src=\"";
      output_html << save_path;
      output_html << "\" style=\"position:absolute; width:";
      output_html << tree_leaves_[i]->position_.width_ - 1;
      output_html << "px; height:";
      output_html << tree_leaves_[i]->position_.height_ - 1;
      output_html << "px; left:";
      output_html << tree_leaves_[i]->position_.x_ - 1;
      output_html << "px; top:";
      output_html << tree_leaves_[i]->position_.y_ - 1;
      output_html << "px;\">\n";
      output_html << "\t\t\t</a>\n";
    }
  } else {
    for (int i = 0; i < image_num_; ++i) {
      // ***************Print Html*******************
      output_html << "\t\t\t<a href=\"";
      output_html << tree_leaves_[i]->img_path_.c_str();
      output_html << "\" rel=\"prettyPhoto[pp_gal]\">\n";
      output_html << "\t\t\t\t<img src=\"";
      output_html << tree_leaves_[i]->img_path_.c_str();
      output_html << "\" style=\"position:absolute; width:";
      output_html << tree_leaves_[i]->position_.width_ - 1;
      output_html << "px; height:";
      output_html << tree_leaves_[i]->position_.height_ - 1;
      output_html << "px; left:";
      output_html << tree_leaves_[i]->position_.x_ - 1;
      output_html << "px; top:";
      output_html << tree_leaves_[i]->position_.y_ - 1;
      output_html << "px;\">\n";
      output_html << "\t\t\t</a>\n";
    }
  }
  output_html << "\t\t</div>\n";
  output_html << "\t</body>\n";
  output_html << "</html>";
  output_html.close();
  return true;
}



