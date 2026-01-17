#include "datamanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

#include <QSettings>
#include <QCoreApplication>

const struct { QTime start; QTime end; } PERIOD_TIMES[] = {
    {QTime(0,0),   QTime(0,0)},   // 占位
    {QTime(8,0),   QTime(8,45)},  // 1
    {QTime(8,55),  QTime(9,40)},  // 2
    {QTime(10,10), QTime(10,55)}, // 3
    {QTime(11,5),  QTime(11,50)}, // 4
    {QTime(14,20), QTime(15,5)},  // 5
    {QTime(15,15), QTime(16,0)},  // 6
    {QTime(16,30), QTime(17,15)}, // 7
    {QTime(17,25), QTime(18,10)}, // 8
    {QTime(19,0),  QTime(19,45)}, // 9
    {QTime(19,55), QTime(20,40)}, // 10
    {QTime(20,50), QTime(21,35)}  // 11
};


DataManager::DataManager(){};//构造函数 无需定义内容

QList<QStringList> DataManager::getStudents(QString name)
{
    QList<QStringList> result; // 准备一个空袋子装结果
    QSqlQuery query;

    QString sql = "SELECT stu_id, stu_name, stu_number, admin_class_name FROM student s LEFT JOIN administrative_class a ON s.admin_class_id = a.admin_class_id";

    if (!name.isEmpty()) {
        sql += " WHERE stu_name LIKE :n";
        query.prepare(sql);
        query.bindValue(":n", "%" + name + "%");
    } else {
        query.prepare(sql);
    }

    if (query.exec()) {
        while (query.next()) {
            QStringList row;
            row << query.value("stu_id").toString();
            row << query.value("stu_name").toString();
            row << query.value("stu_number").toString();
            row << query.value("admin_class_name").toString();
            result.append(row);
        }
    } else {
        qDebug() << "查询失败:" << query.lastError().text();
    }

    return result; // 把装满数据的list交出去
}

bool DataManager::deleteStudents(const QList<int> student_id){
    QSqlQuery query;
    if (student_id.isEmpty()) {
        return false;
    }
    else{
        query.prepare("DELETE FROM student WHERE stu_id = :id ");//这个只需要执行一次即可
        for(auto index : student_id){
            query.bindValue(":id", index);
            query.exec();//如果要提高效率，最好使用事务
        }

        return true;
    }
}

bool DataManager::login(QString account, QString password, int role)
{
    QSqlQuery query;
    // 准备 SQL
    QString sql = "SELECT * FROM password_t WHERE account = :acc AND psw = :pwd AND role = :role";
    query.prepare(sql);

    // 绑定参数
    query.bindValue(":acc", account);
    query.bindValue(":pwd", password);
    query.bindValue(":role", role);

    // 执行
    if (!query.exec()) {
        qDebug() << "Login SQL error:" << query.lastError().text();
        return false; // 执行出错肯定算失败
    }
    // 在 query.exec() 之前或之后都可以
    qDebug() << "SQL模板:" << query.lastQuery();
    qDebug() << "绑定的账号:" << query.boundValue(":acc").toString(); // <--- 看这里！
    qDebug() << "绑定的密码:" << query.boundValue(":pwd").toString();
    qDebug() << "绑定的身份:" << query.boundValue(":role").toInt();
    // query.next() 为真，说明查到了人，验证通过
    return query.next();
}

int DataManager::getStudentIdByAccount(QString account)
{
    QSqlQuery query;
    // 假设账号就是学号，通过 student 表查询 stu_id
    query.prepare("SELECT stu_id FROM student WHERE stu_number = :acc");
    query.bindValue(":acc", account);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return -1; // 未找到
}

int DataManager::getTeacherIdByAccount(QString account)
{
    QSqlQuery query;
    // 查询 teacher 表，根据 teacher_number 找 teacher_id
    query.prepare("SELECT teacher_id FROM teacher WHERE teacher_number = :acc");
    query.bindValue(":acc", account);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return -1; // 未找到
}

// 1. 获取可选课程列表 (Tab 1 数据源)
QList<QStringList> DataManager::getAvailableCourses(int termId)
{
    QList<QStringList> result;
    QSqlQuery query;

    // 这是一个多表联合查询：
    // 1. 查课程班级 (course_section)
    // 2. 连课程名 (subject)
    // 3. 连老师名 (teacher)
    // 4. 连学分 (course_credits)
    // 5. 子查询计算当前多少人选了 (count)
    QString sql = R"(
        SELECT
            cs.section_id,
            s.subject_name,
            t.teacher_name,
            cc.credits,
            cs.max_student,
            (SELECT COUNT(*) FROM student_course_choice WHERE session_id = cs.section_id) as current_num
        FROM course_section cs
        LEFT JOIN subject s ON cs.subject_id = s.subject_id
        LEFT JOIN teacher t ON cs.teacher_id = t.teacher_id
        LEFT JOIN course_credits cc ON (s.subject_id = cc.subject_id AND cs.term_id = cc.term_id)
        WHERE cs.term_id = :termId
    )";

    query.prepare(sql);
    query.bindValue(":termId", termId);

    if (query.exec()) {
        while (query.next()) {
            QStringList row;
            row << query.value(0).toString(); // section_id
            row << query.value(1).toString(); // 课程名
            row << query.value(2).toString(); // 教师
            row << query.value(3).toString(); // 学分

            // 拼接 "已选/容量" 格式，例如 "45 / 60"
            int current = query.value(5).toInt();
            int max = query.value(4).toInt();
            row << QString("%1 / %2").arg(current).arg(max);

            result.append(row);
        }
    } else {
        qDebug() << "查询可选课程失败:" << query.lastError().text();
    }
    return result;
}

// 2. 获取我已选的课程 (Tab 2 数据源)
QList<QStringList> DataManager::getMyCourses(int studentId, int termId)
{
    QList<QStringList> result;
    QSqlQuery query;

    // 从 student_course_choice 表出发，反向查课程信息
    QString sql = R"(
        SELECT
            cs.section_id,
            s.subject_name,
            t.teacher_name,
            cc.credits
        FROM student_course_choice scc
        JOIN course_section cs ON scc.session_id = cs.section_id
        LEFT JOIN subject s ON cs.subject_id = s.subject_id
        LEFT JOIN teacher t ON cs.teacher_id = t.teacher_id
        LEFT JOIN course_credits cc ON (s.subject_id = cc.subject_id AND cs.term_id = cc.term_id)
        WHERE scc.stu_id = :sid AND cs.term_id = :tid
    )";

    query.prepare(sql);
    query.bindValue(":sid", studentId);
    query.bindValue(":tid", termId);

    if (query.exec()) {
        while (query.next()) {
            QStringList row;
            row << query.value(0).toString(); // ID
            row << query.value(1).toString(); // Name
            row << query.value(2).toString(); // Teacher
            row << query.value(3).toString(); // Credit
            result.append(row);
        }
    }
    return result;
}

// 3. 选课动作 (核心：查重 -> 查容量 -> 插入)
// DataManager.cpp

EnrollResult DataManager::enrollCourse(int studentId, int sectionId)
{
    QSqlQuery query;

    // 1. 【查重】检查是否已选
    query.prepare("SELECT * FROM student_course_choice WHERE stu_id = :sid AND session_id = :cid");
    query.bindValue(":sid", studentId);
    query.bindValue(":cid", sectionId);
    if (query.exec() && query.next()) {
        qDebug() << "选课失败：重复选课";
        return AlreadyEnrolled; // <--- 返回具体原因
    }

    // 2. 【查冲突】检查时间冲突
    // (注意：这里假设你的 section_schedule 数据是按“具体日期”存的，逻辑同上一条回答)
    QString conflictSql = R"(
        SELECT count(*)
        FROM student_course_choice scc
        JOIN section_schedule existing_sch ON scc.session_id = existing_sch.section_id
        JOIN section_schedule new_sch ON new_sch.section_id = :newId
        WHERE scc.stu_id = :sid
          AND existing_sch.class_day = new_sch.class_day
          AND existing_sch.start_time < new_sch.end_time
          AND existing_sch.end_time > new_sch.start_time
    )";
    query.prepare(conflictSql);
    query.bindValue(":sid", studentId);
    query.bindValue(":newId", sectionId);

    if (query.exec() && query.next()) {
        if (query.value(0).toInt() > 0) {
            qDebug() << "选课失败：时间冲突";
            return TimeConflict; // <--- 返回具体原因
        }
    }

    // 3. 【查容量】检查名额
    query.prepare("SELECT max_student, (SELECT COUNT(*) FROM student_course_choice WHERE session_id = :cid) FROM course_section WHERE section_id = :cid");
    query.bindValue(":cid", sectionId);
    if (query.exec() && query.next()) {
        int maxCap = query.value(0).toInt();
        int currentNum = query.value(1).toInt();
        if (currentNum >= maxCap) {
            qDebug() << "选课失败：名额已满";
            return ClassFull; // <--- 返回具体原因
        }
    }

    // 4. 【执行插入】
    query.prepare("INSERT INTO student_course_choice (stu_id, session_id) VALUES (:sid, :cid)");
    query.bindValue(":sid", studentId);
    query.bindValue(":cid", sectionId);

    if (query.exec()) {
        return EnrollSuccess; // <--- 成功
    } else {
        qDebug() << "SQL错误:" << query.lastError().text();
        return DatabaseError; // <--- 未知错误
    }
}

