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

    enum PrereqResult {
        Success,
        DbError,
        CycleDetected,   // 发现死循环
        SelfLoop,        // 自己依赖自己
        AlreadyExists    // 已经存在
    };

    struct ScheduleItem {
        QString courseName;
        QString teacherName;
        QDate date;
        int periodStart; // <--- 新增
        int periodEnd;   // <--- 新增
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

    // 老师课程概览结构体
    struct TeacherCourseItem {
        int sectionId;
        QString courseName;
        QString timeInfo;    // 例如 "周一 1-2节"
        QString room;
        int enrolledCount;   // 选课人数
        int maxCount;        // 容量
        double avgScore;     // 平均分 (如果还没出分则是 -1 或 0)
        int gradedCount; // <--- 【新增】已打分人数
    };

    // 选课学生结构体
    struct EnrolledStudentItem {
        int studentId;
        QString name;
        QString number;      // 学号
        QString adminClass;  // 行政班
        double score;        // 成绩 (未录入为 -1)
    };

    // 教师个人信息结构体
    struct TeacherPersonalInfor {
        QString name;
        QString number;     // teacher_number (工号)
        QString department; // dprt_name (从 department 表查)
        QString title;      // 职称
    };



    DataManager();

    bool login(QString account, QString password, int role);//登录函数

    QList<QStringList> getStudents(QString name = "");//返回根据名字查找学生的所有信息List

    bool deleteStudents(QList<int> student_id = {-1});//根据学生id(注意不是学号)删除学生

    QList<QPair<int, QString>> getAllClasses();//获取所有班级

    int getStudentIdByAccount(QString account);//通过账号获取 ID 的函数

    int getTeacherIdByAccount(QString account); //获取教师ID

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

    // 老师功能：获取我教的课
    QList<TeacherCourseItem> getTeacherCourses(int teacherId, int termId);

    // 老师功能：获取某门课的学生名单
    QList<EnrolledStudentItem> getCourseStudentList(int sectionId);

    // 【新增】更新某位学生的某门课成绩
    bool updateStudentGrade(int studentId, int sectionId, double score);

    // 获取教师个人信息
    TeacherPersonalInfor getTeacherInfo(int teacherId);

    // ==========================================
    // === 1. 学院管理 (College Management) ===
    // ==========================================

    struct CollegeItem {
        int id;
        QString name;
        QString code; // 学院编号
    };

    // 获取所有学院 (支持搜索)
    QList<CollegeItem> getAllColleges(QString keyword = "");

    // 新增学院
    bool addCollege(QString name, QString code);

    // 修改学院
    bool updateCollege(int id, QString name, QString code);

    // 删除学院
    bool deleteCollege(int id);

    struct DeptItem {
        int id;
        QString name;
        QString code;
        QString collegeName; // 用于显示 "计算机学院"
        int collegeId;       // 用于修改时的默认选中, 对应外键
    };

    // 联合查询：获取所有系及其所属学院
    QList<DeptItem> getAllDepartments(QString keyword = "");

    // 新增 (注意：必须传入 collegeId)
    bool addDepartment(QString name, QString code, int collegeId);

    // 修改
    bool updateDepartment(int id, QString name, QString code, int collegeId);

    // 删除
    bool deleteDepartment(int id);

    // ==========================================
    // === 3. 学科管理 (Subject) ===
    // ==========================================

    struct SubjectItem {
        int id;
        QString name;
        QString code;
        QString deptName;   // 所属系/部门名称
        int deptId;         // 所属系ID (外键)
        QString status;     // 状态 (Active/Inactive)
    };

    // 获取所有学科
    QList<SubjectItem> getAllSubjects(QString keyword = "");

    // 新增学科
    bool addSubject(QString name, QString code, int deptId, QString status);

    // 修改学科
    bool updateSubject(int id, QString name, QString code, int deptId, QString status);

    // 删除学科
    bool deleteSubject(int id);

    // ==========================================
    // === 3.1 先修课管理 (Prerequisites) ===
    // ==========================================

    // 获取某门课的先修课列表 (返回 ID 和 Name)
    QList<QPair<int, QString>> getPrerequisites(int subjectId);

    // 获取“可以作为先修课”的候选列表
    // (排除自己，排除已经是先修课的)
    QList<QPair<int, QString>> getAvailablePrerequisites(int subjectId);

    // 添加先修课关系
    bool addPrerequisite(int subjectId, int preSubjectId);

    // 移除先修课关系
    bool removePrerequisite(int subjectId, int preSubjectId);

    // ==========================================
    // === 4. 排课管理 (Course Scheduling) ===
    // ==========================================

    struct CourseSectionDisplay {
        int sectionId;
        QString termName;
        QString subjectName;
        QString teacherName;
        QString room;        // 教室 (存在 comment 字段或 schedule 中，这里假设存在 course_section.comment)
        int maxStudents;
        int scheduleCount;   // 已排课次数 (统计 section_schedule)
    };

    // 获取所有开课记录 (支持按学期筛选)
    QList<CourseSectionDisplay> getAllCourseSections(int termId = -1);

    // 【核心功能】新增排课
    // 参数说明：
    // - basic info: 学期, 学科, 老师, 容量, 教室
    // - schedule info: 开始日期, 结束日期, 开始时间, 结束时间, 星期几(1-7), 频率(0=每周, 1=单周, 2=双周)
    // bool addCourseWithSchedule(
    //     int termId, int subjectId, int teacherId, int maxCount, QString room,
    //     QDate startDate, QDate endDate, QTime startTime, QTime endTime,
    //     int dayOfWeek, int frequencyType
    //     );                                                                                //这个函数不需要重载，新的定义请见最后几行

    // 删除开课 (级联删除 schedule)
    bool deleteCourseSection(int sectionId);

    // ==========================================
    // === 5. 选课管理 (Selection / Score) ===
    // ==========================================

    struct AdminChoiceItem {
        int studentId;
        int sectionId;
        QString studentName;
        QString studentNum;
        QString courseName;
        QString teacherName;
        double score; // -1 表示未录入
    };

    // 获取所有选课记录 (支持搜学生名或课程名)
    QList<AdminChoiceItem> getAllSelections(QString keyword = "");

    // 管理员强制退课
    bool adminDropCourse(int studentId, int sectionId);

    // 管理员修正成绩
    bool adminUpdateScore(int studentId, int sectionId, double newScore);

    // ==========================================
    // === 6. 学生管理 (Admin Student Ops) ===
    // ==========================================

    // 获取所有行政班级 (用于下拉框选择，返回 ID 和 名称)
    QList<QPair<int, QString>> getAllAdminClasses();

    // 新增学生
    bool addStudent(QString name, QString number, int classId);

    // 修改学生
    bool updateStudent(int id, QString name, QString number, int classId);

    // (deleteStudents 你之前已经写过了，这里复用即可)

    struct TeacherItem {
        int id;
        QString name;
        QString number;     // 工号
        QString title;      // 职称
        QString deptName;   // 系名
        int deptId;         // 系ID (用于修改时回显)
    };

    // 获取所有教师 (联表查询)
    QList<TeacherItem> getAllTeachers(QString keyword = "");

    // 新增教师
    bool addTeacher(QString name, QString number, QString title, int deptId);

    // 修改教师
    bool updateTeacher(int id, QString name, QString number, QString title, int deptId);

    // 删除教师
    bool deleteTeacher(int id);

    // ==========================================
    // === 8. 账号管理 (Account Security) ===
    // ==========================================

    struct AccountItem {
        int id;             // 数据库主键 (stu_id 或 teacher_id)
        QString name;
        QString number;     // 学号或工号 (也是默认密码)
        QString role;       // "学生" 或 "教师"
        bool hasAccount;    // 是否已有密码 (password字段不为空)
    };

    // 获取所有用户账号状态 (联合查询)
    QList<AccountItem> getAllAccounts(QString keyword = "");

    // 一键自动创建所有缺失的账号
    int autoCreateMissingAccounts();

    // 重置单个用户密码
    bool resetUserPassword(int id, QString role, QString number);

    // ==========================================
    // === 9. 数据库管理 (Database Dashboard) ===
    // ==========================================

    struct DbInfo {
        bool isOpen;
        QString driver;      // e.g., QMYSQL
        QString databaseName;
        QString host;
        int port;
        QString user;
        QString version;     // 数据库版本
        QString lastError;   // 如果连接失败，显示错误
    };

    // 获取当前数据库详情
    DbInfo getDatabaseInfo();

    // 重新连接数据库 (修改账号密码)
    bool reconnectDatabase(QString host, int port, QString dbName, QString user, QString password);

    struct LogItem {
        int id;
        QString time;
        QString type;       // INSERT, UPDATE, DELETE
        QString tableName;  // 操作的表
        QString recordId;   // 受影响的ID
        QString details;    // 详情
    };

    // 获取日志
    QList<LogItem> getSystemLogs(QString keyword = "", QString typeFilter = "All");//用来监控做了什么操作

    // 清空日志
    bool clearSystemLogs();

    // ==========================================
    // === 10. 行政班级管理 (Admin Class) ===
    // ==========================================
    struct AdminClassItem {
        int id;
        QString name;
        QString code;
        QString deptName;   // 显示用
        int deptId;         // 修改/归属用
    };

    // 获取所有行政班 (联表 department)
    QList<AdminClassItem> getAllAdminClassesDetailed(QString keyword = "");
    bool addAdminClass(QString name, QString code, int deptId);
    bool updateAdminClass(int id, QString name, QString code, int deptId);
    bool deleteAdminClass(int id);

    // ==========================================
    // === 11. 学期管理 (Semester) ===
    // ==========================================
    struct SemesterManageItem {
        int id;
        QString year;       // e.g. "2023"
        QString type;       // "1", "2", "3" (DB char(1))
        QString displayText;// e.g. "2023-2024 秋季"
    };

    // 获取所有学期
    QList<SemesterManageItem> getAllSemestersForManagement(QString keyword = "");
    bool addSemester(QString year, QString type); // type: 1=秋, 2=春, 3=夏
    bool updateSemester(int id, QString year, QString type);
    bool deleteSemester(int id);

    //额外补充，用于设置学分
    bool setSubjectCredits(int subjectId, int termId, double credits);
    // 同时修改 addCourseWithSchedule 的参数，加入 credits
    bool addCourseWithSchedule(
        int termId, int subjectId, int teacherId, int maxCount, QString room,
        double credits,
        QDate startDate, QDate endDate,
        int startPeriod, int endPeriod, // <--- 改用节次
        int dayOfWeek, int frequencyType
        );

    static QPair<QTime, QTime> getPeriodTimeRange(int startPeriod, int endPeriod);
    static QString getPeriodTimeLabel(int period); // 用于生成 "08:00~08:45" 这种字符串

    QList<QPair<int, QString>> getAllTransitivePrerequisites(int subjectId);//DFS查詢所有先修課

    PrereqResult addPrerequisiteSafe(int subjectId, int preSubjectId);

    struct EnrollmentCheckResult {
        bool allowed;
        QString message;
        QList<QString> missingPrereqs; // 缺失课程的名字列表，按学习顺序排列
    };

    EnrollmentCheckResult checkEnrollmentEligibility(int studentId, int sectionId, int termId);

    // [新增] 获取某学期的详细信息（用于判断是否为最新学期）
    QPair<QString, bool> getTermInfo(int termId); // 返回 {学期名, 是否最新}

    // [新增] 获取课程的详细描述（用于弹窗）
    QString getCourseDescription(int sectionId);

signals:
    void errorOccurred(QString sql_delete_wmsg);

private:
    void dfsPrerequisites(int currentId,  //輔助函數
                          QMap<int, QList<int>>& adjList,
                          QSet<int>& visited,
                          QSet<int>& resultIds);
};



#endif // DATAMANAGER_H
