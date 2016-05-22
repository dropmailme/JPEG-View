#include <QMainWindow>
#include <QImage>
#include <QTimer>

namespace Ui {
    class FormMain;
}

class FormMain : public QMainWindow {
    Q_OBJECT
    public:
        explicit FormMain(QWidget *parent = 0);
        ~FormMain();
        QImage *imgLoad;
        QImage imgScale;
        QTimer *time;
        void getNewImage(int x, int y);

    public slots:
        void newImgSize();

    private:
        Ui::FormMain *ui;
};
