#ifndef SLOT_TAB_CONTROLLER_H
#define SLOT_TAB_CONTROLLER_H

#include <QObject>
#include <QMap>
#include <QFile>

#include "infowidget.h"
#include "log_parser.h"

class QTextEdit;
class LogWatcher;

class SlotTabController : public QObject
{
    Q_OBJECT

public:
    SlotTabController(LogWatcher *watcher, 
        InfoWidget *infoWidget, 
        QTextEdit *logTextEdit, 
        bool enableSave, 
        QFile *logFile, 
        QObject *parent = nullptr);
    void clearLogLine();
    bool hasLogs() const;
    QString toPlainText() const;

private slots:
    void handleNewLogLine(const GambleLog& log);

private:
    InfoWidget *infoWidget_;
    QTextEdit *logTextEdit_;
    bool enableSave_;
    QFile *logFile_;
    
    bool logFileOpened_ = false;
    QStringList logLines_;
    const int maxLogLines_ = 500; 
    
    int totalSpent_ = 0;
    int totalGained_ = 0;
    int spinCount_ = 0;

    QMap<QString, int> roleCount_;  // 役名 → 出現回数
};

#endif // SLOT_TAB_CONTROLLER_H