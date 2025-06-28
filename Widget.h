#ifndef WIDGET_H
#define WIDGET_H

#include <QFileInfo>
#include <QFileDialog>
#include <QMainWindow>
#include "RenderWidget.h"

const int WIDGETWIDTH = 1000;
const int WIDGETHEIGHT = 650;


enum class Option
{
    MUTITHREAD,
    FACECULLING,
    SIMD
};

namespace Ui {
class Widget;
}

class Widget : public QMainWindow
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

    void setOption(Option option, bool val);
    void setCameraPara(CameraPara para, float val);
    void setLightColor(LightColorType type, QColor color);
    void setLightDir();

private slots:
    void on_actionopen_file_triggered();

    void on_actionsave_image_triggered();

    void on_MeshcheckBox_checkStateChanged(const Qt::CheckState &arg1);

    void on_FovSlider_valueChanged(int value);

    void on_NearSlider_valueChanged(int value);

    void on_PitchSlider_valueChanged(int value);

    void on_Yawdial_valueChanged(int value);

    void on_VertexCheckBox_checkStateChanged(const Qt::CheckState &arg1);

    void on_actionMultiThread_triggered();

    void on_actionTbbMultiThread_triggered();

    void on_actionFaceCulling_triggered();

    void on_RasterizationCheckBox_checkStateChanged(const Qt::CheckState &arg1);

    void on_actionSIMD_triggered();

    void on_actionTexture_triggered();

    void on_checkBox_checkStateChanged(const Qt::CheckState &arg1);

private:
    Ui::Widget *ui;
    QColor m_specularColor;
    QColor m_diffuseColor;
    QColor m_ambientColor;

    void initUi();
    void initSignalAndSlot();
};

#endif // WIDGET_H
