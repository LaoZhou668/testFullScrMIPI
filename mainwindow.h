#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

// 主处理线程
#include "mainproc.h"
#include <QThread>

// OpenCV
#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
using namespace cv;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

// -----------------------------  摄像机相关  --------------------------------------
public:
    VideoCapture mVidCap;
    Rect roiRect;
    Mat roi;    // chuli he xianshi yong src  Im
    // 显示用定时器
    int timerDisp;
    void timerEvent(QTimerEvent *event);

// -------------------------------- 处理相关 ----------------------------------------
public:
    // 处理主线程
    mainproc *pMainProc;

private:
    Ui::MainWindow *ui;
    // OpenCV Mat ---> QImage(RGB+Gray)
    QImage Mat2QImage(Mat src);
    // 退出按键检测
    void keyPressEvent(QKeyEvent *event);

};

#endif // MAINWINDOW_H
