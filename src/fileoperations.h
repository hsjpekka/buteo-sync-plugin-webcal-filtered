#ifndef FILEOPERATIONS_H
#define FILEOPERATIONS_H

#include <QObject>
#include <QDir>
#include <QSettings>

class fileOperations: public QObject
{
    Q_OBJECT
public:
    explicit fileOperations(QObject *parent = nullptr);
    Q_INVOKABLE QString error();
    Q_INVOKABLE QString getConfigPath();
    Q_INVOKABLE QString readTxt(QString name = "", QString path = "");
    Q_INVOKABLE bool removeFile(QString name = "", QString path = "");
    Q_INVOKABLE QString setFileName(QString name = "", QString path = "");
    Q_INVOKABLE QString writeTxt(QString text, QString name = "", QString path = "");

private:
    QSettings settings;
    QString errorStr, filePath, fileName;
    QString configPath;
    const QString defaultName = "temporal.ics";
    QString defaultPath();
};

#endif // FILEOPERATIONS_H
