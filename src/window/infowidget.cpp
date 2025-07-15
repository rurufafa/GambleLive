#include <QLocale>
#include <QMap>

#include "infowidget.h"
#include "ui_infowidget.h"

InfoWidget::InfoWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::InfoWidget)
{
    ui->setupUi(this);
    // ヘッダーの幅をウィンドウサイズにフィットさせ、等分配
    QHeaderView *header = ui->tableRoles->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Stretch);  
}

InfoWidget::~InfoWidget()
{
    delete ui;
}

void InfoWidget::setStats(int spent, int gained, int spins) {
    QLocale locale = QLocale::system();

    ui->labelSpent->setText(QString("-%1").arg(locale.toString(spent)));
    ui->labelGained->setText(QString("+%1").arg(locale.toString(gained)));
    ui->labelNet->setText(QString("%1%2")
        .arg(gained - spent >= 0 ? "+" : "")
        .arg(locale.toString(gained - spent)));
    ui->labelSpinCount->setText(QString("%1").arg(locale.toString(spins)));
}

void InfoWidget::updateRoleTable(const QMap<QString, int>& roleCount) {
    int total = 0;
    for (auto count : roleCount.values())
        total += count;

    ui->tableRoles->setRowCount(roleCount.size());

    QList<QString> sortedRoles = roleCount.keys();
    std::sort(sortedRoles.begin(), sortedRoles.end(), [&](const QString& a, const QString& b) {
        return roleCount[a] > roleCount[b]; 
    });

    for (int row = 0; row < sortedRoles.size(); ++row) {
        const QString& role = sortedRoles[row];
        int count = roleCount[role];
        double rate = (total > 0) ? (count * 100.0 / total) : 0.0;

        ui->tableRoles->setItem(row, 0, new QTableWidgetItem(role));
        ui->tableRoles->setItem(row, 1, new QTableWidgetItem(QString::number(count)));
        ui->tableRoles->setItem(row, 2, new QTableWidgetItem(QString("%1%").arg(QString::number(rate, 'f', 2))));
    }
}

void InfoWidget::setSlotName(const QString& slotName) {
    ui->labelSlotName->setText(slotName);
}

void InfoWidget::clearStats() {
    setStats(0, 0, 0);       // 支出、収入、回転数を0に
    QMap<QString, int> emptyRoleCount;   
    updateRoleTable(emptyRoleCount);  // 表もクリア表示
}
