#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QImage>
#include <QVector>
#include <QQueue>
#include <iostream>
using namespace std;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}
QImage imgGray;
QVector<QVector<int>> imgArray;

void MainWindow::on_pushButton_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::homePath(), tr("Images (*.png *.xpm *.jpg *.ppm)"));
    if (!file_name.isEmpty()){

        //open prompt and display image
        QMessageBox::information(this, "...", file_name);
        QImage img(file_name);
        imgGray = img.convertToFormat(QImage::Format_Grayscale8);
        QPixmap pix = QPixmap::fromImage(imgGray);
        int w = imgGray.size().width();
        int h = imgGray.size().height();
        ui->label_pic->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
        ui->label_pic->setPixmap(pix);

        ui->lineEdit->setText(QString::number(w));
        ui->lineEdit_2->setText(QString::number(h));

        //get image width and height, create empty binary matrix
        unsigned int cols = img.width();
        unsigned int rows = img.height();
        unsigned int numBlackPixels = 0;

        for (unsigned int i = 0; i < rows; i++){
            imgArray.push_back(QVector<int>());
            for (unsigned int j = 0; j < cols; j++){
                QColor clrCurrent( imgGray.pixel( i, j ));
                int r = (clrCurrent.red() + clrCurrent.green() + clrCurrent.blue()) / 3;
                imgArray[i].push_back(r);
            }
            cout << endl;
        }
    }

}

void grayscaleToQPixmap(const QVector<QVector<int>> &grayscaleImage, QPixmap &qPixmap) {
    // Convert the grayscale image to a QImage.
    QImage image(grayscaleImage.size(), grayscaleImage[0].size(),QImage::Format_Grayscale8);
    for (int i = 0; i < grayscaleImage.size(); ++i) {
        for (int j = 0; j < grayscaleImage[i].size(); ++j) {
            QRgb value = qRgb(grayscaleImage[i][j],grayscaleImage[i][j],grayscaleImage[i][j]);
            image.setPixel(i,j,value);
        }
    }

    // Convert the QImage to a QPixmap.
    qPixmap = QPixmap::fromImage(image);
}

void getPixel(QVector<QVector<int>> img, int x, int y, unsigned char *R)
{
    // Get the colour at pixel x,y in the image and return it using the provided RGB pointers
    // Requires the image size along the x direction!
    *(R)=img[x][y];
}

double step_x,step_y;      // Step increase as per instructions above
unsigned char R1,R2,R3,R4; // Colours at the four neighbours
double RT1;                // Interpolated colours at T1 and T2
double RT2;
unsigned char R;           // Final colour at a destination pixel
int x,y;                   // Coordinates on destination image
double fx,fy;              // Corresponding coordinates on source image
double dx,dy;              // Fractional component of source image    coordinates

void handleLinearInterpolationX(QVector<QVector<int>> src,QVector<QVector<int>> &dst, int src_x, int src_y, int dest_x, int dest_y) {
    step_x=(double)(src_x-1)/(double)(dest_x-1);
    step_y=(double)(src_y-1)/(double)(dest_y-1);

    for (x=0;x<dest_x;x++) {        // Loop over destination image
        dst.push_back(QVector<int>());
        for (y=0;y<dest_y;y++)
        {
            fx=x*step_x;
            fy=y*step_y;
            dx=fx-(int)fx;
            dy=fy-(int)fy;
            getPixel(src,floor(fx),floor(fy),&R1);      // get N1 colours
            getPixel(src,ceil(fx),floor(fy),&R2);       // get N2 colours
            // Interpolate to get T1 and T2 colours
            RT1=(dx*R2)+(1-dx)*R1;

            // Store the final colour
            dst[x].push_back(RT1);
        }
    }
}

void handleLinearInterpolationY(QVector<QVector<int>> src,QVector<QVector<int>> &dst, int src_x, int src_y, int dest_x, int dest_y) {
    step_x=(double)(src_x-1)/(double)(dest_x-1);
    step_y=(double)(src_y-1)/(double)(dest_y-1);

    for (x=0;x<dest_x;x++) {        // Loop over destination image
        dst.push_back(QVector<int>());
        for (y=0;y<dest_y;y++)
        {
            fx=x*step_x;
            fy=y*step_y;
            dx=fx-(int)fx;
            dy=fy-(int)fy;
            getPixel(src,floor(fx),floor(fy),&R1);    // get N1 colours
            getPixel(src,floor(fx),ceil(fy),&R3); // get N3 colours
            // Interpolate to get T1 and T2 colours
            RT1=(dy*R3)+(1-dy)*R1;

            // Store the final colour
            dst[x].push_back(RT1);
        }
    }
}


void handleBilinearInterpolation( QVector<QVector<int>> src,QVector<QVector<int>> &dst, int src_x, int src_y, int dest_x, int dest_y)
{

    step_x=(double)(src_x-1)/(double)(dest_x-1);
    step_y=(double)(src_y-1)/(double)(dest_y-1);

    for (x=0;x<dest_x;x++) {        // Loop over destination image
        dst.push_back(QVector<int>());
        for (y=0;y<dest_y;y++)
        {
            fx=x*step_x;
            fy=y*step_y;
            dx=fx-(int)fx;
            dy=fy-(int)fy;
            getPixel(src,floor(fx),floor(fy),&R1);    // get N1 colours
            getPixel(src,ceil(fx),floor(fy),&R2); // get N2 colours
            getPixel(src,floor(fx),ceil(fy),&R3); // get N3 colours
            getPixel(src,ceil(fx),ceil(fy),&R4);  // get N4 colours
            // Interpolate to get T1 and T2 colours
            RT1=(dx*R2)+(1-dx)*R1;

            RT2=(dx*R4)+(1-dx)*R3;
            // Obtain final colour by interpolating between T1 and T2
            R=(unsigned char)((dy*RT2)+((1-dy)*RT1));

            // Store the final colour
            dst[x].push_back(R);
        }
    }
}


