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
    void setupSchedulePageUi();     // 构建独立的课表界面
    void updateScheduleTable();     // 刷新课表数据
    void onPrevWeek();              // 上一周
    void onNextWeek();              // 下一周
    void showCourseDetail(int row, int col); // 显示详情

    QWidget      *m_pageGrade;       // 页面容器
    QComboBox    *m_comboGradeTerm;  // 学期选择 (和选课那个分开，互不影响)
    QTableWidget *m_tableGrade;      // 成绩表格
    QLabel       *m_lblGradeSummary; // 显示 "加权平均分和总学分"

    void setupGradePageUi();    // 构建界面
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
    void setupInfoPageUi();     // 构建界面
    void loadStudentInfo();     // 刷新数据

private slots:
    void performRestart(); // 重启槽函数

};

#endif // MAINWINDOW_H