// 4. 退课动作
bool DataManager::dropCourse(int studentId, int sectionId)
{
    QSqlQuery query;
    query.prepare("DELETE FROM student_course_choice WHERE stu_id = :sid AND session_id = :cid");
    query.bindValue(":sid", studentId);
    query.bindValue(":cid", sectionId);
    return query.exec();
}

// 在 datamanager.cpp 末尾添加

QList<DataManager::ScheduleItem> DataManager::getWeeklySchedule(int studentId, QDate startDate, QDate endDate)
{
    QList<ScheduleItem> resultList;
    QSqlQuery query;

    // 修改查询字段，获取 period_start 和 period_end
    QString sql = R"(
        SELECT
            s.subject_name,     -- 0
            t.teacher_name,     -- 1
            sch.class_day,      -- 2
            sch.period_start,   -- 3 (新)
            sch.period_end,     -- 4 (新)
            sch.comment         -- 5 (教室)
        FROM student_course_choice scc
        JOIN course_section cs ON scc.session_id = cs.section_id
        JOIN section_schedule sch ON cs.section_id = sch.section_id
        JOIN subject s ON cs.subject_id = s.subject_id
        JOIN teacher t ON cs.teacher_id = t.teacher_id
        WHERE scc.stu_id = :sid
          AND sch.class_day BETWEEN :start AND :end
    )";

    query.prepare(sql);
    query.bindValue(":sid", studentId);
    query.bindValue(":start", startDate);
    query.bindValue(":end", endDate);

    if (query.exec()) {
        while (query.next()) {
            ScheduleItem item;
            item.courseName  = query.value(0).toString();
            item.teacherName = query.value(1).toString();
            item.date        = query.value(2).toDate();
            item.periodStart = query.value(3).toInt(); // 獲取開始節次
            item.periodEnd   = query.value(4).toInt(); // 獲取結束節次
            item.room        = query.value(5).toString();
            resultList.append(item);
        }
    }
    return resultList;
}

// datamanager.cpp

QList<DataManager::GradeItem> DataManager::getStudentGrades(int studentId, int termId)
{
    QList<GradeItem> list;
    QSqlQuery query;

    // 这是一个标准的四表联查
    // 注意：我们需要查 score (成绩) 和 credits (学分)
    QString sql = R"(
        SELECT
            s.subject_name,
            t.teacher_name,
            cc.credits,
            scc.score
        FROM student_course_choice scc
        JOIN course_section cs ON scc.session_id = cs.section_id
        JOIN subject s ON cs.subject_id = s.subject_id
        JOIN teacher t ON cs.teacher_id = t.teacher_id
        LEFT JOIN course_credits cc ON (s.subject_id = cc.subject_id AND cs.term_id = cc.term_id)
        WHERE scc.stu_id = :sid
          AND cs.term_id = :tid
          AND scc.score IS NOT NULL  -- 只查已经出成绩的课
    )";

    query.prepare(sql);
    query.bindValue(":sid", studentId);
    query.bindValue(":tid", termId);

    if (query.exec()) {
        while (query.next()) {
            GradeItem item;
            item.courseName  = query.value(0).toString();
            item.teacherName = query.value(1).toString();
            item.credit      = query.value(2).toDouble();

            // 处理分数：如果数据库里是 NULL，toDouble 会返回 0.0，
            // 但这里我们要区分 "0分" 和 "没录入"。
            // 不过上面的 SQL 已经加了 IS NOT NULL，所以读出来的都是有效分。
            item.score       = query.value(3).toDouble();

            list.append(item);
        }
    } else {
        qDebug() << "成绩查询失败:" << query.lastError().text();
    }
    return list;
}

QList<DataManager::SemesterItem> DataManager::getAllSemesters()
{
    QList<SemesterItem> list;
    QSqlQuery query;

    // 按年份降序、学期排序 (最新的学期排在最前面)
    query.prepare("SELECT term_id, academic_year, semester_type FROM semester ORDER BY academic_year DESC, semester_type DESC");

    if (query.exec()) {
        while (query.next()) {
            SemesterItem item;
            item.id = query.value(0).toInt();

            QString yearStr = query.value(1).toString(); // 例如 "2023"
            QString typeStr = query.value(2).toString(); // 例如 "1"

            // --- 1. 格式化年份 ---
            // 假设数据库存 "2023"，我们显示 "2023-2024学年"
            int startYear = yearStr.toInt();
            QString displayYear = QString("%1-%2学年").arg(startYear).arg(startYear + 1);

            // --- 2. 格式化季节 ---
            QString displayType;
            if (typeStr == "1") displayType = "秋季学期";
            else if (typeStr == "2") displayType = "春季学期";
            else if (typeStr == "3") displayType = "夏季小学期";
            else displayType = "其他学期";

            // --- 3. 组合最终文本 ---
            item.displayText = displayYear + " " + displayType;

            list.append(item);
        }
    } else {
        qDebug() << "学期查询失败:" << query.lastError().text();
    }

    return list;
}


DataManager::StudentPersonalInfor DataManager::getStudentInfo(int studentId)
{
    StudentPersonalInfor info;
    QSqlQuery query;

    // 联表查询：学生表 (student) JOIN 行政班级表 (administrative_class)
    QString sql = R"(
        SELECT
            s.stu_name,
            s.stu_number,
            ac.admin_class_name
        FROM student s
        LEFT JOIN administrative_class ac ON s.admin_class_id = ac.admin_class_id
        WHERE s.stu_id = :sid
    )";

    query.prepare(sql);
    query.bindValue(":sid", studentId);

    if (query.exec() && query.next()) {
        info.name = query.value(0).toString();
        info.number = query.value(1).toString();
        info.className = query.value(2).toString();

        // === 自动计算年级 ===
        // 逻辑：通常学号的前4位是年份，例如 "20240101" -> "2024级"
        // 如果你的学号规则不同，请修改这里
        if (info.number.length() >= 4) {
            info.grade = info.number.left(4) + "级";
        } else {
            info.grade = "未知年级";
        }
    } else {
        qDebug() << "个人信息查询失败:" << query.lastError().text();
    }

    return info;
}

// datamanager.cpp

QList<DataManager::TeacherCourseItem> DataManager::getTeacherCourses(int teacherId, int termId)
{
    QList<TeacherCourseItem> list;
    QSqlQuery query;

    // 【修改点 1】 使用 DISTINCT 去重
    // 【修改点 2】 不再查 sch.class_day (具体日期)，改为查 WEEKDAY(sch.class_day) (星期几)
    // 注意：MySQL 的 WEEKDAY() 函数返回 0=周一, 1=周二 ... 6=周日
    QString sql = R"(
        SELECT DISTINCT
            cs.section_id,      -- 0
            s.subject_name,     -- 1
            WEEKDAY(sch.class_day), -- 2 (新：获取星期索引)
            sch.period_start,   -- 3
            sch.period_end,     -- 4
            cs.comment,         -- 5 (教室)
            cs.max_student,     -- 6
            (SELECT COUNT(*) FROM student_course_choice WHERE session_id = cs.section_id) as curr_count, -- 7
            (SELECT AVG(score) FROM student_course_choice WHERE session_id = cs.section_id) as avg_score, -- 8
            (SELECT COUNT(*) FROM student_course_choice WHERE session_id = cs.section_id AND score IS NOT NULL) as graded_count -- 9
        FROM course_section cs
        JOIN subject s ON cs.subject_id = s.subject_id
        LEFT JOIN section_schedule sch ON cs.section_id = sch.section_id
        WHERE cs.teacher_id = :tid AND cs.term_id = :termId
        ORDER BY cs.section_id ASC, WEEKDAY(sch.class_day) ASC
    )";

    query.prepare(sql);
    query.bindValue(":tid", teacherId);
    query.bindValue(":termId", termId);

    if (query.exec()) {
        while (query.next()) {
            TeacherCourseItem item;
            item.sectionId = query.value(0).toInt();
            item.courseName = query.value(1).toString();

            // 【修改点 3】 解析时间逻辑变更
            // 数据库返回的是 0(周一) - 6(周日) 的整数，如果 sch 表为空（即未排课），这里可能是 NULL (0)
            // 需要判断 sch.section_id 是否真的存在排课，或者简单判断 period_start
            int weekDayIndex = query.value(2).toInt();
            int pStart = query.value(3).toInt();
            int pEnd = query.value(4).toInt();

            // 如果 pStart 为 0，说明 LEFT JOIN 没连上 schedule（未排课）
            if (pStart <= 0) {
                item.timeInfo = "时间待定";
            } else {
                // MySQL WEEKDAY: 0=Mon, ..., 6=Sun
                // 我们显示习惯用 1-7，或者中文 "周一"
                static const QStringList weekNames = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};
                QString dayStr = (weekDayIndex >= 0 && weekDayIndex < 7) ? weekNames[weekDayIndex] : "未知";

                QString periodStr;
                if (pStart == pEnd) {
                    periodStr = QString(" 第%1节").arg(pStart);
                } else {
                    periodStr = QString(" 第%1-%2节").arg(pStart).arg(pEnd);
                }
                item.timeInfo = dayStr + periodStr;
            }

            item.room = query.value(5).toString();
            item.maxCount = query.value(6).toInt();
            item.enrolledCount = query.value(7).toInt();

            // 处理平均分
            QVariant scoreVal = query.value(8);
            item.avgScore = scoreVal.isNull() ? -1.0 : scoreVal.toDouble();

            item.gradedCount = query.value(9).toInt();

            list.append(item);
        }
    } else {
        qDebug() << "Teacher Course Query Failed:" << query.lastError().text();
    }
    return list;
}

