#include "formmain.h"
#include "ui_formmain.h"
#include <QImage>

FormMain::FormMain(QWidget *parent) : QMainWindow(parent), ui(new Ui::FormMain) {
    ui->setupUi(this);
    int x;
    int y;
    this->resize(960,540);
    ui->IMG = new QLabel(this);
    imgLoad = new QImage("AlexLoi.Dev.JPEG.ppm");
    imgScale = imgLoad->scaled(960,640,Qt::KeepAspectRatio);
    y = imgScale.height();
    x = imgScale.width();
    ui->IMG->show();
    this->resize(x,y);
    ui->IMG->resize(x,y);
    ui->IMG->setAlignment(Qt::AlignCenter);
    ui->IMG->setPixmap(QPixmap::fromImage(imgScale, Qt::AutoColor));
    time  = new QTimer(this);
    connect(time, SIGNAL(timeout()), this, SLOT(newImgSize()));
    time->start(10);
}

void FormMain::newImgSize() {
    int x;
    int y;
    y = this->height();
    x = this->width();
    getNewImage(x,y);
    ui->IMG->resize(x,y);
}

void FormMain::getNewImage(int x, int y) {
    imgScale = imgLoad->scaled(x,y,Qt::KeepAspectRatio);
    ui->IMG->setPixmap(QPixmap::fromImage(imgScale, Qt::AutoColor));
}

FormMain::~FormMain()
{
    delete ui;
}
