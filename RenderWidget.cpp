#include "RenderWidget.h"
#include "ui_RenderWidget.h"
#include "Mat.h"

const int FPS_UPDATE_INTERVAL_MS = 500;

int lastFrameTime = 0;
int deltaTime = 0;
QPoint lastPos;
QPoint currentPos;
int ratio = 0;
Qt::MouseButtons currentBtns;

QElapsedTimer fpsUpdateElapsedTimer;
int frameCount = 0; // 用于统计帧数
float accumulatedDeltaTime = 0.0f; // 累积的帧间时间

RenderWidget::RenderWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RenderWidget)
    ,m_camera(static_cast<float>(DEFAULT_WIDTH) / static_cast<float>(DEFAULT_HEIGHT), FIXED_CAMERA_FAR)
    ,m_width(DEFAULT_WIDTH)
    ,m_height(DEFAULT_HEIGHT)
    ,m_isPaused(true)
    ,m_model(nullptr)
{
    ui->setupUi(this);
    ui->FPSLabel->setStyleSheet("background:transparent");
    setFixedSize(m_width, m_height);
    initDevice();
    connect(&m_timer, &QTimer::timeout, this, &RenderWidget::render);
    m_timer.start(1);
}

RenderWidget::~RenderWidget()
{
    delete ui;
}

void RenderWidget::setLightColor(Color color, LightColorType type)
{
    switch(type)
    {
    case LightColorType::DIFFUSE:
        SRendererDevice::getInstance().m_shader->m_lightList[0].diffuse = color;
        break;
    case LightColorType::SPECULAR:
        SRendererDevice::getInstance().m_shader->m_lightList[0].specular = color;
        break;
    case LightColorType::AMBIENT:
        SRendererDevice::getInstance().m_shader->m_lightList[0].ambient = color;
        break;
    }
}

void RenderWidget::setLightDir(Vector4D dir)
{
    SRendererDevice::getInstance().m_shader->m_lightList[0].dir = dir;
}

void RenderWidget::setRenderMode(RendererMode mode)
{
    SRendererDevice::getInstance().m_rendererMode = mode;
}

void RenderWidget::setFaceCulling(bool val)
{
    SRendererDevice::getInstance().m_faceCulling = val;
}

void RenderWidget::setMultiThread(bool val)
{
    SRendererDevice::getInstance().m_multiThread = val;
}

void RenderWidget::setTBBMultiThread(bool val)
{
    SRendererDevice::getInstance().m_tbbThread = val;
}

 void RenderWidget::setSIMD(bool val)
{
    SRendererDevice::getInstance().m_simd = val;
}

void RenderWidget::showFPS(qint64 &elapsed)
{
    // int nowTime = QTime::currentTime().msecsSinceStartOfDay();
    // if(lastFrameTime != 0){
    //     deltaTime = nowTime - lastFrameTime;
    //     ui->FPSLabel->setText(QString("FPS : %1").arg(1000.0 / deltaTime, 0, 'f', 0));
    // }
    // lastFrameTime = nowTime;

    // 使用QElapsedTimer测量帧间时间
    // qint64 elapsed = fpsUpdateElapsedTimer.elapsed();
    // fpsUpdateElapsedTimer.restart(); // 重启计时器以测量下一帧时间
    // deltaTime = elapsed / 1000.0f; // 将毫秒转换为秒
    // accumulatedDeltaTime += deltaTime;
    // frameCount++;

    // if (fpsUpdateElapsedTimer.hasExpired(FPS_UPDATE_INTERVAL_MS)) {

    //         float avgDeltaTime = accumulatedDeltaTime / frameCount;
    //         float currentFPS = (avgDeltaTime > 0) ? (1.0f / avgDeltaTime) : 0.0f;
    //         ui->FPSLabel->setText(QString("FPS : %1").arg(currentFPS, 0, 'f', 0));
    //     // 重置计数器和累积时间
    //     frameCount = 0;
    //     accumulatedDeltaTime = 0.0f;
    // }
}

void RenderWidget::saveImage(QString path)
{
     std::cout << "it is  RenderWidget::saveImage" << std::endl;
    SRendererDevice::getInstance().saveImage(path);
}

void RenderWidget::loadmodel(QString path)
{
    // 申请新模型内存
    std::unique_ptr<Model> newModel = std::make_unique<Model>(path);
    if(!newModel->m_loadSuccess){
        QMessageBox::critical(this, "Error", "model loading error");
        return;
    }

    if(m_model){
        // 若model指针不为空，即原来已经加载了模型
        // 更新渲染控制状态
        togglePause();
    }

    // 将新模型的数据存入维护的内存中
    m_model = std::move(newModel);

    // 更新模型数据
    sendModelData(m_model->m_triangleCount, m_model->m_vertexCount);
    resetCamera();
    std::cout << "model load success" ;
}

