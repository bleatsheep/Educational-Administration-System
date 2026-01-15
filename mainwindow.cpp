//basic header files
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "dialog.h"

#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "datamanager.h"

#include <QClipboard>

#include <QMessageBox>
#include <QMenu>

#include <QTabWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

#include <QProcess>      // 用于启动新进程
#include <QApplication>  // 用于获取当前程序路径

#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    //细节初始化

    ui->setupUi(this);
    this->setWindowTitle("学生管理系统");
    ui->actionLog_out->setEnabled(false);


    ui->lineEdit_Account->setEnabled(false);//细节要求先选择身份再输入密码
    ui->lineEdit_Psw->setEnabled(false);


    ui->loginCard->setFocus();//细节让焦点在背景，这样就能显示输入框的提示


    //细节展示登录提示文字的动画

    QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect(ui->label_3);
    ui->label_3->setGraphicsEffect(opacityEffect);
    QPropertyAnimation *animation = new QPropertyAnimation(opacityEffect, "opacity", this);
    animation->setDuration(2000);
    animation->setKeyValueAt(0.0, 1.0);
    animation->setKeyValueAt(0.5, 0.0);
    animation->setKeyValueAt(1.0, 1.0);
    animation->setEasingCurve(QEasingCurve::InOutSine);
    animation->setLoopCount(-1);
    animation->start();

    //细节录界面隐藏侧边

    // ui->label->setVisible(false);
    // ui->listWidget->setVisible(false);
    ui->mainUi->setCurrentIndex(0);

    //身份选择按钮 逻辑
    m_roleGroup = new QButtonGroup(this); // 设置按钮组
    m_roleGroup->addButton(ui->login_student, RoleStudent);
    m_roleGroup->addButton(ui->login_teacher, RoleTeacher);
    m_roleGroup->addButton(ui->login_admin,   RoleAdmin);
    m_roleGroup->setExclusive(true);//按钮互斥
    connect(m_roleGroup, &QButtonGroup::buttonClicked, this, [=](QAbstractButton *clickedBtn){

        m_currentRole = m_roleGroup->id(clickedBtn);
        ui->lineEdit_Account->setEnabled(true);//启用输入框
        ui->lineEdit_Psw->setEnabled(true);


        switch (m_currentRole) {
        case RoleStudent:
            ui->lineEdit_Account->setPlaceholderText("请输入学生学号");
            break;
        case RoleTeacher:
            ui->lineEdit_Account->setPlaceholderText("请输入教师工号");
            break;
        case RoleAdmin:
            ui->lineEdit_Account->setPlaceholderText("请输入管理员账号");
            break;
        }

        qDebug() << "切换身份ID:" << m_currentRole;
    });
    ui->login_student->click();
    //限制当输入不完全时禁用登录
    auto checkInputFunc = [=]() {
        // 获取账号 (用 trimmed() 去掉首尾空格，防止只输空格)
        QString acc = ui->lineEdit_Account->text().trimmed();
        // 获取密码 (密码通常不去空格，看你需求)
        QString pwd = ui->lineEdit_Psw->text();

        // 判断：两个都不为空
        bool isOk = !acc.isEmpty() && !pwd.isEmpty();

        // 设置按钮状态
        ui->log_in->setEnabled(isOk);
    };

    // 3. 连接信号：只要你在输入框里打字，就触发检查
    connect(ui->lineEdit_Account, &QLineEdit::textChanged, this, checkInputFunc);
    connect(ui->lineEdit_Psw,     &QLineEdit::textChanged, this, checkInputFunc);


    //左侧菜单栏选项

    // connect(ui->listWidget, &QListWidget::currentRowChanged, this, [=](int row){
    //     if (row == -1) {
    //         ui->stackedWidget->setCurrentIndex(0);
    //     }
    //     if (row == 0) {
    //         ui->stackedWidget->setCurrentIndex(2);
    //     }
    //     else if (row == 1) {
    //         ui->stackedWidget->setCurrentIndex(3);
    //     }
    //     else if (row == 2) {
    //         ui->stackedWidget->setCurrentIndex(4);
    //     }
    //     else if (row == 3) {
    //         ui->stackedWidget->setCurrentIndex(5);
    //     }
    // });

    //登录按钮
    ui->log_in->setEnabled(false);
    connect(ui->log_in, &QPushButton::clicked, this, [=](){
        DataManager dm;
        QString account = ui->lineEdit_Account->text().trimmed();
        QString password = ui->lineEdit_Psw->text();
        int role = m_currentRole;
        ui->label_3->setText("Logging in...");

        bool isSuccess = dm.login(account, password, role);

        if (isSuccess) {
            qDebug() << "登录成功！即将跳转...";
            ui->log_info_button->setText("已登录: " + account);
            switch (role) {
            case RoleStudent:
                ui->mainUi->setCurrentIndex(2);
                break;
            case RoleTeacher:
                ui->mainUi->setCurrentIndex(3);
                break;
            case RoleAdmin:
                ui->mainUi->setCurrentIndex(4);
                break;
            }
        }
        else {
            QMessageBox::warning(this, "登录失败", "账号或密码错误，请检查输入！");
            ui->lineEdit_Psw->clear(); // 清空密码让用户重输
            ui->label_3->setText("账号或密码错误");
        }
    });

    // === 1. 初始化表格表头 (建议放在构造函数或 init 函数里) ===


    //About按钮
    connect(ui->actionAbout, &QAction::triggered, this, [=](){

        Dialog dlg;

        dlg.setWindowTitle("About");
        dlg.exec();
    });

    //查询

    //文本可复制无法修改
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);


    //右键复制功能

    // 1. 设置允许自定义右键菜单
    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    // 2. 连接右键信号
    connect(ui->tableWidget, &QTableWidget::customContextMenuRequested, this, [=](const QPoint &pos){
        QMenu menu;
        QAction *copyAction = menu.addAction("复制");

        // 执行菜单并获取点击项
        if(menu.exec(ui->tableWidget->viewport()->mapToGlobal(pos)) == copyAction) {
            // 执行复制逻辑
            QList<QTableWidgetItem*> selectedItems = ui->tableWidget->selectedItems();
            if(selectedItems.isEmpty()) return;

            QString str;
            for(int i = 0; i < selectedItems.count(); ++i) {
                str += selectedItems.at(i)->text() + (i % 3 == 2 ? "\n" : "\t"); // 假设3列
            }
            QGuiApplication::clipboard()->setText(str);; // 存入系统剪贴板
        }
    });

    connect(ui->lineEdit_Search, &QLineEdit::returnPressed, ui->pushButton_query, &QPushButton::click);//按下回车也能查询
    //额外选项（显示班级）
    QMenu *menu = new QMenu(this);

    ui->pushButton_more->setMenu(menu);

    QAction *actionName = menu->addAction("显示姓名列");
    actionName->setCheckable(true); // 关键：设置为可勾选
    actionName->setChecked(true);   // 默认勾选

    QAction *actionId = menu->addAction("显示学号列");
    actionId->setEnabled(false);
    actionId->setCheckable(true);
    actionId->setChecked(true);

    QAction *actionClass = menu->addAction("显示班级");
    actionClass->setCheckable(true);
    actionClass->setChecked(false);


    ui->tableWidget->setColumnCount(4);
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "序号" << "姓名" << "学号" << "班级");
    ui->tableWidget->setColumnHidden(0, true);
    ui->tableWidget->setColumnHidden(3, true);
    connect(actionName, &QAction::toggled, this, [=](bool checked){
        // 注意：setColumnHidden 的逻辑是 "是否隐藏"，所以 checked 为 true 时，hidden 应该是 false
        ui->tableWidget->setColumnHidden(1, !checked);
    });
    connect(actionClass, &QAction::toggled, this, [=](bool checked){
        ui->tableWidget->setColumnHidden(3, !checked);
    });
    ui->tableWidget->verticalHeader()->setVisible(false);

    connect(ui->pushButton_query, &QPushButton::clicked, this, [=](){
        ui->tableWidget->setRowCount(0);
        QString name = ui->lineEdit_Search->text();
        DataManager dm;
        QList<QStringList> dataList = dm.getStudents(name);
        for(int i = 0; i < dataList.size(); ++i)
        {
            //获取数据
            QStringList oneStudent = dataList[i];

            // 给tablewidget新增一行
            ui->tableWidget->insertRow(i);


            // 第0列：id
            ui->tableWidget->setItem(i, 0, new QTableWidgetItem(oneStudent[0]));
            // 第1列：姓名
            ui->tableWidget->setItem(i, 1, new QTableWidgetItem(oneStudent[1]));
            // 第2列：学号
            ui->tableWidget->setItem(i, 2, new QTableWidgetItem(oneStudent[2]));
            //第3列：班级
            ui->tableWidget->setItem(i, 3, new QTableWidgetItem(oneStudent[3]));
            //内容居中显示
            ui->tableWidget->item(i, 0)->setTextAlignment(Qt::AlignCenter);
            ui->tableWidget->item(i, 1)->setTextAlignment(Qt::AlignCenter);
            ui->tableWidget->item(i, 2)->setTextAlignment(Qt::AlignCenter);
            ui->tableWidget->item(i, 3)->setTextAlignment(Qt::AlignCenter);
        }
    });


    ui->tableWidget->setStyleSheet(
        "QTableWidget { outline: none; }" // 去掉焦点框
        "QTableWidget::item:selected { "
        "   background-color: #454545; "  // 选中时的背景色（深灰色）
        "   border: none; "               // 确保没有边框
        "}"
        );
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    //添加学生按钮
    connect(ui->pushButton_add, &QPushButton::pressed, this, [=](){

    });
    QList<int> *selected_id = new QList<int>;

    ui->pushButton_delete->setEnabled(false);
    ui->pushButton_delete->setStyleSheet(
        "color: rgb(64, 64, 64)"
        );
    connect(ui->tableWidget->selectionModel(), &QItemSelectionModel::selectionChanged, ui->pushButton_delete, [=](){
        selected_id->clear();
        QModelIndexList selected_info = ui->tableWidget->selectionModel()->selectedRows();
        if(selected_info.isEmpty()){
            ui->pushButton_delete->setEnabled(false);
            ui->pushButton_delete->setStyleSheet(
                "color: rgb(64, 64, 64)"
                );
        }
        else{
            ui->pushButton_delete->setEnabled(true);
            ui->pushButton_delete->setStyleSheet(
                "color: rgb(255, 0, 0)"
                );
        }
        for(auto &index:selected_info){          //どうぞ忽略这个warningございます，我就是需要深拷贝这个遍历中的变量だから
            int row = index.row();
            QString text = ui->tableWidget->item(row, 0)->text();
            int id = text.toInt();
            (*selected_id).append(id);
        }
        int count = selected_info.size();
        QString info;
        if(count)
            info = "删除学生(已选中" + QString::number(count) + "Rows)";
        else
            info = "删除学生";
        ui->pushButton_delete->setText(info);
    });

    //删除学生按钮

    DataManager *dm = new DataManager;
    connect(ui->pushButton_delete, &QPushButton::pressed, this, [=](){
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "确认删除",
                                      QString("确定要删除选中的 %1 名学生吗？\n此操作无法撤销！").arg(selected_id->size()),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No);
        if(reply == QMessageBox::No)
            return;
        if(dm->deleteStudents(*selected_id)){
            QMessageBox::information(this, "成功", "删除成功！");
            ui->pushButton_query->click(); // 刷新表格
        }
    });
    // connect(dm, &DataManager::errorOccurred, this, [=](QString msg){
    //     QMessageBox::critical(this, "错误", msg);
    // });


    //登出按钮

    // connect(ui->actionLog_out, &QAction::triggered, this, [=](){
    //     ui->listWidget->setVisible(false);
    //     ui->label->setVisible(false);
    //     ui->stackedWidget->setCurrentIndex(0);
    //     ui->label_3->setText("You have been logged out. To continue, Please log in.");
    //     ui->log_in->setEnabled(true);
    //     ui->log_in->setText("Log in");
    //     ui->listWidget->setCurrentRow(-1);
    //     ui->actionLog_out->setEnabled(false);
    // });

    //初始化学生相关页面的函数

    setupStudentSelectionUi();

    initCourseSelection();

    initStudentNavigation();

    setupSchedulePageUi();

    setupGradePageUi();

    initMenuConnections();

    loadSemesterData();

    setupInfoPageUi();

    //老师相关页面的函数
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initStudentNavigation()
{
    // 选课中心按钮 -> 跳转到选课页 (Stack Index 1)
    connect(ui->btn_CourseSelect, &QToolButton::clicked, this, [=](){
        // 如果你的选课页在 stack 的第2页(index 1)，或者是 ui->student_selection
        ui->stack_student->setCurrentWidget(ui->student_selection);
        refreshCourseTables(); // 进页面时自动刷新数据
    });

    // 返回按钮 -> 返回首页 (Stack Index 0)
    connect(m_btnReturn, &QPushButton::clicked, this, [=](){
        ui->stack_student->setCurrentWidget(ui->student_HomePage);
    });

    // 其他按钮占位逻辑
    connect(ui->btn_Schedule, &QToolButton::clicked, this, [=](){

        // 1. 切换到我们刚才新建的 m_pageSchedule 页面
        ui->stack_student->setCurrentWidget(m_pageSchedule);

        // 2. 每次点进来都刷新一下数据
        updateScheduleTable();

        qDebug() << "进入独立课表页面";
    });
    connect(ui->btn_Grade, &QToolButton::clicked, this, [=](){

        // 1. 切换页面
        ui->stack_student->setCurrentWidget(m_pageGrade);

        // 2. 刷新数据
        updateGradeTable();

        qDebug() << "进入成绩查询页面";
    });
    connect(ui->btn_Info, &QToolButton::clicked, this, [=](){

        // 1. 切换页面
        ui->stack_student->setCurrentWidget(m_pageInfo);

        // 2. 刷新数据
        loadStudentInfo();

        qDebug() << "进入个人信息页面";
    });
}

