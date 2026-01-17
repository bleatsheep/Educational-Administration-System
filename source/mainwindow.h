#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QButtonGroup>

#include <QTabWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

#include <QDate>//给课程表用的

#include <QListWidget>

#include <QSpinBox>   // 解决 QSpinBox 未定义
#include <QDateEdit>  // 解决 QDateEdit 未定义
#include <QTimeEdit>  // 解决 QTimeEdit 未定义
#include <QLineEdit>  // 确保 QLineEdit 正常
#include <QGroupBox>  // 如果用到了 QGroupBox 指针
#include <QFormLayout>// 如果用到了 QFormLayout 指针

#include <QGraphicsColorizeEffect>
#include <QPropertyAnimation>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // 用户身份枚举
    enum UserRole {
        RoleStudent = 0,
        RoleTeacher = 1,
        RoleAdmin = 2
    };

    //admin页面枚举
    enum AdminPageIndex {
        PageAdminWelcome = 0,   // 欢迎页
        PageCollege = 1,        // 学院管理
        PageDepartment = 2,     // 专业(系)管理
        PageAdminClass = 3,     //行政班管理
        PageSubject = 4,        // 学科管理
        PageSemester = 5,       //学期管理
        PageCourse = 6,         // 课程/排课管理
        PageSelection = 7,      // 学生选课管理 (如有)
        PageStudent = 8,        // 学生管理
        PageTeacher = 9,        // 教师管理
        PageAccount = 10,       // 账号管理
        PageLogs = 11,          // 日志
        PageDatabase = 12       // 数据库管理
    };

