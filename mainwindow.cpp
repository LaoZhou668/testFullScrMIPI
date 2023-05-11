#include "mainwindow.h"
#include "ui_mainwindow.h"
#include<QDebug>
#include<QTime>
#include <QKeyEvent>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1 -- 设置视频为全屏模式
    // 首先设置窗口为全屏（标题栏菜单栏已经预先去掉）
    setWindowState(Qt::WindowFullScreen);
    //setWindowFlag(Qt::FramelessWindowHint);
    // 视频窗口预设全屏分辨率
    ui->LBL_IM_SRC->setFixedWidth(1920);
    ui->LBL_IM_SRC->setFixedHeight(1080);
    // 视频窗口全屏并居中
    ui->LBL_IM_SRC->showFullScreen();
    ui->LBL_IM_SRC->setAlignment(Qt::AlignCenter);
    ui->LBL_IM_SRC->setStyleSheet("background-color: rgb(40,44,52)");
    // 视频窗口响应按键消息
    ui->LBL_IM_SRC->installEventFilter(this);

    // 2 -- 打开并测试Camera
//    if (mVidCap.open(9, cv::CAP_V4L2) == false){
//        qDebug("false");
//        return;
//    }
    //    mVidCap = VideoCapture("v4l2src device=/dev/video0 ! video/x-raw,format=NV12,width=1920,height=1080, "
    //                           "framerate=30/1 ! videoconvert ! appsink",cv::CAP_GSTREAMER);
    //    while(mVidCap.isOpened() == false){
    //        //QThread::sleep(100);
    //        qDebug("false");
    //    }
    //open video
    if(mVidCap.open("/opt/personCar/1080.mp4") == false)
    {
        qDebug("false");
        return;
    }
    // 设置视频格式+分辨率
    mVidCap.set(CV_CAP_PROP_FOURCC, CV_FOURCC('M', 'J', 'P', 'G'));
    mVidCap.set(CAP_PROP_FRAME_WIDTH,  1920);
    mVidCap.set(CAP_PROP_FRAME_HEIGHT, 1080);
    // 读取测试帧8f
    Mat tmpSrc;
    for (int i = 0; i < 8; i ++){
        QThread::msleep(33);
        mVidCap.read(tmpSrc);
    }
    qDebug("VIS Camera OK!!!");

    // 3 -- 开始显示原始图像并更新处理主线程的输入图像
    // 初始化中心截取窗口
    roiRect.x = (1920-1080)/2;
    roiRect.y = 0;
    roiRect.width = 1080;
    roiRect.height = 1080;
    // 打开显示线程
    timerDisp = startTimer(33);

    // 4 -- 初始化处理主线程
    pMainProc = new mainproc();
    pMainProc->start();
}


// 主定时器(Proc)
void MainWindow::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == timerDisp){


        // 1 -- 获取原始图像(1920*1080)
        Mat tmpSrc;
        mVidCap.read(tmpSrc);

        // 2 -- 获取中心ROI(1080*1080)
        tmpSrc(roiRect).copyTo(pMainProc->srcIm);
        // 3 -- 叠加RKNN输出的检测框信息
        // ...........
        detect_result_group_t detect_result_groups = pMainProc->detect_result_group;
        char text[256];
        for (int i = 0; i < detect_result_groups.count; i++)
        {
            detect_result_t *det_result = &(detect_result_groups.results[i]);
            sprintf(text, "%s %.1f%%", det_result->name, det_result->prop * 100);
            printf("%s @ (%d %d %d %d) %f\n",
                   det_result->name,
                   det_result->box.left, det_result->box.top, det_result->box.right, det_result->box.bottom,
                   det_result->prop);
            int x1 = det_result->box.left*1.6875;   // 1080/640
            int y1 = det_result->box.top*1.6875;
            int x2 = det_result->box.right*1.6875;
            int y2 = det_result->box.bottom*1.6875;
            rectangle(pMainProc->srcIm, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(0, 0, 255), 1);
            putText(pMainProc->srcIm, text, cv::Point(x1, y1 + 12), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));
        }
        // 4 -- 显示最终结果
        Mat litIm;
        cv::resize(pMainProc->srcIm, litIm, Size(1080, 1080));
        QImage Qim = Mat2QImage(litIm);
        ui->LBL_IM_SRC->setPixmap(QPixmap::fromImage(Qim));

    }   // if event
} // fun timerEvent()

MainWindow::~MainWindow()
{
    // 1 -- 关闭处理模块线程
    pMainProc->isRunning = 0;
    QThread::msleep(100);   // 等待结束
    pMainProc->quit();
    delete pMainProc;
    // 2 -- 关闭界面刷新
    killTimer(timerDisp);
    QThread::msleep(300);   // 等待结束
    // 3 -- 关闭UI
    delete ui;
}


// 退出程序（退出全屏）
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape){
        QApplication::closeAllWindows();
    }
}

// OpenCV Mat ---> QImage(RGB+Gray)
QImage MainWindow::Mat2QImage(Mat src)
{
    QImage imag;

    // RGB
    if (src.channels() == 3){
        cvtColor(src, src, CV_BGR2RGB);
        imag = QImage((const unsigned char *)(src.data), src.cols, src.rows,
                      src.cols*src.channels(), QImage::Format_RGB888);
    }
    // Gray
    else if (src.channels() == 1){
        imag = QImage((const unsigned char *)(src.data), src.cols, src.rows,
                      src.cols*src.channels(), QImage::Format_Grayscale8);
    }
    // others
    else{
        imag = QImage((const unsigned char *)(src.data), src.cols, src.rows,
                      src.cols*src.channels(), QImage::Format_RGB888);
    }
    return imag;
}   // Mat2QImage()




