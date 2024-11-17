#include "fileoperations.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>

fileOperations::fileOperations(QObject *parent) : QObject(parent)
{
    filePath = defaultPath();
    fileName = defaultName;
}

QString fileOperations::defaultPath()
{
    return QDir().homePath() + "/Downloads/";
}

QString fileOperations::error()
{
    return errorStr;
}

QString fileOperations::getConfigPath()
{
    int position;
    QString path, result;
    QDir dir;
    // qDebug() << settings.fileName() << settings.applicationName() << settings.organizationName();
    // remove the name of the default config file
    // Qt default to $HOME/.config/org/appName.conf
    path = settings.fileName();
    position = path.lastIndexOf("/");
    path = path.left(position);
    // path should be $HOME/.config/org/appName/
    if ( !path.contains(settings.applicationName()) ) {
        if (path.at(path.size() - 1) != '/') {
            path.append("/");
        }
        path.append(settings.applicationName());
        path.append("/");
    }
    // qDebug() << path;
    if (!dir.exists(path)) {
        if (!dir.mkpath(path)) {
            qWarning() << "Can't create path" << result;
        }
    }
    result = path;
    return result;
}

/*
QString fileOperations::readFiltersFile(QString fileName, QString path)
{
    QFile fFile;
    QTextStream fData;
    QString result;

    if (fileName.length() > 0) {
        setFiltersFile(fileName, path);
    }

    fFile.setFileName(configPath + filtersFileName);
    if (fFile.exists() && fFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        //fFile.open(QIODevice::ReadOnly | QIODevice::Text);
        fData.setDevice(&fFile);
        result = fData.readAll();
        fFile.close();
    } else {
        qWarning() << "Filters file not found:" << filtersPath << filtersFileName;
    }

    return result;
}
// */

QString fileOperations::readTxt(QString name, QString path)
{
    QString result;
    QFile iFile;
    QTextStream input;

    if (!name.isEmpty()) {
        if (path.isEmpty()) {
            setFileName(name, filePath);
        } else {
            setFileName(name, path);
        }
    }

    errorStr = "";
    iFile.setFileName(filePath + fileName);
    if (!iFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorStr = "Error in opening ";
        errorStr.append(filePath + fileName);
        errorStr.append(": ");
        errorStr.append(iFile.errorString());
        qWarning() << errorStr;
        return result;
    }

    input.setDevice(&iFile);
    result = input.readAll();
    if (input.status() != QTextStream::Ok) {
        errorStr = "Error in reading ";
        errorStr.append(filePath + fileName);
        errorStr.append(". Status: ");
        errorStr.append(input.status());
        qWarning() << errorStr;
    }
    iFile.close();

    return result;
}

bool fileOperations::removeFile(QString name, QString path)
{
    QFile file;
    if (!name.isEmpty()) {
        file.setFileName(setFileName(name, path));
    } else {
        file.setFileName(filePath + fileName);
    }

    return file.remove();
}

QString fileOperations::setFileName(QString name, QString path)
{
    // if name == "" use defaults
    if (name.isEmpty()) {
        fileName = defaultName;
        filePath = defaultPath();
    } else {
        fileName = name;
        if (!path.isEmpty()) {
            if (path.at(path.size()-1) != '/') {
                path.append('/');
            }
            if (path.at(0) != '/') {
                filePath = QDir().homePath() + "/" + path;
            } else {
                filePath = path;
            }
        }
    }

    return filePath + fileName;
}

QString fileOperations::writeTxt(QString text, QString name, QString path)
{
    QString result;
    QFile oFile;
    QTextStream output;

    if (!name.isEmpty()) {
        if (path.isEmpty()) {
            setFileName(name, filePath);
        } else {
            setFileName(name, path);
        }
    }

    errorStr = "";
    oFile.setFileName(filePath + fileName);
    if (!oFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errorStr = "Error in opening ";
        errorStr.append(filePath + fileName);
        qWarning() << "Error in opening the temporary file.";
        return result;
    }

    output.setDevice(&oFile);
    output << text;
    output.flush();
    if (output.status() == QTextStream::Ok) {
        result = filePath + fileName;
    } else {
        errorStr = "Error in writing ";
        errorStr.append(filePath + fileName);
        result = oFile.errorString();
        qWarning() << result;
    }
    oFile.close();

    return result;
}
