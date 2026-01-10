#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QTimer>
#include <memory>
#include <QDebug>
#include <algorithm>

#include "MapWidget.h"
#include "MapData.h"
#include "ConfigLoader.h"

// 辅助函数：创建带标题的容器
QWidget* createGroupedWidget(const QString& title, QWidget* contentWidget) {
    QGroupBox* groupBox = new QGroupBox(title);
    QVBoxLayout* layout = new QVBoxLayout(groupBox);
    layout->setContentsMargins(2, 10, 2, 2);
    layout->addWidget(contentWidget);
    return groupBox;
}

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    // 1. 加载配置
    // 确保 config.json 在运行目录下
    AppConfig config = ConfigLoader::load("config.json");

    // 2. 初始化数据
    auto sharedData = std::make_shared<MapData>();
    // 注入配置到 MapData
    sharedData->setConfig(config);

    // 加载地图
    bool loaded = sharedData->load(config.map.netFilePath);

    // 【新增】初始化数据回放路径
    // 注意：Windows路径中的反斜杠需要转义，或者使用正斜杠
    sharedData->initPlayback("D:/2025data/oop/Json");

    QRectF bounds = sharedData->bounds();
    float cx = 0.0f;
    float cy = 0.0f;
    float mapH = 100.0f;

    if (loaded) {
        cx = bounds.center().x();
        cy = bounds.center().y();
        mapH = bounds.height();
    }

    // 3. 设置主窗口
    QMainWindow window;
    window.resize(config.window.width, config.window.height);
    window.setWindowTitle(config.window.title);

    QWidget* centralWidget = new QWidget();
    window.setCentralWidget(centralWidget);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

    // ==========================================
    // 左侧：主地图 (Main View) - 真实风格
    // ==========================================
    MapWidget* leftMap = new MapWidget();

    // 【关键设置】启用真实渲染风格 (需要 asphalt.jpg)
    leftMap->setStyle(MapWidget::STYLE_REALISTIC);

    leftMap->setConfig(config); // 注入配置
    leftMap->setData(sharedData); // 注入数据

    // 自动计算缩放，使地图垂直充满屏幕
    float mainMapScale = (float)window.height() / std::max(1.0f, mapH) * 0.95f;
    leftMap->setFocus(cx, cy, mainMapScale);

    mainLayout->addWidget(leftMap, 1);

    // ==========================================
    // 右侧：仪表盘
    // ==========================================
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);

    // --- 右上：全局横向视图 (Global Scene) - 扁平风格 ---
    MapWidget* globalSceneMap = new MapWidget();

    // 【关键设置】保持扁平风格
    globalSceneMap->setStyle(MapWidget::STYLE_FLAT);

    globalSceneMap->setConfig(config);
    globalSceneMap->setData(sharedData);
    globalSceneMap->setRotation(-90.0f); // 旋转 -90 度

    // 计算缩放，适配右侧面板宽度
    float globalScale = 600.0f / std::max(1.0f, mapH) * 0.9f;
    globalSceneMap->setFocus(cx, cy, globalScale);

    rightLayout->addWidget(createGroupedWidget("Global Image Scene (Rotated -90)", globalSceneMap), 2);

    // --- 中间：控制栏 (Control Panel) ---
    QGroupBox* controlBox = new QGroupBox("Control Panel");
    controlBox->setFixedHeight(80);
    QHBoxLayout* ctrlLayout = new QHBoxLayout(controlBox);

    // 1. FPS 设置
    QLabel* labelRender = new QLabel("Render Scene @");
    QSpinBox* spinFPS = new QSpinBox();
    spinFPS->setRange(1, 60);
    spinFPS->setValue(config.sim.targetFPS);
    spinFPS->setFixedWidth(50);
    QLabel* labelFPS = new QLabel("FPS");

    // 2. 按钮
    QPushButton* startBtn = new QPushButton("Start");
    QPushButton* stopBtn = new QPushButton("Stop");

    // 3. 文件选项
    QCheckBox* checkToFiles = new QCheckBox("to Files");
    QPushButton* filesBtn = new QPushButton("Files");

    ctrlLayout->addWidget(labelRender);
    ctrlLayout->addWidget(spinFPS);
    ctrlLayout->addWidget(labelFPS);
    ctrlLayout->addSpacing(20);
    ctrlLayout->addWidget(startBtn);
    ctrlLayout->addWidget(stopBtn);
    ctrlLayout->addSpacing(20);
    ctrlLayout->addWidget(checkToFiles);
    ctrlLayout->addWidget(filesBtn);
    ctrlLayout->addStretch();

    rightLayout->addWidget(controlBox, 0);

    // --- 右下：分区域监控 (Local Scene) - 扁平风格 ---
    QWidget* dataZone = new QWidget();
    QHBoxLayout* dataLayout = new QHBoxLayout(dataZone);

    QTextEdit* globalText = new QTextEdit();
    globalText->setText("System Ready... Loading data from D:/2025data/oop/Json");
    dataLayout->addWidget(createGroupedWidget("Global Text Scene", globalText), 1);

    QWidget* localZone = new QWidget();
    QVBoxLayout* localLayout = new QVBoxLayout(localZone);

    MapWidget* localMap = new MapWidget();

    // 【关键设置】保持扁平风格
    localMap->setStyle(MapWidget::STYLE_FLAT);

    localMap->setConfig(config);
    localMap->setData(sharedData);

    // 使用配置中的默认缩放等级
    float junctionZoomLevel = config.map.defaultLocalZoom;

    // 默认显示 J2 路口
    QPointF startPt = sharedData->getJunctionPosition("J2");
    localMap->setFocus(startPt.x(), startPt.y(), junctionZoomLevel);

    localLayout->addWidget(createGroupedWidget("Local Image Scene", localMap), 1);

    // 按钮组
    QGroupBox* btnGroup = new QGroupBox("Select Junction Region");
    btnGroup->setFixedHeight(60);
    QHBoxLayout* btnLayout = new QHBoxLayout(btnGroup);

    QPushButton* btnRegion1 = new QPushButton("1/4"); // J2
    QPushButton* btnRegion2 = new QPushButton("2/4"); // J11
    QPushButton* btnRegion3 = new QPushButton("3/4"); // J9
    QPushButton* btnRegion4 = new QPushButton("4/4"); // J1

    btnLayout->addWidget(btnRegion1);
    btnLayout->addWidget(btnRegion2);
    btnLayout->addWidget(btnRegion3);
    btnLayout->addWidget(btnRegion4);

    localLayout->addWidget(btnGroup);
    dataLayout->addWidget(localZone, 1);
    rightLayout->addWidget(dataZone, 3);

    // --- 底部 ---
    QGroupBox* chatBox = new QGroupBox("Talk to RDM");
    chatBox->setFixedHeight(50);
    rightLayout->addWidget(chatBox);

    mainLayout->addWidget(rightPanel, 1);

    // ==========================================
    // 逻辑连接
    // ==========================================

    QTimer* simTimer = new QTimer(&window);

    // 初始化 FPS
    int initialInterval = 1000 / spinFPS->value();
    simTimer->setInterval(initialInterval);

    // FPS 动态调整
    QObject::connect(spinFPS, &QSpinBox::valueChanged, [=](int val) {
        if (val > 0) simTimer->setInterval(1000 / val);
        });

    // 仿真循环
    QObject::connect(simTimer, &QTimer::timeout, [&]() {
        sharedData->updateSimulationStep();
        // 刷新所有视图
        leftMap->update();
        globalSceneMap->update();
        localMap->update();
        });

    QObject::connect(startBtn, &QPushButton::clicked, [=]() { simTimer->start(); });
    QObject::connect(stopBtn, &QPushButton::clicked, [=]() { simTimer->stop(); });

    // 路口跳转逻辑
    auto jumpToJunction = [=](const QString& juncId) {
        QPointF pt = sharedData->getJunctionPosition(juncId);
        // 如果找不到路口，防止跳到 (0,0)
        if (pt.isNull() && pt.x() == 0 && pt.y() == 0) pt.setX(cx);

        localMap->setFocus(pt.x(), pt.y(), junctionZoomLevel);
        qDebug() << "Jump to" << juncId << "at" << pt;
        };

    QObject::connect(btnRegion1, &QPushButton::clicked, [=]() { jumpToJunction("J2"); });
    QObject::connect(btnRegion2, &QPushButton::clicked, [=]() { jumpToJunction("J11"); });
    QObject::connect(btnRegion3, &QPushButton::clicked, [=]() { jumpToJunction("J9"); });
    QObject::connect(btnRegion4, &QPushButton::clicked, [=]() { jumpToJunction("J1"); });

    window.show();
    return a.exec();
}