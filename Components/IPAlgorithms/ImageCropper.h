#pragma once
#include <tuple>
#include <opencv2/core.hpp>


class ImageCropper {
public:
    ImageCropper(int ymin, int ymax, int xmin, int xmax, int width, int height);
    ~ImageCropper();
    std::tuple<int, int, int, int> offsetTo(int ymin, int ymax, int xmin, int xmax) const;
    //还原绝对坐标到crop区域坐标
    std::tuple<int, int, int, int> offsetBack(int ymin, int ymax, int xmin, int xmax) const;
    std::tuple<int, int, int, int> restore(int ymin, int ymax, int xmin, int xmax) const;
    cv::Mat crop(cv::Mat& mat) const;
    cv::Mat crop(const cv::Mat& mat, int ymin, int ymax, int xmin, int xmax) const;
    int cropAndSaveImage(const cv::Mat& mat, int ymin, int ymax, int xmin, int xmax, const std::string& path) const;

private:
    int fontSizeAdjustVal;

    int ymax;
    int ymin;
    int xmin;
    int xmax;
    int width;
    int height;
    int f_ymin;
    int f_xmin;
    int f_xmax;
    int f_ymax;
};
