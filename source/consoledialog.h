#ifndef CONSOLEDIALOG_H
#define CONSOLEDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QTableView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QLabel>
#include <QKeyEvent>

class ConsoleDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConsoleDialog(QWidget *parent = nullptr);

private:
    QTextEdit *m_inputEdit;      // SQL 输入框
    QTableView *m_resultTable;   // 结果表格
    QTextEdit *m_logEdit;        // 执行日志（显示错误或成功消息）
    QPushButton *m_btnRun;       // 运行按钮

    void executeSql();           // 执行逻辑
};

#endif // CONSOLEDIALOG_H
