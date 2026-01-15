#include "datamanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

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

    // 这是一个 5 表联合查询，非常标准
    QString sql = R"(
        SELECT
            s.subject_name,     -- 0
            t.teacher_name,     -- 1
            sch.class_day,      -- 2
            sch.session_number, -- 3
            cs.comment          -- 4 (假设教室存在这里)
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
            item.session     = query.value(3).toInt();
            item.room        = query.value(4).toString();

            resultList.append(item);
        }
    } else {
        qDebug() << "获取课表失败:" << query.lastError().text();
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
