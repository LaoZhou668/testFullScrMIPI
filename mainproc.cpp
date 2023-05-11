#include "mainproc.h"
#include <QDebug>
#include <QTime>
#include <QMutex>
#include "postprocess.h"
mainproc::mainproc()
{
    // 1 -- 初始化RKNN
    nms_threshold = 0.4;
    box_conf_threshold = 0.2;
    rknn_context ctx = 0;
    post_process_type = "fp";
    init_model();

    // 2 -- 开启处理线程
    srcIm = Mat::zeros(1080,1080,CV_8UC3);
    isRunning = 1;

}
static void printRKNNTensor(rknn_tensor_attr *attr) {
    printf("index=%d name=%s n_dims=%d dims=[%d %d %d %d] n_elems=%d size=%d "
           "fmt=%d type=%d qnt_type=%d fl=%d zp=%d scale=%f\n",
           attr->index, attr->name, attr->n_dims, attr->dims[3], attr->dims[2],
           attr->dims[1], attr->dims[0], attr->n_elems, attr->size, 0, attr->type,
           attr->qnt_type, attr->fl, attr->zp, attr->scale);
}
void letterbox(cv::Mat rgb,cv::Mat &img_resize,int target_width,int target_height){

    float shape_0=rgb.rows;
    float shape_1=rgb.cols;
    float new_shape_0=target_height;
    float new_shape_1=target_width;
    float r=std::min(new_shape_0/shape_0,new_shape_1/shape_1);
    float new_unpad_0=int(round(shape_1*r));
    float new_unpad_1=int(round(shape_0*r));
    float dw=new_shape_1-new_unpad_0;
    float dh=new_shape_0-new_unpad_1;
    dw=dw/2;
    dh=dh/2;
    cv::Mat copy_rgb=rgb.clone();
    if(int(shape_0)!=int(new_unpad_0)&&int(shape_1)!=int(new_unpad_1)){
        cv::resize(copy_rgb,img_resize,cv::Size(new_unpad_0,new_unpad_1));
        copy_rgb=img_resize;
    }
    int top=int(round(dh-0.1));
    int bottom=int(round(dh+0.1));
    int left=int(round(dw-0.1));
    int right=int(round(dw+0.1));
    cv::copyMakeBorder(copy_rgb, img_resize,top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(0,0,0));

}
// 图像处理主框架
void mainproc::run()
{
    while (isRunning){

        const int target_width = 640;
        const int target_height = 640;
        const char *image_process_mode = "letter_box";

        float resize_scale = 0;
        int h_pad=0;
        int w_pad=0;
        const char *labels[] = {"pedestrian", "people", "bicycle", "car", "van", "truck", "tricycle", "awning-tricycle", "bus", "motor"};
        // 1 -- 更新RKNN输入图像
        // RKNN input image(640*640)

        Mat AIInputImage;
        cv::resize(srcIm, AIInputImage, Size(640,640));
        //BGR->RGB
        cv::cvtColor(AIInputImage, AIInputImage, cv::COLOR_BGR2RGB);
        cv::Mat img_resize;
        //float correction[2] = {0, 0};
        //float scale_factor[] = {0, 0};
        width=AIInputImage.cols;
        height=AIInputImage.rows;
        qDebug("width=%d", width);
        qDebug("height=%d", height);
        // Letter box resize
        float img_wh_ratio = (float) width / (float) height;
        float input_wh_ratio = (float) target_width / (float) target_height;
        int resize_width;
        int resize_height;
        if (img_wh_ratio >= input_wh_ratio) {
            //pad height dim
            resize_scale = (float) target_width / (float) width;
            resize_width = target_width;
            resize_height = (int) ((float) height * resize_scale);
            w_pad = 0;
            h_pad = (target_height - resize_height) / 2;
        } else {
            //pad width dim
            resize_scale = (float) target_height / (float) height;
            resize_width = (int) ((float) width * resize_scale);
            resize_height = target_height;
            w_pad = (target_width - resize_width) / 2;;
            h_pad = 0;
        }
        if(strcmp(image_process_mode,"letter_box")==0){
            letterbox(AIInputImage,img_resize,target_width,target_height);
        }else {
            cv::resize(AIInputImage, img_resize, cv::Size(target_width, target_height));
        }


        inputs[0].buf = img_resize.data;
        rknn_inputs_set(ctx, io_num.n_input, inputs);

        // 2 -- RKNN处理过程
        int ret=rknn_run(ctx, NULL);
        if (ret < 0) {
            printf("ctx error ret=%d\n", ret);
        }
//       QTime time;
//       time.start();
//       qDebug()<<time.elapsed()<<"ms";
        rknn_outputs_get(ctx, io_num.n_input, outputs, NULL);
        printf("sucess outputs get!");




        // 3 -- RKNN后处理
//        detect_result_group_t detect_result_group;
        QMutex mutex;
        std::vector<float> out_scales;
        //std::vector<int32_t> out_zps;
        std::vector<uint8_t> out_zps;
        for (int i = 0; i < io_num.n_output; ++i)
        {
            out_scales.push_back(output_attrs[i].scale);
            out_zps.push_back(output_attrs[i].zp);
        }
        mutex.lock();
        if (strcmp(post_process_type, "u8") == 0) {
            post_process_u8((uint8_t *) outputs[0].buf, (uint8_t *) outputs[1].buf, (uint8_t *) outputs[2].buf,
                            height, width,
                            h_pad, w_pad, resize_scale, box_conf_threshold, nms_threshold, out_zps, out_scales,
                            &detect_result_group, labels);
        } else if (strcmp(post_process_type, "fp") == 0) {
            post_process_fp((float *) outputs[0].buf, (float *) outputs[1].buf, (float *) outputs[2].buf, height,
                            width,
                            h_pad, w_pad, resize_scale, box_conf_threshold, nms_threshold, &detect_result_group, labels);
        }
//        post_process((int8_t *)outputs[0].buf, (int8_t *)outputs[1].buf, (int8_t *)outputs[2].buf, 640, 640,
//                    box_conf_threshold, nms_threshold, 1.0f, 1.0f, out_zps, out_scales, &detect_result_group);
        mutex.unlock();
        // 释放输出缓冲区（不释放会引起内存泄露!!!）
        rknn_outputs_release(ctx, io_num.n_output, outputs);
    }
}


