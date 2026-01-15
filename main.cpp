#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QStringList>

// === 1. 读取配置文件的函数 (新增 dbOdbc 参数) ===
bool loadDbConfig(QString &dbUser, QString &dbPass, QString &dbOdbc) {
    QString configPath = QCoreApplication::applicationDirPath() + "/db_config.ini";

    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "错误：无法打开配置文件：" << configPath;
        return false;
    }

    QTextStream in(&file);
    QString content = in.readAll().trimmed();
    file.close();

    // 格式要求: acc:root;psw:123456;odbc:mysql;
    QStringList parts = content.split(';', Qt::SkipEmptyParts);

    for (const QString &part : parts) {
        if (part.startsWith("acc:")) {
            dbUser = part.mid(4); // 截取 "acc:" 后的内容
        } else if (part.startsWith("psw:")) {
            dbPass = part.mid(4); // 截取 "psw:" 后的内容
        } else if (part.startsWith("odbc:")) {
            dbOdbc = part.mid(5); // 截取 "odbc:" 后的内容 (因为odbc:是5个字符)
        }
    }

    // 检查三个参数是否都读到了
    if (dbUser.isEmpty() || dbPass.isEmpty() || dbOdbc.isEmpty()) {
        qDebug() << "错误：配置文件格式不完整！";
        return false;
    }

    return true;
}

// === 2. 连接数据库的函数 (新增 odbcName 参数) ===
bool connectToDatabase(const QString &user, const QString &pass, const QString &odbcName) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");

    // 【关键】使用配置文件里读取到的 ODBC 名称
    db.setDatabaseName(odbcName);

    db.setUserName(user);
    db.setPassword(pass);

    if (db.open()) {
        qDebug() << "数据库连接成功！ODBC源:" << odbcName;
        return true;
    } else {
        qDebug() << "数据库连接失败：" << db.lastError().text();
        return false;
    }
}

// === 3. 主函数 ===
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString dbAccount;
    QString dbPassword;
    QString dbOdbcName; // 新增变量

    // --- 读取配置 ---
    if (!loadDbConfig(dbAccount, dbPassword, dbOdbcName)) {
        QMessageBox::critical(nullptr, "配置错误",
                              "无法读取或解析 db_config.ini。\n\n"
                              "请确保文件存在且格式正确：\n"
                              "acc:账号;psw:密码;odbc:数据源名称;");
        return -1;
    }

    // --- 连接数据库 ---
    if (!connectToDatabase(dbAccount, dbPassword, dbOdbcName)) {
        QMessageBox::critical(nullptr, "连接错误",
                              "数据库连接失败！\n"
                              "请检查 ini 文件中的 ODBC名称、账号、密码是否正确。\n"
                              "当前尝试连接的 ODBC 名称为: " + dbOdbcName);
        return -1;
    }

    MainWindow w;
    w.show();

    return a.exec();
}
