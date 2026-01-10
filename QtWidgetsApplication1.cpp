#include "QtWidgetsApplication1.h"
#include "ConfigLoader.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QTimer>
#include <QDebug>

QtWidgetsApplication1::QtWidgetsApplication1(QWidget* parent)
    : QMainWindow(parent)
{
    // ui.setupUi(this); // 如果不使用 .ui 文件设计界面，这行可以注释掉，或者保留也不影响

    // 按顺序初始化
    initConfig();
    initData();
    initUI();
    initConnections();
}

QtWidgetsApplication1::~QtWidgetsApplication1()
{
}

void QtWidgetsApplication1::initConfig() {
    // 确保 config.json 在运行目录下
    m_config = ConfigLoader::load("config.json");

    // 设置窗口基本属性
    resize(m_config.window.width, m_config.window.height);
    setWindowTitle(m_config.window.title);
}

void QtWidgetsApplication1::initData() {
    m_sharedData = std::make_shared<MapData>();
    m_sharedData->setConfig(m_config);

    // 加载地图
    bool loaded = m_sharedData->load(m_config.map.netFilePath);

    // 初始化回放路径
    m_sharedData->initPlayback("D:/2025data/oop/Json");

    QRectF bounds = m_sharedData->bounds();
    if (loaded) {
        m_centerPos = bounds.center();
        m_mapHeight = bounds.height();
    }
    else {
        m_centerPos = QPointF(0, 0);
    }
}

void QtWidgetsApplication1::initUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

    // ==========================================
    // 左侧：主地图 (Main View)
    // ==========================================
    m_leftMap = new MapWidget(this);
    m_leftMap->setStyle(MapWidget::STYLE_REALISTIC); // 真实风格
    m_leftMap->setConfig(m_config);
    m_leftMap->setData(m_sharedData);

    // 自动缩放
    float mainMapScale = (float)height() / std::max(1.0f, m_mapHeight) * 0.95f;
    m_leftMap->setFocus(m_centerPos.x(), m_centerPos.y(), mainMapScale);

    mainLayout->addWidget(m_leftMap, 1);

    // ==========================================
    // 右侧：仪表盘
    // ==========================================
    QWidget* rightPanel = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);

    // --- 右上：全局视图 ---
    m_globalSceneMap = new MapWidget(this);
    m_globalSceneMap->setStyle(MapWidget::STYLE_FLAT);
    m_globalSceneMap->setConfig(m_config);
    m_globalSceneMap->setData(m_sharedData);
    m_globalSceneMap->setRotation(-90.0f);

    float globalScale = 600.0f / std::max(1.0f, m_mapHeight) * 0.9f;
    m_globalSceneMap->setFocus(m_centerPos.x(), m_centerPos.y(), globalScale);

    rightLayout->addWidget(createGroupedWidget("Global Image Scene (Rotated -90)", m_globalSceneMap), 2);

    // --- 中间：控制栏 ---
    QGroupBox* controlBox = new QGroupBox("Control Panel");
    controlBox->setFixedHeight(80);
    QHBoxLayout* ctrlLayout = new QHBoxLayout(controlBox);

    m_spinFPS = new QSpinBox();
    m_spinFPS->setRange(1, 60);
    m_spinFPS->setValue(m_config.sim.targetFPS);
    m_spinFPS->setFixedWidth(100);

    QPushButton* startBtn = new QPushButton("Start");
    QPushButton* stopBtn = new QPushButton("Stop");

    // 连接控制按钮
    connect(startBtn, &QPushButton::clicked, this, &QtWidgetsApplication1::onStartClicked);
    connect(stopBtn, &QPushButton::clicked, this, &QtWidgetsApplication1::onStopClicked);

    ctrlLayout->addWidget(new QLabel("Render Scene @"));
    ctrlLayout->addWidget(m_spinFPS);
    ctrlLayout->addWidget(new QLabel("FPS"));
    ctrlLayout->addSpacing(20);
    ctrlLayout->addWidget(startBtn);
    ctrlLayout->addWidget(stopBtn);
    ctrlLayout->addStretch(); // 简化了一些不用的按钮布局

    rightLayout->addWidget(controlBox, 0);

    // --- 右下：局部视图 ---
    QWidget* dataZone = new QWidget();
    QHBoxLayout* dataLayout = new QHBoxLayout(dataZone);

    QTextEdit* globalText = new QTextEdit();
    globalText->setText("System Ready... Loading data from D:/2025data/oop/Json");
    dataLayout->addWidget(createGroupedWidget("Global Text Scene", globalText), 1);

    QWidget* localZone = new QWidget();
    QVBoxLayout* localLayout = new QVBoxLayout(localZone);

    m_localMap = new MapWidget(this);
    m_localMap->setStyle(MapWidget::STYLE_FLAT);
    m_localMap->setConfig(m_config);
    m_localMap->setData(m_sharedData);

    // 默认显示 J2
    QPointF startPt = m_sharedData->getJunctionPosition("J2");
    m_localMap->setFocus(startPt.x(), startPt.y(), m_config.map.defaultLocalZoom);

    localLayout->addWidget(createGroupedWidget("Local Image Scene", m_localMap), 1);

    // 路口选择按钮
    QGroupBox* btnGroup = new QGroupBox("Select Junction Region");
    btnGroup->setFixedHeight(60);
    QHBoxLayout* btnLayout = new QHBoxLayout(btnGroup);

    QPushButton* btnRegion1 = new QPushButton("1/4"); connect(btnRegion1, &QPushButton::clicked, [=]() { onJumpToJunction("J2"); });
    QPushButton* btnRegion2 = new QPushButton("2/4"); connect(btnRegion2, &QPushButton::clicked, [=]() { onJumpToJunction("J11"); });
    QPushButton* btnRegion3 = new QPushButton("3/4"); connect(btnRegion3, &QPushButton::clicked, [=]() { onJumpToJunction("J9"); });
    QPushButton* btnRegion4 = new QPushButton("4/4"); connect(btnRegion4, &QPushButton::clicked, [=]() { onJumpToJunction("J1"); });

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
    int initialInterval = 1000 / m_spinFPS->value();
    m_simTimer->setInterval(initialInterval);

    // 连接定时器到槽函数
    connect(m_simTimer, &QTimer::timeout, this, &QtWidgetsApplication1::onTimerTimeout);

    // FPS 动态调整
    connect(m_spinFPS, &QSpinBox::valueChanged, [=](int val) {
        if (val > 0) m_simTimer->setInterval(1000 / val);
        });
}

// 槽函数实现
void QtWidgetsApplication1::onTimerTimeout() {
    m_sharedData->updateSimulationStep();
    // 刷新所有视图
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

void QtWidgetsApplication1::onJumpToJunction(const QString& juncId) {
    if (!m_localMap) return;
    QPointF pt = m_sharedData->getJunctionPosition(juncId);
    if (pt.isNull() && pt.x() == 0 && pt.y() == 0) pt = m_centerPos;

    m_localMap->setFocus(pt.x(), pt.y(), m_config.map.defaultLocalZoom);
    qDebug() << "Jump to" << juncId << "at" << pt;
}

QWidget* QtWidgetsApplication1::createGroupedWidget(const QString& title, QWidget* contentWidget) {
    QGroupBox* groupBox = new QGroupBox(title);
    QVBoxLayout* layout = new QVBoxLayout(groupBox);
    layout->setContentsMargins(2, 10, 2, 2);
    layout->addWidget(contentWidget);
    return groupBox;
}