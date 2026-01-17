CREATE DATABASE IF NOT EXISTS SCHOOL CHARACTER SET utf8mb4;
USE SCHOOL;

drop table if exists student_course_choice;

drop table if exists subject_prerequisite;

drop table if exists section_schedule;

drop table if exists course_credits;

drop table if exists course_section;

drop table if exists semester;

drop table if exists student;

drop table if exists subject;

drop table if exists teacher;

drop table if exists administrative_class;

drop table if exists department;

drop table if exists college;

/*==============================================================*/
/* Table: administrative_class                                  */
/*==============================================================*/
create table administrative_class
(
   admin_class_id       int not null  comment '',
   dprt_id              int  comment '',
   admin_class_name     varchar(50)  comment '',
   admin_class_code     char(10)  comment '',
   comment              varchar(300)  comment '',
   primary key (admin_class_id)
);

/*==============================================================*/
/* Table: college                                               */
/*==============================================================*/
create table college
(
   college_name         varchar(50)  comment '',
   college_id           int not null  comment '',
   college_code         char(20)  comment '',
   comment              varchar(300)  comment '',
   primary key (college_id)
);

/*==============================================================*/
/* Table: course_credits                                        */
/*==============================================================*/
create table course_credits
(
   credits_id           int not null  comment '',
   subject_id           int  comment '',
   term_id              int  comment '',
   credits              decimal(10,1)  comment '',
   primary key (credits_id)
);

/*==============================================================*/
/* Table: course_section                                        */
/*==============================================================*/
create table course_section
(
   section_id           int not null AUTO_INCREMENT  comment '',
   term_id              int  comment '',
   teacher_id           int  comment '',
   subject_id           int  comment '',
   max_student          int  comment '',
   comment              varchar(300)  comment '',
   primary key (section_id)
);

alter table course_section comment '';

/*==============================================================*/
/* Table: department                                            */
/*==============================================================*/
create table department
(
   dprt_id              int not null  comment '',
   college_id           int  comment '',
   dprt_name            varchar(50)  comment '',
   dprt_code            char(20)  comment '',
   comment              varchar(300)  comment '',
   primary key (dprt_id)
);

/*==============================================================*/
/* Table: section_schedule                                      */
/*==============================================================*/


create table section_schedule
(
   schedule_id          int auto_increment primary key,  -- 新增自增主键，方便管理
   section_id           int not null,                    -- 关联课程班级
   class_day            date,                            -- 上课日期
   period_start         int,                             -- 起始节次 (1-11)
   period_end           int,                             -- 结束节次 (1-11)
   start_time           time,                            -- 具体开始时间 (自动计算)
   end_time             time,                            -- 具体结束时间 (自动计算)
   comment              varchar(300),                    -- 教室或其他备注
   
   -- 外键约束
   constraint FK_SCHEDULE_SECTION foreign key (section_id)
      references course_section (section_id) on delete cascade on update restrict
);

/*==============================================================*/
/* Table: semester                                              */
/*==============================================================*/
create table semester
(
   term_id              int not null  comment '',
   academic_year        char(4)  comment '',
   semester_type        char(1)  comment '',
   comment              varchar(300)  comment '',
   primary key (term_id)
);

/*==============================================================*/
/* Table: student                                               */
/*==============================================================*/
create table student
(
   stu_id               int not null  comment '',
   admin_class_id       int  comment '',
   stu_name             varchar(50)  comment '',
   stu_number           char(10)  comment '',
   comment              varchar(300)  comment '',
   primary key (stu_id)
);

/*==============================================================*/
/* Table: student_course_choice                                 */
/*==============================================================*/
create table student_course_choice
(
   stu_id               int not null  comment '',
   session_id           int  comment '',
   score                decimal(10,2)  comment '',
   comment              varchar(300)  comment '',
   primary key (stu_id, session_id)
);

/*==============================================================*/
/* Table: subject                                               */
/*==============================================================*/
create table subject
(
   subject_name         varchar(50)  comment '',
   subject_id           int not null  comment '',
   dprt_id              int not null  comment '',
   status               char(10)  comment '',
   subject_code         varchar(50)  comment '',
   comment              varchar(300)  comment '',
   primary key (subject_id)
);

/*==============================================================*/
/* Table: subject_prerequisite                                  */
/*==============================================================*/
create table subject_prerequisite
(
   subject_id           int not null  comment '',
   pre_subject_id       int not null  comment '',
   comment              varchar(300)  comment '',
   primary key (subject_id, pre_subject_id)
);

/*==============================================================*/
/* Table: teacher                                               */
/*==============================================================*/
create table teacher
(
   teacher_name         varchar(50)  comment '',
   teacher_id           int not null  comment '',
   dprt_id              int  comment '',
   title                varchar(30)  comment '',
   teacher_number       char(10)  comment '',
   comment              varchar(300)  comment '',
   primary key (teacher_id)
);

alter table administrative_class add constraint FK_ADMINIST_REFERENCE_DEPARTME foreign key (dprt_id)
      references department (dprt_id) on delete restrict on update restrict;

alter table course_credits add constraint FK_COURSE_C_REFERENCE_SUBJECT foreign key (subject_id)
      references subject (subject_id) on delete restrict on update restrict;

alter table course_credits add constraint FK_COURSE_C_REFERENCE_SEMESTER foreign key (term_id)
      references semester (term_id) on delete restrict on update restrict;

alter table course_section add constraint FK_COURSE_S_REFERENCE_TEACHER foreign key (teacher_id)
      references teacher (teacher_id) on delete restrict on update restrict;

