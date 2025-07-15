#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QJsonObject>
#include <QJsonArray>
#include <QObject>
#include <QString>

class ConfigManager {
public:
    static ConfigManager& instance();

    void load();                  // ファイルから読み込み
    void save();                  // ファイルへ保存
    QVariant get(const QString &key) const;
    void set(const QString &key, const QVariant &value);
    const QJsonObject& defaultConfig() const;

private:
    ConfigManager();           
    QJsonObject config_;
    QJsonObject default_config_;
    QString path_;

    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
};

#endif // CONFIGMANAGER_H
