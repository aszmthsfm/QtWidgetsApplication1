#pragma once

#include <QtWidgets/QMainWindow>
#include <memory>
#include "ui_QtWidgetsApplication1.h"
#include "MapData.h"
#include "MapWidget.h"
#include "Config.h"

class QTimer;
class QLabel;
class QSpinBox;
class QGroupBox;
class QPushButton;

class QtWidgetsApplication1 : public QMainWindow
{
    Q_OBJECT

public:
    QtWidgetsApplication1(QWidget* parent = nullptr);
    ~QtWidgetsApplication1();

private slots:
    // 定义槽函数，处理交互逻辑
    void onTimerTimeout();
    void onStartClicked();
    void onStopClicked();
    void onRestartClicked();
    void onJumpToJunction(const QString& juncId);

private:
    // 辅助函数：创建带标题的容器
    QWidget* createGroupedWidget(const QString& title, QWidget* contentWidget);

    // 初始化各个模块
    void initConfig();
    void initData();
    void initUI();
    void initConnections();

private:
    Ui::QtWidgetsApplication1Class ui;

    // 核心数据
    AppConfig m_config;
    std::shared_ptr<MapData> m_sharedData;
    QTimer* m_simTimer = nullptr;

    // 界面组件指针 (需要交互的组件存为成员变量)
    MapWidget* m_leftMap = nullptr;       // 主地图
    MapWidget* m_globalSceneMap = nullptr;// 右上全局视图
    MapWidget* m_localMap = nullptr;      // 右下局部视图

    QSpinBox* m_spinFPS = nullptr;
    //2/3d切换
    QPushButton* m_btnToggleView = nullptr;

    // 布局相关参数
    float m_mapHeight = 100.0f;
    QPointF m_centerPos;
};