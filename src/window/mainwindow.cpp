#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QStandardPaths>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "infowidget.h"
#include "config_manager.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // ログディレクトリ未設定ならデフォルトを作成
    QString existing = ConfigManager::instance().get("LogDirectory").toString();
    if (existing.isEmpty()) {
        QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/slotLogs";
        QDir dir(defaultDir);

        if (!dir.exists()) {
            if (!dir.mkpath(".")) {
                QMessageBox::warning(this, "警告", "ログディレクトリの作成に失敗しました。");
            }
        }

        QString absPath = dir.absolutePath();
        ConfigManager::instance().set("LogDirectory", absPath);
        ConfigManager::instance().save();
    }

    // スロットタブ
    infoSlot_ = new InfoWidget(this);
    ui->slotTabLayout->addWidget(infoSlot_);

    // 履歴タブ
    infoHistory_ = new InfoWidget(this);
    ui->historyLayout->addWidget(infoHistory_);

    ui->pathEdit->setText(ConfigManager::instance().get("FilePath").toString());
    ui->chatPrefixEdit->setText(ConfigManager::instance().get("ChatPrefix").toString());
    ui->slotNameEdit->setText("None");
    ui->logDirEdit->setText(ConfigManager::instance().get("LogDirectory").toString());
    ui->saveLogCheckBox->setChecked(ConfigManager::instance().get("EnableLogSave").toBool());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_startButton_clicked() {
    startTime_ = QDateTime::currentDateTime();

    QString path = ui->pathEdit->text();
    QString chatPrefix = ui->chatPrefixEdit->text();
    QString slotName = ui->slotNameEdit->text().trimmed();
    QString logDir = ui->logDirEdit->text().trimmed();
    bool enableSave = ui->saveLogCheckBox->isChecked();

    // ログファイルパスが無効なら警告
    if (!QFile::exists(path)) {
        QMessageBox::warning(this, "警告", "ログファイルパスが無効です。存在するファイルを指定してください。");
        return;
    }

    // 記録有効かつディレクトリまたはスロット名が無効なら警告
    if (enableSave) {
        if (slotName.isEmpty()) {
            QMessageBox::warning(this, "警告", "スロット名が空です。記録できません。");
            return;
        }
        QDir dir(logDir);
        if (!dir.exists()) {
            if (!dir.mkpath(".")) {
                QMessageBox::warning(this, "警告", "ログディレクトリの作成に失敗しました。");
                return;
            }
        }
    }


    // 設定をConfigManagerに保存
    ConfigManager::instance().set("FilePath", path);
    ConfigManager::instance().set("ChatPrefix", chatPrefix); 
    ConfigManager::instance().set("SlotName", slotName);
    ConfigManager::instance().set("LogDirectory", logDir);
    ConfigManager::instance().set("EnableLogSave", enableSave);

    ConfigManager::instance().save();

    // slotlogファイルのパスを作成
    if (enableSave) {
        QString baseName = QString("%1_log_%2.log").arg(slotName, startTime_.toString("yyyyMMdd_HHmmss"));
        logFilePath_ = QDir(logDir).filePath(baseName);
        logFile_ = new QFile(logFilePath_, this); 
    }

    infoSlot_->setSlotName(slotName);

    watcher_ = new LogWatcher(this);
    controller_ = new SlotTabController(
        watcher_, 
        infoSlot_, 
        ui->textLogView, 
        enableSave, 
        logFile_, 
        this
    );

    ui->textLogView->clear();
    infoSlot_->clearStats(); 

    ui->stackedWidget->setCurrentIndex(1);
    watcher_->start();
}

void MainWindow::on_stopButton_clicked() {
    if (ConfigManager::instance().get("EnableLogSave").toBool()
        && controller_ && controller_->hasLogs())  // ログがある場合のみ
    {
        QString slotName = ConfigManager::instance().get("SlotName").toString();
        QString dir = ConfigManager::instance().get("LogDirectory").toString();
        QString baseName = QString("%1_info_%2.log").arg(slotName, startTime_.toString("yyyyMMdd_HHmmss"));
        QString infoPath = QDir(dir).filePath(baseName);

        QFile infoFile(infoPath);
        if (infoFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&infoFile);
            out << controller_->toPlainText(); 
        }
    }

    // 停止処理
    if (watcher_) {
        watcher_->deleteLater();
        watcher_ = nullptr;
    }
    if (controller_) {
        controller_->deleteLater();
        controller_ = nullptr;
    }

    ui->stackedWidget->setCurrentIndex(0); // 設定画面へ
}

void MainWindow::on_dirSelectButton_clicked() {
    // モーダルなファイル選択を開く
    QString dirName = QFileDialog::getExistingDirectory(
        this, 
        tr("フォルダを選択"),
        QString(),
        QFileDialog::ShowDirsOnly
    );

    if (!dirName.isEmpty()) {
        ui->logDirEdit->setText(dirName);
    }
}

void MainWindow::on_fileSelectButton_clicked() {
    // モーダルなファイル選択を開く
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("ログファイルを選択"),
        QString(),  
        tr("ログファイル (*.log);;テキストファイル (*.txt);;すべてのファイル (*)")
    );

    if (!fileName.isEmpty()) {
        ui->pathEdit->setText(fileName);
    }
}

void MainWindow::on_historyLoadButton_clicked() {
    QString slotName = ui->historySlotEdit->text().trimmed();
    QString logDir = ConfigManager::instance().get("LogDirectory").toString();

    QDir dir(logDir);
    QStringList filters = { QString("%1_info_*.log").arg(slotName) };
    QStringList fileList = dir.entryList(filters, QDir::Files, QDir::Name);

    int totalSpent = 0;
    int totalGained = 0;
    int totalSpin = 0;
    QMap<QString, int> totalRoleCounts;

    for (const QString& fileName : fileList) {
        QFile file(dir.filePath(fileName));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();

            if (line.startsWith("支出:")) {
                totalSpent += extractNumber(line);
            } else if (line.startsWith("収入:")) {
                totalGained += extractNumber(line);
            } else if (line.startsWith("回転数:")) {
                totalSpin += extractNumber(line);
            } else if (line.contains(":") && line.contains("回")) {
                QRegularExpression re(R"(^(.+?):\s*(\d+)回)");
                QRegularExpressionMatch m = re.match(line);
                if (m.hasMatch()) {
                    QString role = m.captured(1).trimmed();
                    int count = m.captured(2).toInt();
                    totalRoleCounts[role] += count;
                }
            }
        }
    }
    // infoHistory_ に統計更新
    infoHistory_->setSlotName(slotName);
    infoHistory_->setStats(totalSpent, totalGained, totalSpin);
    infoHistory_->updateRoleTable(totalRoleCounts);
}

int MainWindow::extractNumber(const QString& line) {
    QRegularExpression re(R"((\d+))");
    QRegularExpressionMatch m = re.match(line);
    if (m.hasMatch()) {
        return m.captured(1).toInt();
    }
    return 0;
}