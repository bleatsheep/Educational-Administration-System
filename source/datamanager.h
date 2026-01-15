#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <QString>
#include <QList>
#include <QStringList>
#include <QObject>

#include <QDate>

enum EnrollResult {
    EnrollSuccess,      // 成功
    AlreadyEnrolled,    // 已选过
    TimeConflict,       // 时间冲突
    ClassFull,          // 名额已满
    DatabaseError       // 数据库未知错误
};

class DataManager : public QObject {
    Q_OBJECT
public:

    struct ScheduleItem {
        QString courseName;
        QString teacherName;
        QDate date;
        int session;
        QString room;
    };

    //查询成绩的结构体
    struct GradeItem {
        QString courseName;
        QString teacherName;
        double credit;  // 学分
        double score;   // 分数 (如果是 NULL 则为 -1)
    };

    //学期相关结构体
    struct SemesterItem {
        int id;              // term_id (存到 UserData 里)
        QString displayText; // 显示的文本 (如 "2023-2024 秋季")
    };

    //个人信息结构体
    struct StudentPersonalInfor {
        QString name;       // 姓名
        QString number;     // 学号 (stu_number)
        QString className;  // 行政班级 (从 administrative_class 表查)
        QString grade;      // 年级 (通过学号解析，或数据库查)
    };

    DataManager();

    bool login(QString account, QString password, int role);//登录函数

    QList<QStringList> getStudents(QString name = "");//返回根据名字查找学生的所有信息List

    bool deleteStudents(QList<int> student_id = {-1});//根据学生id(注意不是学号)删除学生

    QList<QPair<int, QString>> getAllClasses();//获取所有班级

    // 获取某个学期的所有可选课程（包含学分、已选人数等复杂信息）
    QList<QStringList> getAvailableCourses(int termId);

    // 获取某个学生已选的课程
    QList<QStringList> getMyCourses(int studentId, int termId);

    // 选课
    EnrollResult enrollCourse(int studentId, int sectionId);

    // 退课
    bool dropCourse(int studentId, int sectionId);

    QList<ScheduleItem> getWeeklySchedule(int studentId, QDate startDate, QDate endDate);

    QList<GradeItem> getStudentGrades(int studentId, int termId);

    QList<SemesterItem> getAllSemesters();//获取所有学期

    StudentPersonalInfor getStudentInfo(int studentId);//获取个人信息

signals:
    void errorOccurred(QString sql_delete_wmsg);
};



#endif // DATAMANAGER_H
