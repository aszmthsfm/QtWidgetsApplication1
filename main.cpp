#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QCheckBox> // 新增：用于 to Files 复选框
#include <QTextEdit>
#include <QTimer>
#include <memory>
#include <QDebug>
#include <algorithm> // 用于 std::max

#include "MapWidget.h"
#include "MapData.h"

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

    // 1. 加载数据
    auto sharedData = std::make_shared<MapData>();
    bool loaded = sharedData->load("road.net.xml");

    QRectF bounds = sharedData->bounds();
    float cx = 0.0f;
    float cy = 0.0f;
    float mapH = 100.0f;

    if (loaded) {
        cx = bounds.center().x();
        cy = bounds.center().y();
        mapH = bounds.height();
    }

    QMainWindow window;
    window.resize(1600, 900);
    window.setWindowTitle("SUMO Dashboard (Qt6 + OpenGL)");

    QWidget* centralWidget = new QWidget();
    window.setCentralWidget(centralWidget);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

    // ==========================================
    // 左侧：主地图 (Main View)
    // ==========================================
    MapWidget* leftMap = new MapWidget();
    leftMap->setData(sharedData);

    // 【修改 1】主地图放大逻辑
    // 使用窗口高度 (900) 计算比例，使地图在垂直方向撑满
    float mainMapScale = (float)window.height() / std::max(1.0f, mapH) * 0.95f;
    leftMap->setFocus(cx, cy, mainMapScale);

    mainLayout->addWidget(leftMap, 1);

    // ==========================================
    // 右侧：仪表盘
    // ==========================================
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);

    // --- 右上：全局横向视图 (Global Scene) ---
    MapWidget* globalSceneMap = new MapWidget();
    globalSceneMap->setData(sharedData);
    globalSceneMap->setRotation(-90.0f); // 旋转 -90 度

    // 【修改 2】Global Map 缩放适配
    // 旋转后，地图高度变为屏幕宽度。预估右侧面板宽约 600px
    float globalScale = 600.0f / std::max(1.0f, mapH) * 0.9f;
    globalSceneMap->setFocus(cx, cy, globalScale);

    rightLayout->addWidget(createGroupedWidget("Global Image Scene (Rotated -90)", globalSceneMap), 2);

    // --- 中间：控制栏 (Control Panel) ---
    // 【修改 3】完全重构 Control Panel 布局，仿照参考图
    QGroupBox* controlBox = new QGroupBox("Control Panel");
    controlBox->setFixedHeight(80);
    QHBoxLayout* ctrlLayout = new QHBoxLayout(controlBox);

    // 1. 左侧：Render Scene @ [ 5 ] FPS
    QLabel* labelRender = new QLabel("Render Scene @");
    QSpinBox* spinFPS = new QSpinBox();
    spinFPS->setRange(1, 60);
    spinFPS->setValue(5);      // 默认 5 FPS
    spinFPS->setFixedWidth(50);
    QLabel* labelFPS = new QLabel("FPS");

    // 2. 中间：Start / Stop
    QPushButton* startBtn = new QPushButton("Start");
    QPushButton* stopBtn = new QPushButton("Stop");

    // 3. 右侧：[ ] to Files / Files
    QCheckBox* checkToFiles = new QCheckBox("to Files");
    QPushButton* filesBtn = new QPushButton("Files");

    // 添加控件到布局
    ctrlLayout->addWidget(labelRender);
    ctrlLayout->addWidget(spinFPS);
    ctrlLayout->addWidget(labelFPS);

    ctrlLayout->addSpacing(20); // 间距
    ctrlLayout->addWidget(startBtn);
    ctrlLayout->addWidget(stopBtn);

    ctrlLayout->addSpacing(20); // 间距
    ctrlLayout->addWidget(checkToFiles);
    ctrlLayout->addWidget(filesBtn);

    ctrlLayout->addStretch(); // 靠左对齐

    rightLayout->addWidget(controlBox, 0);

    // --- 右下：分区域监控 (Local Scene) ---
    QWidget* dataZone = new QWidget();
    QHBoxLayout* dataLayout = new QHBoxLayout(dataZone);

    QTextEdit* globalText = new QTextEdit();
    globalText->setText("System Ready...");
    dataLayout->addWidget(createGroupedWidget("Global Text Scene", globalText), 1);

    QWidget* localZone = new QWidget();
    QVBoxLayout* localLayout = new QVBoxLayout(localZone);

    MapWidget* localMap = new MapWidget();
    localMap->setData(sharedData);

    // 【修改 4】局部路口镜头调整
    // 缩放设为 5.0f (拉远镜头)，以便看清整个路口
    float junctionZoomLevel = 5.0f;

    // 默认显示第一个路口 J2
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

    // 1. 车辆仿真定时器
    QTimer* simTimer = new QTimer(&window);

    // 初始化定时器间隔 (1000ms / FPS)
    int initialInterval = 1000 / spinFPS->value();
    simTimer->setInterval(initialInterval);

    // 【修改 5】FPS 动态调整逻辑
    QObject::connect(spinFPS, &QSpinBox::valueChanged, [=](int val) {
        if (val > 0) {
            simTimer->setInterval(1000 / val);
        }
        });

    QObject::connect(simTimer, &QTimer::timeout, [&]() {
        sharedData->updateSimulationStep();
        // 刷新所有视图
        leftMap->update();
        globalSceneMap->update();
        localMap->update();
        });

    QObject::connect(startBtn, &QPushButton::clicked, [=]() { simTimer->start(); });
    QObject::connect(stopBtn, &QPushButton::clicked, [=]() { simTimer->stop(); });

    // 2. 按钮跳转逻辑 (使用修正后的缩放比例)
    auto jumpToJunction = [=](const QString& juncId) {
        QPointF pt = sharedData->getJunctionPosition(juncId);
        // 如果数据未加载或找不到ID，避免跳到 (0,0)
        if (pt.isNull() && pt.x() == 0 && pt.y() == 0) pt.setX(cx); // 保持在 X 中心

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