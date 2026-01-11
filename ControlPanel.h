#pragma once
#include <QWidget>
#include <QPushButton>
#include <QSpinBox>

class ControlPanel : public QWidget {
    Q_OBJECT
public:
    explicit ControlPanel(int defaultFps, QWidget* parent = nullptr);

signals:
    // 对外发送的信号，不关心谁处理，只管发
    void startRequested();
    void stopRequested();
    void restartRequested();
    void fpsChanged(int newFps);
    // 如果有 2D/3D 切换按钮，也可以放这里，或者单独处理
    // void toggleViewRequested(bool is3D); 

private:
    QSpinBox* m_spinFPS;
    QPushButton* m_btnStart;
    QPushButton* m_btnStop;
    QPushButton* m_btnRestart;
};