void MainWindow::initCourseSelection()
{
    // ===========================================
    // 1. 初始化学期下拉框
    // ===========================================
    // 防止重复添加，先清空
    m_comboTerm->clear();


    // ===========================================
    // 2. 配置 Tab 1 (可选课程表格)
    // ===========================================
    QStringList headersAvailable = {"ID", "课程名称", "教师", "学分", "已选/容量", "操作"};
    m_tableAvailable->setColumnCount(6);
    m_tableAvailable->setHorizontalHeaderLabels(headersAvailable);

    // 隐藏 ID 列 (第0列)
    m_tableAvailable->setColumnHidden(0, true);

    // 样式设置
    m_tableAvailable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); // 自动铺满
    m_tableAvailable->verticalHeader()->setVisible(false);                            // 隐藏行号
    m_tableAvailable->setSelectionBehavior(QAbstractItemView::SelectRows);            // 选中整行
    m_tableAvailable->setEditTriggers(QAbstractItemView::NoEditTriggers);             // 禁止编辑

    // ===========================================
    // 3. 配置 Tab 2 (我的课表表格)
    // ===========================================
    QStringList headersMy = {"ID", "课程名称", "教师", "学分", "操作"};
    m_tableMyCourses->setColumnCount(5);
    m_tableMyCourses->setHorizontalHeaderLabels(headersMy);

    // 隐藏 ID 列
    m_tableMyCourses->setColumnHidden(0, true);

    // 样式设置
    m_tableMyCourses->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableMyCourses->verticalHeader()->setVisible(false);
    m_tableMyCourses->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableMyCourses->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // ===========================================
    // 4. 信号连接 (防止重复连接，先 disconnect)
    // ===========================================
    disconnect(m_btnRefresh, nullptr, nullptr, nullptr);
    connect(m_btnRefresh, &QPushButton::clicked, this, &MainWindow::refreshCourseTables);

    disconnect(m_tabWidget_Course, nullptr, nullptr, nullptr);
    connect(m_tabWidget_Course, &QTabWidget::currentChanged, this, &MainWindow::refreshCourseTables);

    // 当下拉框改变学期时，也自动刷新
    disconnect(m_comboTerm, nullptr, nullptr, nullptr);
    connect(m_comboTerm, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::refreshCourseTables);
}
void MainWindow::refreshCourseTables()
{
    // 获取参数
    int currentTermId = m_comboTerm->currentData().toInt();
    int studentId = m_studentId;
    DataManager dm;

    // ===========================================
    // Part A: 刷新 Tab 1 (可选课程)
    // ===========================================
    m_tableAvailable->setRowCount(0);
    QList<QStringList> listAvail = dm.getAvailableCourses(currentTermId);

    for(int i = 0; i < listAvail.size(); ++i) {
        QStringList rowData = listAvail[i];
        m_tableAvailable->insertRow(i);

        for(int k=0; k<5; k++) {
            QTableWidgetItem *item = new QTableWidgetItem(rowData[k]);
            item->setTextAlignment(Qt::AlignCenter);
            m_tableAvailable->setItem(i, k, item);
        }

        QPushButton *btnEnroll = new QPushButton("选课");
        btnEnroll->setCursor(Qt::PointingHandCursor);
        // 去掉边框，加一点圆角
        btnEnroll->setStyleSheet(
            "QPushButton { background-color: #67c23a; color: white; border: none; border-radius: 4px; font-weight: bold; padding: 5px; }"
            "QPushButton:hover { background-color: #85ce61; }"
            "QPushButton:pressed { background-color: #5daf34; }"
            );

        int courseId = rowData[0].toInt();
        connect(btnEnroll, &QPushButton::clicked, this, [=](){
            DataManager tempDm;

            // 调用函数，接收具体结果
            EnrollResult result = tempDm.enrollCourse(studentId, courseId);

            // 根据结果判断
            switch (result) {
            case EnrollSuccess:
                QMessageBox::information(this, "恭喜", "选课成功！");
                refreshCourseTables(); // 刷新界面
                break;

            case AlreadyEnrolled:
                QMessageBox::warning(this, "选课失败", "您已经选修过这门课程了，请勿重复选择。");
                break;

            case TimeConflict:
                QMessageBox::warning(this, "选课失败", "检测到时间冲突！\n该课程的上课时间与您已选的课程重叠。");
                break;

            case ClassFull:
                QMessageBox::warning(this, "选课失败", "手慢了！该课程名额已满。");
                break;

            case DatabaseError:
                QMessageBox::critical(this, "错误", "系统内部错误，请联系管理员。");
                break;
            }
        });

        m_tableAvailable->setCellWidget(i, 5, btnEnroll);
    }

    //tab2已选课程
    m_tableMyCourses->setRowCount(0);
    QList<QStringList> listMy = dm.getMyCourses(studentId, currentTermId);

    double totalCredits = 0.0;

    for(int i = 0; i < listMy.size(); ++i) {
        QStringList rowData = listMy[i];
        m_tableMyCourses->insertRow(i);

        for(int k=0; k<4; k++) {
            QTableWidgetItem *item = new QTableWidgetItem(rowData[k]);
            item->setTextAlignment(Qt::AlignCenter);
            m_tableMyCourses->setItem(i, k, item);
        }

        totalCredits += rowData[3].toDouble();

        // --- 🔴 退课按钮样式优化 ---
        QPushButton *btnDrop = new QPushButton("退课");
        btnDrop->setCursor(Qt::PointingHandCursor);
        // 红色背景，无边框
        btnDrop->setStyleSheet(
            "QPushButton { background-color: #f56c6c; color: white; border: none; border-radius: 4px; font-weight: bold; padding: 5px; }"
            "QPushButton:hover { background-color: #ff7875; }"
            "QPushButton:pressed { background-color: #dd6161; }"
            );

        int courseId = rowData[0].toInt();
        connect(btnDrop, &QPushButton::clicked, this, [=](){
            if(QMessageBox::Yes == QMessageBox::question(this, "确认", "确定要退掉这门课吗？")) {
                DataManager tempDm;
                if(tempDm.dropCourse(studentId, courseId)) {
                    QMessageBox::information(this, "成功", "退课成功。");
                    refreshCourseTables();
                }
            }
        });

        m_tableMyCourses->setCellWidget(i, 4, btnDrop);
    }

    // 更新学分文字颜色已经在 setupUi 里设好了，这里只设文字
    m_labelCredits->setText(QString("当前学期总学分: %1").arg(totalCredits));
}