// 初始化神经网络模型
int mainproc::init_model(void)
{
    // 1 -- 新建RKNN网络
    int model_data_size;
    unsigned char *model_data = load_modelData("/opt/model/RK356X/best.rknn", &model_data_size);
    //unsigned char *model_data = load_modelData("/opt/model/RK356X/yolov5s-640-640.rknn", &model_data_size);

    int ret = rknn_init(&ctx, model_data, model_data_size, 0, 0);
    if (ret < 0){
        qDebug("rknn_init error ret=%d", ret);
        return ret;
    }

    // 2 -- 显示SDK版本
    rknn_sdk_version version;
    ret = rknn_query(ctx, RKNN_QUERY_SDK_VERSION, &version,
                     sizeof(rknn_sdk_version));
    if (ret < 0){
        qDebug("rknn_init error ret=%d", ret);
        return ret;
    }
    qDebug("sdk version: %s driver version: %s", version.api_version,
               version.drv_version);

    // 3 -- 获取输入输出个数
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret < 0){
        qDebug("rknn_init error ret=%d", ret);
        return ret;
    }
    qDebug("model input num: %d, output num: %d", io_num.n_input,
           io_num.n_output);

    // 4 -- 当前RKNN的输入张量属性
    inputNum = io_num.n_input;
    rknn_tensor_attr input_attrs[inputNum];
    //memset(input_attrs, 0, sizeof(rknn_tensor_attr)*(io_num.n_input));
    memset(input_attrs, 0, sizeof(input_attrs));
    for (int i = 0; i < inputNum; i++){
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]),
                         sizeof(rknn_tensor_attr));
        if (ret < 0){
            qDebug("rknn_init error ret=%d", ret);
            return ret;
        }
        //dump_tensor_attr(&(input_attrs[i]));
        //printRKNNTensor(&(input_attrs[i]));
    }

    // 5 -- 当前RKNN的输出张量属性
    rknn_tensor_attr output_attrs[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs));
    for (int i = 0; i < io_num.n_output; i++)
    {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]),
                         sizeof(rknn_tensor_attr));
        //dump_tensor_attr(&(output_attrs[i]));
        //printRKNNTensor(&(output_attrs[i]));
    }

    // 6 -- 获取输入格式(CHW/HWC)
    int channel = 3;
    width = 0;
    height = 0;
    if (input_attrs[0].fmt == RKNN_TENSOR_NCHW){
        qDebug("model is NCHW input fmt");
        //channel = input_attrs[0].dims[0];
        width = input_attrs[0].dims[0];
        height = input_attrs[0].dims[1];
    }
    else{
        qDebug("model is NHWC input fmt");
        width = input_attrs[0].dims[1];
        height = input_attrs[0].dims[2];
        //channel = input_attrs[0].dims[3];
    }
    qDebug("model input height=%d, width=%d, channel=%d", height, width,
           channel);

    // 7 -- 定义输入数据缓冲
    memset(inputs, 0, sizeof(inputs));
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    //inputs[0].size = 640 * 640 * 3;
    inputs[0].size = width * height * channel;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    //inputs[0].fmt = input_attrs[0].fmt;
    inputs[0].pass_through = 0;

    // 8 -- 定义输出数据缓冲
    outputs[io_num.n_output];
    memset(outputs, 0, sizeof(outputs));
    for (int i = 0; i < io_num.n_output; i++) {
        if (strcmp(post_process_type, "fp") == 0) {
            outputs[i].want_float = 1;
        } else if (strcmp(post_process_type, "u8") == 0) {
            outputs[i].want_float = 0;
        }
    }
    printf("img.cols: %d, img.rows: %d\n", img_resize.cols, img_resize.rows);

    return 0;
}
// 获取RKNN网络数据
unsigned char* mainproc::load_modelData(const char *filename, int *model_size)
{
    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL){
        qDebug("Open RKNN failed!!", filename);
        return NULL;
    }
    // Get Model Size
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    // Get Model Data
    fseek(fp, 0, SEEK_SET);
    unsigned char *data = (unsigned char *)malloc(size);
    fread(data, 1, size, fp);
    fclose(fp);
    // return para
    *model_size = size;
    return data;
}