alter table course_section add constraint FK_COURSE_S_REFERENCE_SUBJECT foreign key (subject_id)
      references subject (subject_id) on delete restrict on update restrict;

alter table course_section add constraint FK_COURSE_S_REFERENCE_SEMESTER foreign key (term_id)
      references semester (term_id) on delete restrict on update restrict;

alter table department add constraint FK_DEPARTME_REFERENCE_COLLEGE foreign key (college_id)
      references college (college_id) on delete restrict on update restrict;

alter table section_schedule add constraint FK_SECTION__REFERENCE_COURSE_S foreign key (section_id)
      references course_section (section_id) on delete restrict on update restrict;

alter table student add constraint FK_STUDENT_REFERENCE_ADMINIST foreign key (admin_class_id)
      references administrative_class (admin_class_id) on delete restrict on update restrict;

alter table student_course_choice add constraint FK_STUDENT__REFERENCE_STUDENT foreign key (stu_id)
      references student (stu_id) on delete restrict on update restrict;

alter table student_course_choice add constraint FK_STUDENT__REFERENCE_COURSE_S foreign key (session_id)
      references course_section (section_id) on delete restrict on update restrict;

alter table subject add constraint FK_SUBJECT_REFERENCE_DEPARTME foreign key (dprt_id)
      references department (dprt_id) on delete restrict on update restrict;

alter table subject_prerequisite add constraint FK_SUBJECT__REFERENCE_SUBJECT foreign key (subject_id)
      references subject (subject_id) on delete restrict on update restrict;

alter table subject_prerequisite add constraint FK_SUBJECT__REFERENCE_SUBJECT_PRE foreign key (pre_subject_id)
      references subject (subject_id) on delete restrict on update restrict;

alter table teacher add constraint FK_TEACHER_REFERENCE_DEPARTME foreign key (dprt_id)
      references department (dprt_id) on delete restrict on update restrict;
      
drop table if exists password_t;

CREATE TABLE password_t (
    id      INTEGER PRIMARY KEY AUTO_INCREMENT,  -- 改用 id，并设置自动增长
    account VARCHAR(30) NOT NULL UNIQUE,        -- 不允许为空，且必须唯一！
    psw     VARCHAR(30) NOT NULL,               -- 密码不能为空
    role    INT CHECK (role BETWEEN 0 AND 2)    -- 0:学生, 1:教师, 2:管理员
);

Insert into password_t (account, psw, role) VALUES ('admin', 'admin', 2);


SET foreign_key_checks = 0;


CREATE TABLE IF NOT EXISTS system_log (
    log_id INT AUTO_INCREMENT PRIMARY KEY,
    log_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    action_type VARCHAR(20),      -- INSERT, UPDATE, DELETE
    table_name VARCHAR(50),       -- 操作了哪张表
    record_id VARCHAR(50),        -- 受影响记录的主键ID
    details TEXT                  -- 详细信息（存旧值或新值）
);

DROP TRIGGER IF EXISTS trig_student_insert;

DELIMITER $$

CREATE TRIGGER trig_student_insert
AFTER INSERT ON student
FOR EACH ROW
BEGIN
    INSERT INTO system_log (action_type, table_name, record_id, details)
    VALUES ('INSERT', 'student', NEW.stu_id, CONCAT('新增学生: ', NEW.stu_name, ' (', NEW.stu_number, ')'));
END$$

DELIMITER ;

DROP TRIGGER IF EXISTS trig_student_delete;

DELIMITER $$

CREATE TRIGGER trig_student_delete
AFTER DELETE ON student
FOR EACH ROW
BEGIN
    INSERT INTO system_log (action_type, table_name, record_id, details)
    VALUES ('DELETE', 'student', OLD.stu_id, CONCAT('删除学生: ', OLD.stu_name, ' (', OLD.stu_number, ')'));
END$$

DELIMITER ;

DROP TRIGGER IF EXISTS trig_student_update;

DELIMITER $$

CREATE TRIGGER trig_student_update
AFTER UPDATE ON student
FOR EACH ROW
BEGIN
    -- 只有当关键信息变动时才记录
    IF OLD.stu_name != NEW.stu_name OR OLD.admin_class_id != NEW.admin_class_id THEN
        INSERT INTO system_log (action_type, table_name, record_id, details)
        VALUES ('UPDATE', 'student', NEW.stu_id, 
                CONCAT('修改学生信息: ', OLD.stu_name, ' -> ', NEW.stu_name));
    END IF;
END$$

DELIMITER ;

DROP TRIGGER IF EXISTS check_capacity_before_insert;

DELIMITER $$

CREATE TRIGGER check_capacity_before_insert
BEFORE INSERT ON student_course_choice
FOR EACH ROW
BEGIN
    -- 定义两个变量：最大容量、当前已选人数
    DECLARE max_cap INT;
    DECLARE curr_count INT;
    
    -- 1. 获取该课程的最大容量 (从 course_section 表)
    SELECT max_student INTO max_cap 
    FROM course_section 
    WHERE section_id = NEW.session_id;
    
    -- 2. 实时计算当前已选人数 (从 student_course_choice 表)
    SELECT COUNT(*) INTO curr_count 
    FROM student_course_choice 
    WHERE session_id = NEW.session_id;
    
    -- 3. 比较：如果 (当前人数 >= 最大容量)，则报错拦截
    IF curr_count >= max_cap THEN
        SIGNAL SQLSTATE '45000' 
        SET MESSAGE_TEXT = 'Error: Course is full (Trigger denied)';
    END IF;

END$$

DELIMITER ;