QList<DataManager::EnrolledStudentItem> DataManager::getCourseStudentList(int sectionId)
{
    QList<EnrolledStudentItem> list;
    QSqlQuery query;

    QString sql = R"(
        SELECT
            s.stu_id,
            s.stu_name,
            s.stu_number,
            ac.admin_class_name,
            scc.score
        FROM student_course_choice scc
        JOIN student s ON scc.stu_id = s.stu_id
        LEFT JOIN administrative_class ac ON s.admin_class_id = ac.admin_class_id
        WHERE scc.session_id = :sid
    )";

    query.prepare(sql);
    query.bindValue(":sid", sectionId);

    if (query.exec()) {
        while(query.next()){
            EnrolledStudentItem item;
            item.studentId = query.value(0).toInt();
            item.name = query.value(1).toString();
            item.number = query.value(2).toString();
            item.adminClass = query.value(3).toString();
            item.score = query.value(4).isNull() ? -1.0 : query.value(4).toDouble();
            list.append(item);
        }
    }
    return list;
}

bool DataManager::updateStudentGrade(int studentId, int sectionId, double score)
{
    QSqlQuery query;
    // 使用 UPDATE 语句
    QString sql = "UPDATE student_course_choice SET score = :score WHERE stu_id = :sid AND session_id = :cid";

    query.prepare(sql);
    query.bindValue(":score", score);
    query.bindValue(":sid", studentId);
    query.bindValue(":cid", sectionId);

    if(query.exec()) {
        return true;
    } else {
        qDebug() << "打分失败:" << query.lastError().text();
        return false;
    }
}

DataManager::TeacherPersonalInfor DataManager::getTeacherInfo(int teacherId)
{
    TeacherPersonalInfor info;
    QSqlQuery query;

    // 联表查询：teacher JOIN department
    QString sql = R"(
        SELECT
            t.teacher_name,
            t.teacher_number,
            t.title,
            d.dprt_name
        FROM teacher t
        LEFT JOIN department d ON t.dprt_id = d.dprt_id
        WHERE t.teacher_id = :tid
    )";

    query.prepare(sql);
    query.bindValue(":tid", teacherId);

    if (query.exec() && query.next()) {
        info.name = query.value(0).toString();
        info.number = query.value(1).toString();
        info.title = query.value(2).toString();
        info.department = query.value(3).toString();

        // 数据清洗：如果部门为空，显示暂无
        if(info.department.isEmpty()) info.department = "未分配部门";
    } else {
        qDebug() << "教师信息查询失败:" << query.lastError().text();
    }

    return info;
}

QList<DataManager::CollegeItem> DataManager::getAllColleges(QString keyword)
{
    QList<CollegeItem> list;
    QSqlQuery query;
    QString sql = "SELECT college_id, college_name, college_code FROM college";

    // 如果有搜索关键词
    if (!keyword.isEmpty()) {
        sql += " WHERE college_name LIKE :k OR college_code LIKE :k";
        query.prepare(sql);
        query.bindValue(":k", "%" + keyword + "%");
    } else {
        query.prepare(sql);
    }

    if (query.exec()) {
        while(query.next()) {
            CollegeItem item;
            item.id = query.value(0).toInt();
            item.name = query.value(1).toString();
            item.code = query.value(2).toString();
            list.append(item);
        }
    } else {
        qDebug() << "Query College Failed:" << query.lastError().text();
    }
    return list;
}

// === 新增学院 ===
bool DataManager::addCollege(QString name, QString code)
{
    QSqlQuery query;
    // 逻辑：ID = 当前最大ID + 1 (因为你的 SQL 脚本里 college_id 没有设自增)
    query.prepare("INSERT INTO college (college_id, college_name, college_code) "
                  "VALUES ((SELECT IFNULL(MAX(college_id),0)+1 FROM college c), :name, :code)");
    query.bindValue(":name", name);
    query.bindValue(":code", code);

    if(!query.exec()) {
        qDebug() << "Add College Failed:" << query.lastError().text();
        return false;
    }
    return true;
}

// === 修改学院 ===
bool DataManager::updateCollege(int id, QString name, QString code)
{
    QSqlQuery query;
    query.prepare("UPDATE college SET college_name = :name, college_code = :code WHERE college_id = :id");
    query.bindValue(":name", name);
    query.bindValue(":code", code);
    query.bindValue(":id", id);

    if(!query.exec()) {
        qDebug() << "Update College Failed:" << query.lastError().text();
        return false;
    }
    return true;
}

// === 删除学院 ===
bool DataManager::deleteCollege(int id)
{
    QSqlQuery query;
    // 注意：如果有外键约束（比如系关联了学院），这里可能失败，需要提示用户先删系
    query.prepare("DELETE FROM college WHERE college_id = :id");
    query.bindValue(":id", id);

    if(!query.exec()) {
        qDebug() << "Delete College Failed:" << query.lastError().text();
        return false;
    }
    return true;
}

// === 获取专业列表 (联表查询) ===
QList<DataManager::DeptItem> DataManager::getAllDepartments(QString keyword)
{
    QList<DeptItem> list;
    QSqlQuery query;
    // 核心 SQL：Department JOIN College
    QString sql = "SELECT d.dprt_id, d.dprt_name, d.dprt_code, c.college_name, d.college_id "
                  "FROM department d "
                  "LEFT JOIN college c ON d.college_id = c.college_id";

    if (!keyword.isEmpty()) {
        sql += " WHERE d.dprt_name LIKE :k OR d.dprt_code LIKE :k OR c.college_name LIKE :k";
        query.prepare(sql);
        query.bindValue(":k", "%" + keyword + "%");
    } else {
        query.prepare(sql);
    }

    if (query.exec()) {
        while(query.next()) {
            DeptItem item;
            item.id = query.value(0).toInt();
            item.name = query.value(1).toString();
            item.code = query.value(2).toString();
            item.collegeName = query.value(3).toString();
            // 如果学院被删了导致 NULL，显示未分配
            if(item.collegeName.isEmpty()) item.collegeName = "未分配";
            item.collegeId = query.value(4).toInt();
            list.append(item);
        }
    } else {
        qDebug() << "Query Dept Failed:" << query.lastError().text();
    }
    return list;
}

// === 新增专业 ===
bool DataManager::addDepartment(QString name, QString code, int collegeId)
{
    QSqlQuery query;
    // ID 自增逻辑
    query.prepare("INSERT INTO department (dprt_id, dprt_name, dprt_code, college_id) "
                  "VALUES ((SELECT IFNULL(MAX(dprt_id),0)+1 FROM department d), :name, :code, :cid)");
    query.bindValue(":name", name);
    query.bindValue(":code", code);
    query.bindValue(":cid", collegeId);

    if(!query.exec()) {
        qDebug() << "Add Dept Failed:" << query.lastError().text();
        return false;
    }
    return true;
}

// === 修改专业 ===
bool DataManager::updateDepartment(int id, QString name, QString code, int collegeId)
{
    QSqlQuery query;
    query.prepare("UPDATE department SET dprt_name=:name, dprt_code=:code, college_id=:cid WHERE dprt_id=:id");
    query.bindValue(":name", name);
    query.bindValue(":code", code);
    query.bindValue(":cid", collegeId);
    query.bindValue(":id", id);
    return query.exec();
}

