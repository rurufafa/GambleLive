#include "config_manager.h"
#include "log_parser.h"

QRegularExpression LogParser::timeExp(R"(\[\d{2}:\d{2}:\d{2}\])");
QRegularExpression LogParser::payExp(R"(^([\d,]+)円支払いました$)");
QRegularExpression LogParser::gainExp(R"(^([\d,]+)円受け取りました$)");
QRegularExpression LogParser::loseExp(R"(^\[Man10Slot\]外れました$)");
QRegularExpression LogParser::roleExp(R"(^\[Man10Slot\]おめでとうございます！(.+)です！$)");


LogParser::LogParser(const QString& chatPrefix)
    : chatPrefix_(chatPrefix), prefixLength_(chatPrefix.length()) {}

std::optional<GambleLog> LogParser::parseLine(const QString& line) {
    QRegularExpressionMatch m;

    // チャットプレフィックス検出
    int chatIndex = line.indexOf(chatPrefix_);
    if (chatIndex == -1) return std::nullopt; 

    // チャット本文抽出
    QString body = line.mid(chatIndex + chatPrefix_.length()).trimmed();
    
    GambleLog log;
    log.content = body;

    if ((m = timeExp.match(line)).hasMatch())
        log.time = m.captured(0);

    if ((m = payExp.match(body)).hasMatch()) {
        log.type = GambleLogType::Payment;
        log.amount = m.captured(1).remove(",").toInt();
        return log;
    }

    if ((m = gainExp.match(body)).hasMatch()) {
        log.type = GambleLogType::Gain;
        log.amount = m.captured(1).remove(",").toInt();
        return log;
    }

    if ((m = loseExp.match(body)).hasMatch()) {
        log.type = GambleLogType::Lose;
        return log;
    }

    if ((m = roleExp.match(body)).hasMatch()) {
        log.type = GambleLogType::Role;
        log.roleName = m.captured(1).trimmed();
        return log;
    }

    return std::nullopt;
}
