#include "QtWidgetsApplication1.h"
#include "ConfigLoader.h"
#include "RoadNetworkLoader.h" 
#include "ControlPanel.h"
#include "InfoPanel.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QDebug>

QtWidgetsApplication1::QtWidgetsApplication1(QWidget* parent)
    : QMainWindow(parent)
{
    // ui.setupUi(this); 

    initConfig();
    initData();
    initUI();
    initConnections();
}

QtWidgetsApplication1::~QtWidgetsApplication1()
{
}

void QtWidgetsApplication1::initConfig() {
    m_config = ConfigLoader::load("config.json");
    resize(m_config.window.width, m_config.window.height);
    setWindowTitle(m_config.window.title);
}

void QtWidgetsApplication1::initData() {
    m_sharedData = std::make_shared<MapData>();
    m_sharedData->setConfig(m_config);

    // 【核心修改】使用 RoadNetworkLoader 加载数据
    // 不再调用 m_sharedData->load()
    bool loaded = RoadNetworkLoader::load(m_config.map.netFilePath, m_sharedData);

    // 回放数据初始化保持不变 (仍然在 MapData 中)
    m_sharedData->initPlayback("D:/2025data/oop/Json"); // 请确保路径正确

    QRectF bounds = m_sharedData->bounds();
    if (loaded) {
        m_centerPos = bounds.center();
        m_mapHeight = bounds.height();
    }
    else {
        m_centerPos = QPointF(0, 0);
    }
}

void QtWidgetsApplication1::updateSelectedVehicleInfo() {
    if (!m_sharedData || !m_infoPanel) return;

    QString selId = m_sharedData->getSelectedVehicleId();
    if (selId.isEmpty()) {
        m_infoPanel->updateInfo("No vehicle selected.");
        return;
    }

    const Vehicle* veh = m_sharedData->getVehicle(selId);
    if (veh) {
        // 动态生成最新信息
        QString info = QString("Vehicle Selected:\n"
            "-----------------\n"
            "ID: %1\n"
            "Pos: (%2, %3)\n"
            "Speed: %4 m/s\n"
            "Angle: %5 deg\n"
            "Road ID: %6\n"
            "Lane Idx: %7")
            .arg(veh->id)
            .arg(QString::number(veh->x, 'f', 2))
            .arg(QString::number(veh->y, 'f', 2))
            .arg(QString::number(veh->speed, 'f', 2))
            .arg(QString::number(veh->angle, 'f', 1))
            .arg(veh->currentEdgeId)
            .arg(veh->currentLaneIndex);

        m_infoPanel->updateInfo(info);
    }
    else {
        // 如果车辆跑出了地图消失了
        m_infoPanel->updateInfo("Vehicle " + selId + " left the map.");
    }
}

