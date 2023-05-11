#ifndef MAINPROC_H
#define MAINPROC_H



// 线程操作
#include <QThread>

// OpenCV
#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
using namespace cv;


#include "rknn_api.h"
#include "postprocess.h"

class mainproc : public QThread
{
public:
    mainproc();
    // 主线程的输入图像
    Mat srcIm;
    // 关闭时使用
    int isRunning;

    //jiancekuang
    detect_result_group_t detect_result_group;

// ----------------------------- RKNN相关 --------------------------------------
public:
    // 初始化神经网络模型
    int init_model(void);
    // 检测控制参数
    float nms_threshold;
    float box_conf_threshold;

    // RKNN对象
    rknn_context ctx;
    const char *post_process_type;
    // RKNN的输入对象参数
    Mat img_resize;
    int inputNum;
    int width, height;
    rknn_input inputs[1];
    // RKNN的输出对象参数
    rknn_input_output_num io_num;
    //int outputNum;
    // output Num Max = 5
    rknn_output outputs[5];
    rknn_tensor_attr output_attrs[5];
    static void printRKNNTensor(rknn_tensor_attr *attr);
private:
    // 线程入口函数
    void run();

    // 获取RKNN网络数据
    unsigned char* load_modelData(const char *filename, int *model_size);


};

#endif // MAINPROC_H