private:
    Ui::MainWindow *ui;

    // 身份选择按钮组
    QButtonGroup *m_roleGroup;

    // 当前选中的身份 (0, 1, 2)
    int m_currentRole;

    // 当前登录的学生ID
    int m_studentId;

    // 初始化选课界面 (设置表头、绑定信号等，只运行一次)
    void initCourseSelection();

    // 刷新两个表格的数据 (查询数据库并显示到界面)
    void refreshCourseTables();

    void initStudentNavigation(); // 新增：专门处理学生页面的跳转

    QTabWidget   *m_tabWidget_Course;  // 选课的 Tab 容器
    QComboBox    *m_comboTerm;         // 学期下拉框
    QPushButton  *m_btnRefresh;        // 刷新按钮
    QPushButton  *m_btnReturn;         // 返回按钮
    QTableWidget *m_tableAvailable;    // 表格1：可选课程
    QTableWidget *m_tableMyCourses;    // 表格2：已选课程
    QLabel       *m_labelCredits;      // 学分显示

    void setupStudentSelectionUi();

    // 独立课表页面相关的变量
    QWidget      *m_pageSchedule;   // 课表页面的容器
    QTableWidget *m_tableSchedule;  // 课表网格
    QLabel       *m_lblDateRange;   // 日期显示
    QDate        m_currentMonday;   // 当前周一

    // 函数声明
    void setupStudentSchedulePageUi();     // 构建独立的课表界面
    void updateScheduleTable();     // 刷新课表数据
    void onPrevWeek();              // 上一周
    void onNextWeek();              // 下一周
    void showCourseDetail(int row, int col); // 显示详情

    QWidget      *m_pageGrade;       // 页面容器
    QComboBox    *m_comboGradeTerm;  // 学期选择 (和选课那个分开，互不影响)
    QTableWidget *m_tableGrade;      // 成绩表格
    QLabel       *m_lblGradeSummary; // 显示 "加权平均分和总学分"

    void setupStudentGradePageUi();    // 构建界面
    void updateGradeTable();    // 刷新数据

    void initMenuConnections();//初始化菜单按钮连接

    void loadSemesterData();//统一加载学期数据的函数

    // === 【新增】个人信息页面变量 ===
    QWidget *m_pageInfo;        // 页面容器
    QLabel  *m_valName;         // 显示姓名
    QLabel  *m_valNumber;       // 显示学号
    QLabel  *m_valClass;        // 显示班级
    QLabel  *m_valGrade;        // 显示年级

    // === 函数声明 ===
    void setupStudentInfoPageUi();     // 构建界面
    void loadStudentInfo();     // 刷新数据

    //==老师函数部分==

    // === 教师端：查看教授课程界面 ===
    QWidget      *m_pageTeachingCourse; // 页面容器
    QTableWidget *m_tableTeacherCourses;// 左侧：课程列表
    QTableWidget *m_tableCourseStudents;// 右侧：学生名单列表
    QLabel       *m_lblCourseStats;     // 右侧顶部：显示当前选中课程的统计信息
    // === 新增：教师端学期选择框 ===
    QComboBox *m_comboTeacherTerm;   // 用于教授课程页

    void updateTeacherCourseTable();     // 刷新左侧列表
    void loadCourseStudentList(int sectionId); // 刷新右侧列表

    // === 教师端：成绩录入页面 ===
    QWidget      *m_pageGrading;
    QTableWidget *m_tableGradingCourses;  // 左侧：待打分课程列表
    QTableWidget *m_tableGradingStudents; // 右侧：学生打分列表
    QLabel       *m_lblGradingStatus;     // 右侧顶部状态
    QComboBox *m_comboGradingTerm;   // 用于成绩录入页

    // === 函数 ===
    void updateGradingCourseTable();           // 刷新左侧
    void loadGradingStudentList(int sectionId);// 刷新右侧

    // === 教师端：个人信息页面 ===
    QWidget *m_pageInfoTeacher;
    QLabel  *m_valTeacherName;   // 显示姓名
    QLabel  *m_valTeacherNumber; // 显示工号 (teacher_number)
    QLabel  *m_valTeacherDept;   // 显示部门
    QLabel  *m_valTeacherTitle;  // 显示职称
    // === 函数 ===
    void loadTeacherInfo();


    void initTeacherNavigation();//老师页面跳转

    void setupTeacherCoursePageUi();//构建查看选课界面

    void setupTeacherGradingPageUi();//构建登记分数界面

    void setupTeacherInfoPageUi();//构建个人信息界面

    //admin构建函数
    void setupAdminPageUi();

    // === 各个子页面的初始化函数 ===
    void setupAdminWelcomeUi();     // Index 0: 欢迎页

    QLineEdit    *m_editCollegeSearch; // 搜索输入框
    QTableWidget *m_tableCollege;      // 数据表格

    void setupAdminCollegeUi();     // Index 1: 学院管理
    //void setupAdminDeptUi();        // Index 2: 专业(系)管理
    void setupAdminSubjectUi();     // Index 3: 学科管理
    void setupAdminCourseUi();      // Index 4: 课程/排课管理
    void setupAdminSelectionUi();   // Index 5: 选课管理
    void setupAdminStudentUi();     // Index 6: 学生管理
    void setupAdminTeacherUi();     // Index 7: 老师管理
    void setupAdminAccountUi();     // Index 8: 账号管理
    void setupAdminLogsUi();        // Index 9: 日志
    void setupAdminDatabaseUi();    // Index 10: 数据库管理

    //上面这一大坨函数的辅助函数ですね
    void refreshCollegeTable();        // 刷新表格数据
    void showCollegeEditDialog(bool isEdit, int id = -1, QString name = "", QString code = "");

    // === 专业管理页面控件 ===
    QTableWidget *m_tableDept;      // 左侧表格
    QLineEdit    *m_editDeptSearch; // 搜索框

    // 右侧“快速添加”区域的输入控件
    QLineEdit    *m_inputDeptName;
    QLineEdit    *m_inputDeptCode;
    QComboBox    *m_comboDeptCollege; // 【关键】下拉选择学院

    // === 函数 ===
    void setupAdminDeptUi();        // 构建界面
    void refreshDeptTable();        // 刷新表格
    void loadCollegeComboBox();     // 辅助：刷新下拉框里的学院列表

    // 修改专用弹窗
    void showDeptEditDialog(int id, QString name, QString code, int currentCollegeId);

    // === 学科管理页面控件 ===
    QTabWidget   *m_tabSubject;     // 页面主容器

    // Tab 1: 基本信息
    QTableWidget *m_tableSubject;   // 学科列表
    QLineEdit    *m_editSubjectSearch;
    QLineEdit    *m_inputSubjectName;
    QLineEdit    *m_inputSubjectCode;
    QComboBox    *m_comboSubjectDept; // 下拉选择系 (完整性约束)
    QComboBox    *m_comboSubjectStatus; // 状态 (Active/Inactive)

    // Tab 2: 先修课设置
    QComboBox    *m_comboPrereqTarget;    // 选择要设置哪门课
    QListWidget  *m_listPrereqAvailable;  // 左侧：可选
    QListWidget  *m_listPrereqCurrent;    // 右侧：已选
    QPushButton  *m_btnAddPrereq;         // >> 按钮
    QPushButton  *m_btnRemovePrereq;      // << 按钮

    // === 函数 ===
    void refreshSubjectTable();     // 刷新 Tab1 表格
    void refreshPrereqUI();         // 刷新 Tab2 穿梭框

    // 辅助加载下拉框
    void loadSubjectDeptCombo();    // 加载系别下拉框
    void loadPrereqTargetCombo();   // 加载Tab2的目标课程下拉框

    // 弹窗修改
    void showSubjectEditDialog(int id, QString name, QString code, int deptId, QString status);

    // === 排课管理控件 ===
    QTableWidget *m_tableCourse; // 左侧列表

    // 右侧表单控件
    QComboBox    *m_comboCourseTerm;
    QComboBox    *m_comboCourseSubject;
    QComboBox    *m_comboCourseTeacher;
    QSpinBox     *m_spinCourseMax;
    QLineEdit    *m_editCourseRoom;

    // 排课规则控件
    QDateEdit    *m_dateStart;  // <--- 必须是 QDateEdit *
    QDateEdit    *m_dateEnd;    // <--- 必须是 QDateEdit *
    QSpinBox *m_spinPeriodStart;
    QSpinBox *m_spinPeriodEnd;
    QComboBox    *m_comboWeekDay;
    QComboBox    *m_comboFrequency;

    // === 函数 ===
    void refreshCourseTable();
    void loadCourseCombos(); // 加载下拉框数据

    // === 选课管理控件 ===
    QTableWidget *m_tableSelection;
    QLineEdit    *m_editSelectionSearch;
    QLabel       *m_lblSelectionStats; // 统计信息 (共选课多少人次)

    // === 函数 ===
    void refreshSelectionTable();

    // === 学生管理专用 ===
    void refreshStudentTable(); // 刷新 tableWidget (复用你之前的逻辑，稍微封装一下)

    // 弹窗：isEdit=true 为编辑模式
    void showStudentEditDialog(bool isEdit, int id = -1, QString name = "", QString number = "", QString className = "");

    // === 教师管理控件 ===
    QTableWidget *m_tableTeacher;
    QLineEdit    *m_editTeacherSearch; // 搜索

    // 右侧添加面板控件
    QLineEdit    *m_inputTeacherName;
    QLineEdit    *m_inputTeacherNum;
    QComboBox    *m_comboTeacherTitle; // 职称下拉
    QComboBox    *m_comboTeacherDept;  // 系部下拉 (完整性约束)

    // === 函数 ===
    void refreshTeacherTable();
    void loadTeacherCombos(); // 加载下拉框数据

    // 编辑弹窗
    void showTeacherEditDialog(int id, QString name, QString num, QString title, int deptId);

    // === 账号管理控件 ===
    QTableWidget *m_tableAccount;
    QLineEdit    *m_editAccountSearch;
    QLabel       *m_lblAccountStats; // 显示有多少人没账号

    // === 函数 ===
    void refreshAccountTable();

    // === 数据库管理控件 ===
    // 状态显示区
    QLabel *m_lblDbStatusIcon;  // 显示 ✅ 或 ❌
    QLabel *m_lblDbStatusText;  // 显示 "Connected"
    QLabel *m_lblDbVersion;
    QLabel *m_lblDbDriver;
    QLabel *m_lblDbUser;        // 当前用户

    // 修改配置区
    QLineEdit *m_inputDbHost;
    QLineEdit *m_inputDbPort; // 用 LineEdit 方便
    QLineEdit *m_inputDbName;
    QLineEdit *m_inputDbUser;
    QLineEdit *m_inputDbPwd;

    // === 函数 ===
    void refreshDatabaseInfo(); // 刷新界面显示

    // === 日志管理控件 ===
    QTableWidget *m_tableLogs;
    QLineEdit    *m_editLogSearch;
    QComboBox    *m_comboLogType; // 筛选操作类型
    QLabel       *m_lblLogCount;

    // === 函数声明 ===
    void refreshLogTable();

    QGraphicsColorizeEffect *m_colorEffect;
    QPropertyAnimation *m_colorAnim;

    void initRoleAnimation();
    void switchRoleAnimation(int role);

    // === 行政班级管理 ===
    QWidget      *m_pageAdminClass; // 页面容器
    QTableWidget *m_tableAdminClass;// 左侧表格
    QLineEdit    *m_editAdminClassSearch;
    // 右侧添加/编辑用
    QLineEdit    *m_inputAdminClassName;
    QLineEdit    *m_inputAdminClassCode;
    QComboBox    *m_comboAdminClassDept;

    void setupAdminClassUi();
    void refreshAdminClassTable();
    void loadAdminClassDeptCombo(); // 加载系别
    void showAdminClassEditDialog(int id, QString name, QString code, int deptId);

    // === 学期管理 ===
    QWidget      *m_pageSemester;
    QTableWidget *m_tableSemester;
    // 右侧
    QSpinBox     *m_spinSemesterYear; // 年份用 SpinBox 更方便
    QComboBox    *m_comboSemesterType;

    void setupAdminSemesterUi();
    void refreshSemesterTable();
    void showSemesterEditDialog(int id, QString year, QString type);

    QDoubleSpinBox *m_spinCourseCredit; // 用于输入学分

private slots:
    void performRestart(); // 重启槽函数

protected:
    //void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

};

#endif // MAINWINDOW_H