// === 删除专业 ===
bool DataManager::deleteDepartment(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM department WHERE dprt_id = :id");
    query.bindValue(":id", id);
    return query.exec();
}

// === 获取学科列表 ===
QList<DataManager::SubjectItem> DataManager::getAllSubjects(QString keyword)
{
    QList<SubjectItem> list;
    QSqlQuery query;
    // 联表查询：Subject JOIN Department
    QString sql = "SELECT s.subject_id, s.subject_name, s.subject_code, s.status, d.dprt_name, s.dprt_id "
                  "FROM subject s "
                  "LEFT JOIN department d ON s.dprt_id = d.dprt_id"; // 使用 LEFT JOIN 防止部门被删导致学科消失

    if (!keyword.isEmpty()) {
        sql += " WHERE s.subject_name LIKE :k OR s.subject_code LIKE :k";
        query.prepare(sql);
        query.bindValue(":k", "%" + keyword + "%");
    } else {
        query.prepare(sql);
    }

    if (query.exec()) {
        while(query.next()) {
            SubjectItem item;
            item.id = query.value(0).toInt();
            item.name = query.value(1).toString();
            item.code = query.value(2).toString();
            item.status = query.value(3).toString();
            item.deptName = query.value(4).toString();
            if(item.deptName.isEmpty()) item.deptName = "公共/未知";
            item.deptId = query.value(5).toInt();
            list.append(item);
        }
    }
    return list;
}

// === 新增学科 ===
bool DataManager::addSubject(QString name, QString code, int deptId, QString status)
{
    QSqlQuery query;
    query.prepare("INSERT INTO subject (subject_id, subject_name, subject_code, dprt_id, status) "
                  "VALUES ((SELECT IFNULL(MAX(subject_id),0)+1 FROM subject s), :name, :code, :did, :stat)");
    query.bindValue(":name", name);
    query.bindValue(":code", code);
    query.bindValue(":did", deptId);
    query.bindValue(":stat", status);
    return query.exec();
}

// === 修改学科 ===
bool DataManager::updateSubject(int id, QString name, QString code, int deptId, QString status)
{
    QSqlQuery query;
    query.prepare("UPDATE subject SET subject_name=:name, subject_code=:code, dprt_id=:did, status=:stat WHERE subject_id=:id");
    query.bindValue(":name", name);
    query.bindValue(":code", code);
    query.bindValue(":did", deptId);
    query.bindValue(":stat", status);
    query.bindValue(":id", id);
    return query.exec();
}

// === 删除学科 ===
bool DataManager::deleteSubject(int id)
{
    QSqlQuery query;
    // 先删除该课程相关的先修课记录 (作为主课或作为先修课)
    query.prepare("DELETE FROM subject_prerequisite WHERE subject_id = :id OR pre_subject_id = :id");
    query.bindValue(":id", id);
    query.exec();

    // 再删课程本身
    query.prepare("DELETE FROM subject WHERE subject_id = :id");
    query.bindValue(":id", id);
    return query.exec();
}

// === 获取已设定的先修课 ===
QList<QPair<int, QString>> DataManager::getPrerequisites(int subjectId)
{
    QList<QPair<int, QString>> list;
    QSqlQuery query;
    // 查询 subject_prerequisite 联表 subject (查名字)
    QString sql = "SELECT p.pre_subject_id, s.subject_name "
                  "FROM subject_prerequisite p "
                  "JOIN subject s ON p.pre_subject_id = s.subject_id "
                  "WHERE p.subject_id = :sid";
    query.prepare(sql);
    query.bindValue(":sid", subjectId);

    if(query.exec()) {
        while(query.next()) {
            list.append({query.value(0).toInt(), query.value(1).toString()});
        }
    }
    return list;
}

// === 获取可选的先修课 (排除自己，排除已选) ===
QList<QPair<int, QString>> DataManager::getAvailablePrerequisites(int subjectId)
{
    QList<QPair<int, QString>> list;
    QSqlQuery query;

    // 逻辑：所有课程 - (当前课程 + 已经是先修课的课程)
    QString sql = "SELECT subject_id, subject_name FROM subject "
                  "WHERE subject_id != :sid "
                  "AND subject_id NOT IN (SELECT pre_subject_id FROM subject_prerequisite WHERE subject_id = :sid)";

    query.prepare(sql);
    query.bindValue(":sid", subjectId);

    if(query.exec()) {
        while(query.next()) {
            list.append({query.value(0).toInt(), query.value(1).toString()});
        }
    }
    return list;
}

// === 添加先修课 ===
bool DataManager::addPrerequisite(int subjectId, int preSubjectId)
{
    QSqlQuery query;
    query.prepare("INSERT INTO subject_prerequisite (subject_id, pre_subject_id) VALUES (:sid, :pid)");
    query.bindValue(":sid", subjectId);
    query.bindValue(":pid", preSubjectId);
    return query.exec();
}

// === 移除先修课 ===
bool DataManager::removePrerequisite(int subjectId, int preSubjectId)
{
    QSqlQuery query;
    query.prepare("DELETE FROM subject_prerequisite WHERE subject_id = :sid AND pre_subject_id = :pid");
    query.bindValue(":sid", subjectId);
    query.bindValue(":pid", preSubjectId);
    return query.exec();
}

// === 获取排课列表 ===
QList<DataManager::CourseSectionDisplay> DataManager::getAllCourseSections(int termId)
{
    QList<CourseSectionDisplay> list;
    QSqlQuery query;

    // 联表查询：CourseSection -> Subject, Teacher, Semester
    QString sql = R"(
        SELECT
            cs.section_id,
            sem.academic_year, sem.semester_type,
            s.subject_name,
            t.teacher_name,
            cs.comment,  -- 假设教室存在 comment 里
            cs.max_student,
            (SELECT COUNT(*) FROM section_schedule WHERE section_id = cs.section_id) as sched_count
        FROM course_section cs
        JOIN subject s ON cs.subject_id = s.subject_id
        JOIN teacher t ON cs.teacher_id = t.teacher_id
        JOIN semester sem ON cs.term_id = sem.term_id
    )";

    if (termId != -1) {
        sql += " WHERE cs.term_id = :tid";
        query.prepare(sql);
        query.bindValue(":tid", termId);
    } else {
        sql += " ORDER BY cs.term_id DESC"; // 默认按学期倒序
        query.prepare(sql);
    }

    if (query.exec()) {
        while(query.next()) {
            CourseSectionDisplay item;
            item.sectionId = query.value(0).toInt();

            // 拼接学期名
            QString year = query.value(1).toString();
            QString type = query.value(2).toString(); // 1=秋, 2=春...
            item.termName = QString("%1-%2 %3").arg(year).arg(year.toInt()+1)
                                .arg(type=="1"?"秋季": (type=="2"?"春季":"夏季"));

            item.subjectName = query.value(3).toString();
            item.teacherName = query.value(4).toString();
            item.room = query.value(5).toString();
            item.maxStudents = query.value(6).toInt();
            item.scheduleCount = query.value(7).toInt();
            list.append(item);
        }
    }
    return list;
}

