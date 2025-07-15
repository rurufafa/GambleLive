#ifndef INFOWIDGET_H
#define INFOWIDGET_H

#include <QWidget>

namespace Ui {
class InfoWidget;
}

class InfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit InfoWidget(QWidget *parent = nullptr);
    ~InfoWidget();
    
    void setStats(int spent, int gained, int spins);
    void updateRoleTable(const QMap<QString, int>& roleCount);
    void setSlotName(const QString& slotName);
    void clearStats();

private:
    Ui::InfoWidget *ui;
};

#endif // INFOWIDGET_H