void MainWindow::setupStudentSelectionUi()
{
    // 1. 获取容器
    QWidget *pageContainer = ui->student_selection;
    if (pageContainer->layout()) delete pageContainer->layout();

    // 2. 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(pageContainer);
    mainLayout->setContentsMargins(30, 30, 30, 30); // 边距留大点，显得大气
    mainLayout->setSpacing(20);

    // =======================================================
    // 🎨 【核心】暗黑主题样式表 (Dark Mode QSS)
    // =======================================================
    QString darkQss = R"(
        /* 1. 全局文字与背景 */
        QWidget {
            color: #e0e0e0;             /* 全局文字：灰白色 */
            font-family: 'Microsoft YaHei', sans-serif;
            font-size: 14px;
        }

        /* 2. 顶部返回按钮 */
        QPushButton#btnReturn {
            border: none;
            background: transparent;
            color: #aaaaaa;
            font-size: 16px;
            font-weight: bold;
            text-align: left;
        }
        QPushButton#btnReturn:hover { color: #409eff; } /* 悬停变蓝 */

        /* 3. 顶部刷新按钮 */
        QPushButton#btnRefresh {
            background-color: #3a3a3a;
            border: 1px solid #555555;
            border-radius: 6px;
            color: #ffffff;
            padding: 6px 16px;
        }
        QPushButton#btnRefresh:hover {
            background-color: #4a4a4a;
            border-color: #409eff;
        }

        /* 4. 学期下拉框 */
        QComboBox {
            background-color: #3a3a3a;
            border: 1px solid #555555;
            border-radius: 6px;
            padding: 5px 10px;
            color: white;
        }
        QComboBox::drop-down { border: none; }
        QComboBox::down-arrow {
            image: none; /* 如果你想自定义箭头图片可以在这加，或者保持默认 */
            border-left: 1px solid #555555;
        }
        QComboBox QAbstractItemView {
            background-color: #3a3a3a;
            color: white;
            selection-background-color: #409eff;
            outline: none;
        }

        /* 5. Tab Widget 外框 */
        QTabWidget::pane {
            border: 1px solid #444444;
            background: #2b2b2b; /* 内容区背景：深灰 */
            top: -1px;
            border-radius: 4px;
        }

        /* 6. Tab 标签头 */
        QTabBar::tab {
            background: #1e1e1e; /* 未选中：更黑 */
            border: 1px solid #333333;
            color: #888888;
            padding: 10px 30px;
            margin-right: 2px;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
        }
        QTabBar::tab:selected {
            background: #2b2b2b; /* 选中：深灰（和内容区连成一片） */
            color: #409eff;      /* 选中文字：亮蓝 */
            border-bottom: 2px solid #2b2b2b; /* 隐藏底边框 */
            font-weight: bold;
        }

        /* 7. 表格样式 (最重要) */
        QTableWidget {
            background-color: #2b2b2b;
            gridline-color: #3c3c3c; /* 网格线颜色 */
            border: none;
            color: #dddddd;
            selection-background-color: #409eff; /* 选中行颜色 */
            outline: none;
        }
        /* 表头 */
        QHeaderView::section {
            background-color: #333333;
            color: #ffffff;
            padding: 8px;
            border: none;
            border-bottom: 2px solid #409eff; /* 表头下方的蓝线 */
            font-weight: bold;
        }
        /* 垂直滚动条美化 (可选) */
        QScrollBar:vertical {
            background: #2b2b2b;
            width: 10px;
        }
        QScrollBar::handle:vertical {
            background: #555555;
            border-radius: 5px;
        }
    )";
    pageContainer->setStyleSheet(darkQss);

    // =========================================
    // 3. 顶部栏布局
    // =========================================
    QHBoxLayout *topBarLayout = new QHBoxLayout();

    m_btnReturn = new QPushButton("← 返回首页", pageContainer);
    m_btnReturn->setObjectName("btnReturn"); // 用于QSS匹配
    m_btnReturn->setCursor(Qt::PointingHandCursor);

    QLabel *lblTerm = new QLabel("当前学期：", pageContainer);
    // 这里不需要设样式，会继承全局的 color: #e0e0e0

    m_comboTerm = new QComboBox(pageContainer);
    m_comboTerm->setFixedWidth(220);
    m_comboTerm->setCursor(Qt::PointingHandCursor);

    m_btnRefresh = new QPushButton("刷新数据", pageContainer);
    m_btnRefresh->setObjectName("btnRefresh"); // 用于QSS匹配
    m_btnRefresh->setCursor(Qt::PointingHandCursor);

    topBarLayout->addWidget(m_btnReturn);
    topBarLayout->addStretch();
    topBarLayout->addWidget(lblTerm);
    topBarLayout->addWidget(m_comboTerm);
    topBarLayout->addWidget(m_btnRefresh);

    mainLayout->addLayout(topBarLayout);

    // =========================================
    // 4. Tab 和 表格布局
    // =========================================
    m_tabWidget_Course = new QTabWidget(pageContainer);

    // --- Tab 1: 选课 ---
    QWidget *tab1 = new QWidget();
    tab1->setStyleSheet("background-color: #2b2b2b;"); // 必须显式设置背景
    QVBoxLayout *layout1 = new QVBoxLayout(tab1);
    layout1->setContentsMargins(0,0,0,0); // 去掉内边距，让表格贴边

    m_tableAvailable = new QTableWidget();
    layout1->addWidget(m_tableAvailable);
    m_tabWidget_Course->addTab(tab1, "选课中心 (Shopping Mall)");

    // --- Tab 2: 已选 ---
    QWidget *tab2 = new QWidget();
    tab2->setStyleSheet("background-color: #2b2b2b;");
    QVBoxLayout *layout2 = new QVBoxLayout(tab2);
    layout2->setContentsMargins(0,0,0,0);

    m_tableMyCourses = new QTableWidget();
    layout2->addWidget(m_tableMyCourses);

    // 底部学分 (放到 Tab2 下面或者悬浮在表格下)
    // 这里我们把它放在 Tab2 的垂直布局底部
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(10, 10, 10, 10);

    m_labelCredits = new QLabel("当前学期总学分: 0.0");
    m_labelCredits->setStyleSheet("font-weight: bold; font-size: 16px; color: #409eff;"); // 亮蓝色文字

    bottomLayout->addStretch();
    bottomLayout->addWidget(m_labelCredits);

    layout2->addLayout(bottomLayout);

    m_tabWidget_Course->addTab(tab2, "我的已选课程 (Cart)");

    mainLayout->addWidget(m_tabWidget_Course);
}

