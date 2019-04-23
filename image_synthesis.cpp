#include <iostream>
#include <fstream>
#include <string>
#include <chrono>

#include <opencv2/highgui/highgui.hpp>

#include <boost/config.hpp>
#include <boost/graph/edmonds_karp_max_flow.hpp>
#include <boost/graph/push_relabel_max_flow.hpp>
#include <boost/graph/boykov_kolmogorov_max_flow.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/read_dimacs.hpp>
#include <boost/graph/graph_utility.hpp>

#include "./image_synthesis.h"

struct edge_t {
    int s , t , cost;
    edge_t(int s,int t,int cost):s(s),t(t),cost(cost){}
};
template<typename T>
T square (T t){
    return t*t;
}

cv::Rect get_intersection_rect(size_t width1,size_t height1,size_t width2,size_t height2,int x,int y) {
    int sx = std::max(0, x);
    int sy = std::max(0, y);
    int ex = std::min(width1 , x + width2 );
    int ey = std::min(height1, y + height2);

    int intersection_width  = ex - sx;
    int intersection_height = ey - sy;
    if (intersection_width > 0 && intersection_height > 0) {
        return cv::Rect(sx, sy, intersection_width, intersection_height);
    }
}

int cost(const cv::Vec3b &src1t,const cv::Vec3b &src2t,const cv::Vec3b &src1s,const cv::Vec3b &src2s) {
    int weight = 0;
    for(int c=0;c<3;++c){
        weight += square(int(src1t[c])-int(src2t[c])) + square(int(src1s[c])-int(src2s[c]));
    }
    return weight;
}
cv::Mat image_synthesizer::operator()(const cv::Mat &src1,const cv::Mat &src2,const cv::Point2i &p,const cv::Rect &rect) {
    size_t bounding_width = 0;
    if( p.x >= 0 ) {
        bounding_width = std::max(src1.cols,p.x+src2.cols);
    } else {
        bounding_width = std::max(-p.x+src1.cols,src2.cols);
    }
    size_t bounding_height = 0;
    if( p.y >= 0 ) {
        bounding_height = std::max(src1.rows,p.y+src2.rows);
    } else {
        bounding_height = std::max(-p.y+src1.rows,src2.rows);
    }

    std::cout << "(w,h)=(" << bounding_width << "," << bounding_height << ")" << std::endl;
    cv::Mat total_dst(bounding_height,bounding_width,CV_8UC3);
    total_dst = cv::Scalar(0,0,0);
    int global_row1 = 0 , global_col1 = 0;
    int global_row2 = 0 , global_col2 = 0;
    if( p.x >= 0 ) {
        global_col2 = p.x;
        global_col1 = 0;
    } else {
        global_col2 = 0;
        global_col1 = -p.x;
    }
    if( p.y >= 0 ) {
        global_row2 = p.y;
        global_row1 = 0;
    } else {
        global_row2 = 0;
        global_row1 = -p.y;
    }

    src1.copyTo(total_dst.rowRange(global_row1, global_row1+src1.rows).colRange(global_col1, global_col1+src1.cols));
    src2.copyTo(total_dst.rowRange(global_row2, global_row2+src2.rows).colRange(global_col2, global_col2+src2.cols));

    cv::Rect intersection_rect = get_intersection_rect(src1.cols,src1.rows,src2.cols,src2.rows,p.x,p.y);

    size_t overlapped_height = intersection_rect.height;
    size_t overlapped_width  = intersection_rect.width;
    int sx = intersection_rect.x;
    int sy = intersection_rect.y;

    size_t src_node  = overlapped_height * overlapped_width;
    size_t sink_node = overlapped_height * overlapped_width + 1;

    int offset_row1 = 0 , offset_col1 = 0;
    int offset_row2 = 0 , offset_col2 = 0;
    if( p.x >= 0 ){
        offset_col1 = p.x;
        offset_col2 = 0;
    } else {
        offset_col1 = 0;
        offset_col2 = -p.x;
    }
    if( p.y >= 0 ){
        offset_row1 = p.y;
        offset_row2 = 0;
    } else {
        offset_row1 = 0;
        offset_row2 = -p.y;
    }

    constexpr int DIR_ROW[] = { 0 , 1 , 0, -1 };
    constexpr int DIR_COL[] = { 1 , 0 , -1, 0 };
    std::ofstream ofs("./network.dat");
    std::vector<edge_t> edges;
    for (int row = 0; row < overlapped_height; row++) {
        for (int col = 0; col < overlapped_width; col++) {
            int idx = row * overlapped_width + col;
            for (int dir =0; dir < 4 ; dir++) {
                int row_adj = row + DIR_ROW[dir];
                int col_adj = col + DIR_COL[dir];
                int idx_adj = row_adj * overlapped_width + col_adj;
                if ( 0 <= row_adj && row_adj < overlapped_height && 0 <= col_adj && col_adj < overlapped_width ) {
                    int row1 = offset_row1 + row, col1 = offset_col1 + col;
                    int row2 = offset_row2 + row, col2 = offset_col2 + col;
                    int weight = cost(src1.at<cv::Vec3b>(row1, col1),
                                      src2.at<cv::Vec3b>(row2, col2),
                                      src1.at<cv::Vec3b>(row1 + DIR_ROW[dir], col1 + DIR_COL[dir]),
                                      src2.at<cv::Vec3b>(row2 + DIR_ROW[dir], col2 + DIR_COL[dir]));
                    edges.emplace_back(idx,idx_adj,weight);
                }
            }
        }
    }

    //constraint
    constexpr int inf_weight = 1000000;
    // left side
    for(int row = 0; row < overlapped_height; row++) {
        int row1 = offset_row1 + row, col1 = offset_col1 + 0;
        int row2 = offset_row2 + row, col2 = offset_col2 + 0;
        int idx = row*overlapped_width;
        if(0<=row1&&row1<src1.rows&&0<=col1-1&&col1-1<src1.cols){
            edges.emplace_back(src_node, idx, inf_weight);
        }
        if(0<=row2&&row2<src2.rows&&0<=col2-1&&col2-1<src2.cols){
            edges.emplace_back(idx, sink_node, inf_weight);
        }
    }
    // right side
    for(int row = 0; row < overlapped_height; row++) {
        int row1 = offset_row1 + row, col1 = offset_col1 + overlapped_width-1;
        int row2 = offset_row2 + row, col2 = offset_col2 + overlapped_width-1;
        int idx = row*overlapped_width;
        if(0<=row1&&row1<src1.rows&&0<=col1-1&&col1-1<src1.cols){
            edges.emplace_back(src_node, idx, inf_weight);
        }
        if(0<=row2&&row2<src2.rows&&0<=col2-1&&col2-1<src2.cols){
            edges.emplace_back(idx, sink_node, inf_weight);
        }
    }
    // up side
    for(int col = 0; col < overlapped_width; col++) {
        int row1 = offset_row1 + 0, col1 = offset_col1 + col;
        int row2 = offset_row2 + 0, col2 = offset_col2 + col;
        int idx = col;
        if (0 <= row1 - 1 && row1 - 1 < src1.rows && 0 <= col1 && col1 < src1.cols){
            edges.emplace_back(src_node, idx, inf_weight);
        }
        if (0 <= row2 - 1 && row2 - 1 < src2.rows && 0 <= col2 && col2 < src2.cols){
            edges.emplace_back(idx, sink_node, inf_weight);
        }
    }
    // down side
    for(int col = 0; col < overlapped_width; col++) {
        int row1 = offset_row1 + overlapped_height - 1 , col1 = offset_col1 + col;
        int row2 = offset_row2 + overlapped_height - 1 , col2 = offset_col2 + col;
        int idx = overlapped_width * ( overlapped_height - 1 ) + col;
        if (0 <= row1 + 1 && row1 + 1 < src1.rows && 0 <= col1 && col1 < src1.cols){
            edges.emplace_back(src_node, idx, inf_weight);
        }
        if (0 <= row2 + 1 && row2 + 1 < src2.rows && 0 <= col2 && col2 < src2.cols){
            edges.emplace_back(idx, sink_node, inf_weight);
        }
    }

    for(int row=0;row<rect.height;row++){
        for(int col=0;col<rect.width;col++){
            int y = rect.y - sy;
            int x = rect.x - sx;
            int idx = overlapped_width * y + x;
            edges.emplace_back(idx,sink_node, inf_weight);
        }
    }



    int n = overlapped_width*overlapped_height+2;
    ofs << "p max " << n << " " << edges.size() << std::endl;
    ofs << "n " << src_node+1 << " s" << std::endl;
    ofs << "n " << sink_node+1 << " t" << std::endl;
    for(auto e:edges){
        ofs << "a " << e.s+1 << " " << e.t+1 << " " << e.cost << std::endl;
    }
    ofs.close();

    using namespace boost;

    typedef adjacency_list_traits < vecS, vecS, directedS > Traits;
    typedef adjacency_list < vecS, vecS, directedS,
      property < vertex_name_t, std::string,
      property < vertex_index_t, long,
      property < vertex_color_t, boost::default_color_type,
      property < vertex_distance_t, long,
      property < vertex_predecessor_t, Traits::edge_descriptor > > > > >,

      property < edge_capacity_t, long,
      property < edge_residual_capacity_t, long,
      property < edge_reverse_t, Traits::edge_descriptor > > > > Graph;

    Graph g;

    property_map < Graph, edge_capacity_t >::type
                    capacity = get(edge_capacity, g);
    property_map < Graph, edge_reverse_t >::type rev = get(edge_reverse, g);
    property_map < Graph, edge_residual_capacity_t >::type
                    residual_capacity = get(edge_residual_capacity, g);

    Traits::vertex_descriptor s, t;
    std::ifstream ifs("./network.dat", std::ios::in);
    read_dimacs_max_flow(g, capacity, rev, s, t, ifs);

    property_map < Graph, vertex_color_t >::type color = get(vertex_color, g);
    std::vector<Traits::edge_descriptor> pred(num_vertices(g));
    std::cout << "Graph Cut" << std::endl;
    auto start = std::chrono::system_clock::now();
    long flow = boykov_kolmogorov_max_flow(g, s, t);
    std::cout << "End" << std::endl;
    auto end = std::chrono::system_clock::now();
    auto dur = end - start;
    auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
    std::cout << msec << " milli sec" << std::endl;

    cv::Mat dst(overlapped_height,overlapped_width,CV_8UC3);
    graph_traits < Graph >::vertex_iterator u_iter, u_end;

    for (boost::tie(u_iter, u_end) = vertices(g); u_iter != u_end; ++u_iter){
      int idx = boost::get(boost::vertex_index, g, *u_iter);
      if(idx >= overlapped_width*overlapped_height) continue;
      int row = idx / overlapped_width;
      int col = idx % overlapped_width;
      if(color[s] == color[*u_iter]){
          dst.at<cv::Vec3b>(row,col) = src1.at<cv::Vec3b>(offset_row1 + row,offset_col1 + col);
      } else {
          dst.at<cv::Vec3b>(row,col) = src2.at<cv::Vec3b>(offset_row2 + row,offset_col2 + col);
      }
    }
    int global_row_dst = std::abs(p.y);
    int global_col_dst = std::abs(p.x);
    dst.copyTo(total_dst.rowRange(global_row_dst, global_row2+overlapped_height).colRange(global_col2, global_col2+overlapped_width));
    return total_dst;
}

