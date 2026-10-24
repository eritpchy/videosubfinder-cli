#include <algorithm>
#include <iostream>
#include <opencv2/opencv.hpp>
#include "ImageCropper.h"

ImageCropper::ImageCropper(int ymin, int ymax, int xmin, int xmax, int width, int height):
    fontSizeAdjustVal(20), ymin(ymin), ymax(ymax), xmin(xmin), xmax(xmax), width(width), height(height) {
    const auto [f_ymin, f_ymax, f_xmin, f_xmax] = this->offsetTo(ymin, ymax, xmin, xmax);
    this->f_ymin = f_ymin;
    this->f_ymax = f_ymax;
    this->f_xmin = f_xmin;
    this->f_xmax = f_xmax;
}

ImageCropper::~ImageCropper() {

}

std::tuple<int, int, int, int> ImageCropper::offsetTo(int ymin, int ymax, int xmin, int xmax) const {
    int fontSize = int(std::min(height, width) / fontSizeAdjustVal);
    int f_ymin = ymin - fontSize;
    int f_ymax = ymax + fontSize;
    int f_xmin = xmin - (fontSize * 4);
    int f_xmax = xmax + (fontSize * 4);
    return std::make_tuple(std::max(f_ymin, 0), std::min(f_ymax, height), std::max(f_xmin, 0), std::min(f_xmax, width));
}

std::tuple<int, int, int, int> ImageCropper::offsetBack(int ymin, int ymax, int xmin, int xmax) const {
    int h = std::abs(ymax - ymin);
    int w = std::abs(xmax - xmin);
    ymin = ymin - this->f_ymin;
    xmin = xmin - this->f_xmin;
    return std::make_tuple(ymin, ymin + h, xmin, xmin + w);
}

std::tuple<int, int, int, int> ImageCropper::restore(int ymin, int ymax, int xmin, int xmax) const {
    int new_ymin = std::max(ymin + this->f_ymin, 0);
    int new_ymax = std::min(ymax + this->f_ymin, height);
    int new_xmin = std::max(xmin + this->f_xmin, 0);
    int new_xmax = std::min(xmax + this->f_xmin, width);
    return std::make_tuple(new_ymin, new_ymax, new_xmin, new_xmax);
}

cv::Mat ImageCropper::crop(cv::Mat& mat) const {
    // 检查坐标是否在图像范围内
    if (xmin < 0 || ymin < 0 || xmax > mat.cols || ymax > mat.rows || xmin >= xmax || ymin >= ymax) {
        throw std::out_of_range(
            "Crop coordinates are out of image bounds or invalid.\n"
            "xmin: " + std::to_string(xmin) + "\n"
            "ymin: " + std::to_string(ymin) + "\n"
            "xmax: " + std::to_string(xmax) + "\n"
            "ymax: " + std::to_string(ymax) + "\n"
            "Image dimensions: " + std::to_string(mat.cols) + "x" + std::to_string(mat.rows)
        );
    }
    // 定义裁剪区域
    cv::Rect crop_region(xmin, ymin, xmax - xmin, ymax - ymin);
    return crop(mat, this->f_ymin, this->f_ymax, this->f_xmin, this->f_xmax);
}

cv::Mat ImageCropper::crop(const cv::Mat& mat, int ymin, int ymax, int xmin, int xmax) const {
    // 检查坐标是否在图像范围内
    if (xmin < 0 || ymin < 0 || xmax > mat.cols || ymax > mat.rows || xmin >= xmax || ymin >= ymax) {
        throw std::out_of_range(
            "Crop coordinates are out of image bounds or invalid.\n"
            "xmin: " + std::to_string(xmin) + "\n"
            "ymin: " + std::to_string(ymin) + "\n"
            "xmax: " + std::to_string(xmax) + "\n"
            "ymax: " + std::to_string(ymax) + "\n"
            "Image dimensions: " + std::to_string(mat.cols) + "x" + std::to_string(mat.rows)
        );
    }
    // 定义裁剪区域
    cv::Rect crop_region(xmin, ymin, xmax - xmin, ymax - ymin);
    return mat(crop_region);
}

int ImageCropper::cropAndSaveImage(const cv::Mat& mat, int ymin, int ymax, int xmin, int xmax, const std::string& path) const {
    cv::Mat cropiedMat = this->crop(mat, ymin, ymax, xmin, xmax);
    // cv::Mat bgrMat;
    // cv::cvtColor(cropiedMat, bgrMat, cv::COLOR_RGB2BGR);
    if (!cv::imwrite(path, cropiedMat)) {
        std::cerr << "Could not save the image to " << path << std::endl;
        return -1;
    }
    return 0;
}

