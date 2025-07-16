#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QHBoxLayout>
#include <QDesktopServices>

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
    ui->mainLayout->addWidget(infoSlot_);
    // 履歴タブ
    infoHistory_ = new InfoWidget(this);
    ui->historyLayout->addWidget(infoHistory_);
    
    // スロットリストを読み込み
    QString slotListPath = QCoreApplication::applicationDirPath() + "/slotList.txt";
    auto categories = loadSlotList(slotListPath);
    populateSlotComboBox(ui->slotComboBox, categories);
    
    // 初期選択を "None" に
    int noneIndex = ui->slotComboBox->findText("None");
    if (noneIndex != -1)
    ui->slotComboBox->setCurrentIndex(noneIndex);
    
    ui->pathEdit->setText(ConfigManager::instance().get("FilePath").toString());
    ui->chatPrefixEdit->setText(ConfigManager::instance().get("ChatPrefix").toString());
    ui->logDirEdit->setText(ConfigManager::instance().get("LogDirectory").toString());
    ui->saveLogCheckBox->setChecked(ConfigManager::instance().get("EnableLogSave").toBool());

    ui->pauseButton->setText("一時停止");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_startButton_clicked() {
    startTime_ = QDateTime::currentDateTime();

    QString path = ui->pathEdit->text();
    QString chatPrefix = ui->chatPrefixEdit->text();
    QString slotName = ui->slotComboBox->currentText().trimmed();
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

void MainWindow::on_pauseButton_clicked() {
    if (!watcher_) return;

    if (!isPaused_) {
        watcher_->pause();
        ui->pauseButton->setText("再開");
    } else {
        watcher_->resume();
        ui->pauseButton->setText("一時停止");
    }
    isPaused_ = !isPaused_;
}

void MainWindow::on_stopButton_clicked() {
    // 確認ダイアログ
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "確認",
        "本当に終了しますか？",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply != QMessageBox::Yes)
        return; 

    bool saved = false;

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
            saved = true;
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

    // 成功メッセージ
    if (saved) {
        QMessageBox::information(this, "情報", "統計情報は正常に記録されました。");
    }

}

void MainWindow::on_swapButton_clicked() {
    QWidget *widget1 = infoSlot_;
    QWidget *widget2 = ui->logWidget;

    QHBoxLayout *layout = qobject_cast<QHBoxLayout*>(ui->mainLayout); 
    if (!layout) return;

    // 現在の順序を取得
    int index1 = layout->indexOf(widget1);
    int index2 = layout->indexOf(widget2);

    if (index1 == -1 || index2 == -1) return;

    layout->removeWidget(widget1);
    layout->removeWidget(widget2);

    // 順番を逆に挿入
    if (index1 < index2) {
        layout->insertWidget(index1, widget2);
        layout->insertWidget(index2, widget1);
    } else {
        layout->insertWidget(index2, widget1);
        layout->insertWidget(index1, widget2);
    }
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

void MainWindow::on_editSlotListButton_clicked() {
    QString path = QCoreApplication::applicationDirPath() + "/SlotList.txt";
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
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

QList<SlotCategory> MainWindow::loadSlotList(const QString& path) {
    QList<SlotCategory> result;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return result;

    QTextStream in(&file);
    SlotCategory current{"その他", {}};

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        if (line.startsWith("[") && line.endsWith("]")) {
            if (!current.items.isEmpty()) 
                result.append(current);
            current.label = line.mid(1, line.length() - 2);
            current.items.clear();
        } else {
            current.items << line;
        }
    }
    if (!current.items.isEmpty()) 
        result.append(current);
    return result;
}

void MainWindow::populateSlotComboBox(QComboBox* comboBox, const QList<SlotCategory>& categories) {
    auto* model = new QStandardItemModel(comboBox);

    for (const SlotCategory& category : categories) {
        QStandardItem* headerItem = new QStandardItem("— " + category.label + " —");
        headerItem->setFlags(Qt::NoItemFlags);  // 非選択
        headerItem->setForeground(Qt::gray);
        model->appendRow(headerItem);

        for (const QString& item : category.items) {
            QStandardItem* slotItem = new QStandardItem(item);
            model->appendRow(slotItem);
        }
    }

    comboBox->setModel(model);
}
