#ifndef RENDERWIGET_H
#define RENDERWIGET_H

#include <iostream>
#include <memory>
#include <QTime>
#include <QTimer>
#include <QElapsedTimer>
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QMessageBox>

#include "Camera.h"
#include "Model.h"
#include "BlinnPhongShader.h"
#include "SRendererDevice.h"
#include "BasicDataStructure.h"


const int DEFAULT_WIDTH = 800;
const int DEFAULT_HEIGHT = 600;
const float FIXED_CAMERA_FAR = 100.f;
static constexpr float SHININESS = 150.f;

namespace Ui {
class RenderWidget;
}

class RenderWidget : public QWidget
{
    Q_OBJECT

public:
    Camera m_camera;

    explicit RenderWidget(QWidget *parent = nullptr);
    ~RenderWidget();
    void setLightColor(Color color, LightColorType type);
    void setLightDir(Vector4D dir);
    void setRenderMode(RendererMode mode);
    void setFaceCulling(bool val);
    void setMultiThread(bool val);
    void setSIMD(bool val);
    void saveImage(QString path);
    void loadmodel(QString path);
    void initDevice();
    void togglePause();
    void showFPS(qint64& elapsed);

protected:
    void paintEvent(QPaintEvent *event)override;
    void mousePressEvent(QMouseEvent *event)override;
    void mouseReleaseEvent(QMouseEvent *event)override;
    void mouseMoveEvent(QMouseEvent *event)override;
    void wheelEvent(QWheelEvent *event)override;

signals:
    void sendModelData(int triangleCount, int vertexCount);
public slots:
    void render();
private:
    int m_width;
    int m_height;
    bool m_isPaused;
    QTimer m_timer;
    QElapsedTimer m_elapsedTimer;
    std::thread m_fpsShowThread;
    Ui::RenderWidget *ui;
    std::unique_ptr<Model> m_model;

    void processInput();
    void resetCamera();
};

#endif // RENDERWIGET_H
