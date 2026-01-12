#pragma once

#include <QtWidgets/QMainWindow>
#include <memory>
#include "ui_QtWidgetsApplication1.h"
#include "MapData.h"
#include "MapWidget.h"
#include "Config.h"

// 前置声明，减少头文件依赖
class QTimer;
class QLabel;
class QGroupBox;
class QPushButton;
class ControlPanel; 
class InfoPanel;    

class QtWidgetsApplication1 : public QMainWindow
{
    Q_OBJECT

public:
    QtWidgetsApplication1(QWidget* parent = nullptr);
    ~QtWidgetsApplication1();

private slots:
    // 槽函数：处理定时器和交互逻辑
    void onTimerTimeout();
    void onStartClicked();
    void onStopClicked();
    void onRestartClicked();
    void onJumpToJunction(const QString& juncId);
    void updateSelectedVehicleInfo();

private:
    // 辅助函数
    QWidget* createGroupedWidget(const QString& title, QWidget* contentWidget);

    // 初始化流程
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

    // --- 视图组件 ---
    MapWidget* m_leftMap = nullptr;       // 主地图
    MapWidget* m_globalSceneMap = nullptr;// 全局鹰眼图
    MapWidget* m_localMap = nullptr;      // 局部路口图

    // --- UI 模块 (替换掉了原来的散乱控件) ---
    ControlPanel* m_ctrlPanel = nullptr;  // 底部控制面板
    InfoPanel* m_infoPanel = nullptr;     // 右侧信息面板

    // 2D/3D 切换按钮 
    QPushButton* m_btnToggleView = nullptr;
    //set view
    QPushButton* m_btnSetView = nullptr;

    // 布局参数
    float m_mapHeight = 100.0f;
    QPointF m_centerPos;
};