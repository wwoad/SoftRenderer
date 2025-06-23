#include "Widget.h"
#include "RenderWidget.h"
#include "ui_Widget.h"


Widget::Widget(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    initUi();
    initSignalAndSlot();
}

Widget::~Widget()
{
    delete ui;
}

void Widget::setOption(Option option, bool val)
{
    if(option == Option::MUTITHREAD){
        ui->actionMultithread->setChecked(val);
        ui->renderWidget->setMultiThread(val);
    }
    else if(option == Option::FACECULLING){
        ui->actionFaceCulling->setChecked(val);
        ui->renderWidget->setFaceCulling(val);
    }
    else if(option == Option::SIMD){
        ui->actionSIMD->setChecked(val);
        ui->renderWidget->setSIMD(val);
    }
    else{
        return;
    }
}

void Widget::setCameraPara(CameraPara para, float val)
{
    if(para == CameraPara::FOV){
        ui->Fovlabel_val->setText(QString::number(static_cast<int>(val)));
        ui->renderWidget->m_camera.m_fov = val;
    }
    else if(para == CameraPara::NEAR){
        ui->Nearlabel_val->setText(QString::number((static_cast<int>(val))));
        ui->renderWidget->m_camera.m_zNear = val;
    }
    else{
        return;
    }
}

void Widget::setLightColor(LightColorType type, QColor color)
{
    switch (type) {
    case LightColorType::SPECULAR:
        m_specularColor = color;
        break;
    case LightColorType::DIFFUSE:
        m_diffuseColor = color;
        break;
    case LightColorType::AMBIENT:
        m_ambientColor = color;
        break;
    default:
        break;
    }
    ui->renderWidget->setLightColor({color.red() / 255.f,
                                     color.green() / 255.f,
                                     color.blue() / 255.f}, type);
}

void Widget::setLightDir()
{
    Vector3D lightDir;
    float pitch = glm::radians(glm::clamp(static_cast<float>(ui->PitchSlider->value()), - 89.9f, 89.9f));
    float yaw = - glm::radians(static_cast<float>(ui->Yawdial->value()));

    lightDir.x = (1.f * static_cast<float>(std::cos(pitch)) * static_cast<float>(std::sin(yaw)));
    lightDir.y = (1.f * static_cast<float>(std::sin(pitch)));
    lightDir.z = (1.f * static_cast<float>(std::cos(pitch)) * static_cast<float>(std::cos(yaw)));

    ui->renderWidget->setLightDir(Vector4D(-lightDir, 0.f));
}

void Widget::initUi()
{
    setFixedSize(WIDGETWIDTH, WIDGETHEIGHT);
    setOption(Option::MUTITHREAD, true);
    setOption(Option::FACECULLING, true);
    setOption(Option::SIMD, true);
    setCameraPara(CameraPara::FOV, 60.f);
    setCameraPara(CameraPara::NEAR, 1.f);
    setLightColor(LightColorType::SPECULAR, QColor(255, 255, 255));
    setLightColor(LightColorType::DIFFUSE, QColor(153, 153, 153));
    setLightColor(LightColorType::AMBIENT, QColor(102, 102, 102));
    setLightDir();
}

void Widget::initSignalAndSlot()
{
    connect(ui->renderWidget, &RenderWidget::sendModelData, this,
            [this](int triangleCount, int vertexCount){
                ui->TriangleNumber->setText(QString::number(triangleCount));
                ui->VertexNumber->setText(QString::number(vertexCount));
            });
    ui->MeshcheckBox->checkState();
}

void Widget::on_actionopen_file_triggered()
{
    ui->renderWidget->togglePause();
    QString modelFilelPath = QFileDialog::getOpenFileName(this, "Open a Model File", "",
                                                          "OBJ(*.obj);;MTL(*.mtl)");
    if(!modelFilelPath.isEmpty()){
        std::cout << " loading model" << std::endl;
        ui->renderWidget->loadmodel(modelFilelPath);
    }

    else{
        return;
    }
}


void Widget::on_actionsave_image_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this, "save Image", "", "PNG(*.png)");
    if(!fileName.isEmpty()){
        ui->renderWidget->saveImage(fileName);
    }
    else{
        return;
    }
}


void Widget::on_MeshcheckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    if(ui->MeshcheckBox->isChecked()){
        ui->renderWidget->setRenderMode(RendererMode::Mesh);
        ui->VertexCheckBox->setChecked(false);
        ui->RasterizationCheckBox->setChecked(false);
    }
}

void Widget::on_VertexCheckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    if(ui->VertexCheckBox->isChecked()){
        ui->renderWidget->setRenderMode(RendererMode::VERTEX);
        ui->RasterizationCheckBox->setChecked(false);
        ui->MeshcheckBox->setChecked(false);
    }
}

void Widget::on_RasterizationCheckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    if(ui->RasterizationCheckBox->isChecked()){
        ui->renderWidget->setRenderMode(RendererMode::Rasterization);
        ui->MeshcheckBox->setChecked(false);
        ui->VertexCheckBox->setChecked(false);
    }
}


void Widget::on_FovSlider_valueChanged(int value)
{
    setCameraPara(CameraPara::FOV, static_cast<float>(value));
}


void Widget::on_NearSlider_valueChanged(int value)
{
    setCameraPara(CameraPara::NEAR, static_cast<float>(value));
}


void Widget::on_PitchSlider_valueChanged(int value)
{
    setLightDir();
}


void Widget::on_Yawdial_valueChanged(int value)
{
    setLightDir();
}

void Widget::on_actionMultithread_triggered()
{
    if(ui->actionMultithread->isChecked()){
        ui->renderWidget->setMultiThread(true);
    }
    else{
        ui->renderWidget->setMultiThread(false);
    }
}

void Widget::on_actionFaceCulling_triggered()
{
    if(ui->actionFaceCulling->isChecked()){
        ui->renderWidget->setFaceCulling(true);
    }
    else{
        ui->renderWidget->setFaceCulling(false);
    }
}

void Widget::on_actionSIMD_triggered()
{
    if(ui->actionSIMD->isChecked()){
        ui->renderWidget->setSIMD(true);
    }
    else{
        ui->renderWidget->setSIMD(false);
    }
}

