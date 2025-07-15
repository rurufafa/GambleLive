#ifndef LOGPARSER_H
#define LOGPARSER_H

#include <QString>
#include <QRegularExpression>
#include <optional>

enum class GambleLogType {
    Payment,
    Gain,
    Lose,
    Role
};

struct GambleLog {
    GambleLogType type;
    int amount = 0;              // 支払 or 受取
    QString roleName;           // 当たり役名
    QString time;               // [HH:MM:SS]
    QString content;            // 元のログ内容（プレフィックス除去後）
};

class LogParser {
public:
    explicit LogParser(const QString& chatPrefix);

    std::optional<GambleLog> parseLine(const QString& line);

private:
    QString chatPrefix_;
    int prefixLength_;

    static QRegularExpression timeExp;
    static QRegularExpression payExp;
    static QRegularExpression gainExp;
    static QRegularExpression loseExp;
    static QRegularExpression roleExp;
};

#endif // LOGPARSER_H