// === 【核心】自动排课逻辑 ===
bool DataManager::addCourseWithSchedule(
    int termId, int subjectId, int teacherId, int maxCount, QString room,
    double credits,
    QDate startDate, QDate endDate,
    int startPeriod, int endPeriod, // <--- 參數已改為節次 (int)
    int dayOfWeek, int frequencyType)
{
    QSqlDatabase::database().transaction(); // 開啟事務
    QSqlQuery query;

    // ==========================================================
    // 第一步：設置學分 (Upsert 邏輯)
    // ==========================================================
    query.prepare("UPDATE course_credits SET credits = :c WHERE subject_id = :sid AND term_id = :tid");
    query.bindValue(":c", credits);
    query.bindValue(":sid", subjectId);
    query.bindValue(":tid", termId);
    if (!query.exec()) {
        qDebug() << "Update Credits Failed:" << query.lastError().text();
        QSqlDatabase::database().rollback(); return false;
    }
    if (query.numRowsAffected() == 0) {
        int newCreditId = 1;
        QSqlQuery idQ("SELECT MAX(credits_id) FROM course_credits");
        if(idQ.next()) newCreditId = idQ.value(0).toInt() + 1;

        QSqlQuery insertCredit;
        insertCredit.prepare("INSERT INTO course_credits (credits_id, subject_id, term_id, credits) VALUES (:id, :sid, :tid, :c)");
        insertCredit.bindValue(":id", newCreditId);
        insertCredit.bindValue(":sid", subjectId);
        insertCredit.bindValue(":tid", termId);
        insertCredit.bindValue(":c", credits);
        if (!insertCredit.exec()) {
            qDebug() << "Insert Credits Failed:" << insertCredit.lastError().text();
            QSqlDatabase::database().rollback(); return false;
        }
    }

    // ==========================================================
    // 第二步：插入 Course Section (教學班)
    // ==========================================================
    int newSectionId = 1;
    QSqlQuery secQ("SELECT MAX(section_id) FROM course_section");
    if(secQ.next()) newSectionId = secQ.value(0).toInt() + 1;

    query.prepare("INSERT INTO course_section (section_id, term_id, teacher_id, subject_id, max_student, comment) "
                  "VALUES (:id, :tid, :tea, :sub, :max, :room)");
    query.bindValue(":id", newSectionId);
    query.bindValue(":tid", termId);
    query.bindValue(":tea", teacherId);
    query.bindValue(":sub", subjectId);
    query.bindValue(":max", maxCount);
    query.bindValue(":room", room); // 教室存入 comment

    if(!query.exec()) {
        qDebug() << "Insert Section Failed:" << query.lastError().text();
        QSqlDatabase::database().rollback(); return false;
    }

    // ==========================================================
    // 第三步：生成 Schedule (按節次)
    // ==========================================================

    // 計算具體的開始和結束時間 (僅用於冗餘存儲，實際邏輯用 period)
    QPair<QTime, QTime> timeRange = getPeriodTimeRange(startPeriod, endPeriod);

    for (QDate date = startDate; date <= endDate; date = date.addDays(1)) {
        // 1. 檢查星期
        if (date.dayOfWeek() != dayOfWeek) continue;

        // 2. 檢查單雙週
        int weekNum = date.weekNumber();
        bool isOdd = (weekNum % 2 != 0);
        if (frequencyType == 1 && !isOdd) continue; // 單週模式 & 偶數週 -> 跳過
        if (frequencyType == 2 && isOdd) continue;  // 雙週模式 & 奇數週 -> 跳過

        // 3. 插入 Schedule
        // 注意：這裡插入了 period_start 和 period_end
        query.prepare("INSERT INTO section_schedule (section_id, class_day, period_start, period_end, start_time, end_time, comment) "
                      "VALUES (:sid, :day, :p_start, :p_end, :t_start, :t_end, :room)");
        query.bindValue(":sid", newSectionId);
        query.bindValue(":day", date);
        query.bindValue(":p_start", startPeriod);
        query.bindValue(":p_end", endPeriod);
        query.bindValue(":t_start", timeRange.first);
        query.bindValue(":t_end", timeRange.second);
        query.bindValue(":room", room);

        if(!query.exec()) {
            qDebug() << "Insert Schedule Failed:" << query.lastError().text();
            QSqlDatabase::database().rollback(); return false;
        }
    }

    QSqlDatabase::database().commit(); // 提交事務
    return true;
}

// === 删除排课 ===
bool DataManager::deleteCourseSection(int sectionId)
{
    QSqlDatabase::database().transaction();
    QSqlQuery query;

    // 1. 删 schedule
    query.prepare("DELETE FROM section_schedule WHERE section_id = :id");
    query.bindValue(":id", sectionId);
    query.exec();

    // 2. 删选课记录 (如果有人选了，也要级联删除，或者提示报错)
    query.prepare("DELETE FROM student_course_choice WHERE session_id = :id");
    query.bindValue(":id", sectionId);
    query.exec();

    // 3. 删 section
    query.prepare("DELETE FROM course_section WHERE section_id = :id");
    query.bindValue(":id", sectionId);
    bool ok = query.exec();

    if(ok) QSqlDatabase::database().commit();
    else QSqlDatabase::database().rollback();

    return ok;
}

// === 获取所有选课记录 ===
QList<DataManager::AdminChoiceItem> DataManager::getAllSelections(QString keyword)
{
    QList<AdminChoiceItem> list;
    QSqlQuery query;

    // 这是一个 5 表联合查询，信息量很大
    QString sql = R"(
        SELECT
            scc.stu_id,
            scc.session_id,
            s.stu_name,
            s.stu_number,
            sub.subject_name,
            t.teacher_name,
            scc.score
        FROM student_course_choice scc
        JOIN student s ON scc.stu_id = s.stu_id
        JOIN course_section cs ON scc.session_id = cs.section_id
        JOIN subject sub ON cs.subject_id = sub.subject_id
        JOIN teacher t ON cs.teacher_id = t.teacher_id
    )";

    if (!keyword.isEmpty()) {
        // 支持搜：学生姓名、学号、课程名
        sql += " WHERE s.stu_name LIKE :k OR s.stu_number LIKE :k OR sub.subject_name LIKE :k";
        query.prepare(sql);
        query.bindValue(":k", "%" + keyword + "%");
    } else {
        query.prepare(sql);
    }

    if (query.exec()) {
        while(query.next()) {
            AdminChoiceItem item;
            item.studentId = query.value(0).toInt();
            item.sectionId = query.value(1).toInt();
            item.studentName = query.value(2).toString();
            item.studentNum = query.value(3).toString();
            item.courseName = query.value(4).toString();
            item.teacherName = query.value(5).toString();

            // 处理成绩 NULL
            item.score = query.value(6).isNull() ? -1.0 : query.value(6).toDouble();

            list.append(item);
        }
    } else {
        qDebug() << "Query Selections Failed:" << query.lastError().text();
    }
    return list;
}

// === 强制退课 ===
bool DataManager::adminDropCourse(int studentId, int sectionId)
{
    QSqlQuery query;
    query.prepare("DELETE FROM student_course_choice WHERE stu_id = :sid AND session_id = :cid");
    query.bindValue(":sid", studentId);
    query.bindValue(":cid", sectionId);

    if(!query.exec()) {
        qDebug() << "Admin Drop Failed:" << query.lastError().text();
        return false;
    }
    return true;
}

// === 修正成绩 ===
bool DataManager::adminUpdateScore(int studentId, int sectionId, double newScore)
{
    QSqlQuery query;
    query.prepare("UPDATE student_course_choice SET score = :s WHERE stu_id = :sid AND session_id = :cid");
    query.bindValue(":s", newScore);
    query.bindValue(":sid", studentId);
    query.bindValue(":cid", sectionId);

    return query.exec();
}

// === 获取所有行政班级 (用于下拉框) ===
QList<QPair<int, QString>> DataManager::getAllAdminClasses()
{
    QList<QPair<int, QString>> list;
    QSqlQuery query("SELECT admin_class_id, admin_class_name FROM administrative_class");
    while(query.next()) {
        list.append({query.value(0).toInt(), query.value(1).toString()});
    }
    return list;
}

// === 新增学生 ===
bool DataManager::addStudent(QString name, QString number, int classId)
{
    QSqlQuery query;
    // 逻辑：ID自增
    query.prepare("INSERT INTO student (stu_id, stu_name, stu_number, admin_class_id) "
                  "VALUES ((SELECT IFNULL(MAX(stu_id),0)+1 FROM student s), :name, :num, :cid)");
    query.bindValue(":name", name);
    query.bindValue(":num", number);
    query.bindValue(":cid", classId);

    if(!query.exec()) {
        qDebug() << "Add Student Failed:" << query.lastError().text();
        return false;
    }
    return true;
}

// === 修改学生 ===
bool DataManager::updateStudent(int id, QString name, QString number, int classId)
{
    QSqlQuery query;
    query.prepare("UPDATE student SET stu_name=:name, stu_number=:num, admin_class_id=:cid WHERE stu_id=:id");
    query.bindValue(":name", name);
    query.bindValue(":num", number);
    query.bindValue(":cid", classId);
    query.bindValue(":id", id);

    return query.exec();
}

// === 获取教师列表 ===
QList<DataManager::TeacherItem> DataManager::getAllTeachers(QString keyword)
{
    QList<TeacherItem> list;
    QSqlQuery query;
    // 联表: Teacher -> Department
    QString sql = "SELECT t.teacher_id, t.teacher_name, t.teacher_number, t.title, d.dprt_name, t.dprt_id "
                  "FROM teacher t "
                  "LEFT JOIN department d ON t.dprt_id = d.dprt_id";

    if (!keyword.isEmpty()) {
        sql += " WHERE t.teacher_name LIKE :k OR t.teacher_number LIKE :k";
        query.prepare(sql);
        query.bindValue(":k", "%" + keyword + "%");
    } else {
        query.prepare(sql);
    }

    if (query.exec()) {
        while(query.next()) {
            TeacherItem item;
            item.id = query.value(0).toInt();
            item.name = query.value(1).toString();
            item.number = query.value(2).toString();
            item.title = query.value(3).toString();
            item.deptName = query.value(4).toString();
            if(item.deptName.isEmpty()) item.deptName = "未分配";
            item.deptId = query.value(5).toInt();
            list.append(item);
        }
    }
    return list;
}