void RenderWidget::initDevice() // ////////////////////
{
    SRendererDevice::init(m_width, m_height);
    SRendererDevice::getInstance().m_shader = std::make_unique<BlinnPhongShader>();
    SRendererDevice::getInstance().m_shader->m_lightList.push_back(Light());
}

void RenderWidget::togglePause()
{
    m_isPaused = !m_isPaused;
}

void RenderWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.drawImage(0, 0, SRendererDevice::getInstance().getInstance().getBuffer());
}

void RenderWidget::mousePressEvent(QMouseEvent *event)
{
    currentBtns = event->buttons();
    currentPos = event->pos();
    lastPos = {0, 0};
}

void RenderWidget::mouseReleaseEvent(QMouseEvent *event)
{
    currentBtns = event->buttons();
}

void RenderWidget::mouseMoveEvent(QMouseEvent *event)
{
    currentPos = event->pos();
}

void RenderWidget::wheelEvent(QWheelEvent *event)
{
    QPoint numPixels = event->pixelDelta();
    QPoint numDegrees = event->angleDelta() / 8;

    QPoint res;
    if(!numPixels.isNull()){
        res = numPixels;
    }
    else if(!numDegrees.isNull()){
        QPoint numSteps = numDegrees / 15;
        res = numSteps;
    }
    ratio += res.y();
}


void RenderWidget::render()
{
    if(m_isPaused){
        return;
    }

    auto& renderDevice = SRendererDevice::getInstance();

    renderDevice.clearBuffer(); // 清屏
    if(this->m_model == nullptr){return;}

    int nowTime = QTime::currentTime().msecsSinceStartOfDay();
    if(lastFrameTime != 0){
        deltaTime = nowTime - lastFrameTime;
        ui->FPSLabel->setText(QString("FPS : %1").arg(1000.0 / deltaTime, 0, 'f', 0));
    }
    lastFrameTime = nowTime;

    // qint64 elapsed = fpsUpdateElapsedTimer.elapsed();
    // fpsUpdateElapsedTimer.restart(); // 重启计时器以测量下一帧时间
    // deltaTime = elapsed / 1000.0f; // 将毫秒转换为秒
    // accumulatedDeltaTime += deltaTime;
    // frameCount++;

    // if (fpsUpdateElapsedTimer.hasExpired(FPS_UPDATE_INTERVAL_MS)) {

    //     float avgDeltaTime = accumulatedDeltaTime / frameCount;
    //     float currentFPS = (avgDeltaTime > 0) ? (1.0f / avgDeltaTime) : 0.0f;
    //     ui->FPSLabel->setText(QString("FPS : %1").arg(currentFPS, 0, 'f', 0));
    //     // 重置计数器和累积时间
    //     frameCount = 0;
    //     accumulatedDeltaTime = 0.0f;
    // }

    processInput();
    renderDevice.m_shader->m_modelTransformation = m_model->getModelTansformation();
    renderDevice.m_shader->m_viewTransformation = m_camera.getViewMatrix();
    renderDevice.m_shader->m_projectionTransformation = m_camera.getProjectionMatrix();
    renderDevice.m_shader->m_eyePos = m_camera.m_position;
    renderDevice.m_shader->m_material.shininess = SHININESS;

    this->m_model->draw();
    update();
}

void RenderWidget::processInput()
{
    if((currentBtns & Qt::LeftButton) || (currentBtns & Qt::RightButton)){
        if(!lastPos.isNull()){
            Vector2D motion = {static_cast<float>((currentPos - lastPos).x()), static_cast<float>((currentPos - lastPos).y())};
            motion.x = (motion.x / m_width);
            motion.y = (motion.y) / m_height;
            if(currentBtns & Qt::LeftButton){
                m_camera.rotateAroundTarget(motion);
            }
            if(currentBtns & Qt::RightButton){
                m_camera.rotateAroundTarget(motion);
            }
        }
        lastPos = currentPos;
    }
    if(ratio != 0){
        m_camera.cloaseToTarget(ratio);
        ratio = 0;
    }
}

void RenderWidget::resetCamera()
{
    ui->FPSLabel->setVisible(true);
    m_camera.setCamera(m_model->m_centre, m_model->getYRange());
    //m_camera.setCamera(Vector3D(0.f, 0.f, -1.f), m_model->getYRange());
}