void QtWidgetsApplication1::initUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

    // ==========================================
    // 左侧：主地图 (View)
    // ==========================================
    m_leftMap = new MapWidget(this);
    m_leftMap->setStyle(MapWidget::STYLE_REALISTIC);
    m_leftMap->setConfig(m_config);
    m_leftMap->setData(m_sharedData);

    // 悬浮在地图上的 2D/3D 切换按钮
    m_btnToggleView = new QPushButton("2D View", m_leftMap);
    m_btnToggleView->setGeometry(20, 20, 80, 30);
    m_btnToggleView->setCheckable(true);
    m_btnToggleView->setChecked(false);
    m_btnToggleView->setStyleSheet(
        "QPushButton { background-color: rgba(255, 255, 255, 220); border: 1px solid #8f8f91; border-radius: 4px; font-weight: bold; color: black; }"
        "QPushButton:checked { background-color: #4a90e2; color: white; }"
        "QPushButton:hover { background-color: white; }"
    );
    m_btnToggleView->show();

    connect(m_btnToggleView, &QPushButton::toggled, [=](bool checked) {
        if (checked) {
            m_btnToggleView->setText("3D View");
            m_leftMap->set3D(true);
        }
        else {
            m_btnToggleView->setText("2D View");
            m_leftMap->set3D(false);
        }
        });


    // --- Set View 按钮 ---
    m_btnSetView = new QPushButton("Set View", m_leftMap);
    m_btnSetView->setGeometry(110, 20, 80, 30);
    m_btnSetView->setStyleSheet(
        "QPushButton { background-color: rgba(255, 255, 255, 220); border: 1px solid #8f8f91; border-radius: 4px; font-weight: bold; color: black; }"
        "QPushButton:hover { background-color: white; }"
        "QPushButton:pressed { background-color: #e0e0e0; }"
    );
    m_btnSetView->show();
    // 连接信号：点击按钮 -> 调用左侧地图的 onResetView
    connect(m_btnSetView, &QPushButton::clicked, m_leftMap, &MapWidget::onResetView);

    float mainMapScale = (float)height() / std::max(1.0f, m_mapHeight) * 0.95f;
    m_leftMap->setFocus(m_centerPos.x(), m_centerPos.y(), mainMapScale);

    mainLayout->addWidget(m_leftMap, 1);

    // ==========================================
    // 右侧布局
    // ==========================================
    QWidget* rightPanel = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);

    // 1. 全局鹰眼图
    m_globalSceneMap = new MapWidget(this);
    m_globalSceneMap->setStyle(MapWidget::STYLE_FLAT);
    m_globalSceneMap->setConfig(m_config);
    m_globalSceneMap->setData(m_sharedData);
    m_globalSceneMap->setRotation(-90.0f);
    float globalScale = 600.0f / std::max(1.0f, m_mapHeight) * 0.9f;
    m_globalSceneMap->setFocus(m_centerPos.x(), m_centerPos.y(), globalScale);

    rightLayout->addWidget(createGroupedWidget("Global Image Scene (Rotated -90)", m_globalSceneMap), 2);

    // 2. 控制面板 (使用新模块)
    m_ctrlPanel = new ControlPanel(m_config.sim.targetFPS, this);

    connect(m_ctrlPanel, &ControlPanel::startRequested, this, &QtWidgetsApplication1::onStartClicked);
    connect(m_ctrlPanel, &ControlPanel::stopRequested, this, &QtWidgetsApplication1::onStopClicked);
    connect(m_ctrlPanel, &ControlPanel::restartRequested, this, &QtWidgetsApplication1::onRestartClicked);
    connect(m_ctrlPanel, &ControlPanel::fpsChanged, [this](int fps) {
        if (m_simTimer && fps > 0) m_simTimer->setInterval(1000 / fps);
        });

    rightLayout->addWidget(m_ctrlPanel, 0);

    // 3. 数据与局部视图区域
    QWidget* dataZone = new QWidget();
    QHBoxLayout* dataLayout = new QHBoxLayout(dataZone);

    // 3.1 信息面板 (使用新模块)
    m_infoPanel = new InfoPanel(this);
    // 连接：地图选中车辆 -> 信息面板更新
    connect(m_leftMap, &MapWidget::selectionChanged, this, &QtWidgetsApplication1::updateSelectedVehicleInfo);

    dataLayout->addWidget(m_infoPanel, 1);

    // 3.2 局部视图
    QWidget* localZone = new QWidget();
    QVBoxLayout* localLayout = new QVBoxLayout(localZone);

    m_localMap = new MapWidget(this);
    m_localMap->setStyle(MapWidget::STYLE_FLAT);
    m_localMap->setConfig(m_config);
    m_localMap->setData(m_sharedData);
    QPointF startPt = m_sharedData->getJunctionPosition("J2");
    m_localMap->setFocus(startPt.x(), startPt.y(), m_config.map.defaultLocalZoom);

    localLayout->addWidget(createGroupedWidget("Local Image Scene", m_localMap), 1);

    // 路口选择按钮组
    QGroupBox* btnGroup = new QGroupBox("Select Junction Region");
    btnGroup->setFixedHeight(60);
    QHBoxLayout* btnLayout = new QHBoxLayout(btnGroup);

    QPushButton* btnRegion1 = new QPushButton("1"); connect(btnRegion1, &QPushButton::clicked, [=]() { onJumpToJunction("J2"); });
    QPushButton* btnRegion2 = new QPushButton("2"); connect(btnRegion2, &QPushButton::clicked, [=]() { onJumpToJunction("J11"); });
    QPushButton* btnRegion3 = new QPushButton("3"); connect(btnRegion3, &QPushButton::clicked, [=]() { onJumpToJunction("J9"); });
    QPushButton* btnRegion4 = new QPushButton("4"); connect(btnRegion4, &QPushButton::clicked, [=]() { onJumpToJunction("J1"); });

    btnLayout->addWidget(btnRegion1);
    btnLayout->addWidget(btnRegion2);
    btnLayout->addWidget(btnRegion3);
    btnLayout->addWidget(btnRegion4);

    localLayout->addWidget(btnGroup);
    dataLayout->addWidget(localZone, 1);
    rightLayout->addWidget(dataZone, 3);

    mainLayout->addWidget(rightPanel, 1);
}

void QtWidgetsApplication1::initConnections() {
    m_simTimer = new QTimer(this);
    int initialInterval = 1000 / m_config.sim.targetFPS;
    m_simTimer->setInterval(initialInterval);

    connect(m_simTimer, &QTimer::timeout, this, &QtWidgetsApplication1::onTimerTimeout);
}

void QtWidgetsApplication1::onTimerTimeout() {
    if (m_sharedData) m_sharedData->updateSimulationStep();
    updateSelectedVehicleInfo();
    if (m_leftMap) m_leftMap->update();
    if (m_globalSceneMap) m_globalSceneMap->update();
    if (m_localMap) m_localMap->update();
}

void QtWidgetsApplication1::onStartClicked() {
    if (m_simTimer) m_simTimer->start();
}

void QtWidgetsApplication1::onStopClicked() {
    if (m_simTimer) m_simTimer->stop();
}

void QtWidgetsApplication1::onRestartClicked() {
    if (m_sharedData) m_sharedData->reset();

    if (m_leftMap) m_leftMap->update();
    if (m_globalSceneMap) m_globalSceneMap->update();
    if (m_localMap) m_localMap->update();

    qDebug() << "Simulation reset.";
}

void QtWidgetsApplication1::onJumpToJunction(const QString& juncId) {
    if (!m_localMap) return;
    QPointF pt = m_sharedData->getJunctionPosition(juncId);
    if (pt.isNull() && pt.x() == 0 && pt.y() == 0) pt = m_centerPos;

    m_localMap->setFocus(pt.x(), pt.y(), m_config.map.defaultLocalZoom);
    qDebug() << "Jump to" << juncId;
}

QWidget* QtWidgetsApplication1::createGroupedWidget(const QString& title, QWidget* contentWidget) {
    QGroupBox* groupBox = new QGroupBox(title);
    QVBoxLayout* layout = new QVBoxLayout(groupBox);
    layout->setContentsMargins(2, 10, 2, 2);
    layout->addWidget(contentWidget);
    return groupBox;
}