// === 新增教师 ===
// === 新增教师 (修复 ODBC 函数序列错误) ===
bool DataManager::addTeacher(QString name, QString number, QString title, int deptId)
{
    // 开启事务：保证 插入老师信息 和 创建登录账号 要么都成功，要么都失败
    QSqlDatabase::database().transaction();
    QSqlQuery query;

    // ==========================================================
    // 第一步：获取新的 Teacher ID (解决 ODBC 函数序列错误)
    // ==========================================================
    int newId = 1;
    // 执行查询，获取当前最大ID
    if (query.exec("SELECT MAX(teacher_id) FROM teacher")) {
        if (query.next()) {
            // 注意：如果表是空的，value(0) 是 0，+1 变成 1，逻辑正确
            newId = query.value(0).toInt() + 1;
        }
    } else {
        qDebug() << "Get Max ID Failed:" << query.lastError().text();
        QSqlDatabase::database().rollback();
        return false;
    }

    // ==========================================================
    // 第二步：插入 Teacher 表
    // (注意：去掉了 password 列，因为你的表定义里没有该列)
    // ==========================================================
    query.prepare("INSERT INTO teacher (teacher_id, teacher_name, teacher_number, title, dprt_id) "
                  "VALUES (:id, :name, :num, :tit, :did)");
    query.bindValue(":id", newId);
    query.bindValue(":name", name);
    query.bindValue(":num", number);
    query.bindValue(":tit", title);
    query.bindValue(":did", deptId);

    if (!query.exec()) {
        qDebug() << "Add Teacher Failed:" << query.lastError().text();
        QSqlDatabase::database().rollback();
        return false;
    }

    // ==========================================================
    // 第三步：自动在 password_t 表中创建账号
    // (因为 teacher 表不存密码，所以必须存到 password_t 表，否则老师无法登录)
    // ==========================================================
    // 假设 role = 1 代表教师 (根据你之前的代码推断)
    query.prepare("INSERT INTO password_t (account, psw, role) VALUES (:acc, :pwd, 1)");
    query.bindValue(":acc", number); // 账号 = 工号
    query.bindValue(":pwd", number); // 默认密码 = 工号

    if (!query.exec()) {
        qDebug() << "Add Password Entry Failed:" << query.lastError().text();
        // 如果账号创建失败，为了数据一致性，刚刚插入的老师信息也要回滚
        QSqlDatabase::database().rollback();
        return false;
    }

    // 全部成功，提交事务
    QSqlDatabase::database().commit();
    return true;
}

// === 修改教师 ===
bool DataManager::updateTeacher(int id, QString name, QString number, QString title, int deptId)
{
    QSqlQuery query;
    query.prepare("UPDATE teacher SET teacher_name=:name, teacher_number=:num, title=:tit, dprt_id=:did WHERE teacher_id=:id");
    query.bindValue(":name", name);
    query.bindValue(":num", number);
    query.bindValue(":tit", title);
    query.bindValue(":did", deptId);
    query.bindValue(":id", id);
    return query.exec();
}

// === 删除教师 ===
bool DataManager::deleteTeacher(int id)
{
    QSqlQuery query;
    // 注意：如果老师已经排课，可能无法删除，需要先删排课
    query.prepare("DELETE FROM teacher WHERE teacher_id = :id");
    query.bindValue(":id", id);
    return query.exec();
}

// === 获取所有账号 ===
QList<DataManager::AccountItem> DataManager::getAllAccounts(QString keyword)
{
    QList<AccountItem> list;
    QSqlQuery query;

    // 逻辑：
    // 1. 查学生表，LEFT JOIN password_t (条件: account=stu_number 且 role=0)
    // 2. 查教师表，LEFT JOIN password_t (条件: account=teacher_number 且 role=1)
    // 3. UNION ALL 合并

    QString sql = R"(
        SELECT s.stu_id, s.stu_name, s.stu_number, '学生' as role_name,
               (p.id IS NOT NULL) as has_pwd
        FROM student s
        LEFT JOIN password_t p ON s.stu_number = p.account AND p.role = 0
        WHERE s.stu_name LIKE :k OR s.stu_number LIKE :k

        UNION ALL

        SELECT t.teacher_id, t.teacher_name, t.teacher_number, '教师' as role_name,
               (p.id IS NOT NULL) as has_pwd
        FROM teacher t
        LEFT JOIN password_t p ON t.teacher_number = p.account AND p.role = 1
        WHERE t.teacher_name LIKE :k OR t.teacher_number LIKE :k
    )";

    query.prepare(sql);
    query.bindValue(":k", "%" + keyword + "%");

    if (query.exec()) {
        while(query.next()) {
            AccountItem item;
            item.id = query.value(0).toInt();
            item.name = query.value(1).toString();
            item.number = query.value(2).toString();
            item.role = query.value(3).toString();
            item.hasAccount = query.value(4).toBool(); // 如果p.id不是NULL，说明有账号
            list.append(item);
        }
    } else {
        qDebug() << "Query Accounts Failed:" << query.lastError().text();
    }
    return list;
}

// === 一键生成缺失账号 ===
int DataManager::autoCreateMissingAccounts()
{
    int count = 0;
    QSqlQuery query;

    // 1. 批量插入缺失的学生账号 (role=0)
    // 逻辑：选择那些在 password_t 里找不到对应 account 的学生
    QString sqlStudent = R"(
        INSERT INTO password_t (account, psw, role)
        SELECT s.stu_number, s.stu_number, 0
        FROM student s
        LEFT JOIN password_t p ON s.stu_number = p.account AND p.role = 0
        WHERE p.id IS NULL
    )";

    if(query.exec(sqlStudent)) {
        count += query.numRowsAffected();
    }

    // 2. 批量插入缺失的教师账号 (role=1)
    QString sqlTeacher = R"(
        INSERT INTO password_t (account, psw, role)
        SELECT t.teacher_number, t.teacher_number, 1
        FROM teacher t
        LEFT JOIN password_t p ON t.teacher_number = p.account AND p.role = 1
        WHERE p.id IS NULL
    )";

    if(query.exec(sqlTeacher)) {
        count += query.numRowsAffected();
    }

    return count;
}

// === 重置单个密码 ===
bool DataManager::resetUserPassword(int id, QString roleStr, QString number)
{
    // 将中文角色转换为数字 (0:学生, 1:教师)
    int roleId = (roleStr == "学生") ? 0 : 1;

    QSqlQuery query;

    // 使用 REPLACE INTO：如果 account 存在则覆盖，不存在则插入
    // 这样既能用于“重置密码”，也能用于“单个创建账号”
    query.prepare("REPLACE INTO password_t (account, psw, role) VALUES (:acc, :pwd, :r)");

    query.bindValue(":acc", number);
    query.bindValue(":pwd", number); // 默认重置为账号本身
    query.bindValue(":r", roleId);

    if(!query.exec()) {
        qDebug() << "Reset Password Failed:" << query.lastError().text();
        return false;
    }
    return true;
}

// === 获取数据库信息 ===
DataManager::DbInfo DataManager::getDatabaseInfo()
{
    DbInfo info;
    QSqlDatabase db = QSqlDatabase::database(); // 获取默认连接

    info.isOpen = db.isOpen();
    info.driver = db.driverName();
    info.databaseName = db.databaseName();
    info.host = db.hostName();
    info.port = db.port();
    info.user = db.userName();
    info.lastError = db.lastError().text();

    if (info.isOpen) {
        // 查询具体版本号
        QSqlQuery query("SELECT VERSION()");
        if (query.next()) {
            info.version = query.value(0).toString();
        } else {
            info.version = "Unknown";
        }
    } else {
        info.version = "--";
    }

    return info;
}

// === 重新连接数据库 ===
bool DataManager::reconnectDatabase(QString host, int port, QString dbName, QString user, QString password)
{
    // 1. 获取当前数据库连接并断开
    QSqlDatabase db = QSqlDatabase::database();
    if (db.isOpen()) {
        db.close();
    }

    // 2. 设置新的连接参数
    db.setHostName(host);
    db.setPort(port);
    db.setDatabaseName(dbName);
    db.setUserName(user);
    db.setPassword(password);

    // 3. 尝试连接
    if (db.open()) {
        qDebug() << "Database reconnected successfully to" << host;

        // =======================================================
        // 【核心修改】连接成功后，同步更新 ini 配置文件
        // =======================================================

        // A. 确保路径和您 main.cpp 里读取的路径一模一样
        // QCoreApplication::applicationDirPath() 指向 exe 所在的文件夹
        QString iniPath = QCoreApplication::applicationDirPath() + "/db_config.ini";

        // B. 创建 QSettings 对象以操作 ini
        QSettings settings(iniPath, QSettings::IniFormat);

        // C. 写入新数据 (覆盖旧数据)
        // 注意：这里的 Key ("Database/Host" 等) 必须和您 main.cpp 里读取时的 Key 保持一致！
        settings.setValue("Database/Host", host);
        settings.setValue("Database/Port", port);
        settings.setValue("Database/Name", dbName);
        settings.setValue("Database/User", user);

        // 存储密码 (建议存之前做个简单的 Base64 编码，防止明文太显眼)
        // 如果您 main.cpp 里是直接读明文的，这里就直接存 password
        settings.setValue("Database/Password", password);

        // D. 强制写入硬盘 (虽然析构时会自动写，但手动 sync 更保险)
        settings.sync();

        qDebug() << "Configuration updated in:" << iniPath;
        return true;
    } else {
        qDebug() << "Reconnection failed:" << db.lastError().text();
        return false;
    }
}