// mainwindow.cpp -> setupSchedulePageUi() 最终修复版

void MainWindow::setupSchedulePageUi()
{
    // 1. 创建 Widget
    m_pageSchedule = new QWidget(this);

    // 页面全局样式
    QString darkQss = R"(
        QWidget { background-color: #2b2b2b; color: #e0e0e0; font-family: 'Microsoft YaHei'; }
        QPushButton { border: none; background: transparent; color: #aaaaaa; font-weight: bold; }
        QPushButton:hover { color: #409eff; }
        QPushButton#btnWeek {
            border: 1px solid #409eff; border-radius: 4px; color: #409eff; padding: 4px 12px;
        }
        QPushButton#btnWeek:hover { background-color: #409eff; color: white; }
    )";
    m_pageSchedule->setStyleSheet(darkQss);
    ui->stack_student->addWidget(m_pageSchedule);

    // 2. 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(m_pageSchedule);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 3. 顶部栏
    QHBoxLayout *topLayout = new QHBoxLayout();

    QPushButton *btnReturn = new QPushButton("← 返回首页", m_pageSchedule);
    btnReturn->setCursor(Qt::PointingHandCursor);
    btnReturn->setFont(QFont("Microsoft YaHei", 12, QFont::Bold));
    connect(btnReturn, &QPushButton::clicked, this, [=](){
        ui->stack_student->setCurrentIndex(0);
    });

    QPushButton *btnPrev = new QPushButton("◀ 上一周", m_pageSchedule);
    btnPrev->setObjectName("btnWeek");
    btnPrev->setCursor(Qt::PointingHandCursor);

    QPushButton *btnNext = new QPushButton("下一周 ▶", m_pageSchedule);
    btnNext->setObjectName("btnWeek");
    btnNext->setCursor(Qt::PointingHandCursor);

    m_lblDateRange = new QLabel("2024.xx.xx - 2024.xx.xx", m_pageSchedule);
    m_lblDateRange->setStyleSheet("font-size: 16px; font-weight: bold; color: white; margin: 0 15px;");

    topLayout->addWidget(btnReturn);
    topLayout->addStretch();
    topLayout->addWidget(btnPrev);
    topLayout->addWidget(m_lblDateRange);
    topLayout->addWidget(btnNext);
    topLayout->addStretch();
    QWidget *dummy = new QWidget(); dummy->setFixedWidth(100); topLayout->addWidget(dummy); // 占位保持居中

    mainLayout->addLayout(topLayout);

    // =========================================
    // 4. 核心：课表网格
    // =========================================
    m_tableSchedule = new QTableWidget(12, 7, m_pageSchedule);

    // 【关键修复 1】: 强制在代码层面关闭网格线，解决“双眼皮”问题
    m_tableSchedule->setShowGrid(false);

    // 表头设置
    m_tableSchedule->setHorizontalHeaderLabels({"周一", "周二", "周三", "周四", "周五", "周六", "周日"});
    QStringList rows; for(int i=1; i<=12; i++) rows << QString("第%1节").arg(i);
    m_tableSchedule->setVerticalHeaderLabels(rows);

    m_tableSchedule->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableSchedule->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableSchedule->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableSchedule->setSelectionMode(QAbstractItemView::SingleSelection);

    // 【关键修复 2】: 课表样式表
    m_tableSchedule->setStyleSheet(R"(
        /* 1. 表格整体 */
        QTableWidget {
            background-color: #2b2b2b;
            border: none;
            outline: none;
        }

        /* 2. 表头 (保持不变) */
        QHeaderView::section {
            background-color: #333333;
            color: #aaaaaa;
            border: 1px solid #444444;
            height: 35px;
        }

        /* 3. 单元格 - 普通状态 (关键修改！) */
        QTableWidget::item {
            /* 【核心逻辑】：
               为了防止选中时的 2px 白框残留，我们在普通状态下也定义 2px 的边框。
               上/左：设为透明 (transparent) 用来占位，确保重绘时能擦除白线。
               下/右：设为灰色 (gridline) 用来显示网格。
            */
            border-top: 2px solid transparent;
            border-left: 2px solid transparent;
            border-bottom: 1px solid #444444;
            border-right: 1px solid #444444;

            padding: 2px;
        }

        /* 4. 单元格 - 选中状态 */
        QTableWidget::item:selected {
            /* 选中时，边框变成 2px 白色实线 */
            border: 2px solid #ffffff;
            background-color: #2b2b2b;
            color: white;
        }

        /* 5. 单元格 - 焦点状态 */
        QTableWidget::item:focus {
            outline: none;
            border: 2px solid #ffffff;
        }
    )");

    mainLayout->addWidget(m_tableSchedule);

    // 5. 绑定信号
    connect(btnPrev, &QPushButton::clicked, this, &MainWindow::onPrevWeek);
    connect(btnNext, &QPushButton::clicked, this, &MainWindow::onNextWeek);
    connect(m_tableSchedule, &QTableWidget::cellClicked, this, &MainWindow::showCourseDetail);

    // 初始化
    m_currentMonday = QDate::currentDate().addDays(1 - QDate::currentDate().dayOfWeek());
}

void MainWindow::onPrevWeek() {
    m_currentMonday = m_currentMonday.addDays(-7);
    updateScheduleTable();
}

void MainWindow::onNextWeek() {
    m_currentMonday = m_currentMonday.addDays(7);
    updateScheduleTable();
}

// mainwindow.cpp

void MainWindow::updateScheduleTable()
{
    // 1. 更新顶部日期显示
    QDate currentSunday = m_currentMonday.addDays(6);
    m_lblDateRange->setText(QString("%1 - %2").arg(m_currentMonday.toString("yyyy.MM.dd"), currentSunday.toString("yyyy.MM.dd")));

    // 2. 清空表格内容 (保留表头)
    m_tableSchedule->clearContents();

    // 3. 【核心修改】调用 DataManager 获取数据，不直接写 SQL
    DataManager dm;

    // 获取数据列表
    QList<DataManager::ScheduleItem> scheduleList = dm.getWeeklySchedule(m_studentId, m_currentMonday, currentSunday);

    // 4. 遍历数据并填表
    for (const auto &item : scheduleList) {
        // 计算坐标
        // 列：周一=0 ... 周日=6
        int col = item.date.dayOfWeek() - 1;
        // 行：第1节=0 ... 第12节=11
        int row = item.session - 1;

        // 越界检查 (防止数据错误导致崩溃)
        if (col < 0 || col > 6 || row < 0 || row > 11) continue;

        // 创建显示内容
        QString displayText = QString("%1\n@%2").arg(item.courseName, item.room);
        QTableWidgetItem *uiItem = new QTableWidgetItem(displayText);

        uiItem->setTextAlignment(Qt::AlignCenter);
        uiItem->setBackground(QColor("#409eff")); // 蓝色背景
        uiItem->setForeground(Qt::white);         // 白色文字

        // 存详情数据 (供点击查看)
        QString detailText = QString("课程：%1\n老师：%2\n时间：%3 (周%4) 第%5节\n地点：%6")
                                 .arg(item.courseName,
                                      item.teacherName,
                                      item.date.toString("yyyy-MM-dd"),
                                      QString::number(item.date.dayOfWeek()),
                                      QString::number(item.session),
                                      item.room);

        uiItem->setData(Qt::UserRole, detailText);

        m_tableSchedule->setItem(row, col, uiItem);
    }
}

void MainWindow::showCourseDetail(int row, int col) {
    QTableWidgetItem *item = m_tableSchedule->item(row, col);
    if(item) QMessageBox::information(this, "课程详情", item->data(Qt::UserRole).toString());
}

void MainWindow::setupGradePageUi()
{
    // 1. 创建页面容器
    m_pageGrade = new QWidget(this);
    ui->stack_student->addWidget(m_pageGrade); // 加入 Stack

    // 2. 全局暗色 QSS (和之前一样，保持风格统一)
    QString darkQss = R"(
        QWidget { background-color: #2b2b2b; color: #e0e0e0; font-family: 'Microsoft YaHei'; }
        QPushButton { border: none; background: transparent; color: #aaaaaa; font-weight: bold; }
        QPushButton:hover { color: #409eff; }

        QComboBox {
            background-color: #3a3a3a; border: 1px solid #555555; border-radius: 4px; padding: 4px 10px; color: white;
        }
        QComboBox::drop-down { border: none; }
        QComboBox QAbstractItemView { background-color: #3a3a3a; color: white; selection-background-color: #409eff; }
    )";
    m_pageGrade->setStyleSheet(darkQss);

    // 3. 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(m_pageGrade);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);

    // ================= Top Bar =================
    QHBoxLayout *topLayout = new QHBoxLayout();

    // 返回按钮
    QPushButton *btnReturn = new QPushButton("← 返回首页", m_pageGrade);
    btnReturn->setCursor(Qt::PointingHandCursor);
    btnReturn->setFont(QFont("Microsoft YaHei", 12, QFont::Bold));
    connect(btnReturn, &QPushButton::clicked, this, [=](){
        ui->stack_student->setCurrentIndex(0);
    });

    // 学期选择
    QLabel *lblTerm = new QLabel("选择学期：", m_pageGrade);
    m_comboGradeTerm = new QComboBox(m_pageGrade);
    m_comboGradeTerm->setFixedWidth(200);
    m_comboGradeTerm->setCursor(Qt::PointingHandCursor);

    // 查询按钮
    QPushButton *btnQuery = new QPushButton("查询成绩", m_pageGrade);
    btnQuery->setCursor(Qt::PointingHandCursor);
    btnQuery->setStyleSheet(
        "QPushButton { background-color: #409eff; color: white; border-radius: 4px; padding: 6px 20px; }"
        "QPushButton:hover { background-color: #66b1ff; }"
        );

    // 连接查询信号
    connect(btnQuery, &QPushButton::clicked, this, &MainWindow::updateGradeTable);
    // 切换学期自动查询
    connect(m_comboGradeTerm, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::updateGradeTable);

    topLayout->addWidget(btnReturn);
    topLayout->addStretch();
    topLayout->addWidget(lblTerm);
    topLayout->addWidget(m_comboGradeTerm);
    topLayout->addWidget(btnQuery);

    mainLayout->addLayout(topLayout);

    // ================= Grade Table =================
    m_tableGrade = new QTableWidget(m_pageGrade);

    // 表头: 课程名, 教师, 学分, 成绩, 绩点
    QStringList headers = {"课程名称", "任课教师", "学分", "成绩", "单科绩点"};
    m_tableGrade->setColumnCount(5);
    m_tableGrade->setHorizontalHeaderLabels(headers);

    // 样式设置 (使用修复了残留问题的 QSS)
    m_tableGrade->setShowGrid(false); // 关掉默认网格
    m_tableGrade->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableGrade->verticalHeader()->setVisible(false);
    m_tableGrade->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableGrade->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_tableGrade->setStyleSheet(R"(
        QTableWidget { background-color: #2b2b2b; border: none; outline: none; }
        QHeaderView::section { background-color: #333333; color: #aaaaaa; border: 1px solid #444444; height: 38px; }
        QTableWidget::item {
            border-top: 2px solid transparent;
            border-left: 2px solid transparent;
            border-bottom: 1px solid #444444;
            border-right: 1px solid #444444;
            padding: 5px;
        }
        QTableWidget::item:selected {
            border: 2px solid #409eff; /* 成绩页用蓝色边框好看 */
            background-color: #2b2b2b;
            color: white;
        }
        QTableWidget::item:focus { outline: none; border: 2px solid #409eff; }
    )");

    mainLayout->addWidget(m_tableGrade);

    // ================= Bottom Summary =================
    QHBoxLayout *bottomLayout = new QHBoxLayout();

    // 成绩汇总卡片
    m_lblGradeSummary = new QLabel("暂无数据", m_pageGrade);
    m_lblGradeSummary->setStyleSheet(
        "font-size: 16px; font-weight: bold; color: #67c23a; " // 绿色文字
        "border: 1px solid #555555; border-radius: 6px; padding: 10px 20px; background-color: #333333;"
        );

    bottomLayout->addStretch();
    bottomLayout->addWidget(m_lblGradeSummary);

    mainLayout->addLayout(bottomLayout);
}

void MainWindow::updateGradeTable()
{
    // 1. 获取参数
    int termId = m_comboGradeTerm->currentData().toInt();
    int studentId = m_studentId;

    // 2. 查库
    DataManager dm;
    QList<DataManager::GradeItem> list = dm.getStudentGrades(studentId, termId);

    // 3. 清空表格
    m_tableGrade->setRowCount(0);

    // 4. 定义变量用于计算统计数据
    double totalCredits = 0.0;
    double weightedScoreSum = 0.0;

    // 5. 遍历填表
    for(int i = 0; i < list.size(); ++i) {
        DataManager::GradeItem item = list[i];
        m_tableGrade->insertRow(i);

        // 简单计算一下单科绩点 (这里用标准 4.0 算法示例：分数/10 - 5)
        // 你可以根据学校实际规则修改
        double gpa = (item.score >= 60) ? (item.score / 10.0 - 5.0) : 0.0;
        if(gpa < 0) gpa = 0; // 保护一下

        // 填入数据
        // 0: 课程名
        m_tableGrade->setItem(i, 0, new QTableWidgetItem(item.courseName));
        // 1: 教师
        m_tableGrade->setItem(i, 1, new QTableWidgetItem(item.teacherName));
        // 2: 学分
        m_tableGrade->setItem(i, 2, new QTableWidgetItem(QString::number(item.credit, 'f', 1)));
        // 3: 成绩 (如果是 <60 分，可以标红)
        QTableWidgetItem *scoreItem = new QTableWidgetItem(QString::number(item.score, 'f', 1));
        if(item.score < 60) {
            scoreItem->setForeground(QColor("#f56c6c")); // 挂科红色
        } else {
            scoreItem->setForeground(QColor("#67c23a")); // 通过绿色
        }
        m_tableGrade->setItem(i, 3, scoreItem);

        // 4: 绩点
        m_tableGrade->setItem(i, 4, new QTableWidgetItem(QString::number(gpa, 'f', 2)));

        // 居中对齐
        for(int k=0; k<5; k++) {
            if(m_tableGrade->item(i, k))
                m_tableGrade->item(i, k)->setTextAlignment(Qt::AlignCenter);
        }

        // 累加统计数据
        totalCredits += item.credit;
        weightedScoreSum += (item.score * item.credit);
    }

    // 6. 更新底部汇总条
    if (totalCredits > 0) {
        double avgScore = weightedScoreSum / totalCredits;
        m_lblGradeSummary->setText(QString("本学期总学分: %1   |   加权平均分: %2")
                                       .arg(totalCredits)
                                       .arg(QString::number(avgScore, 'f', 2)));
    } else {
        m_lblGradeSummary->setText("本学期暂无成绩记录");
    }
}

void MainWindow::performRestart()
{
    // 1. 获取当前程序的执行路径 (就是你的 exe 全路径)
    QString program = QApplication::applicationFilePath();

    // 2. 获取当前运行的参数 (如果有命令行参数，也一并传给新的实例)
    QStringList arguments = QApplication::arguments();

    // 3. 获取当前的工作目录
    QString workingDirectory = QDir::currentPath();

    // 4. 启动一个新的实例 (Detached 模式，与当前进程分离)
    QProcess::startDetached(program, arguments, workingDirectory);

    // 5. 退出当前程序
    QApplication::exit();
}

void MainWindow::initMenuConnections(){
    connect(ui->actionRestart, &QAction::triggered, this, [=](){
        performRestart();
    });

    connect(ui->actionExit, &QAction::triggered, this, [=](){
        QApplication::exit();
    });
}

// mainwindow.cpp

void MainWindow::loadSemesterData()
{
    DataManager dm;
    QList<DataManager::SemesterItem> semesters = dm.getAllSemesters();

    // 1. 给【选课中心】的下拉框填数据
    if (m_comboTerm) {
        m_comboTerm->clear();
        for (const auto &item : semesters) {
            m_comboTerm->addItem(item.displayText, item.id);
        }
        // 默认选中最新的学期 (第0个)
        if (!semesters.isEmpty()) m_comboTerm->setCurrentIndex(0);
    }

    // 2. 给【成绩查询】的下拉框填数据
    if (m_comboGradeTerm) {
        m_comboGradeTerm->clear();
        for (const auto &item : semesters) {
            m_comboGradeTerm->addItem(item.displayText, item.id);
        }
        if (!semesters.isEmpty()) m_comboGradeTerm->setCurrentIndex(0);
    }

    // 如果以后还有【我的课表】的学期选择，也可以加在这里
}

void MainWindow::setupInfoPageUi()
{
    // 1. 创建页面容器
    m_pageInfo = new QWidget(this);
    ui->stack_student->addWidget(m_pageInfo);

    // 2. 页面样式
    QString darkQss = R"(
        QWidget { background-color: #2b2b2b; color: #e0e0e0; font-family: 'Microsoft YaHei'; }

        /* 标题样式 */
        QLabel#lblTitle {
            font-size: 26px; font-weight: bold; color: #409eff;
            margin-bottom: 10px;
        }

        /* 字段名 (Key) */
        QLabel#lblKey {
            font-size: 16px; color: #aaaaaa;
            background-color: transparent; /* 确保背景透明 */
        }

        /* 字段值 (Value) - 【关键修复】 */
        QLabel#lblVal {
            font-size: 18px; font-weight: bold; color: white;
            padding: 5px;
            background-color: transparent; /* 去掉“黑条”背景 */
            border: none;                  /* 去掉边框，纯文字显示 */
        }

        /* 卡片容器 */
        QWidget#infoCard {
            background-color: #333333;
            border-radius: 12px;
            border: 1px solid #444444;
        }

        /* 返回按钮 */
        QPushButton { border: none; background: transparent; color: #aaaaaa; font-weight: bold; font-size: 14px; }
        QPushButton:hover { color: #409eff; }
    )";
    m_pageInfo->setStyleSheet(darkQss);

    // 3. 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(m_pageInfo);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    // === 【新增】顶部栏：返回按钮 ===
    QHBoxLayout *topLayout = new QHBoxLayout();
    QPushButton *btnReturn = new QPushButton("← 返回首页", m_pageInfo);
    btnReturn->setCursor(Qt::PointingHandCursor);

    // 连接返回功能
    connect(btnReturn, &QPushButton::clicked, this, [=](){
        ui->stack_student->setCurrentIndex(0);
    });

    topLayout->addWidget(btnReturn);
    topLayout->addStretch(); // 弹簧，把按钮顶在左边
    mainLayout->addLayout(topLayout);

    // === 中间内容区域 ===
    mainLayout->addStretch(); // 顶部的弹簧，让卡片垂直居中

    // 创建卡片
    QWidget *card = new QWidget(m_pageInfo);
    card->setObjectName("infoCard");
    card->setFixedSize(550, 350); //稍微调宽一点，比例更好看

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(50, 40, 50, 40);
    cardLayout->setSpacing(10);

    // 卡片标题
    QLabel *title = new QLabel("个人信息 | Student Profile", card);
    title->setObjectName("lblTitle");
    title->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(title);

    // 分割线 (装饰用)
    QFrame *line = new QFrame(card);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #555555;");
    cardLayout->addWidget(line);
    cardLayout->addSpacing(20);

    // 表单布局
    QGridLayout *formLayout = new QGridLayout();
    formLayout->setHorizontalSpacing(20);
    formLayout->setVerticalSpacing(15);

    // 辅助lambda
    auto createRow = [&](int row, QString key, QLabel* &valLabel) {
        QLabel *k = new QLabel(key, card);
        k->setObjectName("lblKey");
        k->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        valLabel = new QLabel("Loading...", card);
        valLabel->setObjectName("lblVal");
        valLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter); // 左对齐

        formLayout->addWidget(k, row, 0);
        formLayout->addWidget(valLabel, row, 1);
    };

    createRow(0, "姓名 / Name:", m_valName);
    createRow(1, "学号 / ID:", m_valNumber);
    createRow(2, "行政班 / Class:", m_valClass);
    createRow(3, "年级 / Grade:", m_valGrade);

    // 调整列宽比例，让Key占少一点，Value占多一点
    formLayout->setColumnStretch(0, 4);
    formLayout->setColumnStretch(1, 6);

    cardLayout->addLayout(formLayout);
    cardLayout->addStretch();

    // 把卡片加入主布局并居中
    mainLayout->addWidget(card, 0, Qt::AlignCenter);

    mainLayout->addStretch(); // 底部的弹簧
}

void MainWindow::loadStudentInfo()
{
    DataManager dm;
    DataManager::StudentPersonalInfor info = dm.getStudentInfo(m_studentId);

    // 填入数据
    m_valName->setText(info.name);
    m_valNumber->setText(info.number);
    m_valClass->setText(info.className.isEmpty() ? "未分配班级" : info.className);
    m_valGrade->setText(info.grade);
}
