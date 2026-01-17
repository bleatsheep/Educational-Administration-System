# 教务管理系统 (Educational Administration System)

基于 Qt C++ 和 MySQL 数据库开发的教务管理系统。本项目采用 C/S 架构，实现了前后端分离的设计思想，旨在提供一个界面现代、交互流畅的教育管理工具。

[![Qt](https://img.shields.io/badge/Qt-6.0%2B-green.svg)](https://www.qt.io/)
[![MySQL](https://img.shields.io/badge/MySQL-9.5-blue.svg)](https://www.mysql.com/)


> **注意**：本项目为数据库课程设计作品，主要展示以Qt为前端框架构建数据库



## 主要功能 (Features)

### 学生端 (Student)
* **选课中心**: 浏览可选课程，查看学分、教师及实时容量。选课时自动进行**时间冲突检测**和**名额检查**。
* **我的课表**: 7x12 格式的可视化周课表，支持按周切换视图，不同课程颜色区分。
* **成绩查询**: 查看已修课程成绩，自动计算**加权平均绩** (GPA) 和总学分。
* **个人中心**: 查看学籍信息及班级归属。

### 教师端 (Teacher)
* **课程概览**: 查看教授的课程列表，实时显示选课人数、评分进度和课程平均分。
* **成绩录入**: 支持对学生进行打分和修改，根据分数段自动显示颜色（及格绿/挂科红）。
* **学生名单**: 查看某门课程的具体选课学生名单。

### 管理员端 (Admin)
* **全能管理**: 对学院、专业、学科、学生、教师进行 CRUD (增删改查) 操作。
* **智能排课**: 设置课程时间、教室、单双周频率，系统自动生成一学期的排课记录。
* **账号管理**: 一键自动为没有账号的师生生成默认账号，支持重置密码。
* **SQL 控制台**: 内置隐藏的开发者控制台 (按 `~` 键呼出)，支持直接执行 SQL 语句。
* **系统日志**: 监控数据库的关键变动操作。

## 技术栈 (Tech Stack)

*   **前端/逻辑**：C++ / Qt Framework (Widgets 模块)
*   **数据库**：MySQL Community Server
*   **连接方式**：ODBC (Open Database Connectivity)
*   **构建工具**：QMake / CMake

## 项目结构 (Project Structure)
```
Educational-Administration-System/ 
├── SQL/ # 数据库构建与初始化脚本 
│ ├── CREAT_TABLE.sql # 建表语句及数据库结构定义 
│ └── TEST_DATA.sql # 初始测试数据
├── source/ # Qt源代码
│ ├── mainwindow.ui # 主窗口ui设计
│ ├── mainwindow.h # 主窗口头文件 
│ ├── mainwindow.cpp # 主窗口逻辑实现
│ └── ... # 其他业务逻辑源码 
├── release/ #可执行文件及依赖 
│ ├── test.exe # 编译好的教务系统程序
│ ├── db_config.ini # ODBC连接配置文件 
│ └── ... # 其他依赖文件
└── README.md # 项目说明文档
```


## 快速开始 (Getting Started)

### 1. 环境准备
*   安装 **MySQL** 数据库。
*   配置 **ODBC 数据源**：确保你的系统已安装 MySQL ODBC Connector，并在 Windows "ODBC 数据源管理器" 中配置好驱动。
> 本项目ODBC下载地址：[![ODBC](https://img.shields.io/badge/ODBC-9.5-blue.svg)](https://dev.mysql.com/downloads/connector/odbc/)

### 2. 数据库配置
1.  找到项目目录下的 `SQL/CREATE_TABLE.sql`
2.  在 MySQL 中执行该脚本，创建数据库 `SCHOOL` 及相关表结构
3.  **重要提示**：在记事本中修改 `release/db_config.ini` 中的数据库连接信息：
	将连接账号改为与数据库建立连接的账号，密码设置为该账号的密码，连接名称改为ODBC设置的连接
```
// db_config.ini
acc:连接账号;psw:密码;odbc:连接名称;
```
>**PS**：不建议使用root权限连接数据库，误操作风险高
### 3. 启动主程序
1.	找到`release/test.exe`，双击运行
> **注意**：若出现错误，请仔细检查同级目录中`db_config.ini`是否正确修改
2.	成功运行后，请先以管理员admin登录填充数据库：我们在`SQL/CREATE_TABLE.sql`中提供了初始管理员**账号**`admin` **密码**`admin`
3. 我们还提供了部分用于功能展示的测试sql插入语句于`SQL/TEST_DATA.sql`，执行后可测试大部分功能
> 测试样例中给出了若干学生和老师账号密码，可登录查看
