#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QDateTime>
#include <QComboBox>

#include "log_watcher.h"
#include "slot_tab_controller.h"

namespace Ui {
class MainWindow;
}

struct SlotCategory {
    QString label;
    QStringList items;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_startButton_clicked();
    void on_pauseButton_clicked();
    void on_stopButton_clicked();
    void on_swapButton_clicked();
    void on_dirSelectButton_clicked();
    void on_fileSelectButton_clicked();
    void on_editSlotListButton_clicked();
    void on_historyLoadButton_clicked();

private:
    int extractNumber(const QString& line);
    QList<SlotCategory> loadSlotList(const QString& path);
    void populateSlotComboBox(QComboBox* comboBox, const QList<SlotCategory>& categories);

    Ui::MainWindow *ui;

    QDateTime startTime_;
    QString logFilePath_;
    QFile *logFile_;
    bool isPaused_ = false;

    InfoWidget* infoSlot_ = nullptr;
    InfoWidget* infoHistory_ = nullptr;
    LogWatcher* watcher_ = nullptr;
    SlotTabController* controller_ = nullptr;
};

#endif // MAINWINDOW_H
