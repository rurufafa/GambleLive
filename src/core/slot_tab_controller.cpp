#include <QTextEdit>
#include <QScrollBar>
#include <QMessageBox>

#include "slot_tab_controller.h"
#include "log_watcher.h"

SlotTabController::SlotTabController(LogWatcher* watcher,
                                    InfoWidget* infoWidget,
                                    QTextEdit* logTextEdit,
                                    bool enableSave, 
                                    QFile* logFile, 
                                    QObject* parent)

    : QObject(parent)
    , infoWidget_(infoWidget)
    , logTextEdit_(logTextEdit)
    , enableSave_(enableSave)
    , logFile_(logFile)
{
    connect(watcher, &LogWatcher::newLogLine, this, &SlotTabController::handleNewLogLine);
}

void SlotTabController::handleNewLogLine(const GambleLog& log)
{
    // logWidget: 表示用テキスト構築
    QString text;
    if (!log.time.isEmpty())
        text += log.time + " ";
        
    text += log.content;
    
    logLines_ << log.content;
    while (logLines_.size() > maxLogLines_) {
        logLines_.removeFirst();
    }
    
    // スクロールが最下部か確認
    QScrollBar* scrollBar = logTextEdit_->verticalScrollBar();
    bool atBottom = (scrollBar->value() == scrollBar->maximum());
    
    logTextEdit_->setPlainText(logLines_.join("\n"));
    
    // もともと最下部にいた場合のみ、自動で下にスクロール
    if (atBottom) {
        scrollBar->setValue(scrollBar->maximum());
    }

    // ログ保存（初回のみ open）
    if (enableSave_ && logFile_) {
        if (!logFileOpened_) {
            if (!logFile_->open(QIODevice::Append | QIODevice::Text)) {
                QMessageBox::warning(nullptr, "警告", "ログファイルを開けません。記録できません。");
                enableSave_ = false;
                logFile_ = nullptr;
            } else {
                logFileOpened_ = true;
            }
        }

        if (logFileOpened_) {
            QTextStream out(logFile_);
            out << text << "\n";
            logFile_->flush();
        }
    }

    switch (log.type) {
    case GambleLogType::Payment:
        totalSpent_ += log.amount;
        spinCount_++;
        break;
    case GambleLogType::Gain:
        totalGained_ += log.amount;
        break;
    case GambleLogType::Lose:
        spinCount_++;
        break;
    case GambleLogType::Role:
        roleCount_[log.roleName]++;
        break;
    }

    // infoWidget に統計更新
    infoWidget_->setStats(totalSpent_, totalGained_, spinCount_);
    infoWidget_->updateRoleTable(roleCount_);
}

bool SlotTabController::hasLogs() const {
    return !logLines_.isEmpty();
}

QString SlotTabController::toPlainText() const {
    QString text;
    text += QString("支出: %1\n").arg(totalSpent_);
    text += QString("収入: %1\n").arg(totalGained_);
    text += QString("収支: %1\n").arg(totalGained_ - totalSpent_);
    text += QString("回転数: %1\n").arg(spinCount_);

    text += "\n役情報:\n";

    int totalRole = 0;
    for (const auto count : roleCount_.values())
        totalRole += count;

    for (const auto& role : roleCount_.keys()) {
        int count = roleCount_[role];
        double rate = (totalRole > 0) ? count * 100.0 / totalRole : 0.0;
        text += QString("%1: %2回 (%3%)\n")
            .arg(role)
            .arg(count)
            .arg(QString::number(rate, 'f', 2));
    }

    return text;
}