// === 获取日志 (适配触发器表结构) ===
QList<DataManager::LogItem> DataManager::getSystemLogs(QString keyword, QString typeFilter)
{
    QList<LogItem> list;
    QSqlQuery query;

    // 假设您的触发器日志表结构为:
    // log_id, log_time, action_type, table_name, record_id, details

    QString sql = "SELECT log_id, "
                  "date_format(log_time, '%Y-%m-%d %H:%i:%s'), "
                  "action_type, table_name, record_id, details "
                  "FROM system_log WHERE 1=1";

    // 1. 筛选类型 (INSERT / UPDATE / DELETE)
    if(typeFilter != "All") {
        sql += QString(" AND action_type = '%1'").arg(typeFilter);
    }

    // 2. 关键词搜索
    if(!keyword.isEmpty()) {
        sql += " AND (table_name LIKE :k OR details LIKE :k OR record_id LIKE :k)";
    }

    sql += " ORDER BY log_time DESC LIMIT 1000"; // 限制显示最近1000条

    query.prepare(sql);
    if(!keyword.isEmpty()) {
        query.bindValue(":k", "%" + keyword + "%");
    }

    if(query.exec()) {
        while(query.next()) {
            LogItem item;
            item.id = query.value(0).toInt();
            item.time = query.value(1).toString();
            item.type = query.value(2).toString();      // INSERT/UPDATE/DELETE
            item.tableName = query.value(3).toString(); // student/teacher...
            item.recordId = query.value(4).toString();
            item.details = query.value(5).toString();
            list.append(item);
        }
    } else {
        qDebug() << "Log Query Failed:" << query.lastError().text();
    }
    return list;
}

// === 清空日志 ===
bool DataManager::clearSystemLogs()
{
    QSqlQuery query;
    return query.exec("TRUNCATE TABLE system_log");
}

// ==========================================
// === 行政班级管理实现 ===
// ==========================================
QList<DataManager::AdminClassItem> DataManager::getAllAdminClassesDetailed(QString keyword)
{
    QList<AdminClassItem> list;
    QSqlQuery query;
    // 联表查询: administrative_class -> department
    QString sql = "SELECT ac.admin_class_id, ac.admin_class_name, ac.admin_class_code, d.dprt_name, ac.dprt_id "
                  "FROM administrative_class ac "
                  "LEFT JOIN department d ON ac.dprt_id = d.dprt_id";

    if (!keyword.isEmpty()) {
        sql += " WHERE ac.admin_class_name LIKE :k OR ac.admin_class_code LIKE :k";
        query.prepare(sql);
        query.bindValue(":k", "%" + keyword + "%");
    } else {
        query.prepare(sql);
    }

    if (query.exec()) {
        while(query.next()) {
            AdminClassItem item;
            item.id = query.value(0).toInt();
            item.name = query.value(1).toString();
            item.code = query.value(2).toString();
            item.deptName = query.value(3).toString();
            if(item.deptName.isEmpty()) item.deptName = "未分配专业";
            item.deptId = query.value(4).toInt();
            list.append(item);
        }
    }
    return list;
}

// === 新增行政班级 (修复 ODBC 函数序列错误) ===
bool DataManager::addAdminClass(QString name, QString code, int deptId)
{
    // 开启事务，保证原子性
    QSqlDatabase::database().transaction();
    QSqlQuery query;

    // 第一步：先查询当前最大的 admin_class_id
    // 注意：如果是 SQL Server，建议用 ISNULL；如果是 MySQL，用 IFNULL。
    // 这里为了通用，我们在 C++ 层处理 NULL 的情况。
    int newId = 1; // 默认为 1
    query.exec("SELECT MAX(admin_class_id) FROM administrative_class");
    if (query.next()) {
        // 如果表里有数据，ID = 最大值 + 1
        // value(0) 如果是 NULL，toInt() 会返回 0，所以这里逻辑是安全的
        newId = query.value(0).toInt() + 1;
        if(newId == 1 && !query.value(0).isNull()) {
            // 这是一个边缘情况：如果 max 是 0，+1 是 1。
            // 但通常 max 不会是 0。只要 toInt() 没问题即可。
        }
    }

    // 第二步：执行插入
    query.prepare("INSERT INTO administrative_class (admin_class_id, admin_class_name, admin_class_code, dprt_id) "
                  "VALUES (:id, :name, :code, :did)");
    query.bindValue(":id", newId);
    query.bindValue(":name", name);
    query.bindValue(":code", code);
    query.bindValue(":did", deptId);

    if (query.exec()) {
        QSqlDatabase::database().commit(); // 提交事务
        return true;
    } else {
        qDebug() << "Add AdminClass Failed:" << query.lastError().text();
        QSqlDatabase::database().rollback(); // 回滚事务
        return false;
    }
}

bool DataManager::updateAdminClass(int id, QString name, QString code, int deptId)
{
    QSqlQuery query;
    query.prepare("UPDATE administrative_class SET admin_class_name=:name, admin_class_code=:code, dprt_id=:did WHERE admin_class_id=:id");
    query.bindValue(":name", name);
    query.bindValue(":code", code);
    query.bindValue(":did", deptId);
    query.bindValue(":id", id);
    return query.exec();
}

bool DataManager::deleteAdminClass(int id)
{
    QSqlQuery query;
    // 注意：如果有学生在这个班，删除会失败(或需级联)，这里暂且直接删
    query.prepare("DELETE FROM administrative_class WHERE admin_class_id = :id");
    query.bindValue(":id", id);
    return query.exec();
}

// ==========================================
// === 学期管理实现 ===
// ==========================================
QList<DataManager::SemesterManageItem> DataManager::getAllSemestersForManagement(QString keyword)
{
    QList<SemesterManageItem> list;
    QSqlQuery query;
    QString sql = "SELECT term_id, academic_year, semester_type FROM semester ORDER BY academic_year DESC, semester_type DESC";

    // 如果有keyword，稍微过滤一下年份
    if (!keyword.isEmpty()) {
        sql = "SELECT term_id, academic_year, semester_type FROM semester WHERE academic_year LIKE :k ORDER BY academic_year DESC";
        query.prepare(sql);
        query.bindValue(":k", "%" + keyword + "%");
    } else {
        query.prepare(sql);
    }

    if (query.exec()) {
        while(query.next()) {
            SemesterManageItem item;
            item.id = query.value(0).toInt();
            item.year = query.value(1).toString();
            item.type = query.value(2).toString();

            // 构造显示文本
            QString typeStr;
            if(item.type == "1") typeStr = "秋季 (1)";
            else if(item.type == "2") typeStr = "春季 (2)";
            else if(item.type == "3") typeStr = "夏季 (3)";
            else typeStr = "未知";

            item.displayText = QString("%1-%2学年 %3").arg(item.year).arg(item.year.toInt()+1).arg(typeStr);
            list.append(item);
        }
    }
    return list;
}

// === 新增学期 (修复 ODBC 函数序列错误) ===
bool DataManager::addSemester(QString year, QString type)
{
    QSqlDatabase::database().transaction();
    QSqlQuery query;

    // 第一步：获取新 ID
    int newId = 1;
    query.exec("SELECT MAX(term_id) FROM semester");
    if (query.next()) {
        newId = query.value(0).toInt() + 1;
    }

    // 第二步：插入
    query.prepare("INSERT INTO semester (term_id, academic_year, semester_type) "
                  "VALUES (:id, :year, :type)");
    query.bindValue(":id", newId);
    query.bindValue(":year", year);
    query.bindValue(":type", type);

    if (query.exec()) {
        QSqlDatabase::database().commit();
        return true;
    } else {
        qDebug() << "Add Semester Failed:" << query.lastError().text();
        QSqlDatabase::database().rollback();
        return false;
    }
}

