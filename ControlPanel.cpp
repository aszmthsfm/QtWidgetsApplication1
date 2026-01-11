#include "ControlPanel.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>

ControlPanel::ControlPanel(int defaultFps, QWidget* parent) : QWidget(parent) {
    // 使用垂直布局包含一个 GroupBox
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QGroupBox* group = new QGroupBox("Control Panel");
    group->setFixedHeight(80);
    QHBoxLayout* layout = new QHBoxLayout(group);

    // 1. FPS 控件
    layout->addWidget(new QLabel("Render Scene @"));
    m_spinFPS = new QSpinBox();
    m_spinFPS->setRange(1, 60);
    m_spinFPS->setValue(defaultFps);
    m_spinFPS->setFixedWidth(100);
    layout->addWidget(m_spinFPS);
    layout->addWidget(new QLabel("FPS"));

    layout->addSpacing(20);

    // 2. 按钮
    m_btnStart = new QPushButton("Start");
    m_btnStop = new QPushButton("Stop");
    m_btnRestart = new QPushButton("Restart");

    layout->addWidget(m_btnStart);
    layout->addWidget(m_btnStop);
    layout->addWidget(m_btnRestart);
    layout->addStretch();

    mainLayout->addWidget(group);

    // 3. 内部信号转发
    // 当按钮被点击时，发射本类的信号
    connect(m_btnStart, &QPushButton::clicked, this, &ControlPanel::startRequested);
    connect(m_btnStop, &QPushButton::clicked, this, &ControlPanel::stopRequested);
    connect(m_btnRestart, &QPushButton::clicked, this, &ControlPanel::restartRequested);
    connect(m_spinFPS, &QSpinBox::valueChanged, this, &ControlPanel::fpsChanged);
}