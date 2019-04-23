#ifndef IMAGE_SYNTHESIS_H
#define IMAGE_SYNTHESIS_H

#include <opencv2/core/core.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/one_bit_color_map.hpp>
#include <boost/graph/stoer_wagner_min_cut.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/typeof/typeof.hpp>

class image_synthesizer {
  public:
    image_synthesizer() {}
    cv::Mat operator()(const cv::Mat &src1,const cv::Mat &src2,const cv::Point2i &p,const cv::Rect &rect);
};


#endif
