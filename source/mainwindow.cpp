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
#include <QInputDialog> // 必须引入这个头文件用于输入成绩

#include <QDir>

#include "consoledialog.h"

#include <QFormLayout>       // 解决 'QFormLayout' was not declared
#include <QDialogButtonBox>  // 解决 'QDialogButtonBox' 报错
#include <QMessageBox>       // 解决弹窗报错
#include <QLineEdit>         // 确保输入框正常
#include <QTableWidget>      // 确保表格正常

#include <QGroupBox>

#include <QSpinBox>   // 修复 QSpinBox 报错
#include <QDateEdit>  // 修复 QDateEdit 报错
#include <QTimeEdit>  // 修复 QTimeEdit 报错
#include <QLineEdit>
#include <QComboBox>
#include <QGroupBox>
#include <QFormLayout>

#include <QDesktopServices> // 用於打開瀏覽器
#include <QUrl>             // 用於處理網址

int if_admin = 0;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    //细节初始化

    ui->setupUi(this);
    this->setWindowTitle("教务管理系统");
    ui->actionLog_out->setEnabled(false);

    ui->label_admin_title->setVisible(false);

    ui->lineEdit_Account->setEnabled(false);//细节要求先选择身份再输入密码
    ui->lineEdit_Psw->setEnabled(false);
    initRoleAnimation();
    ui->lineEdit_Psw->setEchoMode(QLineEdit::Password);

    ui->loginCard->setFocus();//细节让焦点在背景，这样就能显示输入框的提示

    qApp->installEventFilter(this);

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
    ui->listWidget_admin->setVisible(false);
    ui->mainUi->setCurrentIndex(0);

    //身份选择按钮 逻辑
    m_roleGroup = new QButtonGroup(this); // 设置按钮组
    m_roleGroup->addButton(ui->login_student, RoleStudent);
    m_roleGroup->addButton(ui->login_teacher, RoleTeacher);
    m_roleGroup->addButton(ui->login_admin,   RoleAdmin);
    m_roleGroup->setExclusive(true);//按钮互斥
    connect(m_roleGroup, &QButtonGroup::buttonClicked, this, [=](QAbstractButton *clickedBtn){
        int newRole = m_roleGroup->id(clickedBtn);
        m_currentRole = newRole;

        // 用这个新 ID 去刷新动画和文字，必须要先构建ui，这样就不会出现空指针导致程序闪退了，むべむべ
        switchRoleAnimation(m_currentRole);

        ui->lineEdit_Account->setEnabled(true);
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
        // 获取账号
        QString acc = ui->lineEdit_Account->text().trimmed();
        // 获取密码
        QString pwd = ui->lineEdit_Psw->text();

        // 判断：两个都不为空
        bool isOk = !acc.isEmpty() && !pwd.isEmpty();

        // 设置按钮状态
        ui->log_in->setEnabled(isOk);
    };

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
            if (role == RoleStudent) {
                m_studentId = dm.getStudentIdByAccount(account);
                qDebug() << "当前登录学生ID:" << m_studentId;
            }

            else if (role == RoleTeacher) {
                // 获取教师ID并存入 m_studentId (代码复用了这个变量名)
                m_studentId = dm.getTeacherIdByAccount(account);
                qDebug() << "当前教师ID:" << m_studentId;
            }

            ui->log_studentinfo_button->setText("已登录: " + account);
            ui->log_teacherinfo_button->setText("已登录: " + account);
            switch (role) {
            case RoleStudent:
                ui->mainUi->setCurrentIndex(2);
                break;
            case RoleTeacher:
                ui->mainUi->setCurrentIndex(3);
                break;
            case RoleAdmin:
                ui->mainUi->setCurrentIndex(4);
                if_admin = 1;
                ui->listWidget_admin->setVisible(true);
                ui->label_admin_title->setVisible(false);
                setupAdminPageUi();
                break;
            }
        }
        else {
            QMessageBox::warning(this, "登录失败", "账号或密码错误，请检查输入！");
            ui->lineEdit_Psw->clear(); // 清空密码让用户重输
            ui->label_3->setText("账号或密码错误");
        }
    });

    //enter登录
    connect(ui->lineEdit_Psw, &QLineEdit::returnPressed, ui->log_in, &QPushButton::click);
    connect(ui->lineEdit_Account, &QLineEdit::returnPressed, ui->log_in, &QPushButton::click);


    connect(ui->pushButton_enter, &QPushButton::clicked, this, [=](){
        ui->stackedTeacher->setCurrentIndex(1);
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
            info = "删除学生(已选中" + QString::number(count) + " Rows)";
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

    setupStudentSchedulePageUi();

    setupStudentGradePageUi();

    initMenuConnections();

    setupStudentInfoPageUi();

    //老师相关页面的函数
    initTeacherNavigation();

    setupTeacherCoursePageUi();

    setupTeacherGradingPageUi();

    setupTeacherInfoPageUi();

    loadSemesterData();


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
    connect(m_tableAvailable, &QTableWidget::cellClicked, this, [=](int row, int col){
        // 假设第5列是按钮，点击按钮不弹详情，点击其他列弹详情
        if (col == 5) return;

        // 获取 section_id (我们在刷新表格时会把它存在第0列或UserRole里)
        // 假设你在 refreshCourseTables 里 setItem(i, 0, ...) 存了 ID
        int sectionId = m_tableAvailable->item(row, 0)->text().toInt();

        DataManager dm;
        QString details = dm.getCourseDescription(sectionId);

        QMessageBox::information(this, "课程详细信息", details);
    });

}
void MainWindow::refreshCourseTables()
{
    // 1. 獲取當前選擇的參數
    // (注意: currentData 可能為空，加個校驗)
    if (m_comboTerm->count() == 0) return;

    int currentTermId = m_comboTerm->currentData().toInt();
    int studentId = m_studentId;
    DataManager dm; // 實例化數據管理器

    // ===========================================
    // Part A: 刷新 Tab 1 (可選課程 / 選課中心)
    // ===========================================
    m_tableAvailable->setRowCount(0); // 清空舊數據

    // 獲取課程列表 (返回結構: ID, 名稱, 教師, 學分, 人數信息)
    QList<QStringList> listAvail = dm.getAvailableCourses(currentTermId);

    for(int i = 0; i < listAvail.size(); ++i) {
        QStringList rowData = listAvail[i];
        m_tableAvailable->insertRow(i);

        // 填入文本數據 (前5列)
        for(int k=0; k<5; k++) {
            QTableWidgetItem *item = new QTableWidgetItem(rowData[k]);
            item->setTextAlignment(Qt::AlignCenter);
            m_tableAvailable->setItem(i, k, item);
        }

        // --- 核心邏輯：選課按鈕 ---
        QPushButton *btnEnroll = new QPushButton("選課");
        btnEnroll->setCursor(Qt::PointingHandCursor);
        btnEnroll->setStyleSheet(
            "QPushButton { background-color: #67c23a; color: white; border: none; border-radius: 4px; font-weight: bold; padding: 5px; }"
            "QPushButton:hover { background-color: #85ce61; }"
            "QPushButton:pressed { background-color: #5daf34; }"
        );

        // 獲取課程 ID (第0列是 ID)
        int sectionId = rowData[0].toInt();

        // 連接點擊信號
        connect(btnEnroll, &QPushButton::clicked, this, [=](){
            DataManager tempDm; // Lambda 內使用局部實例

            // ---------------------------------------------------------
            // 【步驟 1】: 資格檢查 (學期限制 + 遞歸先修課檢查)
            // ---------------------------------------------------------
            auto eligibility = tempDm.checkEnrollmentEligibility(studentId, sectionId, currentTermId);

            if (!eligibility.allowed) {
                // 檢查不通過，根據原因彈窗
                if (!eligibility.missingPrereqs.isEmpty()) {
                    // --- 展示拓撲排序路徑 (Learning Path) ---
                    // 將缺失的課程列表拼接成 A -> B -> C 的形式
                    QString path = eligibility.missingPrereqs.join(" -> ");

                    // 獲取當前課程名稱方便顯示
                    QString currentCourseName = m_tableAvailable->item(i, 1)->text();

                    QString msg = QString("無法選課！您未滿足先修要求。\n\n"
                                          "📚 推薦學習路徑 (Topology Order):\n"
                                          "%1 -> [%2]\n\n"
                                          "請按照順序先完成上述紅色課程的學習。")
                                          .arg(path, currentCourseName);

                    QMessageBox::warning(this, "先修課未滿足", msg);
                } else {
                    // 其他原因 (如：不是最新學期)
                    QMessageBox::warning(this, "選課受限", eligibility.message);
                }
                return; // 阻止後續選課邏輯
            }

            // ---------------------------------------------------------
            // 【步驟 2】: 執行選課 (查重、查容量、查時間衝突)
            // ---------------------------------------------------------
            EnrollResult result = tempDm.enrollCourse(studentId, sectionId);

            switch (result) {
            case EnrollSuccess:
                QMessageBox::information(this, "恭喜", "選課成功！");
                refreshCourseTables(); // 刷新界面顯示最新狀態
                break;

            case AlreadyEnrolled:
                QMessageBox::warning(this, "提示", "您已經選修過這門課程了，請勿重複選擇。");
                break;

            case TimeConflict:
                QMessageBox::warning(this, "失敗", "檢測到時間衝突！\n該課程的上課時間與您已選的課程重疊。");
                break;

            case ClassFull:
                QMessageBox::warning(this, "失敗", "手慢了！該課程名額已滿。");
                break;

            case DatabaseError:
                QMessageBox::critical(this, "錯誤", "系統內部錯誤，請聯繫管理員。");
                break;
            }
        });

        // 將按鈕放入表格第 5 列
        m_tableAvailable->setCellWidget(i, 5, btnEnroll);
    }

    // ===========================================
    // Part B: 刷新 Tab 2 (我的已選課程)
    // ===========================================
    m_tableMyCourses->setRowCount(0);
    QList<QStringList> listMy = dm.getMyCourses(studentId, currentTermId);

    double totalCredits = 0.0;

    for(int i = 0; i < listMy.size(); ++i) {
        QStringList rowData = listMy[i];
        m_tableMyCourses->insertRow(i);

        // 填入數據 (ID, 名稱, 教師, 學分)
        for(int k=0; k<4; k++) {
            QTableWidgetItem *item = new QTableWidgetItem(rowData[k]);
            item->setTextAlignment(Qt::AlignCenter);
            m_tableMyCourses->setItem(i, k, item);
        }

        // 累加學分 (第3列是學分)
        totalCredits += rowData[3].toDouble();

        // --- 退課按鈕 ---
        QPushButton *btnDrop = new QPushButton("退課");
        btnDrop->setCursor(Qt::PointingHandCursor);
        btnDrop->setStyleSheet(
            "QPushButton { background-color: #f56c6c; color: white; border: none; border-radius: 4px; font-weight: bold; padding: 5px; }"
            "QPushButton:hover { background-color: #ff7875; }"
            "QPushButton:pressed { background-color: #dd6161; }"
        );

        int sectionId = rowData[0].toInt();
        connect(btnDrop, &QPushButton::clicked, this, [=](){
            QString courseName = m_tableMyCourses->item(i, 1)->text();
            if(QMessageBox::Yes == QMessageBox::question(this, "確認退課",
                QString("確定要退掉 [%1] 嗎？\n名額釋放後可能無法重新搶到。").arg(courseName)))
            {
                DataManager tempDm;
                if(tempDm.dropCourse(studentId, sectionId)) {
                    QMessageBox::information(this, "成功", "退課成功。");
                    refreshCourseTables(); // 刷新
                } else {
                    QMessageBox::critical(this, "失敗", "退課失敗，請檢查網絡。");
                }
            }
        });

        m_tableMyCourses->setCellWidget(i, 4, btnDrop);
    }

    // 更新底部學分統計
    m_labelCredits->setText(QString("當前學期總學分: %1").arg(totalCredits));
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

void MainWindow::setupStudentSchedulePageUi() //课表的方格点太快会有残留的白色横线bug，どうして
{
    // 1. 創建頁面容器 Widget
    m_pageSchedule = new QWidget(this);

    // 頁面全局樣式 (Dark Mode)
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

    // 2. 主佈局
    QVBoxLayout *mainLayout = new QVBoxLayout(m_pageSchedule);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 3. 頂部欄 (返回按鈕、日期切換)
    QHBoxLayout *topLayout = new QHBoxLayout();

    QPushButton *btnReturn = new QPushButton("← 返回首頁", m_pageSchedule);
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

    // 佔位控件，保持中間內容居中
    QWidget *dummy = new QWidget();
    dummy->setFixedWidth(100);
    topLayout->addWidget(dummy);

    mainLayout->addLayout(topLayout);

    // =========================================
    // 4. 核心：課表網格
    // =========================================
    m_tableSchedule = new QTableWidget(12, 7, m_pageSchedule);

    // 關閉默認網格線 (解決雙眼皮問題)
    m_tableSchedule->setShowGrid(false);

    // --- 【核心修改開始】：設置帶時間的表頭 ---

    // 設置列頭 (週一 ~ 週日)
    m_tableSchedule->setHorizontalHeaderLabels({"週一", "週二", "週三", "週四", "週五", "週六", "週日"});

    // 設置行頭 (第X節 + 時間段)
    QStringList rowLabels;
    for(int i = 1; i <= 12; i++) {
        // 調用 DataManager 獲取時間段字符串
        QString timeStr = DataManager::getPeriodTimeLabel(i);

        if (timeStr.isEmpty()) {
            rowLabels << QString("第%1節").arg(i);
        } else {
            // 使用 \n 換行顯示
            rowLabels << QString("第%1節\n%2").arg(i).arg(timeStr);
        }
    }
    m_tableSchedule->setVerticalHeaderLabels(rowLabels);

    // --- 【核心修改結束】 ---

    // 設置 Resize Mode
    m_tableSchedule->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableSchedule->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 【重要】設置默認行高，確保兩行文字能完整顯示
    m_tableSchedule->verticalHeader()->setDefaultSectionSize(55);

    // 禁止編輯和選擇模式設置
    m_tableSchedule->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableSchedule->setSelectionMode(QAbstractItemView::SingleSelection);

    // 課表樣式表
    m_tableSchedule->setStyleSheet(R"(
        /* 1. 表格整體 */
        QTableWidget {
            background-color: #2b2b2b;
            border: none;
            outline: none;
        }

        /* 2. 表頭 */
        QHeaderView::section {
            background-color: #333333;
            color: #aaaaaa;
            border: 1px solid #444444;
            height: 45px; /* 稍微增高表頭以適應內容 */
        }

        /* 3. 單元格 - 普通狀態 */
        QTableWidget::item {
            /* 邊框技巧：用透明邊框佔位，避免選中時抖動 */
            border-top: 2px solid transparent;
            border-left: 2px solid transparent;
            border-bottom: 1px solid #444444;
            border-right: 1px solid #444444;
            padding: 2px;
        }

        /* 4. 單元格 - 選中狀態 */
        QTableWidget::item:selected {
            border: 2px solid #ffffff;
            background-color: #2b2b2b;
            color: white;
        }

        /* 5. 單元格 - 焦點狀態 */
        QTableWidget::item:focus {
            outline: none;
            border: 2px solid #ffffff;
        }
    )");

    mainLayout->addWidget(m_tableSchedule);

    // 5. 綁定信號
    connect(btnPrev, &QPushButton::clicked, this, &MainWindow::onPrevWeek);
    connect(btnNext, &QPushButton::clicked, this, &MainWindow::onNextWeek);
    connect(m_tableSchedule, &QTableWidget::cellClicked, this, &MainWindow::showCourseDetail);

    // 初始化當前週一日期
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

    // 2. 清空表格 (内容和跨行设置)
    m_tableSchedule->clearContents();
    m_tableSchedule->clearSpans(); // 【重要】清除之前的合并单元格，防止错位

    // 3. 获取数据
    DataManager dm;
    QList<DataManager::ScheduleItem> scheduleList = dm.getWeeklySchedule(m_studentId, m_currentMonday, currentSunday);

    // 4. 遍历填表
    for (const auto &item : scheduleList) {
        // 计算坐标
        int col = item.date.dayOfWeek() - 1; // 周一=0 ...

        // 【修正】使用 periodStart 计算行号 (数据库存的是 1-based, 表格是 0-based)
        // 确保你的 DataManager::ScheduleItem 结构体里有 periodStart 和 periodEnd
        int startRow = item.periodStart - 1;
        int endRow = item.periodEnd - 1;

        // 越界保护
        if (col < 0 || col > 6 || startRow < 0 || startRow > 11) continue;
        if (endRow > 11) endRow = 11;

        // 计算跨度
        int spanHeight = endRow - startRow + 1;
        if (spanHeight < 1) spanHeight = 1;

        // 创建显示内容
        QString displayText = QString("%1\n@%2").arg(item.courseName, item.room);
        QTableWidgetItem *uiItem = new QTableWidgetItem(displayText);

        uiItem->setTextAlignment(Qt::AlignCenter);
        uiItem->setBackground(QColor("#409eff")); // 蓝色背景
        uiItem->setForeground(Qt::white);         // 白色文字

        // 【修正】生成详情数据 (使用 periodStart)
        QString timeStr = QString("周%1 第%2-%3节")
                              .arg(item.date.dayOfWeek())
                              .arg(item.periodStart)
                              .arg(item.periodEnd);

        QString detailText = QString("课程名称：%1\n任课教师：%2\n上课时间：%3\n上课地点：%4\n日期：%5")
                                 .arg(item.courseName)
                                 .arg(item.teacherName)
                                 .arg(timeStr)
                                 .arg(item.room)
                                 .arg(item.date.toString("yyyy-MM-dd"));

        // 将详情存入 UserRole
        uiItem->setData(Qt::UserRole, detailText);

        // 【关键】设置 Item
        m_tableSchedule->setItem(startRow, col, uiItem);

        // 【关键】如果有跨节（比如2节连堂），合并单元格
        if (spanHeight > 1) {
            m_tableSchedule->setSpan(startRow, col, spanHeight, 1);
        }
    }
}

void MainWindow::showCourseDetail(int row, int col) {
    QTableWidgetItem *item = m_tableSchedule->item(row, col);

    // 【核心修复】如果点击的是空白处（item为空），需要检查是不是因为“跨行”导致的
    // 尝试向上查找非空 item (因为 setSpan 的主 item 都在最上面)
    if (!item) {
        // 向上最多查 11 行 (假设最大跨度不会超过整个课表)
        for (int r = row - 1; r >= 0; r--) {
            // 检查 (r, col) 是否有 item
            QTableWidgetItem *prevItem = m_tableSchedule->item(r, col);
            if (prevItem) {
                // 检查这个 item 是否跨行覆盖到了当前的 row
                int spanH = m_tableSchedule->rowSpan(r, col);
                if (r + spanH > row) {
                    // 找到了！就是这个 item 覆盖了当前点击的位置
                    item = prevItem;
                    break;
                } else {
                    // 遇到了另一个不相关的 item，说明上面没有覆盖下来的了
                    break;
                }
            }
        }
    }

    // 如果找到了 item，且有数据，则弹窗
    if (item && !item->data(Qt::UserRole).toString().isEmpty()) {
        QMessageBox::information(this, "课程详情", item->data(Qt::UserRole).toString());
    }
}

void MainWindow::setupStudentGradePageUi()
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

void MainWindow::performRestart()   //实现restart按钮，某种意义上的退出登录，ふんふん
{

    QString program = QApplication::applicationFilePath();

    QStringList arguments = QApplication::arguments();

    QString workingDirectory = QDir::currentPath();

    QProcess::startDetached(program, arguments, workingDirectory);

    QApplication::exit();
}

void MainWindow::initMenuConnections(){
    connect(ui->actionRestart, &QAction::triggered, this, [=](){
        performRestart();
    });

    connect(ui->actionExit, &QAction::triggered, this, [=](){
        QApplication::exit();
    });

    connect(ui->actionUser_manual, &QAction::triggered, this, [=](){

            QString link = "https://github.com/bleatsheep/course-choosing-database/blob/main/README.md";
            QDesktopServices::openUrl(QUrl(link));
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
        for (const auto &item : semesters) {        //どうぞ忽略这个warningございます，我就是需要深拷贝这个遍历中的变量だから
            m_comboTerm->addItem(item.displayText, item.id);
        }
        // 默认选中最新的学期 (第0个)
        if (!semesters.isEmpty()) m_comboTerm->setCurrentIndex(0);
    }

    // 2. 给【成绩查询】的下拉框填数据
    if (m_comboGradeTerm) {
        m_comboGradeTerm->clear();
        for (const auto &item : semesters) {        //どうぞ忽略这个warningございます，我就是需要深拷贝这个遍历中的变量だから
            m_comboGradeTerm->addItem(item.displayText, item.id);
        }
        if (!semesters.isEmpty()) m_comboGradeTerm->setCurrentIndex(0);
    }
    //给教师授课查询的下拉框填数据
    if (m_comboTeacherTerm) {
        m_comboTeacherTerm->clear();
        for (const auto &item : semesters) {        //どうぞ忽略这个warningございます，我就是需要深拷贝这个遍历中的变量だから
            m_comboTeacherTerm->addItem(item.displayText, item.id);
        }
        if (!semesters.isEmpty()) m_comboTeacherTerm->setCurrentIndex(0);
    }
    //给教师登记成绩的下拉框填数据
    if (m_comboGradingTerm) {
        m_comboGradingTerm->clear();
        for (const auto &item : semesters) {        //どうぞ忽略这个warningございます，我就是需要深拷贝这个遍历中的变量だから
            m_comboGradingTerm->addItem(item.displayText, item.id);
        }
        if (!semesters.isEmpty()) m_comboGradingTerm->setCurrentIndex(0);
    }

    // 如果以后还有【我的课表】的学期选择，也可以加在这里
}

void MainWindow::setupStudentInfoPageUi()
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

        /* 字段值 (Value)  */
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

void MainWindow::initTeacherNavigation()
{
    // 1. 教授课程按钮 -> 跳转到课程概览页
    connect(ui->btn_TeachingCourse, &QToolButton::clicked, this, [=](){

        // 切换页面
        ui->stackedTeacher->setCurrentWidget(m_pageTeachingCourse);

        // 刷新数据：加载“我的课程列表” (左侧表格)
        updateTeacherCourseTable();

        qDebug() << "进入教授课程页面";
    });

    // 2. 登记成绩按钮 -> 跳转到打分页
    connect(ui->btn_Grading, &QToolButton::clicked, this, [=](){

        // 切换页面
        ui->stackedTeacher->setCurrentWidget(m_pageGrading);

        // 刷新数据：加载“待打分课程列表” (左侧表格)
        updateGradingCourseTable();

        qDebug() << "进入成绩录入页面";
    });

    // 3. 个人信息按钮 -> 跳转到教师信息页
    connect(ui->btn_Info_teacher, &QToolButton::clicked, this, [=](){

        // 切换页面
        ui->stackedTeacher->setCurrentWidget(m_pageInfoTeacher);

        // 刷新数据：加载教师个人的工号、职称等信息
        loadTeacherInfo();

        qDebug() << "进入个人信息页面";
    });
}

void MainWindow::setupTeacherCoursePageUi()
{
    // === A. 创建容器与基础样式 ===
    m_pageTeachingCourse = new QWidget(this);
    ui->stackedTeacher->addWidget(m_pageTeachingCourse);

    QString darkQss = R"(
        QWidget { background-color: #2b2b2b; color: #e0e0e0; font-family: 'Microsoft YaHei'; }

        /* 表格通用样式 */
        QTableWidget { background-color: #333333; border: none; gridline-color: #444444; }
        QHeaderView::section { background-color: #222222; color: #aaaaaa; padding: 5px; border: none; border-bottom: 2px solid #409eff; }
        QTableWidget::item { padding: 5px; border-bottom: 1px solid #444444; }
        QTableWidget::item:selected { background-color: #409eff; color: white; }

        /* 标题Label */
        QLabel#SectionTitle { font-size: 16px; font-weight: bold; color: #409eff; margin-bottom: 5px; }

        /* 统计Label */
        QLabel#StatsLabel { font-size: 14px; color: #67c23a; font-weight: bold; }

        /* 按钮 */
        QPushButton { border: none; background: transparent; color: #aaaaaa; font-weight: bold; }
        QPushButton:hover { color: #409eff; }
    )";
    m_pageTeachingCourse->setStyleSheet(darkQss);

    // === B. 主布局 (垂直) ===
    QVBoxLayout *mainLayout = new QVBoxLayout(m_pageTeachingCourse);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // --- 顶部导航栏 ---
    QHBoxLayout *topLayout = new QHBoxLayout();
    QPushButton *btnReturn = new QPushButton("← 返回仪表盘", m_pageTeachingCourse);
    btnReturn->setCursor(Qt::PointingHandCursor);
    btnReturn->setFont(QFont("Microsoft YaHei", 12, QFont::Bold));
    connect(btnReturn, &QPushButton::clicked, this, [=](){
        ui->stackedTeacher->setCurrentIndex(1);
    });

    topLayout->addWidget(btnReturn);
    //新增下拉框选择学期
    QLabel *lblTerm = new QLabel("选择学期：", m_pageTeachingCourse);
    m_comboTeacherTerm = new QComboBox(m_pageTeachingCourse);
    m_comboTeacherTerm->setFixedWidth(180);
    m_comboTeacherTerm->setCursor(Qt::PointingHandCursor);

    // 连接信号：切学期时刷新表格
    connect(m_comboTeacherTerm, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::updateTeacherCourseTable);

    topLayout->addWidget(lblTerm);
    topLayout->addWidget(m_comboTeacherTerm);

    topLayout->addStretch();
    mainLayout->addLayout(topLayout);

    // === C. 内容区域 (水平分割：左侧课程表，右侧学生表) ===
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(20);

    // --- 左侧：我的课程列表 ---
    QVBoxLayout *leftLayout = new QVBoxLayout();
    QLabel *lblLeftTitle = new QLabel("我的教学课程 (My Courses)", m_pageTeachingCourse);
    lblLeftTitle->setObjectName("SectionTitle");

    m_tableTeacherCourses = new QTableWidget(m_pageTeachingCourse);
    QStringList headerLeft = {"ID", "课程名称", "时间/地点", "人数", "平均分"};
    m_tableTeacherCourses->setColumnCount(5);
    m_tableTeacherCourses->setHorizontalHeaderLabels(headerLeft);
    m_tableTeacherCourses->setColumnHidden(0, true); // 隐藏ID列
    m_tableTeacherCourses->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableTeacherCourses->verticalHeader()->setVisible(false);
    m_tableTeacherCourses->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableTeacherCourses->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableTeacherCourses->setSelectionMode(QAbstractItemView::SingleSelection);

    // 点击左侧课程，刷新右侧学生
    connect(m_tableTeacherCourses, &QTableWidget::itemClicked, this, [=](QTableWidgetItem *item){
        int row = item->row();
        // 获取隐藏的 ID (第0列)
        int sectionId = m_tableTeacherCourses->item(row, 0)->text().toInt();
        loadCourseStudentList(sectionId);
    });

    leftLayout->addWidget(lblLeftTitle);
    leftLayout->addWidget(m_tableTeacherCourses);

    // --- 右侧：学生名单详情 ---
    QVBoxLayout *rightLayout = new QVBoxLayout();
    QLabel *lblRightTitle = new QLabel("选课学生名单 (Student List)", m_pageTeachingCourse);
    lblRightTitle->setObjectName("SectionTitle");

    m_lblCourseStats = new QLabel("请在左侧选择一门课程查看详情", m_pageTeachingCourse);
    m_lblCourseStats->setObjectName("StatsLabel");

    m_tableCourseStudents = new QTableWidget(m_pageTeachingCourse);
    QStringList headerRight = {"学号", "姓名", "行政班级", "当前成绩"};
    m_tableCourseStudents->setColumnCount(4);
    m_tableCourseStudents->setHorizontalHeaderLabels(headerRight);
    m_tableCourseStudents->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableCourseStudents->verticalHeader()->setVisible(false);
    m_tableCourseStudents->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableCourseStudents->setSelectionBehavior(QAbstractItemView::SelectRows);

    rightLayout->addWidget(lblRightTitle);
    rightLayout->addWidget(m_lblCourseStats);
    rightLayout->addWidget(m_tableCourseStudents);

    // 将左右布局加入内容布局 (比例 4:6)
    contentLayout->addLayout(leftLayout, 4);
    contentLayout->addLayout(rightLayout, 6);

    mainLayout->addLayout(contentLayout);
}

// 2. 刷新左侧课程列表的逻辑
void MainWindow::updateTeacherCourseTable()
{
    int termId = m_comboTeacherTerm->currentData().toInt();

    DataManager dm;
    QList<DataManager::TeacherCourseItem> list = dm.getTeacherCourses(m_studentId, termId);

    m_tableTeacherCourses->setRowCount(0);
    m_tableCourseStudents->setRowCount(0); // 清空右侧
    m_lblCourseStats->setText("请在左侧选择一门课程查看详情");

    for(int i = 0; i < list.size(); ++i) {
        const auto &data = list[i];
        m_tableTeacherCourses->insertRow(i);

        // 0. ID
        m_tableTeacherCourses->setItem(i, 0, new QTableWidgetItem(QString::number(data.sectionId)));
        // 1. 课程名
        m_tableTeacherCourses->setItem(i, 1, new QTableWidgetItem(data.courseName));
        // 2. 时间地点
        m_tableTeacherCourses->setItem(i, 2, new QTableWidgetItem(QString("%1 @%2").arg(data.timeInfo, data.room)));
        // 3. 人数 (已选/容量)
        QString countStr = QString("%1 / %2").arg(data.enrolledCount).arg(data.maxCount);
        m_tableTeacherCourses->setItem(i, 3, new QTableWidgetItem(countStr));
        // 4. 平均分
        QString scoreStr = (data.avgScore < 0) ? "--" : QString::number(data.avgScore, 'f', 1);
        m_tableTeacherCourses->setItem(i, 4, new QTableWidgetItem(scoreStr));

        // 居中
        for(int k=0; k<5; k++) {
            if(m_tableTeacherCourses->item(i, k))
                m_tableTeacherCourses->item(i, k)->setTextAlignment(Qt::AlignCenter);
        }
    }
}

// 3. 刷新右侧学生名单的逻辑
void MainWindow::loadCourseStudentList(int sectionId){
    DataManager dm;
    QList<DataManager::EnrolledStudentItem> list = dm.getCourseStudentList(sectionId);

    m_tableCourseStudents->setRowCount(0);

    double totalScore = 0;
    int gradedCount = 0;

    for(int i = 0; i < list.size(); ++i) {
        const auto &data = list[i];
        m_tableCourseStudents->insertRow(i);

        m_tableCourseStudents->setItem(i, 0, new QTableWidgetItem(data.number));
        m_tableCourseStudents->setItem(i, 1, new QTableWidgetItem(data.name));
        m_tableCourseStudents->setItem(i, 2, new QTableWidgetItem(data.adminClass));

        QString scoreText = (data.score < 0) ? "未录入" : QString::number(data.score, 'f', 1);
        QTableWidgetItem *scoreItem = new QTableWidgetItem(scoreText);

        if (data.score < 0) {
            scoreItem->setForeground(QColor("#aaaaaa")); // 灰色
        } else if (data.score < 60) {
            scoreItem->setForeground(QColor("#f56c6c")); // 挂科红
            totalScore += data.score;
            gradedCount++;
        } else {
            scoreItem->setForeground(QColor("#67c23a")); // 绿色
            totalScore += data.score;
            gradedCount++;
        }

        m_tableCourseStudents->setItem(i, 3, scoreItem);

        for(int k=0; k<4; k++)
            m_tableCourseStudents->item(i, k)->setTextAlignment(Qt::AlignCenter);
    }

    // 更新顶部统计 Label
    QString stats = QString("总人数: %1 人").arg(list.size());
    if (gradedCount > 0) {
        stats += QString(" | 已录入: %1 人 | 实时平均分: %2")
                     .arg(gradedCount)
                     .arg(QString::number(totalScore / gradedCount, 'f', 2));
    } else {
        stats += " | 暂无成绩数据";
    }
    m_lblCourseStats->setText(stats);
}

// 1. 初始化 UI
void MainWindow::setupTeacherGradingPageUi()
{
    m_pageGrading = new QWidget(this);
    ui->stackedTeacher->addWidget(m_pageGrading);

    // 沿用之前的 Dark Mode 样式，增加 QPushButton#ActionButton 的样式
    QString darkQss = R"(
        QWidget { background-color: #2b2b2b; color: #e0e0e0; font-family: 'Microsoft YaHei'; }
        QTableWidget { background-color: #333333; border: none; gridline-color: #444444; }
        QHeaderView::section { background-color: #222222; color: #aaaaaa; padding: 5px; border: none; border-bottom: 2px solid #409eff; }
        QTableWidget::item { padding: 5px; border-bottom: 1px solid #444444; }
        QTableWidget::item:selected { background-color: #409eff; color: white; }

        /* 标题 */
        QLabel#SectionTitle { font-size: 16px; font-weight: bold; color: #409eff; margin-bottom: 5px; }

        /* 打分按钮 */
        QPushButton#BtnGrade {
            background-color: #409eff; color: white; border-radius: 4px; padding: 4px 10px; font-weight: bold;
        }
        QPushButton#BtnGrade:hover { background-color: #66b1ff; }

        /* 批量导入按钮 */
        QPushButton#BtnBatch {
            background-color: #333333; border: 1px solid #555555; color: #e0e0e0; padding: 6px 15px; border-radius: 4px;
        }
        QPushButton#BtnBatch:hover { border-color: #409eff; color: #409eff; }
    )";
    m_pageGrading->setStyleSheet(darkQss);

    QVBoxLayout *mainLayout = new QVBoxLayout(m_pageGrading);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // --- Top Bar ---
    QHBoxLayout *topLayout = new QHBoxLayout();
    QPushButton *btnReturn = new QPushButton("← 返回仪表盘", m_pageGrading);
    btnReturn->setCursor(Qt::PointingHandCursor);
    btnReturn->setFont(QFont("Microsoft YaHei", 12, QFont::Bold));
    connect(btnReturn, &QPushButton::clicked, this, [=](){
        ui->stackedTeacher->setCurrentIndex(1);
    });
    topLayout->addWidget(btnReturn);
    //新增下拉框选择学期
    QLabel *lblTerm2 = new QLabel("选择学期：", m_pageGrading);
    m_comboGradingTerm = new QComboBox(m_pageGrading);
    m_comboGradingTerm->setFixedWidth(180);
    m_comboGradingTerm->setCursor(Qt::PointingHandCursor);

    // 连接信号：切学期时刷新表格
    connect(m_comboGradingTerm, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::updateGradingCourseTable);

    topLayout->addWidget(lblTerm2);
    topLayout->addWidget(m_comboGradingTerm);

    topLayout->addStretch();
    mainLayout->addLayout(topLayout);

    // --- Content Area ---
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(20);

    // [左侧] 课程列表
    QVBoxLayout *leftLayout = new QVBoxLayout();
    QLabel *lblLeft = new QLabel("选择课程录入 (Select Course)", m_pageGrading);
    lblLeft->setObjectName("SectionTitle");

    m_tableGradingCourses = new QTableWidget(m_pageGrading);
    m_tableGradingCourses->setColumnCount(4);
    m_tableGradingCourses->setHorizontalHeaderLabels({"ID", "课程名称", "进度", "状态"});
    m_tableGradingCourses->setColumnHidden(0, true);
    m_tableGradingCourses->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableGradingCourses->verticalHeader()->setVisible(false);
    m_tableGradingCourses->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableGradingCourses->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableGradingCourses->setSelectionMode(QAbstractItemView::SingleSelection);

    // 点击左侧 -> 刷新右侧
    connect(m_tableGradingCourses, &QTableWidget::itemClicked, this, [=](QTableWidgetItem *item){
        int row = item->row();
        int sectionId = m_tableGradingCourses->item(row, 0)->text().toInt();
        loadGradingStudentList(sectionId);
    });

    leftLayout->addWidget(lblLeft);
    leftLayout->addWidget(m_tableGradingCourses);

    // [右侧] 评分区域
    QVBoxLayout *rightLayout = new QVBoxLayout();

    // 右侧头部：标题 + 批量导入按钮
    QHBoxLayout *rightHeader = new QHBoxLayout();
    QLabel *lblRight = new QLabel("成绩录入面板", m_pageGrading);
    lblRight->setObjectName("SectionTitle");

    QPushButton *btnBatch = new QPushButton("批量导入 (Excel)", m_pageGrading);
    btnBatch->setObjectName("BtnBatch");
    btnBatch->setCursor(Qt::PointingHandCursor);
    connect(btnBatch, &QPushButton::clicked, this, [=](){
        QMessageBox::information(this, "提示", "批量导入功能正在开发中...\n(Feature under construction)");
    });

    rightHeader->addWidget(lblRight);
    rightHeader->addStretch();
    rightHeader->addWidget(btnBatch);

    // 状态提示
    m_lblGradingStatus = new QLabel("请在左侧选择课程", m_pageGrading);
    m_lblGradingStatus->setStyleSheet("color: #aaaaaa; margin-bottom: 5px;");

    m_tableGradingStudents = new QTableWidget(m_pageGrading);
    m_tableGradingStudents->setColumnCount(5); // ID, 学号, 姓名, 成绩, 操作
    m_tableGradingStudents->setHorizontalHeaderLabels({"S_ID", "学号", "姓名", "当前成绩", "操作"});
    m_tableGradingStudents->setColumnHidden(0, true); // 隐藏学生ID
    m_tableGradingStudents->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableGradingStudents->verticalHeader()->setVisible(false);
    m_tableGradingStudents->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableGradingStudents->setSelectionBehavior(QAbstractItemView::SelectRows);

    rightLayout->addLayout(rightHeader);
    rightLayout->addWidget(m_lblGradingStatus);
    rightLayout->addWidget(m_tableGradingStudents);

    contentLayout->addLayout(leftLayout, 4);
    contentLayout->addLayout(rightLayout, 6);
    mainLayout->addLayout(contentLayout);
}

// 2. 刷新左侧：显示课程及打分进度
void MainWindow::updateGradingCourseTable()
{
    int termId = m_comboGradingTerm->currentData().toInt();
    DataManager dm;
    // 复用 getTeacherCourses，因为它现在包含了 gradedCount
    QList<DataManager::TeacherCourseItem> list = dm.getTeacherCourses(m_studentId, termId);

    m_tableGradingCourses->setRowCount(0);
    m_tableGradingStudents->setRowCount(0);
    m_lblGradingStatus->setText("请在左侧选择课程");

    for(int i = 0; i < list.size(); ++i) {
        const auto &data = list[i];
        m_tableGradingCourses->insertRow(i);

        // 0. ID
        m_tableGradingCourses->setItem(i, 0, new QTableWidgetItem(QString::number(data.sectionId)));

        // 1. 课程名
        m_tableGradingCourses->setItem(i, 1, new QTableWidgetItem(data.courseName));

        // 2. 进度 (已打分 / 总人数)
        QString progressStr = QString("%1 / %2").arg(data.gradedCount).arg(data.enrolledCount);
        m_tableGradingCourses->setItem(i, 2, new QTableWidgetItem(progressStr));

        // 3. 状态 (区分颜色)
        QTableWidgetItem *statusItem;
        if (data.enrolledCount == 0) {
            statusItem = new QTableWidgetItem("无人选课");
            statusItem->setForeground(QColor("#aaaaaa"));
        } else if (data.gradedCount >= data.enrolledCount) {
            statusItem = new QTableWidgetItem("已完成");
            statusItem->setForeground(QColor("#67c23a")); // 绿色
        } else if (data.gradedCount == 0) {
            statusItem = new QTableWidgetItem("未开始");
            statusItem->setForeground(QColor("#f56c6c")); // 红色
        } else {
            statusItem = new QTableWidgetItem("进行中");
            statusItem->setForeground(QColor("#e6a23c")); // 橙色
        }
        statusItem->setTextAlignment(Qt::AlignCenter);
        m_tableGradingCourses->setItem(i, 3, statusItem);

        // 居中
        m_tableGradingCourses->item(i, 1)->setTextAlignment(Qt::AlignCenter);
        m_tableGradingCourses->item(i, 2)->setTextAlignment(Qt::AlignCenter);
    }
}

// 3. 刷新右侧：加载学生并提供打分按钮
void MainWindow::loadGradingStudentList(int sectionId)
{
    DataManager dm;
    QList<DataManager::EnrolledStudentItem> list = dm.getCourseStudentList(sectionId);

    m_tableGradingStudents->setRowCount(0);

    // 刷新状态栏信息
    m_lblGradingStatus->setText(QString("当前课程 ID: %1   总人数: %2").arg(sectionId).arg(list.size()));

    for(int i = 0; i < list.size(); ++i) {
        const auto &student = list[i];
        m_tableGradingStudents->insertRow(i);

        // 0. 学生ID (隐藏)
        m_tableGradingStudents->setItem(i, 0, new QTableWidgetItem(QString::number(student.studentId)));
        // 1. 学号
        m_tableGradingStudents->setItem(i, 1, new QTableWidgetItem(student.number));
        // 2. 姓名
        m_tableGradingStudents->setItem(i, 2, new QTableWidgetItem(student.name));

        // 3. 当前成绩
        QString scoreStr;
        if(student.score < 0) scoreStr = "--";
        else scoreStr = QString::number(student.score, 'f', 1); // 保留1位小数

        QTableWidgetItem *scoreItem = new QTableWidgetItem(scoreStr);
        scoreItem->setTextAlignment(Qt::AlignCenter);
        // 已有成绩标绿，没有标灰
        if(student.score >= 0) scoreItem->setForeground(QColor("#67c23a"));
        m_tableGradingStudents->setItem(i, 3, scoreItem);

        // 4. 操作按钮 (单个导入)
        QPushButton *btnAction = new QPushButton();
        btnAction->setObjectName("BtnGrade");
        btnAction->setCursor(Qt::PointingHandCursor);

        if(student.score < 0) {
            btnAction->setText("打分");
            btnAction->setStyleSheet("background-color: #409eff;"); // 蓝
        } else {
            btnAction->setText("修改");
            btnAction->setStyleSheet("background-color: #e6a23c;"); // 橙
        }

        // 按钮点击事件 -> 弹出输入框
        connect(btnAction, &QPushButton::clicked, this, [=](){
            bool ok;
            // 默认值：如果是修改，显示原分；如果是新打分，显示0
            double defaultVal = (student.score < 0) ? 0.0 : student.score;

            double newScore = QInputDialog::getDouble(this, "成绩录入",
                                                      QString("请输入 %1 (%2) 的成绩:").arg(student.name, student.number),
                                                      defaultVal, 0, 100, 1, &ok);
            if (ok) {
                DataManager tempDm;
                if(tempDm.updateStudentGrade(student.studentId, sectionId, newScore)) {
                    // 成功后：刷新当前右侧列表
                    loadGradingStudentList(sectionId);
                    // 同时也刷新左侧列表，以更新进度条 (gradedCount)
                    // 为了用户体验，我们可以只刷新右侧，左侧等下次进来再刷，
                    // 但为了同步显示进度，最好调用一下：

                    // 这里有个小问题：直接调用 updateGradingCourseTable 会导致左侧选中项丢失
                    // 简单的办法：只在打完分后给用户一个状态栏提示，或者不做全局刷新
                    // 完美的办法：记录当前选中的row，刷新后再setCurrentCell

                    // 简单起见，这里只刷新右侧数据。
                } else {
                    QMessageBox::critical(this, "错误", "成绩写入数据库失败！");
                }
            }
        });

        m_tableGradingStudents->setCellWidget(i, 4, btnAction);

        // 居中设置
        m_tableGradingStudents->item(i, 1)->setTextAlignment(Qt::AlignCenter);
        m_tableGradingStudents->item(i, 2)->setTextAlignment(Qt::AlignCenter);
    }
}

void MainWindow::setupTeacherInfoPageUi()
{
    // 1. 创建页面容器
    m_pageInfoTeacher = new QWidget(this);
    ui->stackedTeacher->addWidget(m_pageInfoTeacher);

    // 2. 页面样式 (完全复用学生端的 Dark Mode QSS，保持风格统一)
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
            background-color: transparent;
        }

        /* 字段值 (Value) */
        QLabel#lblVal {
            font-size: 18px; font-weight: bold; color: white;
            padding: 5px;
            background-color: transparent;
            border: none;
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
    m_pageInfoTeacher->setStyleSheet(darkQss);

    // 3. 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(m_pageInfoTeacher);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    // === 顶部栏：返回按钮 ===
    QHBoxLayout *topLayout = new QHBoxLayout();
    QPushButton *btnReturn = new QPushButton("← 返回仪表盘", m_pageInfoTeacher);
    btnReturn->setCursor(Qt::PointingHandCursor);
    btnReturn->setFont(QFont("Microsoft YaHei", 12, QFont::Bold));
    connect(btnReturn, &QPushButton::clicked, this, [=](){
        ui->stackedTeacher->setCurrentIndex(1);
    });

    topLayout->addWidget(btnReturn);
    topLayout->addStretch();
    mainLayout->addLayout(topLayout);

    // === 中间内容区域 ===
    mainLayout->addStretch(); // 顶部弹簧

    // 创建卡片
    QWidget *card = new QWidget(m_pageInfoTeacher);
    card->setObjectName("infoCard");
    card->setFixedSize(550, 350);

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(50, 40, 50, 40);
    cardLayout->setSpacing(10);

    // 卡片标题
    QLabel *title = new QLabel("教师信息 | Teacher Profile", card);
    title->setObjectName("lblTitle");
    title->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(title);

    // 分割线
    QFrame *line = new QFrame(card);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #555555;");
    cardLayout->addWidget(line);
    cardLayout->addSpacing(20);

    // 表单布局
    QGridLayout *formLayout = new QGridLayout();
    formLayout->setHorizontalSpacing(20);
    formLayout->setVerticalSpacing(15);

    // 辅助lambda (和学生端一样)
    auto createRow = [&](int row, QString key, QLabel* &valLabel) {
        QLabel *k = new QLabel(key, card);
        k->setObjectName("lblKey");
        k->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        valLabel = new QLabel("Loading...", card);
        valLabel->setObjectName("lblVal");
        valLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        formLayout->addWidget(k, row, 0);
        formLayout->addWidget(valLabel, row, 1);
    };

    // === 这里显示教师特有的属性 ===
    // 1. 姓名
    createRow(0, "姓名 / Name:", m_valTeacherName);
    // 2. 工号 (对应 teacher_number)
    createRow(1, "工号 / Emp ID:", m_valTeacherNumber);
    // 3. 学院 (对应 department name)
    createRow(2, "学院 / Dept:", m_valTeacherDept);
    // 4. 职称 (对应 title)
    createRow(3, "职称 / Title:", m_valTeacherTitle);

    // 调整列宽比例
    formLayout->setColumnStretch(0, 4);
    formLayout->setColumnStretch(1, 6);

    cardLayout->addLayout(formLayout);
    cardLayout->addStretch();

    // 把卡片加入主布局并居中
    mainLayout->addWidget(card, 0, Qt::AlignCenter);
    mainLayout->addStretch(); // 底部弹簧
}

void MainWindow::loadTeacherInfo()
{
    // 假设登录时，m_studentId 存储的是当前登录用户的 ID (无论是学生ID还是教师ID)
    // 这里传入 m_studentId 作为 teacher_id
    DataManager dm;
    DataManager::TeacherPersonalInfor info = dm.getTeacherInfo(m_studentId);

    // 填入数据
    m_valTeacherName->setText(info.name);
    m_valTeacherNumber->setText(info.number); // 显示 teacher_number
    m_valTeacherDept->setText(info.department);
    m_valTeacherTitle->setText(info.title);
}

// void MainWindow::keyPressEvent(QKeyEvent *event)
// {
//     // 飘号键在 Qt 中通常对应 Qt::Key_QuoteLeft (也就是键盘上 Tab 上面那个键)
//     // 也有可能是 Qt::Key_AsciiTilde，为了保险，我们可以都判断
//     if (event->type() == QEvent::KeyPress)
//     {
//         QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
//         // 打印当前按下的键值
//         qDebug() << "Key Pressed:" << keyEvent->key();

//         // ... 后续逻辑
//     }
//     if (event->key() == Qt::Key_QuoteLeft || event->key() == Qt::Key_AsciiTilde)
//     {
//         // 1. 检查是否是管理员身份
//         if (if_admin)
//         {
//             // 2. 检查当前页面是否是日志页面 (page_logs)
//             if (true)
//             {
//                 // 打开控制台
//                 ConsoleDialog console(this);
//                 console.exec(); // 模态显示，阻塞主窗口直到关闭控制台

//                 // 事件已处理，不再向下传递
//                 event->accept();
//                 return;
//             }
//         }
//     }

//     // 如果不符合条件，调用父类默认处理（保证其他快捷键正常工作）
//     QMainWindow::keyPressEvent(event);
// }

void MainWindow::setupAdminPageUi()
{
    ui->admin_stack->setCurrentIndex(0);

    connect(ui->listWidget_admin, &QListWidget::currentRowChanged, this, [=](int row){

        // 打印日志，确认点击是否有反应 (调试用)
        qDebug() << "Admin Menu Clicked, Row:" << row;

        switch (row + 1) {

        case 1: // 菜单第1项：学院管理
            ui->admin_stack->setCurrentIndex(PageCollege);
            refreshCollegeTable();
            break;

        case 2: // 菜单第2项：专业管理
            ui->admin_stack->setCurrentIndex(PageDepartment);
            setupAdminDeptUi();
            break;
        case 3: //第3页，行政班管理
            ui->admin_stack->setCurrentIndex(PageAdminClass);
            setupAdminClassUi();
            break;
        case 4: // 菜单第4项：学科管理
            ui->admin_stack->setCurrentIndex(PageSubject);
            setupAdminSubjectUi();
            break;
        case 5: //学期管理
            ui->admin_stack->setCurrentIndex(PageSemester);
            setupAdminSemesterUi();
            break;
        case 6: // 菜单第6项：排课管理
            ui->admin_stack->setCurrentIndex(PageCourse);
            setupAdminCourseUi();
            break;

        case 7: // 菜单第7项：选课管理
            ui->admin_stack->setCurrentIndex(PageSelection);
            setupAdminSelectionUi();
            break;

        case 8: // 菜单第8项：学生管理
            ui->admin_stack->setCurrentIndex(PageStudent);
            setupAdminStudentUi();
            break;

        case 9: // 菜单第9项：教师管理
            ui->admin_stack->setCurrentIndex(PageTeacher);
            setupAdminTeacherUi();
            break;

        case 10: // 菜单第10项：账号管理
            ui->admin_stack->setCurrentIndex(PageAccount);
            setupAdminAccountUi();
            break;

        case 11: // 菜单第11项：日志
            ui->admin_stack->setCurrentIndex(PageLogs);
            setupAdminLogsUi();
            break;

        case 12: // 菜单第12项：数据库
            ui->admin_stack->setCurrentIndex(PageDatabase);
            setupAdminDatabaseUi();
            break;

        default:
            qDebug() << "未知的菜单项:" << row;
            break;
        }
    });

    setupAdminCollegeUi();     // Index 1: 学院管理
    setupAdminDeptUi();        // Index 2: 专业(系)管理
    setupAdminSubjectUi();     // Index 3: 学科管理
    setupAdminClassUi();
    setupAdminSemesterUi();
    setupAdminCourseUi();      // Index 4: 课程/排课管理
    setupAdminSelectionUi();   // Index 5: 选课管理
    setupAdminStudentUi();     // Index 6: 学生管理
    setupAdminTeacherUi();     // Index 7: 老师管理
    setupAdminAccountUi();     // Index 8: 账号管理
    setupAdminLogsUi();        // Index 9: 日志
    setupAdminDatabaseUi();    // Index 10: 数据库管理


}

// mainwindow.cpp

void MainWindow::setupAdminCollegeUi()
{


    QWidget *page = ui->admin_college_manage;

    if (page->layout()) {
        refreshCollegeTable(); // 只刷新表格
        return;
    }

    QVBoxLayout *mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);

    // ==================================================
    // A. 顶部工具栏 (Top Bar)
    // ==================================================
    QHBoxLayout *toolLayout = new QHBoxLayout();
    toolLayout->setContentsMargins(0, 0, 0, 0);
    toolLayout->setSpacing(10); // 控件间距

    // 1. 左侧标题
    QLabel *title = new QLabel("学院管理 (College Management)", page);
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #e0e0e0;");
    toolLayout->addWidget(title);

    // 2. 弹簧 (撑开左右)
    toolLayout->addStretch();

    // 3. 搜索框 【关键修改：定死宽度，防止重叠】
    m_editCollegeSearch = new QLineEdit(page);
    m_editCollegeSearch->setPlaceholderText("搜索学院名称或编号...");
    m_editCollegeSearch->setFixedWidth(220); // <--- 固定宽度，不再伸缩
    m_editCollegeSearch->setFixedHeight(35); // <--- 固定高度
    m_editCollegeSearch->setStyleSheet(
        "QLineEdit { background-color: #3a3a3a; border: 1px solid #555; border-radius: 5px; color: white; padding-left: 10px; }"
        "QLineEdit:focus { border: 1px solid #409eff; }"
        );
    connect(m_editCollegeSearch, &QLineEdit::returnPressed, this, &MainWindow::refreshCollegeTable);
    toolLayout->addWidget(m_editCollegeSearch);

    // 4. 查询按钮
    QPushButton *btnQuery = new QPushButton("查询", page);
    btnQuery->setFixedSize(80, 35);
    btnQuery->setCursor(Qt::PointingHandCursor);
    btnQuery->setStyleSheet("QPushButton { background-color: #409eff; color: white; border-radius: 5px; font-weight: bold; } QPushButton:hover { background-color: #66b1ff; }");
    connect(btnQuery, &QPushButton::clicked, this, &MainWindow::refreshCollegeTable);
    toolLayout->addWidget(btnQuery);

    // 5. 新增按钮
    QPushButton *btnAdd = new QPushButton("+ 新增学院", page);
    btnAdd->setFixedSize(120, 35);
    btnAdd->setCursor(Qt::PointingHandCursor);
    btnAdd->setStyleSheet("QPushButton { background-color: #67c23a; color: white; border-radius: 5px; font-weight: bold; } QPushButton:hover { background-color: #85ce61; }");
    connect(btnAdd, &QPushButton::clicked, this, [=](){ showCollegeEditDialog(false); });
    toolLayout->addWidget(btnAdd);

    mainLayout->addLayout(toolLayout);

    // ==================================================
    // B. 数据表格 (Table)
    // ==================================================
    m_tableCollege = new QTableWidget(page);
    m_tableCollege->setColumnCount(4);
    m_tableCollege->setHorizontalHeaderLabels({"ID", "学院名称", "学院编号", "操作"});
    m_tableCollege->setColumnHidden(0, true);

    // 布局设置
    m_tableCollege->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableCollege->verticalHeader()->setVisible(false);
    m_tableCollege->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableCollege->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableCollege->setAlternatingRowColors(false);

    // 【关键修改：设置行高为 55px，防止按钮被挤扁】
    m_tableCollege->verticalHeader()->setDefaultSectionSize(55);

    m_tableCollege->setStyleSheet(R"(
        QTableWidget { background-color: #2b2b2b; border: 1px solid #444; gridline-color: #444; color: #ddd; }
        QHeaderView::section { background-color: #333; color: white; border: none; border-bottom: 2px solid #409eff; height: 40px; font-weight: bold; }
        QTableWidget::item { border-bottom: 1px solid #333; }
        QTableWidget::item:selected { background-color: #3a3a3a; color: white; }
    )");

    mainLayout->addWidget(m_tableCollege);

    refreshCollegeTable();
}

// mainwindow.cpp -> refreshCollegeTable

void MainWindow::refreshCollegeTable()
{
    DataManager dm;
    QString key = m_editCollegeSearch->text().trimmed();
    auto list = dm.getAllColleges(key);

    m_tableCollege->setRowCount(0);

    for(int i = 0; i < list.size(); ++i) {
        const auto &data = list[i];
        m_tableCollege->insertRow(i);

        m_tableCollege->setItem(i, 0, new QTableWidgetItem(QString::number(data.id)));
        m_tableCollege->setItem(i, 1, new QTableWidgetItem(data.name));
        m_tableCollege->setItem(i, 2, new QTableWidgetItem(data.code));

        // --- 3. 操作栏 ---
        QWidget *actionWidget = new QWidget();
        QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(5, 5, 5, 5); // 给点内边距
        actionLayout->setSpacing(10);

        QPushButton *btnEdit = new QPushButton("修改");
        btnEdit->setCursor(Qt::PointingHandCursor);
        // 【关键修复】给定固定大小，防止被压扁
        btnEdit->setFixedSize(60, 32);
        btnEdit->setStyleSheet("color: #409eff; background: transparent; border: 1px solid #409eff; border-radius: 4px;");

        QPushButton *btnDel = new QPushButton("删除");
        btnDel->setCursor(Qt::PointingHandCursor);
        // 【关键修复】给定固定大小
        btnDel->setFixedSize(60, 32);
        btnDel->setStyleSheet("color: #f56c6c; background: transparent; border: 1px solid #f56c6c; border-radius: 4px;");

        actionLayout->addStretch();
        actionLayout->addWidget(btnEdit);
        actionLayout->addWidget(btnDel);
        actionLayout->addStretch();

        m_tableCollege->setCellWidget(i, 3, actionWidget);

        // 文字居中
        m_tableCollege->item(i, 1)->setTextAlignment(Qt::AlignCenter);
        m_tableCollege->item(i, 2)->setTextAlignment(Qt::AlignCenter);

        // 绑定事件
        connect(btnEdit, &QPushButton::clicked, this, [=](){
            showCollegeEditDialog(true, data.id, data.name, data.code);
        });

        connect(btnDel, &QPushButton::clicked, this, [=](){
            if(QMessageBox::question(this, "确认", QString("确定删除 [%1] 吗？").arg(data.name)) == QMessageBox::Yes) {
                DataManager tempDm;
                if(tempDm.deleteCollege(data.id)) {
                    QMessageBox::information(this, "成功", "删除成功");
                    refreshCollegeTable();
                } else {
                    QMessageBox::warning(this, "失败", "删除失败，请检查是否有关联数据。");
                }
            }
        });
    }
}

// ==================================================
// 3. 弹窗：新增/修改 (showCollegeEditDialog)
// ==================================================
// 这是一个手写的 Dialog，不需要 .ui 文件
void MainWindow::showCollegeEditDialog(bool isEdit, int id, QString name, QString code)
{
    QDialog dlg(this);
    dlg.setWindowTitle(isEdit ? "修改学院信息" : "新增学院");
    dlg.setFixedSize(350, 200);

    // 简单表单布局
    QFormLayout *layout = new QFormLayout(&dlg);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(15);

    QLineEdit *edtName = new QLineEdit(&dlg);
    edtName->setText(name);
    edtName->setPlaceholderText("例如：计算机学院");

    QLineEdit *edtCode = new QLineEdit(&dlg);
    edtCode->setText(code);
    edtCode->setPlaceholderText("例如：CS");

    layout->addRow("学院名称:", edtName);
    layout->addRow("学院编号:", edtCode);

    // 按钮框
    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addRow(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    // 执行弹窗 loop
    if (dlg.exec() == QDialog::Accepted) {
        QString newName = edtName->text().trimmed();
        QString newCode = edtCode->text().trimmed();

        if (newName.isEmpty()) {
            QMessageBox::warning(this, "警告", "学院名称不能为空");
            return;
        }

        DataManager dm;
        bool success = false;
        if (isEdit) {
            success = dm.updateCollege(id, newName, newCode);
        } else {
            success = dm.addCollege(newName, newCode);
        }

        if (success) {
            QMessageBox::information(this, "成功", "操作成功");
            refreshCollegeTable(); // 刷新表格显示最新数据
        } else {
            QMessageBox::critical(this, "错误", "数据库操作失败，请检查编号是否重复或网络连接。");
        }
    }
}

// ==================================================
// 1. 构建专业管理界面 (Side-by-Side Layout)
// ==================================================
void MainWindow::setupAdminDeptUi()
{
    QWidget *page = ui->admin_department_manage;
    if (page->layout()) {
        loadCollegeComboBox(); // 刷新下拉框
        refreshDeptTable();    // 刷新表格
        return;
    }

    // 主布局：水平分割 (左表 | 右控)
    QHBoxLayout *mainLayout = new QHBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // ------------------------------------
    // A. 左侧：数据表格 (占用 70% 宽度)
    // ------------------------------------
    m_tableDept = new QTableWidget(page);
    m_tableDept->setColumnCount(5); // ID, 名称, 编号, 所属学院, 操作
    m_tableDept->setHorizontalHeaderLabels({"ID", "专业名称", "专业代码", "所属学院", "操作"});
    m_tableDept->setColumnHidden(0, true); // 隐藏ID

    // 样式与行为
    m_tableDept->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableDept->verticalHeader()->setVisible(false);
    m_tableDept->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableDept->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableDept->verticalHeader()->setDefaultSectionSize(50); // 设置行高，防止按钮挤扁

    // 表格 QSS (深色磨砂感)
    m_tableDept->setStyleSheet(R"(
        QTableWidget { background-color: #2b2b2b; border: none; gridline-color: #444; color: #ddd; }
        QHeaderView::section { background-color: #1e1e1e; color: #409eff; height: 40px; border: none; border-bottom: 2px solid #409eff; }
        QTableWidget::item { border-bottom: 1px solid #333; }
        QTableWidget::item:selected { background-color: #409eff; color: white; }
    )");

    mainLayout->addWidget(m_tableDept, 7); // 权重 7

    // ------------------------------------
    // B. 右侧：控制面板 (占用 30% 宽度)
    // ------------------------------------
    QFrame *rightPanel = new QFrame(page);
    rightPanel->setStyleSheet("QFrame { background-color: #333333; border-radius: 10px; }");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(20, 20, 20, 20);
    rightLayout->setSpacing(20);

    // --- B1. 搜索模块 ---
    QLabel *lblSearch = new QLabel("🔍 搜索 / Search", rightPanel);
    lblSearch->setStyleSheet("color: white; font-weight: bold; font-size: 16px;");

    m_editDeptSearch = new QLineEdit(rightPanel);
    m_editDeptSearch->setPlaceholderText("专业名 / 代码 / 学院...");
    m_editDeptSearch->setFixedHeight(35);
    m_editDeptSearch->setStyleSheet("background-color: #2b2b2b; border: 1px solid #555; border-radius: 5px; color: white; padding: 5px;");
    connect(m_editDeptSearch, &QLineEdit::returnPressed, this, &MainWindow::refreshDeptTable);

    QPushButton *btnSearch = new QPushButton("执行查询", rightPanel);
    btnSearch->setFixedHeight(35);
    btnSearch->setCursor(Qt::PointingHandCursor);
    btnSearch->setStyleSheet("QPushButton { background-color: #409eff; color: white; border-radius: 5px; } QPushButton:hover { background-color: #66b1ff; }");
    connect(btnSearch, &QPushButton::clicked, this, &MainWindow::refreshDeptTable);

    rightLayout->addWidget(lblSearch);
    rightLayout->addWidget(m_editDeptSearch);
    rightLayout->addWidget(btnSearch);

    // 分割线
    QFrame *line = new QFrame; line->setFrameShape(QFrame::HLine); line->setStyleSheet("color: #555;");
    rightLayout->addWidget(line);

    // --- B2. 快速添加模块 ---
    QLabel *lblAdd = new QLabel("➕ 快速添加 / Quick Add", rightPanel);
    lblAdd->setStyleSheet("color: #67c23a; font-weight: bold; font-size: 16px;");

    m_inputDeptName = new QLineEdit(rightPanel);
    m_inputDeptName->setPlaceholderText("输入专业名称...");
    m_inputDeptName->setFixedHeight(35);
    m_inputDeptName->setStyleSheet("background-color: #2b2b2b; border: 1px solid #555; border-radius: 5px; color: white; padding: 5px;");

    m_inputDeptCode = new QLineEdit(rightPanel);
    m_inputDeptCode->setPlaceholderText("输入专业代码 (如 CS01)...");
    m_inputDeptCode->setFixedHeight(35);
    m_inputDeptCode->setStyleSheet(m_inputDeptName->styleSheet());

    // 【关键】所属学院下拉框
    QLabel *lblCollege = new QLabel("所属学院:", rightPanel);
    lblCollege->setStyleSheet("color: #aaa;");

    m_comboDeptCollege = new QComboBox(rightPanel);
    m_comboDeptCollege->setFixedHeight(35);
    m_comboDeptCollege->setCursor(Qt::PointingHandCursor);
    m_comboDeptCollege->setStyleSheet("QComboBox { background-color: #2b2b2b; border: 1px solid #555; border-radius: 5px; color: white; padding: 5px; } QComboBox::drop-down{border:none;}");

    QPushButton *btnAdd = new QPushButton("立即添加", rightPanel);
    btnAdd->setFixedHeight(40);
    btnAdd->setCursor(Qt::PointingHandCursor);
    btnAdd->setStyleSheet("QPushButton { background-color: #67c23a; color: white; border-radius: 5px; font-weight: bold; } QPushButton:hover { background-color: #85ce61; }");

    // 添加逻辑
    connect(btnAdd, &QPushButton::clicked, this, [=](){
        QString name = m_inputDeptName->text().trimmed();
        QString code = m_inputDeptCode->text().trimmed();

        // 校验输入
        if(name.isEmpty() || code.isEmpty()) {
            QMessageBox::warning(this, "提示", "名称和代码不能为空");
            return;
        }
        // 校验下拉框
        if(m_comboDeptCollege->currentIndex() < 0) {
            QMessageBox::warning(this, "提示", "请先在“学院管理”中添加学院数据！");
            return;
        }

        int collegeId = m_comboDeptCollege->currentData().toInt(); // 获取绑定的 ID

        DataManager dm;
        if(dm.addDepartment(name, code, collegeId)) {
            QMessageBox::information(this, "成功", "添加成功！");
            m_inputDeptName->clear();
            m_inputDeptCode->clear();
            refreshDeptTable(); // 刷新左侧表格
        } else {
            QMessageBox::critical(this, "失败", "数据库操作失败");
        }
    });

    rightLayout->addWidget(lblAdd);
    rightLayout->addWidget(m_inputDeptName);
    rightLayout->addWidget(m_inputDeptCode);
    rightLayout->addWidget(lblCollege);
    rightLayout->addWidget(m_comboDeptCollege);
    rightLayout->addWidget(btnAdd);

    rightLayout->addStretch(); // 底部弹簧，把内容顶上去

    mainLayout->addWidget(rightPanel, 3); // 权重 3

    // 初始化数据
    loadCollegeComboBox(); // 先填下拉框
    refreshDeptTable();    // 再填表格
}

// ==================================================
// 2. 辅助函数：加载学院下拉框
// ==================================================
void MainWindow::loadCollegeComboBox()
{
    if(!m_comboDeptCollege) return;

    m_comboDeptCollege->clear();

    DataManager dm;
    // 复用之前的 getAllColleges 函数
    auto colleges = dm.getAllColleges();

    for(const auto &c : colleges) {
        // addItem(显示的文本, 隐藏的数据ID)
        m_comboDeptCollege->addItem(c.name, c.id);
    }
}

// ==================================================
// 3. 刷新表格
// ==================================================
void MainWindow::refreshDeptTable()
{
    DataManager dm;
    QString key = m_editDeptSearch->text().trimmed();
    auto list = dm.getAllDepartments(key); // 这是一个联表查询结果

    m_tableDept->setRowCount(0);

    for(int i = 0; i < list.size(); ++i) {
        const auto &data = list[i];
        m_tableDept->insertRow(i);

        m_tableDept->setItem(i, 0, new QTableWidgetItem(QString::number(data.id)));
        m_tableDept->setItem(i, 1, new QTableWidgetItem(data.name));
        m_tableDept->setItem(i, 2, new QTableWidgetItem(data.code));
        m_tableDept->setItem(i, 3, new QTableWidgetItem(data.collegeName)); // 显示学院名字

        // 操作栏
        QWidget *actionWidget = new QWidget();
        QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(5, 5, 5, 5);
        actionLayout->setSpacing(10);

        QPushButton *btnEdit = new QPushButton("修改");
        btnEdit->setFixedSize(60, 30);
        btnEdit->setCursor(Qt::PointingHandCursor);
        btnEdit->setStyleSheet("color: #409eff; background: transparent; border: 1px solid #409eff; border-radius: 4px;");

        QPushButton *btnDel = new QPushButton("删除");
        btnDel->setFixedSize(60, 30);
        btnDel->setCursor(Qt::PointingHandCursor);
        btnDel->setStyleSheet("color: #f56c6c; background: transparent; border: 1px solid #f56c6c; border-radius: 4px;");

        actionLayout->addStretch();
        actionLayout->addWidget(btnEdit);
        actionLayout->addWidget(btnDel);
        actionLayout->addStretch();

        m_tableDept->setCellWidget(i, 4, actionWidget);

        // 居中
        for(int k=1; k<=3; k++) m_tableDept->item(i, k)->setTextAlignment(Qt::AlignCenter);

        // 修改逻辑
        connect(btnEdit, &QPushButton::clicked, this, [=](){
            showDeptEditDialog(data.id, data.name, data.code, data.collegeId);
        });

        // 删除逻辑
        connect(btnDel, &QPushButton::clicked, this, [=](){
            if(QMessageBox::question(this, "确认", QString("确定删除专业 [%1] 吗？").arg(data.name)) == QMessageBox::Yes) {
                DataManager tempDm;
                if(tempDm.deleteDepartment(data.id)) {
                    QMessageBox::information(this, "成功", "删除成功");
                    refreshDeptTable();
                } else {
                    QMessageBox::warning(this, "失败", "删除失败，该专业下可能存在学生或学科。");
                }
            }
        });
    }
}

// ==================================================
// 4. 修改弹窗 (需要带下拉框回显)
// ==================================================
void MainWindow::showDeptEditDialog(int id, QString name, QString code, int currentCollegeId)
{
    QDialog dlg(this);
    dlg.setWindowTitle("修改专业信息");
    dlg.setFixedSize(350, 250);

    QFormLayout *layout = new QFormLayout(&dlg);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(15);

    QLineEdit *edtName = new QLineEdit(&dlg);
    edtName->setText(name);

    QLineEdit *edtCode = new QLineEdit(&dlg);
    edtCode->setText(code);

    // 下拉框回显
    QComboBox *comboCol = new QComboBox(&dlg);
    DataManager dm;
    auto colleges = dm.getAllColleges();
    for(const auto &c : colleges) {
        comboCol->addItem(c.name, c.id);
    }
    // 找到当前 ID 对应的索引并选中
    int idx = comboCol->findData(currentCollegeId);
    if(idx >= 0) comboCol->setCurrentIndex(idx);

    layout->addRow("专业名称:", edtName);
    layout->addRow("专业代码:", edtCode);
    layout->addRow("所属学院:", comboCol);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addRow(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        if(edtName->text().isEmpty()) return;

        int newCollegeId = comboCol->currentData().toInt();

        if(dm.updateDepartment(id, edtName->text(), edtCode->text(), newCollegeId)) {
            QMessageBox::information(this, "成功", "修改成功");
            refreshDeptTable();
        } else {
            QMessageBox::critical(this, "失败", "修改失败");
        }
    }
}

// ==================================================
// 1. 构建学科管理界面 (Tab Layout)
// ==================================================
void MainWindow::setupAdminSubjectUi()
{
    QWidget *page = ui->admin_subject_manage;
    if (page->layout()) {
        loadSubjectDeptCombo();     // 刷新“所属系”下拉框
        loadPrereqTargetCombo();    // 刷新“先修课”下拉框
        refreshSubjectTable();      // 刷新表格
        return;
    }

    QVBoxLayout *mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // 创建 Tab Widget
    m_tabSubject = new QTabWidget(page);
    m_tabSubject->setStyleSheet(R"(
        QTabWidget::pane { border: 1px solid #444; background: #2b2b2b; }
        QTabBar::tab { background: #1e1e1e; color: #aaa; padding: 8px 20px; border-top-left-radius: 4px; border-top-right-radius: 4px; }
        QTabBar::tab:selected { background: #2b2b2b; color: #409eff; font-weight: bold; border-bottom: 2px solid #2b2b2b; }
    )");

    // ------------------------------------
    // Tab 1: 学科基本信息 (Basic Info)
    // ------------------------------------
    QWidget *tab1 = new QWidget();
    QHBoxLayout *tab1Layout = new QHBoxLayout(tab1); // 左右布局

    // --- 左侧表格 ---
    m_tableSubject = new QTableWidget();
    m_tableSubject->setColumnCount(6); // ID, Name, Code, Dept, Status, Ops
    m_tableSubject->setHorizontalHeaderLabels({"ID", "学科名称", "学科代码", "所属专业/系", "状态", "操作"});
    m_tableSubject->setColumnHidden(0, true);
    m_tableSubject->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableSubject->verticalHeader()->setDefaultSectionSize(50);
    m_tableSubject->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableSubject->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableSubject->setStyleSheet("QTableWidget { background-color: #2b2b2b; border: none; color: #ddd; } QHeaderView::section { background-color: #333; color: white; height: 35px; border:none; }");

    // --- 右侧控制面板 ---
    QFrame *rightPanel = new QFrame();
    rightPanel->setFixedWidth(320); // 固定宽度
    rightPanel->setStyleSheet("background-color: #333; border-radius: 8px;");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(15);

    // 搜索区
    m_editSubjectSearch = new QLineEdit();
    m_editSubjectSearch->setPlaceholderText("搜索学科...");
    m_editSubjectSearch->setFixedHeight(35);
    m_editSubjectSearch->setStyleSheet("background: #2b2b2b; color: white; border: 1px solid #555; border-radius: 4px; padding-left:5px;");
    connect(m_editSubjectSearch, &QLineEdit::returnPressed, this, &MainWindow::refreshSubjectTable);

    QPushButton *btnSearch = new QPushButton("🔍 查询");
    btnSearch->setFixedHeight(35);
    btnSearch->setCursor(Qt::PointingHandCursor);
    btnSearch->setStyleSheet("background: #409eff; color: white; border-radius: 4px; font-weight:bold;");
    connect(btnSearch, &QPushButton::clicked, this, &MainWindow::refreshSubjectTable);

    // 添加区
    QLabel *lblAdd = new QLabel("➕ 新增学科 (New Subject)");
    lblAdd->setStyleSheet("color: #67c23a; font-weight: bold; margin-top: 20px;");

    m_inputSubjectName = new QLineEdit(); m_inputSubjectName->setPlaceholderText("学科名称 (如: 高等数学)");
    m_inputSubjectCode = new QLineEdit(); m_inputSubjectCode->setPlaceholderText("学科代码 (如: MATH01)");

    // 部门选择 (完整性约束)
    m_comboSubjectDept = new QComboBox();
    m_comboSubjectDept->setPlaceholderText("选择所属专业/系");

    m_comboSubjectStatus = new QComboBox();
    m_comboSubjectStatus->addItems({"Active", "Inactive"}); // 状态

    // 统一输入框样式
    QString inputStyle = "QLineEdit, QComboBox { background: #2b2b2b; color: white; border: 1px solid #555; border-radius: 4px; height: 35px; padding-left: 5px; }";
    m_inputSubjectName->setStyleSheet(inputStyle);
    m_inputSubjectCode->setStyleSheet(inputStyle);
    m_comboSubjectDept->setStyleSheet(inputStyle);
    m_comboSubjectStatus->setStyleSheet(inputStyle);

    QPushButton *btnAdd = new QPushButton("立即添加");
    btnAdd->setFixedHeight(40);
    btnAdd->setCursor(Qt::PointingHandCursor);
    btnAdd->setStyleSheet("background: #67c23a; color: white; border-radius: 4px; font-weight:bold;");

    connect(btnAdd, &QPushButton::clicked, this, [=](){
        if(m_inputSubjectName->text().isEmpty() || m_comboSubjectDept->currentIndex() < 0) {
            QMessageBox::warning(this, "提示", "名称和所属系不能为空！"); return;
        }
        DataManager dm;
        if(dm.addSubject(m_inputSubjectName->text(), m_inputSubjectCode->text(),
                          m_comboSubjectDept->currentData().toInt(), m_comboSubjectStatus->currentText())) {
            QMessageBox::information(this, "成功", "学科添加成功");
            m_inputSubjectName->clear(); m_inputSubjectCode->clear();
            refreshSubjectTable();
            // 添加成功后，也要刷新 Tab2 的下拉框
            loadPrereqTargetCombo();
        }
    });

    rightLayout->addWidget(new QLabel("搜索:", rightPanel));
    rightLayout->addWidget(m_editSubjectSearch);
    rightLayout->addWidget(btnSearch);
    rightLayout->addWidget(lblAdd);
    rightLayout->addWidget(new QLabel("名称:", rightPanel));
    rightLayout->addWidget(m_inputSubjectName);
    rightLayout->addWidget(new QLabel("代码:", rightPanel));
    rightLayout->addWidget(m_inputSubjectCode);
    rightLayout->addWidget(new QLabel("所属专业/系 (公共课请选公共部):", rightPanel));
    rightLayout->addWidget(m_comboSubjectDept);
    rightLayout->addWidget(new QLabel("状态:", rightPanel));
    rightLayout->addWidget(m_comboSubjectStatus);
    rightLayout->addWidget(btnAdd);
    rightLayout->addStretch();

    tab1Layout->addWidget(m_tableSubject);
    tab1Layout->addWidget(rightPanel);

    m_tabSubject->addTab(tab1, "学科列表管理");

    // ------------------------------------
    // Tab 2: 先修课设置 (Prerequisites)
    // ------------------------------------
    QWidget *tab2 = new QWidget();
    QVBoxLayout *tab2Layout = new QVBoxLayout(tab2);

    // 顶部选择
    QHBoxLayout *topSelLayout = new QHBoxLayout();
    QLabel *lblTarget = new QLabel("当前配置学科:");
    lblTarget->setStyleSheet("color: white; font-size: 16px; font-weight: bold;");

    m_comboPrereqTarget = new QComboBox();
    m_comboPrereqTarget->setMinimumWidth(300);
    m_comboPrereqTarget->setStyleSheet(inputStyle);
    // 当选择改变时，刷新下面的穿梭框
    connect(m_comboPrereqTarget, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::refreshPrereqUI);

    topSelLayout->addWidget(lblTarget);
    topSelLayout->addWidget(m_comboPrereqTarget);
    topSelLayout->addStretch();

    // 中间穿梭框
    QHBoxLayout *shuttleLayout = new QHBoxLayout();

    // 左边：可选
    QVBoxLayout *leftListLayout = new QVBoxLayout();
    leftListLayout->addWidget(new QLabel("👈 可选课程 (Available)", tab2));
    m_listPrereqAvailable = new QListWidget();
    m_listPrereqAvailable->setStyleSheet("background: #2b2b2b; color: white; border: 1px solid #555;");
    leftListLayout->addWidget(m_listPrereqAvailable);

    // 中间：按钮
    QVBoxLayout *btnLayout = new QVBoxLayout();
    m_btnAddPrereq = new QPushButton("添加 >>");
    m_btnAddPrereq->setStyleSheet("background: #409eff; color: white; padding: 8px;");
    m_btnRemovePrereq = new QPushButton("<< 移除");
    m_btnRemovePrereq->setStyleSheet("background: #f56c6c; color: white; padding: 8px;");
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnAddPrereq);
    btnLayout->addWidget(m_btnRemovePrereq);
    btnLayout->addStretch();

    // 右边：已选
    QVBoxLayout *rightListLayout = new QVBoxLayout();
    rightListLayout->addWidget(new QLabel("👉 已设先修课 (Required)", tab2));
    m_listPrereqCurrent = new QListWidget();
    m_listPrereqCurrent->setStyleSheet("background: #2b2b2b; color: #67c23a; border: 1px solid #555; font-weight: bold;");
    rightListLayout->addWidget(m_listPrereqCurrent);

    shuttleLayout->addLayout(leftListLayout);
    shuttleLayout->addLayout(btnLayout);
    shuttleLayout->addLayout(rightListLayout);

    tab2Layout->addLayout(topSelLayout);
    tab2Layout->addLayout(shuttleLayout);

    // 穿梭框逻辑
    connect(m_btnAddPrereq, &QPushButton::clicked, this, [=](){
        // 1. 获取选中项
        if(!m_listPrereqAvailable->currentItem()) return;

        // 当前正在编辑的课程 (Target)
        int targetId = m_comboPrereqTarget->currentData().toInt();

        // 想要添加的先修课 (Pre)
        int preId = m_listPrereqAvailable->currentItem()->data(Qt::UserRole).toInt();

        DataManager dm;

        // 2. 调用新的安全添加函数
        DataManager::PrereqResult result = dm.addPrerequisiteSafe(targetId, preId);

        // 3. 根据结果反馈
        switch (result) {
        case DataManager::Success:
            // 成功，静默刷新即可，或者提示
            refreshPrereqUI();
            break;

        case DataManager::CycleDetected:
            QMessageBox::warning(this, "此操作は許可されていません",
                "DFS遍歷發現：先修課之間產生了循環依賴\n≧ ﹏ ≦！\n"
                "无法添加该课程，因为它是当前课程的后续课程。\n"
                "禁止形成閉環 (A->B->A)。");
            break;

        case DataManager::SelfLoop:
            QMessageBox::warning(this, "操作禁止", "不能将课程自身设为先修课。");
            break;

        case DataManager::AlreadyExists:
            QMessageBox::warning(this, "提示", "该先修关系已存在。");
            break;

        case DataManager::DbError:
            QMessageBox::critical(this, "错误", "数据库写入失败。");
            break;
        }
    });

    connect(m_btnRemovePrereq, &QPushButton::clicked, this, [=](){
        if(!m_listPrereqCurrent->currentItem()) return;
        int targetId = m_comboPrereqTarget->currentData().toInt();
        int preId = m_listPrereqCurrent->currentItem()->data(Qt::UserRole).toInt();

        DataManager dm;
        dm.removePrerequisite(targetId, preId);
        refreshPrereqUI(); // 刷新
    });

    m_tabSubject->addTab(tab2, "先修课关系设置");
    mainLayout->addWidget(m_tabSubject);

    // 初始化加载
    loadSubjectDeptCombo();
    refreshSubjectTable();
    loadPrereqTargetCombo();
}

// 辅助：加载“系”下拉框
void MainWindow::loadSubjectDeptCombo()
{
    m_comboSubjectDept->clear();
    DataManager dm;
    // 使用之前的 getAllDepartments (需要稍微改一下，或者直接用 getAllDepartments 返回的数据)
    // 这里为了方便，我们假设 DataManager 有一个 getAllDepartments() 返回 DeptItem
    auto depts = dm.getAllDepartments();
    for(const auto &d : depts) {
        // displayText: "计算机科学与技术 (计算机学院)"
        m_comboSubjectDept->addItem(QString("%1 (%2)").arg(d.name, d.collegeName), d.id);
    }
}

// 辅助：加载 Tab2 的目标课程下拉框
void MainWindow::loadPrereqTargetCombo()
{
    m_comboPrereqTarget->blockSignals(true);
    m_comboPrereqTarget->clear();
    DataManager dm;
    auto subs = dm.getAllSubjects();
    for(const auto &s : subs) {
        m_comboPrereqTarget->addItem(s.name, s.id);
    }
    m_comboPrereqTarget->blockSignals(false);

    // 主动触发一次刷新
    if(m_comboPrereqTarget->count() > 0) refreshPrereqUI();
}

// 刷新 Tab1 表格
void MainWindow::refreshSubjectTable()
{
    DataManager dm;
    auto list = dm.getAllSubjects(m_editSubjectSearch->text().trimmed());

    m_tableSubject->setRowCount(0);
    for(int i=0; i<list.size(); ++i) {
        const auto &data = list[i];
        m_tableSubject->insertRow(i);
        m_tableSubject->setItem(i, 0, new QTableWidgetItem(QString::number(data.id)));
        m_tableSubject->setItem(i, 1, new QTableWidgetItem(data.name));
        m_tableSubject->setItem(i, 2, new QTableWidgetItem(data.code));
        m_tableSubject->setItem(i, 3, new QTableWidgetItem(data.deptName));
        m_tableSubject->setItem(i, 4, new QTableWidgetItem(data.status));

        // 操作栏
        QWidget *actionWidget = new QWidget();
        QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(5,5,5,5);

        QPushButton *btnEdit = new QPushButton("修改");
        btnEdit->setFixedSize(50, 30);
        btnEdit->setStyleSheet("color:#409eff; background:transparent; border:1px solid #409eff; border-radius:4px;");

        QPushButton *btnDel = new QPushButton("删除");
        btnDel->setFixedSize(50, 30);
        btnDel->setStyleSheet("color:#f56c6c; background:transparent; border:1px solid #f56c6c; border-radius:4px;");

        actionLayout->addStretch();
        actionLayout->addWidget(btnEdit);
        actionLayout->addWidget(btnDel);
        actionLayout->addStretch();
        m_tableSubject->setCellWidget(i, 5, actionWidget);

        for(int k=1; k<=4; k++) m_tableSubject->item(i, k)->setTextAlignment(Qt::AlignCenter);

        connect(btnEdit, &QPushButton::clicked, [=](){
            showSubjectEditDialog(data.id, data.name, data.code, data.deptId, data.status);
        });
        connect(btnDel, &QPushButton::clicked, [=](){
            if(QMessageBox::Yes == QMessageBox::question(this, "确认", "确定删除该学科吗？")) {
                DataManager tempDm;
                tempDm.deleteSubject(data.id);
                refreshSubjectTable();
                loadPrereqTargetCombo(); // 删了课，先修课列表也要刷
            }
        });
    }
}

// 刷新 Tab2 穿梭框
void MainWindow::refreshPrereqUI()
{
    m_listPrereqAvailable->clear();
    m_listPrereqCurrent->clear();

    int currentSubjectId = m_comboPrereqTarget->currentData().toInt();
    if(currentSubjectId <= 0) return;

    DataManager dm;

    // --- 修改部分开始 ---

    // 1. 获取所有递归先修课
    auto allPrereqs = dm.getAllTransitivePrerequisites(currentSubjectId);

    // 2. 获取直接先修课（用于区分显示颜色或标记）
    auto directPrereqs = dm.getPrerequisites(currentSubjectId);
    QSet<int> directIds;
    for(auto p : directPrereqs) directIds.insert(p.first);

    // 3. 显示
    for(const auto &p : allPrereqs) {
        QString label = p.second;
        QListWidgetItem *item = new QListWidgetItem();

        // 区分显示：直接先修课显示正常，间接先修课显示 "(间接)"
        if (directIds.contains(p.first)) {
            item->setText(label + " [直接]");
            item->setForeground(QColor("#67c23a")); // 绿色
        } else {
            item->setText(label + " [间接]");
            item->setForeground(QColor("#e6a23c")); // 橙色，表示是递归出来的
        }

        item->setData(Qt::UserRole, p.first);
        m_listPrereqCurrent->addItem(item);
    }

    // --- 修改部分结束 ---

    // 4. 获取可选的 (逻辑不变，依然排除所有已选的)
    // 注意：getAvailablePrerequisites 内部逻辑是 NOT IN (...)
    // 你可能需要修改 getAvailablePrerequisites 让它也排除掉那些间接先修课，
    // 否则用户可能会尝试把“爷爷课”再次加为“爸爸课”，虽然逻辑上没错但有点冗余。

    // 简单做法：这里手动过滤一下 UI 列表
    auto availList = dm.getAllSubjects(); // 获取全部
    QSet<int> existingIds;
    for(auto p : allPrereqs) existingIds.insert(p.first); // 所有祖先都不能再选
    existingIds.insert(currentSubjectId); // 自己也不能选

    for(const auto &s : availList) {
        if (!existingIds.contains(s.id)) {
            QListWidgetItem *item = new QListWidgetItem(s.name);
            item->setData(Qt::UserRole, s.id);
            m_listPrereqAvailable->addItem(item);
        }
    }
}

// 修改弹窗
void MainWindow::showSubjectEditDialog(int id, QString name, QString code, int deptId, QString status)
{
    QDialog dlg(this);
    dlg.setWindowTitle("修改学科信息");
    QFormLayout *layout = new QFormLayout(&dlg);

    QLineEdit *edtName = new QLineEdit(name);
    QLineEdit *edtCode = new QLineEdit(code);
    QComboBox *comboDept = new QComboBox();
    QComboBox *comboStat = new QComboBox();

    // 填充下拉框
    DataManager dm;
    auto depts = dm.getAllDepartments();
    for(auto d : depts) comboDept->addItem(d.name, d.id);
    comboDept->setCurrentIndex(comboDept->findData(deptId));

    comboStat->addItems({"Active", "Inactive"});
    comboStat->setCurrentText(status);

    layout->addRow("名称:", edtName);
    layout->addRow("代码:", edtCode);
    layout->addRow("所属系:", comboDept);
    layout->addRow("状态:", comboStat);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addRow(btnBox);
    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if(dlg.exec() == QDialog::Accepted) {
        if(dm.updateSubject(id, edtName->text(), edtCode->text(), comboDept->currentData().toInt(), comboStat->currentText())) {
            QMessageBox::information(this, "成功", "修改成功");
            refreshSubjectTable();
        }
    }
}

// ==================================================
// 1. 构建排课管理界面
// ==================================================

// 辅助：加载所有下拉框
void MainWindow::loadCourseCombos()
{
    DataManager dm;

    // 1. 学期
    m_comboCourseTerm->clear();
    auto terms = dm.getAllSemesters();
    for(auto t : terms) m_comboCourseTerm->addItem(t.displayText, t.id);

    // 2. 学科
    m_comboCourseSubject->clear();
    auto subs = dm.getAllSubjects();
    for(auto s : subs) m_comboCourseSubject->addItem(s.name + " (" + s.code + ")", s.id);

    // 3. 教师 (这里需要一个新的getAllTeachers函数，或者复用之前的)
    // 假设 DataManager 里已经有了 getAllTeachers()
    // m_comboCourseTeacher->clear();
    // auto teachers = dm.getAllTeachers(); // 假设返回 {id, name...}
    // for(auto t : teachers) m_comboCourseTeacher->addItem(t.name, t.id);

    // === 临时补丁：如果 DataManager 还没有 getAllTeachers，先手动查一下 ===
    m_comboCourseTeacher->clear();
    QSqlQuery q("SELECT teacher_id, teacher_name FROM teacher");
    while(q.next()) {
        m_comboCourseTeacher->addItem(q.value(1).toString(), q.value(0).toInt());
    }
}

// 刷新表格
void MainWindow::refreshCourseTable()
{
    DataManager dm;
    // 获取当前选中的学期，如果下拉框空则传 -1
    int termId = m_comboCourseTerm->currentData().isValid() ? m_comboCourseTerm->currentData().toInt() : -1;

    auto list = dm.getAllCourseSections(termId);

    m_tableCourse->setRowCount(0);
    for(int i=0; i<list.size(); ++i) {
        const auto &data = list[i];
        m_tableCourse->insertRow(i);

        m_tableCourse->setItem(i, 0, new QTableWidgetItem(QString::number(data.sectionId)));
        m_tableCourse->setItem(i, 1, new QTableWidgetItem(data.termName));
        m_tableCourse->setItem(i, 2, new QTableWidgetItem(data.subjectName));
        m_tableCourse->setItem(i, 3, new QTableWidgetItem(data.teacherName));
        m_tableCourse->setItem(i, 4, new QTableWidgetItem(data.room));

        QString info = QString("%1人 / %2次课").arg(data.maxStudents).arg(data.scheduleCount);
        m_tableCourse->setItem(i, 5, new QTableWidgetItem(info));

        // 删除按钮
        QPushButton *btnDel = new QPushButton("删除排课");
        btnDel->setFixedSize(70, 30);
        btnDel->setCursor(Qt::PointingHandCursor);
        btnDel->setStyleSheet("color: #f56c6c; background: transparent; border: 1px solid #f56c6c; border-radius: 4px;");

        QWidget *w = new QWidget(); QHBoxLayout *l = new QHBoxLayout(w);
        l->setContentsMargins(0,0,0,0); l->addWidget(btnDel); l->setAlignment(Qt::AlignCenter);
        m_tableCourse->setCellWidget(i, 6, w);

        connect(btnDel, &QPushButton::clicked, [=](){
            if(QMessageBox::Yes == QMessageBox::question(this, "警告", "删除此排课将连带删除所有具体的上课时间安排。\n确定要继续吗？")) {
                DataManager tempDm;
                if(tempDm.deleteCourseSection(data.sectionId)) {
                    refreshCourseTable();
                } else {
                    QMessageBox::warning(this, "失败", "删除失败");
                }
            }
        });

        for(int k=1; k<=5; k++) m_tableCourse->item(i, k)->setTextAlignment(Qt::AlignCenter);
    }
}

// ==================================================
// 1. 构建选课管理界面
// ==================================================
void MainWindow::setupAdminSelectionUi()
{
    QWidget *page = ui->admin_courseSelect; // 假设你在 UI 设计器里叫这个名字
    // 如果还没改名，可能是 admin_selection_manage 之类的，请检查 UI 文件
    if (!page) return; // 容错
    if (page->layout()) {
        refreshSelectionTable(); // 只刷新表格
        return;
    }

    QHBoxLayout *mainLayout = new QHBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // ------------------------------------
    // A. 左侧：选课总表 (Table)
    // ------------------------------------
    m_tableSelection = new QTableWidget(page);
    m_tableSelection->setColumnCount(6);
    m_tableSelection->setHorizontalHeaderLabels({"学号", "姓名", "课程名称", "任课教师", "成绩", "操作"});

    m_tableSelection->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableSelection->verticalHeader()->setDefaultSectionSize(50); // 行高
    m_tableSelection->verticalHeader()->setVisible(false);
    m_tableSelection->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableSelection->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 深色样式
    m_tableSelection->setStyleSheet(R"(
        QTableWidget { background: #2b2b2b; border: none; color: #ddd; }
        QHeaderView::section { background: #333; color: white; height: 35px; border:none; border-bottom: 2px solid #409eff; }
        QTableWidget::item { border-bottom: 1px solid #333; }
        QTableWidget::item:selected { background: #409eff; color: white; }
    )");

    mainLayout->addWidget(m_tableSelection, 7); // 左侧占 70%

    // ------------------------------------
    // B. 右侧：控制面板 (Panel)
    // ------------------------------------
    QFrame *rightPanel = new QFrame(page);
    rightPanel->setStyleSheet("QFrame { background-color: #333; border-radius: 8px; } QLabel { color: #ddd; }");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(20);
    rightLayout->setContentsMargins(20, 30, 20, 30);

    // 标题
    QLabel *lblTitle = new QLabel("📋 选课管理控制台", rightPanel);
    lblTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #fff;");
    rightLayout->addWidget(lblTitle);

    // 统计标签
    m_lblSelectionStats = new QLabel("共加载 0 条记录", rightPanel);
    m_lblSelectionStats->setStyleSheet("color: #aaa; margin-bottom: 10px;");
    rightLayout->addWidget(m_lblSelectionStats);

    // 分割线
    QFrame *line1 = new QFrame; line1->setFrameShape(QFrame::HLine); line1->setStyleSheet("color:#555");
    rightLayout->addWidget(line1);

    // 搜索区
    QLabel *lblSearch = new QLabel("🔍 检索学生/课程:", rightPanel);
    lblSearch->setStyleSheet("font-weight: bold;");

    m_editSelectionSearch = new QLineEdit(rightPanel);
    m_editSelectionSearch->setPlaceholderText("输入学号、姓名或课名...");
    m_editSelectionSearch->setFixedHeight(35);
    m_editSelectionSearch->setStyleSheet("background: #2b2b2b; color: white; border: 1px solid #555; border-radius: 4px; padding-left: 5px;");
    connect(m_editSelectionSearch, &QLineEdit::returnPressed, this, &MainWindow::refreshSelectionTable);

    QPushButton *btnSearch = new QPushButton("执行筛选", rightPanel);
    btnSearch->setFixedHeight(35);
    btnSearch->setCursor(Qt::PointingHandCursor);
    btnSearch->setStyleSheet("background: #409eff; color: white; border-radius: 4px; font-weight: bold;");
    connect(btnSearch, &QPushButton::clicked, this, &MainWindow::refreshSelectionTable);

    rightLayout->addWidget(lblSearch);
    rightLayout->addWidget(m_editSelectionSearch);
    rightLayout->addWidget(btnSearch);

    // 底部弹簧
    rightLayout->addStretch();

    // 提示信息
    QLabel *lblTip = new QLabel("💡 提示：\n点击左侧表格中的按钮\n可进行 [强制退课] 或 [修正成绩]。", rightPanel);
    lblTip->setStyleSheet("color: #888; font-size: 12px; font-style: italic;");
    rightLayout->addWidget(lblTip);

    mainLayout->addWidget(rightPanel, 3); // 右侧占 30%

    // 初始加载
    refreshSelectionTable();
}

// ==================================================
// 2. 刷新表格逻辑
// ==================================================
void MainWindow::refreshSelectionTable()
{
    DataManager dm;
    QString key = m_editSelectionSearch->text().trimmed();
    auto list = dm.getAllSelections(key);

    m_tableSelection->setRowCount(0);
    m_lblSelectionStats->setText(QString("共加载 %1 条选课记录").arg(list.size()));

    for(int i = 0; i < list.size(); ++i) {
        const auto &data = list[i];
        m_tableSelection->insertRow(i);

        // 填表
        m_tableSelection->setItem(i, 0, new QTableWidgetItem(data.studentNum));
        m_tableSelection->setItem(i, 1, new QTableWidgetItem(data.studentName));
        m_tableSelection->setItem(i, 2, new QTableWidgetItem(data.courseName));
        m_tableSelection->setItem(i, 3, new QTableWidgetItem(data.teacherName));

        // 成绩显示
        QString scoreStr = (data.score < 0) ? "--" : QString::number(data.score, 'f', 1);
        QTableWidgetItem *scoreItem = new QTableWidgetItem(scoreStr);
        if(data.score >= 0 && data.score < 60) scoreItem->setForeground(QColor("#f56c6c")); // 挂科红
        else if(data.score >= 60) scoreItem->setForeground(QColor("#67c23a")); // 及格绿

        scoreItem->setTextAlignment(Qt::AlignCenter);
        m_tableSelection->setItem(i, 4, scoreItem);

        // --- 操作栏 (双按钮) ---
        QWidget *actionWidget = new QWidget();
        QHBoxLayout *actLayout = new QHBoxLayout(actionWidget);
        actLayout->setContentsMargins(5, 5, 5, 5);
        actLayout->setSpacing(10);

        // 1. 改分按钮
        QPushButton *btnEdit = new QPushButton("改分");
        btnEdit->setFixedSize(50, 30);
        btnEdit->setCursor(Qt::PointingHandCursor);
        btnEdit->setStyleSheet("color: #e6a23c; background: transparent; border: 1px solid #e6a23c; border-radius: 4px;");

        // 2. 退课按钮
        QPushButton *btnDrop = new QPushButton("退课");
        btnDrop->setFixedSize(50, 30);
        btnDrop->setCursor(Qt::PointingHandCursor);
        btnDrop->setStyleSheet("color: #f56c6c; background: transparent; border: 1px solid #f56c6c; border-radius: 4px;");

        actLayout->addStretch();
        actLayout->addWidget(btnEdit);
        actLayout->addWidget(btnDrop);
        actLayout->addStretch();

        m_tableSelection->setCellWidget(i, 5, actionWidget);

        // 居中其他列
        for(int k=0; k<4; k++) m_tableSelection->item(i, k)->setTextAlignment(Qt::AlignCenter);

        // --- 逻辑连接 ---

        // 改分逻辑
        connect(btnEdit, &QPushButton::clicked, this, [=](){
            bool ok;
            double oldScore = (data.score < 0) ? 0 : data.score;
            double newScore = QInputDialog::getDouble(this, "修正成绩",
                                                      QString("正在修改 %1 的 [%2] 成绩:\n请输入新成绩 (0-100):").arg(data.studentName, data.courseName),
                                                      oldScore, 0, 100, 1, &ok);

            if(ok) {
                DataManager tempDm;
                if(tempDm.adminUpdateScore(data.studentId, data.sectionId, newScore)) {
                    QMessageBox::information(this, "成功", "成绩已修正。");
                    refreshSelectionTable();
                } else {
                    QMessageBox::critical(this, "失败", "修改失败。");
                }
            }
        });

        // 退课逻辑
        connect(btnDrop, &QPushButton::clicked, this, [=](){
            QString msg = QString("⚠️ 警告：强制退课\n\n学生: %1 (%2)\n课程: %3\n\n确定要强制删除这条选课记录吗？").arg(data.studentName, data.studentNum, data.courseName);
            if(QMessageBox::Yes == QMessageBox::question(this, "确认操作", msg)) {
                DataManager tempDm;
                if(tempDm.adminDropCourse(data.studentId, data.sectionId)) {
                    QMessageBox::information(this, "成功", "已强制退课。");
                    refreshSelectionTable();
                } else {
                    QMessageBox::warning(this, "失败", "操作失败。");
                }
            }
        });
    }
}

// ==================================================
// 1. 初始化学生管理页面 (连接 UI 控件)
// ==================================================
void MainWindow::setupAdminStudentUi()
{
    // 这个页面是你用 UI Designer 画的 (page_admin_dashboard)，不需要 new 布局
    // 我们只需要连接信号槽，并初始化表格样式

    // 1. 设置表格样式 (tableWidget)
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 设置表头 (UI里可能没设，这里强制设一下)
    // 假设你的 getStudents 返回顺序是: ID, Name, Number, ClassName
    ui->tableWidget->setColumnCount(4);
    ui->tableWidget->setHorizontalHeaderLabels({"ID", "姓名", "学号", "行政班级"});
    ui->tableWidget->setColumnHidden(0, true); // 隐藏 ID 列

    // 2. 连接“查询”按钮 (你之前写过，这里封装一下)
    disconnect(ui->pushButton_query, nullptr, nullptr, nullptr); // 防止重复连接
    connect(ui->pushButton_query, &QPushButton::clicked, this, &MainWindow::refreshStudentTable);

    // 3. 连接“添加学生”按钮 (pushButton_add)
    disconnect(ui->pushButton_add, nullptr, nullptr, nullptr);
    connect(ui->pushButton_add, &QPushButton::clicked, this, [=](){
        showStudentEditDialog(false); // false = 新增模式
    });

    // 4. 连接“编辑学生信息”按钮 (pushButton_2)
    disconnect(ui->pushButton_edit, nullptr, nullptr, nullptr);
    connect(ui->pushButton_edit, &QPushButton::clicked, this, [=](){
        // 获取当前选中行
        int row = ui->tableWidget->currentRow();
        if(row < 0) {
            QMessageBox::warning(this, "提示", "请先选择一名学生！");
            return;
        }

        // 获取表格里的数据 (假设顺序: 0=ID, 1=Name, 2=Number, 3=ClassName)
        int id = ui->tableWidget->item(row, 0)->text().toInt();
        QString name = ui->tableWidget->item(row, 1)->text();
        QString num = ui->tableWidget->item(row, 2)->text();
        QString clsName = ui->tableWidget->item(row, 3)->text();

        // 弹出编辑框
        showStudentEditDialog(true, id, name, num, clsName);
    });

    // 5. 初始刷新一次数据
    refreshStudentTable();
}

// ==================================================
// 2. 刷新学生表格 (封装你之前的逻辑)
// ==================================================
void MainWindow::refreshStudentTable()
{
    ui->tableWidget->setRowCount(0);
    QString name = ui->lineEdit_Search->text().trimmed();

    DataManager dm;
    QList<QStringList> dataList = dm.getStudents(name);

    for(int i = 0; i < dataList.size(); ++i)
    {
        QStringList rowData = dataList[i];
        ui->tableWidget->insertRow(i);

        // 0. ID
        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(rowData[0]));
        // 1. Name
        ui->tableWidget->setItem(i, 1, new QTableWidgetItem(rowData[1]));
        // 2. Number
        ui->tableWidget->setItem(i, 2, new QTableWidgetItem(rowData[2]));
        // 3. ClassName
        ui->tableWidget->setItem(i, 3, new QTableWidgetItem(rowData[3]));

        // 居中
        for(int k=0; k<4; k++)
            if(ui->tableWidget->item(i, k))
                ui->tableWidget->item(i, k)->setTextAlignment(Qt::AlignCenter);
    }
}

// ==================================================
// 3. 学生编辑/新增弹窗 (含班级下拉框)
// ==================================================
void MainWindow::showStudentEditDialog(bool isEdit, int id, QString name, QString number, QString className)
{
    QDialog dlg(this);
    dlg.setWindowTitle(isEdit ? "编辑学生信息" : "添加新学生");
    dlg.setFixedSize(350, 250);

    QFormLayout *layout = new QFormLayout(&dlg);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(15);

    QLineEdit *edtName = new QLineEdit(&dlg);
    edtName->setText(name);
    edtName->setPlaceholderText("请输入姓名");

    QLineEdit *edtNum = new QLineEdit(&dlg);
    edtNum->setText(number);
    edtNum->setPlaceholderText("请输入学号");

    // 班级下拉框 (从数据库加载)
    QComboBox *comboClass = new QComboBox(&dlg);

    DataManager dm;
    auto classes = dm.getAllAdminClasses(); // 获取 {id, name} 列表
    for(const auto &c : classes) {
        comboClass->addItem(c.second, c.first); // Text=班级名, Data=班级ID
    }

    //如果是编辑模式，自动选中当前的班级
    if(isEdit) {
        // 通过文本查找 (因为表格里只显示了班级名，没存ID，用名字匹配最方便)
        int idx = comboClass->findText(className);
        if(idx >= 0) comboClass->setCurrentIndex(idx);
    }

    layout->addRow("姓名:", edtName);
    layout->addRow("学号:", edtNum);
    layout->addRow("行政班级:", comboClass);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addRow(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        QString newName = edtName->text().trimmed();
        QString newNum = edtNum->text().trimmed();
        int classId = comboClass->currentData().toInt(); // 获取选中的 ID

        if (newName.isEmpty() || newNum.isEmpty()) {
            QMessageBox::warning(this, "警告", "姓名和学号不能为空");
            return;
        }

        bool success = false;
        if (isEdit) {
            success = dm.updateStudent(id, newName, newNum, classId);
        } else {
            success = dm.addStudent(newName, newNum, classId);
        }

        if (success) {
            QMessageBox::information(this, "成功", "操作成功");
            refreshStudentTable(); // 刷新表格
        } else {
            QMessageBox::critical(this, "失败", "数据库操作失败");
        }
    }
}


// 构建教师管理界面

void MainWindow::setupAdminTeacherUi()
{
    QWidget *page = ui->admin_teacher_manage;
    if (!page) return;
    if (page->layout()) {
        loadTeacherCombos();   // 刷新下拉框
        refreshTeacherTable(); // 刷新表格
        return;
    }

    QHBoxLayout *mainLayout = new QHBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // ------------------------------------
    // A. 左侧：教师列表 (Table)
    // ------------------------------------
    m_tableTeacher = new QTableWidget();
    m_tableTeacher->setColumnCount(6); // ID, 姓名, 工号, 职称, 所属系, 操作
    m_tableTeacher->setHorizontalHeaderLabels({"ID", "教师姓名", "工号", "职称", "所属院系", "操作"});
    m_tableTeacher->setColumnHidden(0, true);

    m_tableTeacher->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableTeacher->verticalHeader()->setDefaultSectionSize(50);
    m_tableTeacher->verticalHeader()->setVisible(false);
    m_tableTeacher->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableTeacher->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_tableTeacher->setStyleSheet(R"(
        QTableWidget { background: #2b2b2b; border: none; color: #ddd; }
        QHeaderView::section { background: #333; color: white; height: 35px; border:none; border-bottom: 2px solid #409eff; }
        QTableWidget::item { border-bottom: 1px solid #333; }
        QTableWidget::item:selected { background: #409eff; color: white; }
    )");

    mainLayout->addWidget(m_tableTeacher, 7);

    // ------------------------------------
    // B. 右侧：控制面板 (Panel)
    // ------------------------------------
    QFrame *rightPanel = new QFrame();
    rightPanel->setStyleSheet("QFrame { background-color: #333; border-radius: 8px; } QLabel { color: #ddd; font-size: 14px; }");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(15);
    rightLayout->setContentsMargins(20, 20, 20, 20);

    // B1. 搜索
    QLabel *lblSearch = new QLabel("查找教师", rightPanel);
    lblSearch->setStyleSheet("font-weight: bold; font-size: 16px; color: white;");

    m_editTeacherSearch = new QLineEdit();
    m_editTeacherSearch->setPlaceholderText("姓名或工号...");
    m_editTeacherSearch->setFixedHeight(35);
    m_editTeacherSearch->setStyleSheet("background: #2b2b2b; color: white; border: 1px solid #555; border-radius: 4px; padding-left:5px;");
    connect(m_editTeacherSearch, &QLineEdit::returnPressed, this, &MainWindow::refreshTeacherTable);

    QPushButton *btnSearch = new QPushButton("查询");
    btnSearch->setFixedHeight(35);
    btnSearch->setCursor(Qt::PointingHandCursor);
    btnSearch->setStyleSheet("background: #409eff; color: white; border-radius: 4px; font-weight: bold;");
    connect(btnSearch, &QPushButton::clicked, this, &MainWindow::refreshTeacherTable);

    rightLayout->addWidget(lblSearch);
    rightLayout->addWidget(m_editTeacherSearch);
    rightLayout->addWidget(btnSearch);

    // 分割线
    QFrame *line = new QFrame; line->setFrameShape(QFrame::HLine); line->setStyleSheet("color:#555");
    rightLayout->addWidget(line);

    // B2. 快速添加
    QLabel *lblAdd = new QLabel("录入新教师", rightPanel);
    lblAdd->setStyleSheet("font-weight: bold; font-size: 16px; color: #67c23a;");

    QString inputStyle = "background: #2b2b2b; color: white; border: 1px solid #555; border-radius: 4px; height: 35px; padding-left: 5px;";

    m_inputTeacherName = new QLineEdit(); m_inputTeacherName->setPlaceholderText("教师姓名"); m_inputTeacherName->setStyleSheet(inputStyle);
    m_inputTeacherNum = new QLineEdit(); m_inputTeacherNum->setPlaceholderText("工号 (Login ID)"); m_inputTeacherNum->setStyleSheet(inputStyle);

    // 职称下拉框
    m_comboTeacherTitle = new QComboBox();
    m_comboTeacherTitle->addItems({"教授", "副教授", "讲师", "助教", "研究员"});
    m_comboTeacherTitle->setStyleSheet(inputStyle);

    // 院系下拉框 (完整性约束)
    m_comboTeacherDept = new QComboBox(); m_comboTeacherDept->setStyleSheet(inputStyle);

    QPushButton *btnAdd = new QPushButton("确认录入");
    btnAdd->setFixedHeight(40);
    btnAdd->setCursor(Qt::PointingHandCursor);
    btnAdd->setStyleSheet("background: #67c23a; color: white; border-radius: 4px; font-weight: bold;");

    // 添加逻辑
    connect(btnAdd, &QPushButton::clicked, this, [=](){
        if(m_inputTeacherName->text().isEmpty() || m_inputTeacherNum->text().isEmpty()) {
            QMessageBox::warning(this, "提示", "姓名和工号不能为空"); return;
        }
        if(m_comboTeacherDept->currentIndex() < 0) {
            QMessageBox::warning(this, "提示", "请选择所属院系"); return;
        }

        DataManager dm;
        if(dm.addTeacher(m_inputTeacherName->text(), m_inputTeacherNum->text(),
                          m_comboTeacherTitle->currentText(), m_comboTeacherDept->currentData().toInt())) {
            QMessageBox::information(this, "成功", "教师录入成功！\n默认密码为工号。");
            m_inputTeacherName->clear(); m_inputTeacherNum->clear();
            refreshTeacherTable();
        } else {
            QMessageBox::critical(this, "失败", "录入失败 (工号可能重复)");
        }
    });

    rightLayout->addWidget(lblAdd);
    rightLayout->addWidget(new QLabel("姓名:", rightPanel));
    rightLayout->addWidget(m_inputTeacherName);
    rightLayout->addWidget(new QLabel("工号:", rightPanel));
    rightLayout->addWidget(m_inputTeacherNum);
    rightLayout->addWidget(new QLabel("职称:", rightPanel));
    rightLayout->addWidget(m_comboTeacherTitle);
    rightLayout->addWidget(new QLabel("所属院系:", rightPanel));
    rightLayout->addWidget(m_comboTeacherDept);
    rightLayout->addWidget(btnAdd);
    rightLayout->addStretch();

    mainLayout->addWidget(rightPanel, 3);

    // 初始化数据
    loadTeacherCombos();
    refreshTeacherTable();
}

// 辅助：加载下拉框
void MainWindow::loadTeacherCombos()
{
    m_comboTeacherDept->clear();
    DataManager dm;
    // 复用之前的部门查询
    auto depts = dm.getAllDepartments();
    for(const auto &d : depts) {
        m_comboTeacherDept->addItem(d.name, d.id);
    }
}

// 刷新表格
void MainWindow::refreshTeacherTable()
{
    DataManager dm;
    auto list = dm.getAllTeachers(m_editTeacherSearch->text().trimmed());

    m_tableTeacher->setRowCount(0);
    for(int i=0; i<list.size(); ++i) {
        const auto &data = list[i];
        m_tableTeacher->insertRow(i);

        m_tableTeacher->setItem(i, 0, new QTableWidgetItem(QString::number(data.id)));
        m_tableTeacher->setItem(i, 1, new QTableWidgetItem(data.name));
        m_tableTeacher->setItem(i, 2, new QTableWidgetItem(data.number));
        m_tableTeacher->setItem(i, 3, new QTableWidgetItem(data.title));
        m_tableTeacher->setItem(i, 4, new QTableWidgetItem(data.deptName));

        // 操作栏
        QWidget *actionWidget = new QWidget();
        QHBoxLayout *actLayout = new QHBoxLayout(actionWidget);
        actLayout->setContentsMargins(5, 5, 5, 5);
        actLayout->setSpacing(10);

        QPushButton *btnEdit = new QPushButton("修改");
        btnEdit->setFixedSize(50, 30);
        btnEdit->setCursor(Qt::PointingHandCursor);
        btnEdit->setStyleSheet("color: #409eff; background: transparent; border: 1px solid #409eff; border-radius: 4px;");

        QPushButton *btnDel = new QPushButton("删除");
        btnDel->setFixedSize(50, 30);
        btnDel->setCursor(Qt::PointingHandCursor);
        btnDel->setStyleSheet("color: #f56c6c; background: transparent; border: 1px solid #f56c6c; border-radius: 4px;");

        actLayout->addStretch();
        actLayout->addWidget(btnEdit);
        actLayout->addWidget(btnDel);
        actLayout->addStretch();

        m_tableTeacher->setCellWidget(i, 5, actionWidget);

        for(int k=1; k<=4; k++) m_tableTeacher->item(i, k)->setTextAlignment(Qt::AlignCenter);

        // 修改事件
        connect(btnEdit, &QPushButton::clicked, [=](){
            showTeacherEditDialog(data.id, data.name, data.number, data.title, data.deptId);
        });

        // 删除事件
        connect(btnDel, &QPushButton::clicked, [=](){
            if(QMessageBox::Yes == QMessageBox::question(this, "警告", "确定删除该教师吗？\n如果该教师已有排课记录，操作可能会失败。")) {
                DataManager tempDm;
                if(tempDm.deleteTeacher(data.id)) {
                    QMessageBox::information(this, "成功", "删除成功");
                    refreshTeacherTable();
                } else {
                    QMessageBox::warning(this, "失败", "删除失败，请检查是否有关联排课。");
                }
            }
        });
    }
}

// 编辑弹窗
void MainWindow::showTeacherEditDialog(int id, QString name, QString num, QString title, int deptId)
{
    QDialog dlg(this);
    dlg.setWindowTitle("编辑教师信息");
    dlg.setFixedSize(350, 300);
    QFormLayout *layout = new QFormLayout(&dlg);

    QLineEdit *edtName = new QLineEdit(name);
    QLineEdit *edtNum = new QLineEdit(num);

    QComboBox *comboTitle = new QComboBox();
    comboTitle->addItems({"教授", "副教授", "讲师", "助教", "研究员"});
    comboTitle->setCurrentText(title);

    QComboBox *comboDept = new QComboBox();
    DataManager dm;
    auto depts = dm.getAllDepartments();
    for(auto d : depts) comboDept->addItem(d.name, d.id);
    comboDept->setCurrentIndex(comboDept->findData(deptId));

    layout->addRow("姓名:", edtName);
    layout->addRow("工号:", edtNum);
    layout->addRow("职称:", comboTitle);
    layout->addRow("院系:", comboDept);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addRow(btnBox);
    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if(dlg.exec() == QDialog::Accepted) {
        if(dm.updateTeacher(id, edtName->text(), edtNum->text(), comboTitle->currentText(), comboDept->currentData().toInt())) {
            QMessageBox::information(this, "成功", "修改成功");
            refreshTeacherTable();
        }
    }
}

// ==================================================
// 1. 构建账号管理界面
// ==================================================
void MainWindow::setupAdminAccountUi()
{
    QWidget *page = ui->admin_account_manage;
    if (!page) return;
    if (page->layout()) {
        refreshAccountTable(); // 只刷新表格
        return;
    }

    QHBoxLayout *mainLayout = new QHBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // ------------------------------------
    // A. 左侧：账号列表 (70%)
    // ------------------------------------
    m_tableAccount = new QTableWidget();
    m_tableAccount->setColumnCount(6); // 角色, 姓名, 学号/工号, 账号状态, 密码, 操作
    m_tableAccount->setHorizontalHeaderLabels({"角色", "姓名", "学号/工号", "状态", "密码", "操作"});

    m_tableAccount->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableAccount->verticalHeader()->setDefaultSectionSize(50);
    m_tableAccount->verticalHeader()->setVisible(false);
    m_tableAccount->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableAccount->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_tableAccount->setStyleSheet(R"(
        QTableWidget { background: #2b2b2b; border: none; color: #ddd; }
        QHeaderView::section { background: #333; color: white; height: 35px; border:none; border-bottom: 2px solid #409eff; }
        QTableWidget::item { border-bottom: 1px solid #333; }
    )");

    mainLayout->addWidget(m_tableAccount, 7);

    // ------------------------------------
    // B. 右侧：控制面板 (30%)
    // ------------------------------------
    QFrame *rightPanel = new QFrame();
    rightPanel->setStyleSheet("QFrame { background-color: #333; border-radius: 8px; } QLabel { color: #ddd; }");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(20);
    rightLayout->setContentsMargins(20, 30, 20, 30);

    // 标题
    QLabel *lblTitle = new QLabel("账号管理中心", rightPanel);
    lblTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #fff;");
    rightLayout->addWidget(lblTitle);

    // 统计状态
    m_lblAccountStats = new QLabel("detecting...", rightPanel);
    m_lblAccountStats->setStyleSheet("color: #f56c6c; font-weight: bold; margin-bottom: 10px;");
    rightLayout->addWidget(m_lblAccountStats);

    // 分割线
    QFrame *line = new QFrame; line->setFrameShape(QFrame::HLine); line->setStyleSheet("color:#555");
    rightLayout->addWidget(line);

    // 搜索
    m_editAccountSearch = new QLineEdit();
    m_editAccountSearch->setPlaceholderText("搜姓名或学号...");
    m_editAccountSearch->setFixedHeight(35);
    m_editAccountSearch->setStyleSheet("background: #2b2b2b; color: white; border: 1px solid #555; border-radius: 4px; padding-left:5px;");
    connect(m_editAccountSearch, &QLineEdit::returnPressed, this, &MainWindow::refreshAccountTable);
    rightLayout->addWidget(new QLabel("检索用户:", rightPanel));
    rightLayout->addWidget(m_editAccountSearch);

    // 查询按钮
    QPushButton *btnSearch = new QPushButton("查询");
    btnSearch->setFixedHeight(35);
    btnSearch->setCursor(Qt::PointingHandCursor);
    btnSearch->setStyleSheet("background: #409eff; color: white; border-radius: 4px; font-weight: bold;");
    connect(btnSearch, &QPushButton::clicked, this, &MainWindow::refreshAccountTable);
    rightLayout->addWidget(btnSearch);

    rightLayout->addSpacing(20);

    // 【核心功能】一键生成
    QLabel *lblAuto = new QLabel("快捷操作:", rightPanel);
    lblAuto->setStyleSheet("font-weight: bold; margin-top: 10px;");
    rightLayout->addWidget(lblAuto);

    QPushButton *btnAutoCreate = new QPushButton("一键生成缺失账号");
    btnAutoCreate->setFixedHeight(45);
    btnAutoCreate->setCursor(Qt::PointingHandCursor);
    btnAutoCreate->setStyleSheet(R"(
        QPushButton { background-color: #67c23a; color: white; border-radius: 6px; font-weight: bold; font-size: 14px; }
        QPushButton:hover { background-color: #85ce61; }
    )");

    // 一键生成逻辑
    connect(btnAutoCreate, &QPushButton::clicked, this, [=](){
        DataManager dm;
        int count = dm.autoCreateMissingAccounts();
        if(count > 0) {
            QMessageBox::information(this, "成功", QString("已成功为 %1 名用户创建了初始账号。\n默认密码与学号/工号一致。").arg(count));
            refreshAccountTable();
        } else {
            QMessageBox::information(this, "提示", "所有用户都已有账号，无需操作。");
        }
    });
    rightLayout->addWidget(btnAutoCreate);

    // 说明
    QLabel *lblTip = new QLabel("说明：\n1. 点击上方绿色按钮，系统会自动扫描所有没密码的用户，并将其密码初始化为学号/工号。\n2. 在左侧列表中可单独重置某人密码。", rightPanel);
    lblTip->setWordWrap(true);
    lblTip->setStyleSheet("color: #888; font-size: 12px; font-style: italic; margin-top: 10px;");
    rightLayout->addWidget(lblTip);

    rightLayout->addStretch();
    mainLayout->addWidget(rightPanel, 3);

    refreshAccountTable();
}

// ==================================================
// 2. 刷新账号表格
// ==================================================
void MainWindow::refreshAccountTable()
{
    DataManager dm;
    auto list = dm.getAllAccounts(m_editAccountSearch->text().trimmed());

    m_tableAccount->setRowCount(0);

    int noAccountCount = 0;

    for(int i = 0; i < list.size(); ++i) {
        const auto &data = list[i];
        m_tableAccount->insertRow(i);

        // 0. 角色 (用图标或颜色区分更好，这里用文字)
        QTableWidgetItem *roleItem = new QTableWidgetItem(data.role);
        if(data.role == "教师") roleItem->setForeground(QColor("#409eff")); // 老师蓝色
        m_tableAccount->setItem(i, 0, roleItem);

        // 1. 姓名
        m_tableAccount->setItem(i, 1, new QTableWidgetItem(data.name));

        // 2. 账号
        m_tableAccount->setItem(i, 2, new QTableWidgetItem(data.number));

        // 3. 状态
        QTableWidgetItem *statusItem = new QTableWidgetItem();
        if(data.hasAccount) {
            statusItem->setText("✅ 已激活");
            statusItem->setForeground(QColor("#67c23a")); // 绿
        } else {
            statusItem->setText("❌ 未创建");
            statusItem->setForeground(QColor("#f56c6c")); // 红
            noAccountCount++;
        }
        statusItem->setTextAlignment(Qt::AlignCenter);
        m_tableAccount->setItem(i, 3, statusItem);

        // 4. 密码 (隐私保护)
        QTableWidgetItem *pwdItem = new QTableWidgetItem(data.hasAccount ? "********" : "(空)");
        pwdItem->setTextAlignment(Qt::AlignCenter);
        pwdItem->setForeground(QColor("#888"));
        m_tableAccount->setItem(i, 4, pwdItem);

        // 5. 操作栏 (重置按钮)
        QPushButton *btnReset = new QPushButton("重置密码");
        btnReset->setFixedSize(70, 30);
        btnReset->setCursor(Qt::PointingHandCursor);
        btnReset->setStyleSheet("color: #e6a23c; background: transparent; border: 1px solid #e6a23c; border-radius: 4px;");

        QWidget *w = new QWidget; QHBoxLayout *l = new QHBoxLayout(w); l->setContentsMargins(0,0,0,0); l->addWidget(btnReset); l->setAlignment(Qt::AlignCenter);
        m_tableAccount->setCellWidget(i, 5, w);

        // 重置逻辑
        connect(btnReset, &QPushButton::clicked, [=](){
            if(QMessageBox::Yes == QMessageBox::question(this, "高危操作",
                                                          QString("确定要重置 [%1] 的密码吗？\n重置后密码将变更为: %2").arg(data.name, data.number)))
            {
                DataManager tempDm;
                if(tempDm.resetUserPassword(data.id, data.role, data.number)) {
                    QMessageBox::information(this, "成功", "密码已重置成功。");
                    refreshAccountTable();
                } else {
                    QMessageBox::critical(this, "失败", "操作失败");
                }
            }
        });

        // 居中
        m_tableAccount->item(i, 0)->setTextAlignment(Qt::AlignCenter);
        m_tableAccount->item(i, 1)->setTextAlignment(Qt::AlignCenter);
        m_tableAccount->item(i, 2)->setTextAlignment(Qt::AlignCenter);
    }

    // 更新统计
    if(noAccountCount > 0) {
        m_lblAccountStats->setText(QString("发现 %1 个账号未激活").arg(noAccountCount));
        m_lblAccountStats->setStyleSheet("color: #f56c6c; font-weight: bold; margin-bottom: 10px;");
    } else {
        m_lblAccountStats->setText("所有账号状态正常");
        m_lblAccountStats->setStyleSheet("color: #67c23a; font-weight: bold; margin-bottom: 10px;");
    }
}


// 1. 构建数据库管理界面

void MainWindow::setupAdminDatabaseUi()
{
    QWidget *page = ui->admin_database_manage;
    if (!page) return;
    if (page->layout()) {
        refreshDatabaseInfo(); // 刷新连接状态
        return;
    }

    QVBoxLayout *mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(40, 40, 40, 40);
    mainLayout->setSpacing(30);

    // 标题
    QLabel *title = new QLabel("數據庫管理 (Database Management)", page);
    title->setStyleSheet("font-size: 24px; font-weight: bold; color: #fff; margin-bottom: 20px;");
    mainLayout->addWidget(title);

    // ------------------------------------------------
    // A. 状态看板 (Dashboard)
    // ------------------------------------------------
    QHBoxLayout *dashboardLayout = new QHBoxLayout();
    dashboardLayout->setSpacing(20);

    // A1. 左侧：大状态卡片
    QFrame *statusCard = new QFrame();
    statusCard->setStyleSheet("background-color: #333; border-radius: 10px; border: 1px solid #444;");
    statusCard->setFixedSize(250, 150);
    QVBoxLayout *cardLayout = new QVBoxLayout(statusCard);

    m_lblDbStatusIcon = new QLabel("✅");
    m_lblDbStatusIcon->setAlignment(Qt::AlignCenter);
    m_lblDbStatusIcon->setStyleSheet("font-size: 48px;");

    m_lblDbStatusText = new QLabel("Connected");
    m_lblDbStatusText->setAlignment(Qt::AlignCenter);
    m_lblDbStatusText->setStyleSheet("font-size: 18px; font-weight: bold; color: #67c23a;");

    cardLayout->addStretch();
    cardLayout->addWidget(m_lblDbStatusIcon);
    cardLayout->addWidget(m_lblDbStatusText);
    cardLayout->addStretch();

    // A2. 右侧：详细信息 Group
    QGroupBox *grpInfo = new QGroupBox("当前连接信息 (Read Only)");
    grpInfo->setStyleSheet("QGroupBox { color: #aaa; border: 1px solid #555; margin-top: 10px; font-weight: bold; } QGroupBox::title { subcontrol-origin: margin; left: 10px; }");
    QFormLayout *infoLayout = new QFormLayout(grpInfo);
    infoLayout->setContentsMargins(20, 20, 20, 20);
    infoLayout->setSpacing(15);

    // 信息标签样式
    QString valStyle = "color: white; font-size: 14px; font-weight: normal;";

    m_lblDbVersion = new QLabel("Loading..."); m_lblDbVersion->setStyleSheet(valStyle);
    m_lblDbDriver = new QLabel("Loading...");  m_lblDbDriver->setStyleSheet(valStyle);
    m_lblDbUser = new QLabel("Loading...");    m_lblDbUser->setStyleSheet(valStyle);

    infoLayout->addRow("数据库版本:", m_lblDbVersion);
    infoLayout->addRow("驱动类型:", m_lblDbDriver);
    infoLayout->addRow("当前登录用户:", m_lblDbUser);

    dashboardLayout->addWidget(statusCard);
    dashboardLayout->addWidget(grpInfo, 1); // 占满剩余空间

    mainLayout->addLayout(dashboardLayout);

    // ------------------------------------------------
    // B. 配置面板 (Settings)
    // ------------------------------------------------
    QGroupBox *grpConfig = new QGroupBox("修改连接配置 (Connection Settings)");
    grpConfig->setStyleSheet("QGroupBox { color: #409eff; border: 1px solid #555; margin-top: 20px; font-weight: bold; }");

    QGridLayout *configLayout = new QGridLayout(grpConfig);
    configLayout->setContentsMargins(30, 30, 30, 30);
    configLayout->setHorizontalSpacing(20);
    configLayout->setVerticalSpacing(15);

    // 输入框样式
    QString editStyle = "QLineEdit { background: #2b2b2b; color: white; border: 1px solid #555; border-radius: 4px; height: 35px; padding: 0 10px; } QLineEdit:focus { border: 1px solid #409eff; }";

    m_inputDbHost = new QLineEdit(); m_inputDbHost->setPlaceholderText("IP Address (e.g., 127.0.0.1)"); m_inputDbHost->setStyleSheet(editStyle);
    m_inputDbPort = new QLineEdit(); m_inputDbPort->setPlaceholderText("Port (e.g., 3306)"); m_inputDbPort->setStyleSheet(editStyle);
    m_inputDbName = new QLineEdit(); m_inputDbName->setPlaceholderText("Database Name"); m_inputDbName->setStyleSheet(editStyle);
    m_inputDbUser = new QLineEdit(); m_inputDbUser->setPlaceholderText("Username (root)"); m_inputDbUser->setStyleSheet(editStyle);
    m_inputDbPwd = new QLineEdit();  m_inputDbPwd->setPlaceholderText("Password"); m_inputDbPwd->setEchoMode(QLineEdit::Password); m_inputDbPwd->setStyleSheet(editStyle);

    // 布局表单
    configLayout->addWidget(new QLabel("主机 (Host):", grpConfig), 0, 0);
    configLayout->addWidget(m_inputDbHost, 0, 1);

    configLayout->addWidget(new QLabel("端口 (Port):", grpConfig), 0, 2);
    configLayout->addWidget(m_inputDbPort, 0, 3);

    configLayout->addWidget(new QLabel("数据库名 (DB):", grpConfig), 1, 0);
    configLayout->addWidget(m_inputDbName, 1, 1);

    configLayout->addWidget(new QLabel("账号 (User):", grpConfig), 2, 0);
    configLayout->addWidget(m_inputDbUser, 2, 1);

    configLayout->addWidget(new QLabel("密码 (Pwd):", grpConfig), 2, 2);
    configLayout->addWidget(m_inputDbPwd, 2, 3);

    // 提交按钮
    QPushButton *btnConnect = new QPushButton("测试并应用新连接");
    btnConnect->setCursor(Qt::PointingHandCursor);
    btnConnect->setFixedHeight(45);
    btnConnect->setStyleSheet(R"(
        QPushButton { background-color: #e6a23c; color: white; border-radius: 6px; font-weight: bold; font-size: 14px; margin-top: 10px; }
        QPushButton:hover { background-color: #ffb64d; }
    )");

    connect(btnConnect, &QPushButton::clicked, this, [=](){
        DataManager dm;
        bool ok = dm.reconnectDatabase(
            m_inputDbHost->text(),
            m_inputDbPort->text().toInt(),
            m_inputDbName->text(),
            m_inputDbUser->text(),
            m_inputDbPwd->text()
            );

        if (ok) {
            QMessageBox::information(this, "成功", "数据库连接已更新！");
            refreshDatabaseInfo(); // 刷新上方状态
        } else {
            QMessageBox::critical(this, "连接失败", "无法连接到数据库，请检查参数。\n注意：当前连接可能已断开。");
            refreshDatabaseInfo(); // 刷新状态为断开
        }
    });

    configLayout->addWidget(btnConnect, 3, 0, 1, 4); // 跨 4 列

    mainLayout->addWidget(grpConfig);

    // ------------------------------------------------
    // C. 底部提示 (Console Tip)
    // ------------------------------------------------
    mainLayout->addStretch();

    QLabel *lblConsoleTip = new QLabel("在此页面按下键盘上的 [ ~ ] 键可呼出 SQL 执行控制台。", page);
    lblConsoleTip->setStyleSheet("color: #888; font-style: italic; font-size: 13px;");
    lblConsoleTip->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(lblConsoleTip);

    // 初始化显示
    refreshDatabaseInfo();
}


// 刷新数据库信息

void MainWindow::refreshDatabaseInfo()
{
    DataManager dm;
    auto info = dm.getDatabaseInfo();

    // 1. 更新状态卡片
    if (info.isOpen) {
        m_lblDbStatusIcon->setText("✅");
        m_lblDbStatusText->setText("Connected");
        m_lblDbStatusText->setStyleSheet("font-size: 18px; font-weight: bold; color: #67c23a;"); // 绿
    } else {
        m_lblDbStatusIcon->setText("❌");
        m_lblDbStatusText->setText("Disconnected");
        m_lblDbStatusText->setStyleSheet("font-size: 18px; font-weight: bold; color: #f56c6c;"); // 红
    }

    // 处理端口 -1 的情况
    int displayPort = (info.port <= 0) ? 3306 : info.port;

    // 2. 更新详情信息 (Read Only 区域)
    m_lblDbVersion->setText(info.version);
    m_lblDbDriver->setText(info.driver);
    // 使用 displayPort 显示，避免出现 ":-1"
    m_lblDbUser->setText(QString("%1 @ %2:%3").arg(info.user, info.host).arg(displayPort));

    // 3. 回填输入框 (方便用户修改)
    m_inputDbHost->setText(info.host);

    // 输入框也填修正后的值，这样用户如果直接点“测试连接”，不会因为 -1 报错
    m_inputDbPort->setText(QString::number(displayPort));

    m_inputDbName->setText(info.databaseName);
    m_inputDbUser->setText(info.user);

    // 密码出于安全考虑不回填，留空让用户重新输入
    m_inputDbPwd->clear();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // 1. 拦截键盘按下事件
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        // 2. 检测按键：波浪号 (~) 或 反引号 (`)
        if (keyEvent->key() == Qt::Key_QuoteLeft || keyEvent->key() == Qt::Key_AsciiTilde)
        {
            // 3. 权限检查：必须是管理员
            if (m_currentRole == RoleAdmin)
            {

                if (ui->mainUi->currentIndex() == 4)
                {
                    int subIndex = ui->admin_stack->currentIndex();

                    if (subIndex == 11 || subIndex == 12)
                    {
                        ConsoleDialog console(this);
                        console.exec(); // 模态显示，阻塞主窗口

                        return true;
                    }
                }
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

// ==================================================
// 1. 构建日志管理界面 (UI)
// ==================================================
void MainWindow::setupAdminLogsUi()
{
    QWidget *page = ui->admin_checkLogs;
    if (!page) return;
    if (page->layout()) {
        refreshLogTable(); // 只刷新表格
        return;
    }

    QHBoxLayout *mainLayout = new QHBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // ------------------------------------
    // A. 左侧：日志表格 (75%)
    // ------------------------------------
    m_tableLogs = new QTableWidget();
    m_tableLogs->setColumnCount(5);
    // 表头调整为适配触发器数据
    m_tableLogs->setHorizontalHeaderLabels({"时间", "类型", "操作表", "受影响ID", "变更详情"});

    // 列宽优化
    m_tableLogs->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // 时间
    m_tableLogs->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents); // 类型
    m_tableLogs->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents); // 表名
    m_tableLogs->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // ID
    m_tableLogs->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);          // 详情自动拉伸

    m_tableLogs->verticalHeader()->setVisible(false);
    m_tableLogs->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableLogs->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableLogs->setShowGrid(false);
    m_tableLogs->setAlternatingRowColors(true);

    // 样式表
    m_tableLogs->setStyleSheet(R"(
        QTableWidget {
            background-color: #1e1e1e;
            color: #dcdcdc;
            border: 1px solid #333;
            alternate-background-color: #252526;
            selection-background-color: #37373d;
        }
        QHeaderView::section {
            background-color: #2d2d2d;
            color: #aaaaaa;
            padding: 5px;
            border: none;
            border-bottom: 1px solid #333;
        }
    )");

    mainLayout->addWidget(m_tableLogs, 8); // 权重 8


    // 右侧 控制面板

    QFrame *rightPanel = new QFrame();
    rightPanel->setStyleSheet("QFrame { background-color: #333; border-radius: 8px; } QLabel { color: #ddd; }");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(15);
    rightLayout->setContentsMargins(15, 20, 15, 20);

    // 标题
    QLabel *lblTitle = new QLabel("数据库变动监控", rightPanel);
    lblTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #fff; margin-bottom: 10px;");
    rightLayout->addWidget(lblTitle);

    // 统计
    m_lblLogCount = new QLabel("Total: 0", rightPanel);
    m_lblLogCount->setStyleSheet("color: #409eff; font-weight: bold;");
    rightLayout->addWidget(m_lblLogCount);

    QFrame *line = new QFrame; line->setFrameShape(QFrame::HLine); line->setStyleSheet("color:#555");
    rightLayout->addWidget(line);

    // 筛选控件
    rightLayout->addWidget(new QLabel("操作类型 (Type):", rightPanel));
    m_comboLogType = new QComboBox();
    m_comboLogType->addItems({"All", "INSERT", "UPDATE", "DELETE"});
    m_comboLogType->setStyleSheet("QComboBox { background: #2b2b2b; color: white; border: 1px solid #555; height: 30px; }");
    // 连接信号：下拉框改变 -> 刷新表格
    connect(m_comboLogType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::refreshLogTable);
    rightLayout->addWidget(m_comboLogType);

    rightLayout->addWidget(new QLabel("关键词搜索 (Keyword):", rightPanel));
    m_editLogSearch = new QLineEdit();
    m_editLogSearch->setPlaceholderText("搜表名/详情/ID...");
    m_editLogSearch->setStyleSheet("QLineEdit { background: #2b2b2b; color: white; border: 1px solid #555; height: 30px; padding-left:5px;}");
    // 连接信号：回车 -> 刷新表格
    connect(m_editLogSearch, &QLineEdit::returnPressed, this, &MainWindow::refreshLogTable);
    rightLayout->addWidget(m_editLogSearch);

    // 按钮组
    QPushButton *btnRefresh = new QPushButton("刷新 (Refresh)");
    btnRefresh->setCursor(Qt::PointingHandCursor);
    btnRefresh->setStyleSheet("background: #409eff; color: white; border-radius: 4px; height: 35px; font-weight:bold;");
    connect(btnRefresh, &QPushButton::clicked, this, &MainWindow::refreshLogTable);

    QPushButton *btnClear = new QPushButton("清空所有日志");
    btnClear->setCursor(Qt::PointingHandCursor);
    btnClear->setStyleSheet("background: #f56c6c; color: white; border-radius: 4px; height: 35px; font-weight:bold; margin-top: 20px;");
    connect(btnClear, &QPushButton::clicked, this, [=](){
        if(QMessageBox::Yes == QMessageBox::question(this, "警告", "确定要清空所有监控日志吗？\n此操作不可恢复！")) {
            DataManager dm;
            if(dm.clearSystemLogs()) {
                refreshLogTable();
                QMessageBox::information(this, "成功", "日志已清空");
            }
        }
    });

    rightLayout->addWidget(btnRefresh);
    rightLayout->addStretch();
    rightLayout->addWidget(btnClear);

    // 底部提示
    QLabel *lblTip = new QLabel("提示: 数据由数据库触发器自动记录", rightPanel);
    lblTip->setStyleSheet("color: #666; font-size: 11px; font-style: italic;");
    lblTip->setAlignment(Qt::AlignCenter);
    rightLayout->addWidget(lblTip);

    mainLayout->addWidget(rightPanel, 2); // 权重 2

    // 初始加载
    refreshLogTable();
}


// 刷新日志表格 (数据填充)

void MainWindow::refreshLogTable()
{
    DataManager dm;
    QString key = m_editLogSearch->text().trimmed();
    QString type = m_comboLogType->currentText();

    auto list = dm.getSystemLogs(key, type);

    m_tableLogs->setRowCount(0);
    m_lblLogCount->setText(QString("Total Records: %1").arg(list.size()));

    for(int i = 0; i < list.size(); ++i) {
        const auto &data = list[i];
        m_tableLogs->insertRow(i);

        // 1. 时间 (灰色)
        QTableWidgetItem *itemTime = new QTableWidgetItem(data.time);
        itemTime->setTextAlignment(Qt::AlignCenter);
        itemTime->setForeground(QColor("#888"));
        m_tableLogs->setItem(i, 0, itemTime);

        // 2. 类型 (彩色区分)
        QTableWidgetItem *itemType = new QTableWidgetItem(data.type);
        itemType->setTextAlignment(Qt::AlignCenter);
        itemType->setFont(QFont("Microsoft YaHei", 9, QFont::Bold));

        if(data.type == "DELETE") itemType->setForeground(QColor("#f56c6c")); // 删: 红
        else if(data.type == "UPDATE") itemType->setForeground(QColor("#e6a23c")); // 改: 橙
        else itemType->setForeground(QColor("#67c23a")); // 增: 绿

        m_tableLogs->setItem(i, 1, itemType);

        // 3. 表名 (蓝色)
        QTableWidgetItem *itemTable = new QTableWidgetItem(data.tableName);
        itemTable->setTextAlignment(Qt::AlignCenter);
        itemTable->setForeground(QColor("#409eff"));
        m_tableLogs->setItem(i, 2, itemTable);

        // 4. ID
        QTableWidgetItem *itemId = new QTableWidgetItem(data.recordId);
        itemId->setTextAlignment(Qt::AlignCenter);
        m_tableLogs->setItem(i, 3, itemId);

        // 5. 详情
        m_tableLogs->setItem(i, 4, new QTableWidgetItem(data.details));
    }
}

// 初始化动画
void MainWindow::initRoleAnimation()
{
    if (!ui->label_role) {
        qDebug() << "Error: ui->label_role is NULL! Check your UI file.";
        return;
    }
    // 挂载到后缀 Label 上
    m_colorEffect = new QGraphicsColorizeEffect(this);
    m_colorEffect->setStrength(1.0);
    ui->label_role->setGraphicsEffect(m_colorEffect);

    // 控制颜色变化
    m_colorAnim = new QPropertyAnimation(m_colorEffect, "color", this);
    m_colorAnim->setDuration(2500);
    m_colorAnim->setLoopCount(-1);
    m_colorAnim->setEasingCurve(QEasingCurve::InOutSine);
}

// 切换身份逻辑
void MainWindow::switchRoleAnimation(int role)
{
    m_colorAnim->stop();

    QString text;
    QColor colorLight; // 亮色 (呼吸的高点)
    QColor colorDark;  // 暗色 (呼吸的低点)

    switch (role) {
    case RoleStudent:
        text = "For Students";
        colorLight = QColor("#409eff"); // 亮蓝
        colorDark  = QColor("#003f7f"); // 深蓝
        break;
    case RoleTeacher:
        text = "For Teachers";
        colorLight = QColor("#67c23a"); // 亮绿
        colorDark  = QColor("#264d16"); // 深绿
        break;
    case RoleAdmin:
        text = "For Administrators";
        colorLight = QColor("#f56c6c"); // 亮红
        colorDark  = QColor("#591b1b"); // 深红
        break;
    default:
        text = "";
        break;
    }


    ui->label_role->setText(text);

    if (!text.isEmpty()) {

        m_colorAnim->setStartValue(colorLight);
        m_colorAnim->setKeyValueAt(0.5, colorDark);
        m_colorAnim->setEndValue(colorLight);

        m_colorAnim->start();
    }
}

// ==========================================
// 1. 构建行政班级管理界面 (复用 Side-by-Side 布局)
// ==========================================
void MainWindow::setupAdminClassUi()
{
    // 注意：请确保你的 stack widget 里有这个对应的 page widget，如果没有，请在 UI Designer 里添加或者 new 一个
    // 假设你在 mainwindow.ui 或者代码里定义了名为 admin_class_manage 的 page
    // 这里为了演示，我先手动 new 一个，你可以把它 add 到 admin_stack 里
    //QWidget *page = new QWidget();
    // ui->admin_stack->addWidget(page); // 如果需要手动加

    // 如果你已经有了名为 admin_class_manage 的 widget，直接用：
    QWidget *page = ui->admin_adminClass_manage;

    if (page->layout()) {
        loadAdminClassDeptCombo(); // 刷新下拉框
        refreshAdminClassTable();  // 刷新表格
        return;
    }

    QHBoxLayout *mainLayout = new QHBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // --- A. 左侧：表格 (70%) ---
    m_tableAdminClass = new QTableWidget();
    m_tableAdminClass->setColumnCount(5);
    m_tableAdminClass->setHorizontalHeaderLabels({"ID", "班级名称", "班级代码", "所属专业", "操作"});
    m_tableAdminClass->setColumnHidden(0, true);
    m_tableAdminClass->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableAdminClass->verticalHeader()->setVisible(false);
    m_tableAdminClass->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableAdminClass->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableAdminClass->verticalHeader()->setDefaultSectionSize(50);

    // 深色样式
    m_tableAdminClass->setStyleSheet(R"(
        QTableWidget { background-color: #2b2b2b; border: none; gridline-color: #444; color: #ddd; }
        QHeaderView::section { background-color: #1e1e1e; color: #409eff; height: 40px; border: none; border-bottom: 2px solid #409eff; }
        QTableWidget::item { border-bottom: 1px solid #333; }
        QTableWidget::item:selected { background-color: #409eff; color: white; }
    )");
    mainLayout->addWidget(m_tableAdminClass, 7);

    // --- B. 右侧：控制面板 (30%) ---
    QFrame *rightPanel = new QFrame();
    rightPanel->setStyleSheet("QFrame { background-color: #333333; border-radius: 10px; } QLabel { color: white; font-weight: bold; }");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(20, 20, 20, 20);
    rightLayout->setSpacing(15);

    // 搜索
    rightLayout->addWidget(new QLabel("🔍 搜索班级:", rightPanel));
    m_editAdminClassSearch = new QLineEdit();
    m_editAdminClassSearch->setPlaceholderText("班级名 / 代码...");
    m_editAdminClassSearch->setStyleSheet("background: #2b2b2b; color: white; border: 1px solid #555; height: 35px; border-radius: 4px; padding-left:5px;");
    connect(m_editAdminClassSearch, &QLineEdit::returnPressed, this, &MainWindow::refreshAdminClassTable);
    rightLayout->addWidget(m_editAdminClassSearch);

    QPushButton *btnSearch = new QPushButton("查询");
    btnSearch->setFixedHeight(35);
    btnSearch->setCursor(Qt::PointingHandCursor);
    btnSearch->setStyleSheet("background: #409eff; color: white; border-radius: 4px; font-weight: bold;");
    connect(btnSearch, &QPushButton::clicked, this, &MainWindow::refreshAdminClassTable);
    rightLayout->addWidget(btnSearch);

    QFrame *line = new QFrame; line->setFrameShape(QFrame::HLine); line->setStyleSheet("color: #555;");
    rightLayout->addWidget(line);

    // 快速添加
    rightLayout->addWidget(new QLabel("➕ 快速建班:", rightPanel));

    m_inputAdminClassName = new QLineEdit(); m_inputAdminClassName->setPlaceholderText("班级名称 (如: 24级计科1班)");
    m_inputAdminClassCode = new QLineEdit(); m_inputAdminClassCode->setPlaceholderText("班级代码 (如: CS2401)");
    m_comboAdminClassDept = new QComboBox(); // 所属专业下拉

    QString inputStyle = "background: #2b2b2b; color: white; border: 1px solid #555; height: 35px; border-radius: 4px; padding-left:5px;";
    m_inputAdminClassName->setStyleSheet(inputStyle);
    m_inputAdminClassCode->setStyleSheet(inputStyle);
    m_comboAdminClassDept->setStyleSheet(inputStyle);

    rightLayout->addWidget(new QLabel("名称:", rightPanel)); rightLayout->addWidget(m_inputAdminClassName);
    rightLayout->addWidget(new QLabel("代码:", rightPanel)); rightLayout->addWidget(m_inputAdminClassCode);
    rightLayout->addWidget(new QLabel("所属专业:", rightPanel)); rightLayout->addWidget(m_comboAdminClassDept);

    QPushButton *btnAdd = new QPushButton("确认添加");
    btnAdd->setFixedHeight(40);
    btnAdd->setCursor(Qt::PointingHandCursor);
    btnAdd->setStyleSheet("background: #67c23a; color: white; border-radius: 4px; font-weight: bold;");

    connect(btnAdd, &QPushButton::clicked, this, [=](){
        if(m_inputAdminClassName->text().isEmpty() || m_comboAdminClassDept->currentIndex() < 0) {
            QMessageBox::warning(this, "提示", "请填写完整信息"); return;
        }
        DataManager dm;
        if(dm.addAdminClass(m_inputAdminClassName->text(), m_inputAdminClassCode->text(), m_comboAdminClassDept->currentData().toInt())) {
            QMessageBox::information(this, "成功", "班级添加成功");
            refreshAdminClassTable();
            m_inputAdminClassName->clear(); m_inputAdminClassCode->clear();
        } else {
            QMessageBox::critical(this, "失败", "操作失败");
        }
    });
    rightLayout->addWidget(btnAdd);
    rightLayout->addStretch();

    mainLayout->addWidget(rightPanel, 3);

    // 赋值给成员变量以便切换页面时引用 (如果你用的是 ui 指针，请修改这里)
    // 这一步取决于你的 stack widget 是怎么管理的，这里假设你要把这个 page 放到 stack 里
    if(ui->admin_stack->count() > 0) { // 这里只是示例，实际上你应该把 page 加进去或者用现有的
        // ui->admin_stack->addWidget(page);
    }

    // 初始化
    loadAdminClassDeptCombo();
    refreshAdminClassTable();
}

void MainWindow::loadAdminClassDeptCombo() {
    if(!m_comboAdminClassDept) return;
    m_comboAdminClassDept->clear();
    DataManager dm;
    auto depts = dm.getAllDepartments();
    for(const auto &d : depts) m_comboAdminClassDept->addItem(d.name, d.id);
}

void MainWindow::refreshAdminClassTable() {
    if(!m_tableAdminClass) return;
    m_tableAdminClass->setRowCount(0);
    DataManager dm;
    auto list = dm.getAllAdminClassesDetailed(m_editAdminClassSearch->text().trimmed());

    for(int i=0; i<list.size(); ++i) {
        const auto &data = list[i];
        m_tableAdminClass->insertRow(i);
        m_tableAdminClass->setItem(i, 0, new QTableWidgetItem(QString::number(data.id)));
        m_tableAdminClass->setItem(i, 1, new QTableWidgetItem(data.name));
        m_tableAdminClass->setItem(i, 2, new QTableWidgetItem(data.code));
        m_tableAdminClass->setItem(i, 3, new QTableWidgetItem(data.deptName));

        // 操作按钮
        QWidget *w = new QWidget(); QHBoxLayout *l = new QHBoxLayout(w); l->setContentsMargins(5,5,5,5);
        QPushButton *btnEdit = new QPushButton("修改");
        btnEdit->setFixedSize(50, 30);
        btnEdit->setStyleSheet("color: #409eff; background: transparent; border: 1px solid #409eff; border-radius: 4px;");
        QPushButton *btnDel = new QPushButton("删除");
        btnDel->setFixedSize(50, 30);
        btnDel->setStyleSheet("color: #f56c6c; background: transparent; border: 1px solid #f56c6c; border-radius: 4px;");
        l->addStretch(); l->addWidget(btnEdit); l->addWidget(btnDel); l->addStretch();
        m_tableAdminClass->setCellWidget(i, 4, w);

        // 居中
        for(int k=1; k<=3; k++) m_tableAdminClass->item(i, k)->setTextAlignment(Qt::AlignCenter);

        connect(btnEdit, &QPushButton::clicked, [=](){ showAdminClassEditDialog(data.id, data.name, data.code, data.deptId); });
        connect(btnDel, &QPushButton::clicked, [=](){
            if(QMessageBox::Yes == QMessageBox::question(this, "确认", "确定删除该班级吗？")) {
                DataManager tempDm;
                if(tempDm.deleteAdminClass(data.id)) refreshAdminClassTable();
            }
        });
    }
}

void MainWindow::showAdminClassEditDialog(int id, QString name, QString code, int deptId) {
    QDialog dlg(this);
    dlg.setWindowTitle("编辑班级信息");
    dlg.setFixedSize(350, 250);
    QFormLayout *layout = new QFormLayout(&dlg);

    QLineEdit *eName = new QLineEdit(name);
    QLineEdit *eCode = new QLineEdit(code);
    QComboBox *eDept = new QComboBox();
    DataManager dm;
    auto depts = dm.getAllDepartments();
    for(auto d : depts) eDept->addItem(d.name, d.id);
    eDept->setCurrentIndex(eDept->findData(deptId));

    layout->addRow("班级名称:", eName);
    layout->addRow("班级代码:", eCode);
    layout->addRow("所属专业:", eDept);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addRow(btnBox);
    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if(dlg.exec() == QDialog::Accepted) {
        if(dm.updateAdminClass(id, eName->text(), eCode->text(), eDept->currentData().toInt())) {
            refreshAdminClassTable();
            QMessageBox::information(this, "成功", "修改成功");
        }
    }
}

// ==========================================
// 2. 构建学期管理界面
// ==========================================
void MainWindow::setupAdminSemesterUi()
{
    // 同样，这里假设你有一个 widget 容器
    QWidget *page = ui->admin_semester_manage;
    if (page->layout()) {
        refreshSemesterTable(); // 只刷新表格
        return;
    }
    QHBoxLayout *mainLayout = new QHBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // --- 左侧表格 ---
    m_tableSemester = new QTableWidget();
    m_tableSemester->setColumnCount(5);
    m_tableSemester->setHorizontalHeaderLabels({"ID", "学年", "学期类型", "完整显示名称", "操作"});
    m_tableSemester->setColumnHidden(0, true);
    m_tableSemester->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableSemester->verticalHeader()->setVisible(false);
    m_tableSemester->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableSemester->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableSemester->setStyleSheet(R"(
        QTableWidget { background-color: #2b2b2b; border: none; gridline-color: #444; color: #ddd; }
        QHeaderView::section { background-color: #1e1e1e; color: #409eff; height: 40px; border: none; border-bottom: 2px solid #409eff; }
        QTableWidget::item { border-bottom: 1px solid #333; }
        QTableWidget::item:selected { background-color: #409eff; color: white; }
    )");
    mainLayout->addWidget(m_tableSemester, 6);

    // --- 右侧控制面板 ---
    QFrame *rightPanel = new QFrame();
    rightPanel->setStyleSheet("QFrame { background-color: #333333; border-radius: 10px; } QLabel { color: white; font-weight: bold; }");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(20);
    rightLayout->setContentsMargins(20, 30, 20, 30);

    rightLayout->addWidget(new QLabel("📅 学期配置中心", rightPanel));
    QFrame *line = new QFrame; line->setFrameShape(QFrame::HLine); line->setStyleSheet("color:#555");
    rightLayout->addWidget(line);

    rightLayout->addWidget(new QLabel("➕ 新增学期:", rightPanel));

    // 年份选择
    rightLayout->addWidget(new QLabel("起始年份 (Academic Year):", rightPanel));
    m_spinSemesterYear = new QSpinBox();
    m_spinSemesterYear->setRange(2020, 2050);
    m_spinSemesterYear->setValue(QDate::currentDate().year());
    m_spinSemesterYear->setStyleSheet("QSpinBox { background: #2b2b2b; color: white; border: 1px solid #555; height: 35px; border-radius: 4px; }");
    rightLayout->addWidget(m_spinSemesterYear);

    // 类型选择
    rightLayout->addWidget(new QLabel("学期类型 (Type):", rightPanel));
    m_comboSemesterType = new QComboBox();
    m_comboSemesterType->addItem("秋季学期 (1)", "1");
    m_comboSemesterType->addItem("春季学期 (2)", "2");
    m_comboSemesterType->addItem("夏季小学期 (3)", "3");
    m_comboSemesterType->setStyleSheet("QComboBox { background: #2b2b2b; color: white; border: 1px solid #555; height: 35px; border-radius: 4px; }");
    rightLayout->addWidget(m_comboSemesterType);

    QPushButton *btnAdd = new QPushButton("创建学期");
    btnAdd->setFixedHeight(45);
    btnAdd->setCursor(Qt::PointingHandCursor);
    btnAdd->setStyleSheet("background: #67c23a; color: white; border-radius: 4px; font-weight: bold; font-size:14px;");

    connect(btnAdd, &QPushButton::clicked, this, [=](){
        DataManager dm;
        if(dm.addSemester(QString::number(m_spinSemesterYear->value()), m_comboSemesterType->currentData().toString())) {
            QMessageBox::information(this, "成功", "学期创建成功");
            refreshSemesterTable();
        } else {
            QMessageBox::critical(this, "失败", "创建失败");
        }
    });
    rightLayout->addWidget(btnAdd);

    rightLayout->addWidget(new QLabel("提示：排课前请先确保此处有对应学期。", rightPanel));
    rightLayout->addStretch();
    mainLayout->addWidget(rightPanel, 4);

    refreshSemesterTable();
}

void MainWindow::refreshSemesterTable() {
    if(!m_tableSemester) return;
    m_tableSemester->setRowCount(0);
    DataManager dm;
    auto list = dm.getAllSemestersForManagement();

    for(int i=0; i<list.size(); ++i) {
        const auto &data = list[i];
        m_tableSemester->insertRow(i);
        m_tableSemester->setItem(i, 0, new QTableWidgetItem(QString::number(data.id)));
        m_tableSemester->setItem(i, 1, new QTableWidgetItem(data.year));

        QString typeLabel = (data.type == "1" ? "秋季" : (data.type == "2" ? "春季" : "夏季"));
        m_tableSemester->setItem(i, 2, new QTableWidgetItem(typeLabel));
        m_tableSemester->setItem(i, 3, new QTableWidgetItem(data.displayText));

        // 操作
        QWidget *w = new QWidget(); QHBoxLayout *l = new QHBoxLayout(w); l->setContentsMargins(5,5,5,5);
        QPushButton *btnEdit = new QPushButton("修改");
        btnEdit->setFixedSize(50, 30);
        btnEdit->setStyleSheet("color: #e6a23c; background: transparent; border: 1px solid #e6a23c; border-radius: 4px;");

        QPushButton *btnDel = new QPushButton("删除");
        btnDel->setFixedSize(50, 30);
        btnDel->setStyleSheet("color: #f56c6c; background: transparent; border: 1px solid #f56c6c; border-radius: 4px;");

        l->addStretch(); l->addWidget(btnEdit); l->addWidget(btnDel); l->addStretch();
        m_tableSemester->setCellWidget(i, 4, w);

        for(int k=1; k<=3; k++) m_tableSemester->item(i, k)->setTextAlignment(Qt::AlignCenter);

        connect(btnEdit, &QPushButton::clicked, [=](){ showSemesterEditDialog(data.id, data.year, data.type); });
        connect(btnDel, &QPushButton::clicked, [=](){
            if(QMessageBox::Yes == QMessageBox::question(this, "警告", "删除学期将导致关联的排课数据失效！\n确定继续吗？")) {
                DataManager tempDm;
                if(tempDm.deleteSemester(data.id)) refreshSemesterTable();
            }
        });
    }
}

void MainWindow::showSemesterEditDialog(int id, QString year, QString type) {
    QDialog dlg(this);
    dlg.setWindowTitle("修改学期");
    QFormLayout *layout = new QFormLayout(&dlg);

    QSpinBox *sYear = new QSpinBox(); sYear->setRange(2000, 2099); sYear->setValue(year.toInt());
    QComboBox *sType = new QComboBox();
    sType->addItem("秋季 (1)", "1"); sType->addItem("春季 (2)", "2"); sType->addItem("夏季 (3)", "3");
    int idx = sType->findData(type);
    if(idx >= 0) sType->setCurrentIndex(idx);

    layout->addRow("年份:", sYear);
    layout->addRow("类型:", sType);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addRow(btnBox);
    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if(dlg.exec() == QDialog::Accepted) {
        DataManager dm;
        if(dm.updateSemester(id, QString::number(sYear->value()), sType->currentData().toString())) {
            refreshSemesterTable();
        }
    }
}

bool DataManager::setSubjectCredits(int subjectId, int termId, double credits)
{
    QSqlQuery query;
    // 使用 REPLACE INTO (MySQL特有) 或 先查后插，这里假设 credits_id 是自增的，
    // 我们需要先判断是否存在 (subject_id, term_id) 的记录

    // 逻辑：如果该学科该学期已有学分记录，则更新；如果没有，则插入。
    query.prepare("SELECT credits_id FROM course_credits WHERE subject_id = :sid AND term_id = :tid");
    query.bindValue(":sid", subjectId);
    query.bindValue(":tid", termId);

    if(query.exec() && query.next()) {
        // 已存在 -> 更新
        int id = query.value(0).toInt();
        QSqlQuery updateQ;
        updateQ.prepare("UPDATE course_credits SET credits = :c WHERE credits_id = :id");
        updateQ.bindValue(":c", credits);
        updateQ.bindValue(":id", id);
        return updateQ.exec();
    } else {
        // 不存在 -> 插入
        // 获取新 ID (模拟自增)
        int newId = 1;
        QSqlQuery idQ("SELECT MAX(credits_id) FROM course_credits");
        if(idQ.next()) newId = idQ.value(0).toInt() + 1;

        QSqlQuery insertQ;
        insertQ.prepare("INSERT INTO course_credits (credits_id, subject_id, term_id, credits) VALUES (:id, :sid, :tid, :c)");
        insertQ.bindValue(":id", newId);
        insertQ.bindValue(":sid", subjectId);
        insertQ.bindValue(":tid", termId);
        insertQ.bindValue(":c", credits);
        return insertQ.exec();
    }
}


void MainWindow::setupAdminCourseUi()
{
    QWidget *page = ui->admin_course_manage;

    // 如果頁面佈局已經存在，只需刷新數據
    if (page->layout()) {
        loadCourseCombos();
        refreshCourseTable();
        return;
    }

    QHBoxLayout *mainLayout = new QHBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // ------------------------------------
    // A. 左側：已排課程列表 (65%)
    // ------------------------------------
    QVBoxLayout *leftLayout = new QVBoxLayout();

    // 篩選欄
    QHBoxLayout *filterLayout = new QHBoxLayout();
    QLabel *lblTitle = new QLabel("排課列表 (Course Schedule)", page);
    lblTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #ddd;");
    QPushButton *btnRefresh = new QPushButton("刷新列表");
    btnRefresh->setStyleSheet("background: #409eff; color: white; border-radius: 4px; padding: 5px 10px;");
    connect(btnRefresh, &QPushButton::clicked, this, &MainWindow::refreshCourseTable);
    filterLayout->addWidget(lblTitle);
    filterLayout->addStretch();
    filterLayout->addWidget(btnRefresh);

    // 表格
    m_tableCourse = new QTableWidget();
    m_tableCourse->setColumnCount(7);
    m_tableCourse->setHorizontalHeaderLabels({"ID", "學期", "學科", "教師", "教室", "容量/課時", "操作"});
    m_tableCourse->setColumnHidden(0, true);
    m_tableCourse->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableCourse->verticalHeader()->setVisible(false);
    m_tableCourse->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableCourse->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableCourse->setStyleSheet("QTableWidget { background: #2b2b2b; border: none; color: #ddd; } QHeaderView::section { background: #333; color: white; height: 35px; border:none;}");

    leftLayout->addLayout(filterLayout);
    leftLayout->addWidget(m_tableCourse);
    mainLayout->addLayout(leftLayout, 65);

    // ------------------------------------
    // B. 右側：排課控制台 (35%)
    // ------------------------------------
    QFrame *rightPanel = new QFrame();
    rightPanel->setStyleSheet("QFrame { background-color: #333; border-radius: 8px; } QLabel { color: #ddd; }");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(15);

    QLabel *lblRightTitle = new QLabel("新增排課 (Schedule New Class)");
    lblRightTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #67c23a; margin-bottom: 10px;");
    rightLayout->addWidget(lblRightTitle);

    QString comboStyle = "QComboBox, QLineEdit, QSpinBox, QDateEdit, QTimeEdit, QDoubleSpinBox { background: #2b2b2b; color: white; border: 1px solid #555; border-radius: 4px; height: 30px; padding-left: 5px; }";

    // --- B1. 基本信息 Group ---
    QGroupBox *grpBasic = new QGroupBox("基本信息");
    grpBasic->setStyleSheet("QGroupBox { color: #409eff; font-weight: bold; border: 1px solid #555; margin-top: 10px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; }");
    QFormLayout *formBasic = new QFormLayout(grpBasic);
    formBasic->setLabelAlignment(Qt::AlignRight);

    m_comboCourseTerm = new QComboBox(); m_comboCourseTerm->setStyleSheet(comboStyle);
    m_comboCourseSubject = new QComboBox(); m_comboCourseSubject->setStyleSheet(comboStyle);
    m_comboCourseTeacher = new QComboBox(); m_comboCourseTeacher->setStyleSheet(comboStyle);

    // 學分輸入
    m_spinCourseCredit = new QDoubleSpinBox();
    m_spinCourseCredit->setRange(0.5, 10.0);
    m_spinCourseCredit->setSingleStep(0.5);
    m_spinCourseCredit->setValue(2.0);
    m_spinCourseCredit->setSuffix(" 學分");
    m_spinCourseCredit->setStyleSheet(comboStyle);

    m_spinCourseMax = new QSpinBox();
    m_spinCourseMax->setRange(1, 500); m_spinCourseMax->setValue(60); m_spinCourseMax->setStyleSheet(comboStyle);

    m_editCourseRoom = new QLineEdit();
    m_editCourseRoom->setPlaceholderText("例如: A101"); m_editCourseRoom->setStyleSheet(comboStyle);

    formBasic->addRow("學期:", m_comboCourseTerm);
    formBasic->addRow("學科:", m_comboCourseSubject);
    formBasic->addRow("教師:", m_comboCourseTeacher);
    formBasic->addRow("學分:", m_spinCourseCredit);
    formBasic->addRow("容量:", m_spinCourseMax);
    formBasic->addRow("教室:", m_editCourseRoom);
    rightLayout->addWidget(grpBasic);

    // --- B2. 時間規則 Group (核心修改：使用節次) ---
    QGroupBox *grpTime = new QGroupBox("排課時間規則");
    grpTime->setStyleSheet(grpBasic->styleSheet());
    QFormLayout *formTime = new QFormLayout(grpTime);
    formTime->setLabelAlignment(Qt::AlignRight);

    m_dateStart = new QDateEdit(QDate::currentDate());
    m_dateStart->setCalendarPopup(true); m_dateStart->setDisplayFormat("yyyy-MM-dd"); m_dateStart->setStyleSheet(comboStyle);

    m_dateEnd = new QDateEdit(QDate::currentDate().addMonths(4));
    m_dateEnd->setCalendarPopup(true); m_dateEnd->setDisplayFormat("yyyy-MM-dd"); m_dateEnd->setStyleSheet(comboStyle);

    // [修改] 節次選擇框
    m_spinPeriodStart = new QSpinBox();
    m_spinPeriodStart->setRange(1, 11); // 假設有11節課
    m_spinPeriodStart->setSuffix(" 節");
    m_spinPeriodStart->setValue(1);
    m_spinPeriodStart->setStyleSheet(comboStyle);

    m_spinPeriodEnd = new QSpinBox();
    m_spinPeriodEnd->setRange(1, 11);
    m_spinPeriodEnd->setSuffix(" 節");
    m_spinPeriodEnd->setValue(2);
    m_spinPeriodEnd->setStyleSheet(comboStyle);

    // [修改] 智能聯動：開始節次不能大於結束節次
    connect(m_spinPeriodStart, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int val){
        if(m_spinPeriodEnd->value() < val) {
            m_spinPeriodEnd->setValue(val);
        }
        m_spinPeriodEnd->setMinimum(val);
    });

    m_comboWeekDay = new QComboBox();
    m_comboWeekDay->addItems({"周一", "周二", "周三", "周四", "周五", "周六", "周日"});
    m_comboWeekDay->setStyleSheet(comboStyle);

    m_comboFrequency = new QComboBox();
    m_comboFrequency->addItem("每週上課", 0);
    m_comboFrequency->addItem("僅單週", 1);
    m_comboFrequency->addItem("僅雙週", 2);
    m_comboFrequency->setStyleSheet(comboStyle);

    formTime->addRow("開始日期:", m_dateStart);
    formTime->addRow("結束日期:", m_dateEnd);
    formTime->addRow("開始節次:", m_spinPeriodStart); // 替換了時間選擇
    formTime->addRow("結束節次:", m_spinPeriodEnd);   // 替換了時間選擇
    formTime->addRow("重複:", m_comboWeekDay);
    formTime->addRow("頻率:", m_comboFrequency);

    rightLayout->addWidget(grpTime);

    // --- 提交按鈕 ---
    QPushButton *btnSubmit = new QPushButton("生成排課數據");
    btnSubmit->setFixedHeight(45);
    btnSubmit->setCursor(Qt::PointingHandCursor);
    btnSubmit->setStyleSheet("background: #67c23a; color: white; font-size: 14px; font-weight: bold; border-radius: 6px;");

    connect(btnSubmit, &QPushButton::clicked, this, [=](){
        // 1. 基礎校驗
        if(m_editCourseRoom->text().isEmpty()) {
            QMessageBox::warning(this, "提示", "請填寫教室！"); return;
        }
        if(m_dateStart->date() > m_dateEnd->date()) {
            QMessageBox::warning(this, "提示", "開始日期不能晚於結束日期！"); return;
        }

        // 2. 調用 DataManager
        DataManager dm;
        bool ok = dm.addCourseWithSchedule(
            m_comboCourseTerm->currentData().toInt(),
            m_comboCourseSubject->currentData().toInt(),
            m_comboCourseTeacher->currentData().toInt(),
            m_spinCourseMax->value(),
            m_editCourseRoom->text(),
            m_spinCourseCredit->value(),
            m_dateStart->date(),
            m_dateEnd->date(),
            m_spinPeriodStart->value(), // 傳入開始節次 (int)
            m_spinPeriodEnd->value(),   // 傳入結束節次 (int)
            m_comboWeekDay->currentIndex() + 1,
            m_comboFrequency->currentData().toInt()
            );

        if(ok) {
            QMessageBox::information(this, "成功", "排課成功！\n已生成對應節次的課表數據。");
            refreshCourseTable();
        } else {
            QMessageBox::critical(this, "失敗", "排課失敗，請檢查數據庫連接。");
        }
    });

    rightLayout->addWidget(btnSubmit);
    rightLayout->addStretch();
    mainLayout->addWidget(rightPanel, 35);

    // 初始化下拉框數據
    loadCourseCombos();
    refreshCourseTable();
}

// [新增] 获取递归先修课的主函数
QList<QPair<int, QString>> DataManager::getAllTransitivePrerequisites(int subjectId)
{
    // 1. 查出数据库中 *所有* 的先修关系，构建内存中的邻接表 (Adjacency List)
    //    Key: subject_id (课程), Value: List of pre_subject_id (它的直接先修课)
    QMap<int, QList<int>> adjList;

    QSqlQuery query("SELECT subject_id, pre_subject_id FROM subject_prerequisite");
    while (query.next()) {
        int sub = query.value(0).toInt();
        int pre = query.value(1).toInt();
        adjList[sub].append(pre);
    }

    // 2. 准备 DFS
    QSet<int> visited;    // 用于防止死循环 (环检测)
    QSet<int> resultIds;  // 存储找到的所有先修课 ID

    // 3. 开始递归搜索
    dfsPrerequisites(subjectId, adjList, visited, resultIds);

    // 4. 将 ID 转换为名称 (最后查一次数据库，或者复用 subject 缓存)
    QList<QPair<int, QString>> finalResult;
    if (resultIds.isEmpty()) return finalResult;

    // 拼接 ID 用于 SQL IN 查询
    QStringList idStrList;
    for (int id : resultIds) idStrList << QString::number(id);

    // 查询这些 ID 对应的名字
    QString sql = QString("SELECT subject_id, subject_name FROM subject WHERE subject_id IN (%1)")
                          .arg(idStrList.join(","));
    QSqlQuery nameQuery(sql);
    while (nameQuery.next()) {
        finalResult.append({
            nameQuery.value(0).toInt(),
            nameQuery.value(1).toString()
        });
    }

    return finalResult;
}

// [新增] DFS 递归逻辑
void DataManager::dfsPrerequisites(int currentId,
                                   QMap<int, QList<int>>& adjList,
                                   QSet<int>& visited,
                                   QSet<int>& resultIds)
{
    // 标记当前节点已访问（防止 A->B->A 这种环导致死循环）
    visited.insert(currentId);

    // 获取当前课程的直接先修课
    if (adjList.contains(currentId)) {
        const QList<int>& directPrereqs = adjList[currentId];

        for (int preId : directPrereqs) {
            // 如果这个先修课还没被加入结果
            if (!resultIds.contains(preId)) {
                resultIds.insert(preId); // 加入结果集

                // 如果没访问过，继续深挖它的先修课
                if (!visited.contains(preId)) {
                    dfsPrerequisites(preId, adjList, visited, resultIds);
                }
            }
        }
    }

    // 回溯（对于单纯找所有祖先，其实不需要把 visited 移除，保留着可以作为 memoization 优化）
    // visited.remove(currentId); // 有环检测需求时通常不需要这就行
}

//終わる