bool DataManager::updateSemester(int id, QString year, QString type)
{
    QSqlQuery query;
    query.prepare("UPDATE semester SET academic_year=:year, semester_type=:type WHERE term_id=:id");
    query.bindValue(":year", year);
    query.bindValue(":type", type);
    query.bindValue(":id", id);
    return query.exec();
}

bool DataManager::deleteSemester(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM semester WHERE term_id = :id");
    query.bindValue(":id", id);
    return query.exec();
}


// 获取某节课的时间段显示字符串 (给课表行头用)
QString DataManager::getPeriodTimeLabel(int period) {
    if (period < 1 || period > 11) return "";
    return QString("%1~%2").arg(PERIOD_TIMES[period].start.toString("HH:mm"))
        .arg(PERIOD_TIMES[period].end.toString("HH:mm"));
}

// 获取排课的起止具体时间 (给数据库存冗余时间用)
QPair<QTime, QTime> DataManager::getPeriodTimeRange(int startPeriod, int endPeriod) {
    if (startPeriod < 1) startPeriod = 1;
    if (endPeriod > 11) endPeriod = 11;
    return qMakePair(PERIOD_TIMES[startPeriod].start, PERIOD_TIMES[endPeriod].end);
}

DataManager::PrereqResult DataManager::addPrerequisiteSafe(int subjectId, int preSubjectId)
{
    // 1. 自环检测：自己不能是自己的先修课
    if (subjectId == preSubjectId) {
        return SelfLoop;
    }

    // 2. 查重检测 (可选，如果数据库主键已处理，这里可以跳过，但为了逻辑清晰写一下)
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM subject_prerequisite WHERE subject_id = :sid AND pre_subject_id = :pid");
    checkQuery.bindValue(":sid", subjectId);
    checkQuery.bindValue(":pid", preSubjectId);
    if (checkQuery.exec() && checkQuery.next()) {
        if (checkQuery.value(0).toInt() > 0) return AlreadyExists;
    }

    // 3. 【核心】环检测 (Cycle Detection)
    // 逻辑：我们要添加 subjectId -> preSubjectId
    // 如果 preSubjectId 的祖先列表中已经包含了 subjectId，说明 preSubjectId 已经依赖于 subjectId
    // 此时再添加依赖就会形成环。

    // 复用之前的 DFS 函数获取 preSubjectId 的所有先修课
    QList<QPair<int, QString>> ancestors = getAllTransitivePrerequisites(preSubjectId);

    for (const auto& item : ancestors) {
        if (item.first == subjectId) {
            qDebug() << "Cycle detected! Subject" << subjectId << "is already a prerequisite for" << preSubjectId;
            return CycleDetected;
        }
    }

    // 4. 通过所有检查，执行插入
    QSqlQuery query;
    query.prepare("INSERT INTO subject_prerequisite (subject_id, pre_subject_id) VALUES (:sid, :pid)");
    query.bindValue(":sid", subjectId);
    query.bindValue(":pid", preSubjectId);

    if (query.exec()) {
        return Success;
    } else {
        qDebug() << "Add Prereq DB Error:" << query.lastError().text();
        return DbError;
    }
}

void dfsCheckPrereqs(int subjectId, const QSet<int>& passedSubjectIds, QList<QString>& missing, QSet<int>& visited)
{
    if (visited.contains(subjectId)) return;
    visited.insert(subjectId);

    // 查当前课程的直接先修课
    QSqlQuery query;
    query.prepare("SELECT pre_subject_id, s.subject_name FROM subject_prerequisite sp "
                  "JOIN subject s ON sp.pre_subject_id = s.subject_id "
                  "WHERE sp.subject_id = :sid");
    query.bindValue(":sid", subjectId);

    if (query.exec()) {
        while (query.next()) {
            int preId = query.value(0).toInt();
            QString preName = query.value(1).toString();

            // 如果这门先修课没过
            if (!passedSubjectIds.contains(preId)) {
                // 先递归检查它的先修课 (深度优先，保证拓扑序)
                dfsCheckPrereqs(preId, passedSubjectIds, missing, visited);

                // 如果列表里还没加这个课，就加上
                if (!missing.contains(preName)) {
                    missing.append(preName);
                }
            }
        }
    }
}

// 2. [核心] 检查选课资格
DataManager::EnrollmentCheckResult DataManager::checkEnrollmentEligibility(int studentId, int sectionId, int termId)
{
    EnrollmentCheckResult result;
    result.allowed = true;

    QSqlQuery query;

    // --- A. 检查是否为最新学期 ---
    // 逻辑：如果存在比当前 termId 更大的 termId，说明不是最新的
    // (假设 term_id 是自增的，或者你有专门的标志位。这里用 MAX(term_id) 判断)
    query.exec("SELECT MAX(term_id) FROM semester");
    if (query.next()) {
        int maxTerm = query.value(0).toInt();
        if (termId < maxTerm) {
            result.allowed = false;
            result.message = "只能选择当前最新学期的课程！";
            return result;
        }
    }

    // --- B. 获取当前要选的课程的 subject_id ---
    int targetSubjectId = -1;
    query.prepare("SELECT subject_id FROM course_section WHERE section_id = :id");
    query.bindValue(":id", sectionId);
    if (query.exec() && query.next()) {
        targetSubjectId = query.value(0).toInt();
    } else {
        result.allowed = false;
        result.message = "课程不存在";
        return result;
    }

    // --- C. 获取学生所有已通过的课程 (Score >= 60) ---
    QSet<int> passedSubjects;
    query.prepare("SELECT cs.subject_id FROM student_course_choice scc "
                  "JOIN course_section cs ON scc.session_id = cs.section_id "
                  "WHERE scc.stu_id = :sid AND scc.score >= 60");
    query.bindValue(":sid", studentId);
    if (query.exec()) {
        while(query.next()) passedSubjects.insert(query.value(0).toInt());
    }

    // --- D. 递归检查先修课 ---
    QList<QString> missingList;
    QSet<int> visited;

    // 我们需要检查 targetSubjectId 的先修课是否在 passedSubjects 里
    // 这里的逻辑稍微调整：直接调用 dfs 检查 targetSubjectId 的先修依赖

    // 获取 target 的直接先修课，对每一个进行递归检查
    query.prepare("SELECT pre_subject_id, s.subject_name FROM subject_prerequisite sp "
                  "JOIN subject s ON sp.pre_subject_id = s.subject_id "
                  "WHERE sp.subject_id = :sid");
    query.bindValue(":sid", targetSubjectId);

    if (query.exec()) {
        while(query.next()) {
            int preId = query.value(0).toInt();
            QString preName = query.value(1).toString();

            if (!passedSubjects.contains(preId)) {
                // 这个直接先修课没过，深入检查它的祖先
                dfsCheckPrereqs(preId, passedSubjects, missingList, visited);
                // 把它自己也加上
                if (!missingList.contains(preName)) missingList.append(preName);
            }
        }
    }

    if (!missingList.isEmpty()) {
        result.allowed = false;
        result.message = "先修课程未满足";
        result.missingPrereqs = missingList;
    }

    return result;
}

// 3. 获取课程详情字符串
QString DataManager::getCourseDescription(int sectionId)
{
    QString desc;
    QSqlQuery query;
    // 查基本信息
    query.prepare("SELECT s.subject_name, s.subject_code, t.teacher_name, cs.max_student, d.dprt_name "
                  "FROM course_section cs "
                  "JOIN subject s ON cs.subject_id = s.subject_id "
                  "JOIN teacher t ON cs.teacher_id = t.teacher_id "
                  "JOIN department d ON s.dprt_id = d.dprt_id "
                  "WHERE cs.section_id = :id");
    query.bindValue(":id", sectionId);
    int subjectId = -1;

    if(query.exec() && query.next()) {
        desc += QString("课程名称: %1\n").arg(query.value(0).toString());
        desc += QString("课程代码: %1\n").arg(query.value(1).toString());
        desc += QString("任课教师: %1\n").arg(query.value(2).toString());
        desc += QString("开设院系: %1\n").arg(query.value(4).toString());

        // 查 subject_id 用于查先修课
        QSqlQuery subQ("SELECT subject_id FROM course_section WHERE section_id = " + QString::number(sectionId));
        if(subQ.next()) subjectId = subQ.value(0).toInt();
    }

    // 查直接先修课
    if(subjectId != -1) {
        desc += "\n【直接先修课要求】:\n";
        query.prepare("SELECT s.subject_name FROM subject_prerequisite sp "
                      "JOIN subject s ON sp.pre_subject_id = s.subject_id "
                      "WHERE sp.subject_id = :sid");
        query.bindValue(":sid", subjectId);
        bool hasPre = false;
        if(query.exec()) {
            while(query.next()) {
                desc += "- " + query.value(0).toString() + "\n";
                hasPre = true;
            }
        }
        if(!hasPre) desc += "无";
    }

    return desc;
}
