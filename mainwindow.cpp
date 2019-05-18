#include <iostream>

#include <QString>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QPainter>
#include <QMouseEvent>
#include <QMessageBox>
#include <QMenuBar>
#include <QWheelEvent>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "./image_synthesis.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    masking=false;
    imgloaded = false;
    scale=1;
    srcx=srcy=0;
    ui->setupUi(this);
    ui->actionMerge->setEnabled(false);
    ui->actionSave->setEnabled(false);
    ui->actionUseResult->setEnabled(false);
    setWindowTitle("GraphCut Image Merging");
}

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::paintEvent(QPaintEvent *){
    if(!imgloaded) return;
    if(!ui->actionSave->isEnabled()){
        this->setFixedHeight(trgqtimg.height());
        this->setFixedWidth(trgqtimg.width());
        QImage image(trgqtimg.size(), QImage::Format_ARGB32_Premultiplied);
        QPainter imagePainter(&image);
        imagePainter.setRenderHint(QPainter::Antialiasing, true);

        qreal alpha=ui->actionOpacity->isChecked()?0.8:1;
        imagePainter.drawImage(0,0,trgqtimg);
        imagePainter.setOpacity(alpha);


        cv::Mat resizesrcimg(srccvmat.rows*scale,srccvmat.cols*scale,srccvmat.type());
        cv::resize(srccvmat, resizesrcimg, resizesrcimg.size(), cv::INTER_CUBIC);
        imagePainter.drawImage(srcx,srcy,cvMatToQImage(resizesrcimg));
        std::cout << srcx << " " << srcy << std::endl;
        imagePainter.setPen(Qt::black);
        imagePainter.drawRect(rect);
        QPainter widgetPainter(this);
        widgetPainter.drawImage(0, 0, image);
    }else{
        this->setFixedHeight(dstqtimg.height());
        this->setFixedWidth(dstqtimg.width());
        QImage image(dstqtimg.size(), QImage::Format_ARGB32_Premultiplied);
        QPainter imagePainter(&image);
        imagePainter.setRenderHint(QPainter::Antialiasing, true);
        imagePainter.drawImage(0,0,dstqtimg);
        QPainter widgetPainter(this);
        widgetPainter.drawImage(0, 0, image);
    }

}

void MainWindow::mouseMoveEvent(QMouseEvent *event){
    if(!ui->actionMerge->isEnabled()) return;
    if(masking){
        int rx1 ,ry1, rx2,ry2;
        rect.getCoords(&rx1,&ry1,&rx2,&ry2);
        rect.setCoords(rx1,ry1,event->x(),event->y());
    }else{
        srcx+=event->x()-mx;
        srcy+=event->y()-my;
    }
    mx=event->x();
    my=event->y();
    update();
}

void MainWindow::mousePressEvent(QMouseEvent *event){
    if(!ui->actionMerge->isEnabled())return;
    mx=event->x();
    my=event->y();
    if(event->button()==Qt::RightButton){
        masking=true;
        rect.setCoords(mx,my,mx,my);
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *){
    masking=false;
}

bool MainWindow::Load(){
    srcfile = QFileDialog::getOpenFileName(this,tr("Source File"),"/home/terasaki/ImageforPoissonImageEditing",tr("Image(*.tif *.png *.jpg *.gif)"));
    if(srcfile=="") return false;
    srccvmat=cv::imread(srcfile.toStdString());
    if(srccvmat.empty()) return false;
    srcqtimg=cvMatToQImage(srccvmat);

    trgfile = QFileDialog::getOpenFileName(this,tr("Target File"),"/home/terasaki/ImageforPoissonImageEditing",tr("Image(*.tif *.png *.jpg *.gif)"));
    if(trgfile=="") return false;
    trgcvmat=cv::imread(trgfile.toStdString());
    if(trgcvmat.empty())return false;

    trgqtimg=cvMatToQImage(trgcvmat);
    setFixedSize(trgqtimg.size());
    srcx=srcy=0;

    return true;
}

void MainWindow::on_actionOpen_triggered()
{
    imgloaded=Load();
    if(imgloaded){
        scale=1;
        srcx=srcy=0;
    }
    ui->actionSave->setEnabled(false);
    ui->actionMerge->setEnabled(imgloaded);
}

void MainWindow::wheelEvent(QWheelEvent *event){
    double step=(double)event->delta();
    if(step>0&&scale<10)
        scale*=1.1;
    else if(step<0&&scale>0.1)
        scale*=1.0/1.1;
    update();
}

void MainWindow::on_actionSave_triggered(){
    QString dstfile = QFileDialog::getSaveFileName(this,tr("dst File"),".",tr("Image(*.tif *.png *.jpg)"));
    if(dstfile=="") return;
    cv::imwrite(dstfile.toStdString(),dstcvmat);
    QMessageBox::about(this,"success","dst file wrote:"+dstfile);
}

void MainWindow::on_actionMerge_triggered()
{
    cv::Mat resizesrcmat(srccvmat.rows*scale,srccvmat.cols*scale,srccvmat.type());
    cv::resize(srccvmat, resizesrcmat, resizesrcmat.size(), cv::INTER_CUBIC);

    cv::Rect cvrect(rect.x(),rect.y(),rect.width(),rect.height());
    dstcvmat = is(trgcvmat,resizesrcmat,cv::Point2i(srcx,srcy),cvrect);

    if(dstcvmat.empty()) return;
    dstqtimg=cvMatToQImage(dstcvmat);
    std::cout << dstqtimg.width() << dstqtimg.height() << std::endl;

    ui->actionMerge->setEnabled(false);
    ui->actionSave->setEnabled(true);
    ui->actionUseResult->setEnabled(true);

    update();
}

void MainWindow::on_actionOpacity_triggered()
{
    update();
}

void MainWindow::on_actionUndo_triggered()
{
    ui->actionMerge->setEnabled(true);
    ui->actionSave->setEnabled(false);

    update();

}

void MainWindow::on_actionUseResult_triggered()
{
    trgqtimg = dstqtimg.copy();
    trgcvmat = dstcvmat.clone();
    ui->actionUseResult->setEnabled(false);
    ui->actionMerge->setEnabled(true);
    ui->actionSave->setEnabled(false);
    update();
}