// Task 1:
void MainWindow::on_pushButton_2_clicked()
{
    // Reset stage
    ui->label_error->setText("");

    QRadioButton *r1 = ui->radioButton;
    QRadioButton *r2 = ui->radioButton_2;
    QRadioButton *r3 = ui->radioButton_3;
    QRadioButton *r4 = ui->radioButton_4;

    int destWidth = ui->lineEdit->text().toInt();
    int destHeight = ui->lineEdit_2->text().toInt();

    QVector<QVector<int>> newImg;
    int w = imgArray.size();
    int h = imgArray[0].size();

    if(r1->isChecked()) {
        // Linear x-axis
        ui->label_error->setText("Generating...");
        handleLinearInterpolationX(imgArray, newImg, w, h, destWidth, destHeight);
        ui->label_error->setText("");

    } else if(r2->isChecked()) {
        // Linear y-axis
        ui->label_error->setText("Generating...");
        handleLinearInterpolationY(imgArray, newImg, w, h, destWidth, destHeight);
        ui->label_error->setText("");

    } else if(r3->isChecked()) {
        // Bilinear
        ui->label_error->setText("Generating...");
        handleBilinearInterpolation(imgArray, newImg, w, h, destWidth, destHeight);
        ui->label_error->setText("");

    } else if(r4->isChecked()) {
       // Nearest Neighbors
        //Use nearest sampling

        double scale_w = w / (double)destWidth;
        double scale_h = h / (double)destHeight;

        int tSrcH = 0, tSrcW = 0;
        int index_src = 0, index_dest = 0;

        for (int y=0; y < destWidth; y++) {
            newImg.push_back(QVector<int>());

            for (int x=0; x < destHeight; x++) {
                newImg[y].push_back(imgArray[y* scale_w][x * scale_h]);
            }
        }

    }
    QPixmap result;
    grayscaleToQPixmap(newImg , result);
    ui->label_pic_2->setPixmap(result);
    if(destWidth < 100 && destHeight < 100) {
    ui->label_pic_3->setPixmap(result.scaled(ui->label_pic_3->width(),ui->label_pic_3->height()));
    }

    ui->label_pic->adjustSize();


    // Showing new size
    ui->label_new_w->setText(QString::number(newImg.size()));
    ui->label_new_h->setText(QString::number(newImg[0].size()));

    // Reset stage
    for(int i=0 ; i< newImg.size(); i++) {
        for(int j=0 ; j< newImg[i].size(); j++) {
            newImg[i].pop_back();
        }
        newImg.pop_front();
    }
//    QString fileSave = "/Users/datnguyen/Downloads/ImgResult/Task1/result" + QString::number(rand() % 1000)+".png";
//    result.save(fileSave, "PNG");
}

void handleReducingBit(QVector<QVector<int>> & src, QVector<QVector<int>> & newImg, int bits) {
    int num_colors = pow(2, bits);
    int divisor = 256 / num_colors;
    int max_quantized_value = 255 / divisor;

    for(int i=0 ; i< src.size(); i++) {
        newImg.push_back(QVector<int>());
        for(int j=0 ; j< src[i].size(); j++) {
            int new_value = ((imgArray[i][j] / divisor) * 255) / max_quantized_value;
            newImg[i].push_back(new_value);
        }
    }


}

// Task 2:
void MainWindow::on_pushButton_3_clicked()
{
    int newBit = ui->spinBox->text().toInt();
    QVector<QVector<int>> newImg;

    handleReducingBit(imgArray, newImg, newBit);



    // Showing result image
    int destWidth = ui->lineEdit->text().toInt();
    int destHeight = ui->lineEdit_2->text().toInt();
    QPixmap result;
    grayscaleToQPixmap(newImg , result);
    ui->label_pic_2->setPixmap(result);
    if(destWidth < 100 && destHeight < 100) {
        ui->label_pic_3->setPixmap(result.scaled(ui->label_pic_3->width(),ui->label_pic_3->height()));
    }

    ui->label_pic->adjustSize();
    // Showing new size
    ui->label_new_w->setText(QString::number(newImg.size()));
    ui->label_new_h->setText(QString::number(newImg[0].size()));

    // Reset stage
    for(int i=0 ; i< newImg.size(); i++) {
        for(int j=0 ; j< newImg[i].size(); j++) {
            newImg[i].pop_back();
        }
        newImg.pop_front();
    }
//    imgArray = newImg;

    QString fileSave = "/Users/datnguyen/Downloads/ImgResult/Task2/result" + QString::number(rand() % 1000)+".png";
    result.save(fileSave, "PNG");
}


// Increase the zooming by 2
void MainWindow::on_pushButton_5_clicked()
{
    int w = ui->lineEdit->text().toInt();
    int h = ui->lineEdit_2->text().toInt();
    ui->lineEdit->setText(QString::number(w*2));
    ui->lineEdit_2->setText(QString::number(h*2));
}


void MainWindow::on_pushButton_4_clicked()
{
    int w = ui->lineEdit->text().toInt();
    int h = ui->lineEdit_2->text().toInt();
    if(w/2 != 0 && h/2 != 0) {
    ui->lineEdit->setText(QString::number(w/2));
    ui->lineEdit_2->setText(QString::number(h/2));
    } else {
    ui->label_error->setText("Cannot make smaller. Please try another dimensions");
    }
}

