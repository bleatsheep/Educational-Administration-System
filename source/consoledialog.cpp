#include "consoledialog.h"
#include <QHeaderView>

ConsoleDialog::ConsoleDialog(QWidget *parent) : QDialog(parent)
{
    this->setWindowTitle("Administrator SQL Console");
    this->resize(800, 600);

    // === 样式美化 (黑客风) ===
    this->setStyleSheet(R"(
        QDialog { background-color: #1e1e1e; color: #00ff00; }
        QTextEdit { background-color: #252526; color: #dcdcdc; font-family: 'Consolas', 'Courier New'; font-size: 14px; border: 1px solid #3e3e42; }
        QTableView { background-color: #252526; color: #dcdcdc; gridline-color: #3e3e42; }
        QHeaderView::section { background-color: #333333; color: #dcdcdc; }
        QPushButton { background-color: #007acc; color: white; border: none; padding: 5px 15px; font-weight: bold; }
        QPushButton:hover { background-color: #0098ff; }
        QLabel { color: #aaaaaa; }
    )");

    QVBoxLayout *layout = new QVBoxLayout(this);

    // 1. 提示标签
    layout->addWidget(new QLabel("输入 SQL 语句 (支持 SELECT, INSERT, UPDATE, DELETE):"));

    // 2. 输入框
    m_inputEdit = new QTextEdit(this);
    m_inputEdit->setPlaceholderText("SELECT * FROM student WHERE ...");
    m_inputEdit->setMaximumHeight(100);
    layout->addWidget(m_inputEdit);

    // 3. 运行按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    m_btnRun = new QPushButton("执行 (Execute)", this);
    m_btnRun->setCursor(Qt::PointingHandCursor);
    btnLayout->addWidget(m_btnRun);
    layout->addLayout(btnLayout);

    // 4. 结果展示区 (表格)
    m_resultTable = new QTableView(this);
    m_resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(m_resultTable);

    // 5. 状态日志区
    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumHeight(80);
    m_logEdit->setStyleSheet("color: #ff5555;"); // 错误默认红色
    layout->addWidget(m_logEdit);

    // === 连接信号 ===
    connect(m_btnRun, &QPushButton::clicked, this, &ConsoleDialog::executeSql);
}

void ConsoleDialog::executeSql()
{
    QString sql = m_inputEdit->toPlainText().trimmed();
    if(sql.isEmpty()) return;

    QSqlQuery query;

    // 区分查询类型
    if(sql.startsWith("SELECT", Qt::CaseInsensitive) ||
        sql.startsWith("SHOW", Qt::CaseInsensitive) ||
        sql.startsWith("DESC", Qt::CaseInsensitive) ||
        sql.startsWith("EXPLAIN", Qt::CaseInsensitive)) {
        // === 查询语句：使用 Model 显示表格 ===
        QSqlQueryModel *model = new QSqlQueryModel(this);
        model->setQuery(sql);

        if (model->lastError().isValid()) {
            m_logEdit->setTextColor(QColor("#ff5555")); // 红
            m_logEdit->setText("查询错误:\n" + model->lastError().text());
            delete model;
        } else {
            m_resultTable->setModel(model);
            m_logEdit->setTextColor(QColor("#00ff00")); // 绿
            m_logEdit->setText(QString("查询成功。返回 %1 行数据。").arg(model->rowCount()));
        }
    }
    else {
        // === 非查询语句 (INSERT/UPDATE/DELETE) ===
        if(query.exec(sql)) {
            m_logEdit->setTextColor(QColor("#00ff00")); // 绿
            m_logEdit->setText(QString("执行成功。\n影响行数: %1").arg(query.numRowsAffected()));
            // 清空表格
            m_resultTable->setModel(nullptr);
        } else {
            m_logEdit->setTextColor(QColor("#ff5555")); // 红
            m_logEdit->setText("执行错误:\n" + query.lastError().text());
        }
    }
}
