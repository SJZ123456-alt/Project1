#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS
#define NOMINMAX
#include <windows.h>
#include "CourseGraph.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <set>
#include <queue>
#include <vector>
#include <string>
#include <cctype>

using namespace std;

// UTF-8 -> GBK
static string UTF8ToGBK(const string& str_utf8) {
    int len = MultiByteToWideChar(CP_UTF8, 0, str_utf8.c_str(), -1, NULL, 0);
    if (len <= 0) return str_utf8;
    wchar_t* wszGBK = new wchar_t[len + 1];
    memset(wszGBK, 0, static_cast<size_t>(len) * 2 + 2);
    MultiByteToWideChar(CP_UTF8, 0, str_utf8.c_str(), -1, wszGBK, len);
    len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
    char* szGBK = new char[len + 1];
    memset(szGBK, 0, static_cast<size_t>(len) + 1);
    WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
    string strTemp(szGBK);
    delete[] wszGBK;
    delete[] szGBK;
    return strTemp;
}

CourseSystem::CourseSystem()
    : maxCoursesPerTerm(6),
    maxCreditsPerTerm(16),
    maxPoliticsPerTerm(2), // 调整为 2：允许同一学期排 2 门公选/外语/思政，解决 8 学期内排不完 9 门公共课的硬性冲突
    maxTerms(8) {          // 限制最大为 8 学期
}

void CourseSystem::setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void CourseSystem::setConstraints(int maxCourse, int maxCredit) {
    maxCoursesPerTerm = maxCourse;
    maxCreditsPerTerm = maxCredit;
}

void CourseSystem::clearData() {
    courses.clear();
    majors.clear();
}

void CourseSystem::clearScheduleWarnings() {
    scheduleWarnings.clear();
}

int CourseSystem::findCourseIndex(string id) {
    for (size_t i = 0; i < courses.size(); ++i) {
        if (courses[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

void CourseSystem::addCourse(string id, string name, int credit, int type, int season, string major) {
    if (findCourseIndex(id) == -1) {
        courses.emplace_back(id, name, credit, type, season, major);
        courses.back().assignedTerm = -1;
    }
}

void CourseSystem::addPrerequisite(string preId, string targetId) {
    int u = findCourseIndex(preId);
    int v = findCourseIndex(targetId);
    if (u != -1 && v != -1) {
        courses[u].nextCourses.push_back(v);
        courses[v].inDegree++;
        courses[u].outDegree++;
    }
}

bool CourseSystem::loadFromFile(string filename) {
    clearData();
    ifstream file(filename);
    if (!file.is_open()) return false;

    int mCount, cCount, eCount;
    if (!(file >> mCount >> cCount >> eCount)) return false;

    for (int i = 0; i < mCount; ++i) {
        string code, name;
        file >> code >> name;
        name = UTF8ToGBK(name);
        majors.emplace_back(code, name);
    }

    for (int i = 0; i < cCount; ++i) {
        string id, name, major; int c, t, s;
        file >> id >> name >> c >> t >> s >> major;
        name = UTF8ToGBK(name);
        addCourse(id, name, c, t, s, major);
    }

    for (int i = 0; i < eCount; ++i) {
        string p, t; file >> p >> t;
        addPrerequisite(p, t);
    }
    file.close();
    return true;
}

bool CourseSystem::hasCycle(string targetMajor) {
    enum Color { WHITE, GRAY, BLACK };
    vector<Color> color(courses.size(), WHITE);

    auto isRelevant = [&](int idx) {
        if (idx < 0 || idx >= static_cast<int>(courses.size())) return false;
        const auto& c = courses[idx];
        return (c.major == "ALL" || c.major.find(targetMajor) != string::npos);
        };

    function<bool(int)> dfs = [&](int u) -> bool {
        color[u] = GRAY;
        for (int v : courses[u].nextCourses) {
            if (!isRelevant(v)) continue;
            if (color[v] == GRAY) return true;
            if (color[v] == WHITE && dfs(v)) return true;
        }
        color[u] = BLACK;
        return false;
        };

    for (size_t i = 0; i < courses.size(); ++i) {
        if (isRelevant(static_cast<int>(i)) && color[i] == WHITE) {
            if (dfs(static_cast<int>(i))) return true;
        }
    }
    return false;
}

bool CourseSystem::generateSchedule(string targetMajor) {
    clearScheduleWarnings();

    bool has_cycle = hasCycle(targetMajor);
    if (has_cycle) {
        setColor(14);
        cout << "[警告] 检测到循环依赖，系统将预先自动解除冲突以实现最紧凑排课。\n";
        setColor(7);
    }

    auto isRelevant = [&](int idx) {
        if (idx < 0 || idx >= static_cast<int>(courses.size())) return false;
        const auto& c = courses[idx];
        return (c.major == "ALL" || c.major.find(targetMajor) != string::npos);
        };

    // 1. 初始化入度与分配状态
    vector<int> currentInDegree(courses.size(), 0);
    for (size_t i = 0; i < courses.size(); ++i) {
        if (isRelevant(static_cast<int>(i))) {
            courses[i].assignedTerm = -1;
        }
        else {
            courses[i].assignedTerm = -99;
        }
    }

    for (size_t i = 0; i < courses.size(); ++i) {
        if (courses[i].assignedTerm == -1) {
            for (int v : courses[i].nextCourses) {
                if (courses[v].assignedTerm == -1) {
                    ++currentInDegree[v];
                }
            }
        }
    }

    // 2. 预先消除循环依赖
    vector<int> tempInDegree = currentInDegree;
    queue<int> tempQ;
    for (size_t i = 0; i < courses.size(); ++i) {
        if (courses[i].assignedTerm == -1 && tempInDegree[i] == 0) {
            tempQ.push(static_cast<int>(i));
        }
    }

    vector<bool> visited(courses.size(), false);

    while (true) {
        while (!tempQ.empty()) {
            int u = tempQ.front(); tempQ.pop();
            visited[u] = true;
            for (int v : courses[u].nextCourses) {
                if (courses[v].assignedTerm == -1 && !visited[v]) {
                    if (--tempInDegree[v] == 0) {
                        tempQ.push(v);
                    }
                }
            }
        }

        int bestU = -1;
        int minInDegree = 1e9;
        for (size_t i = 0; i < courses.size(); ++i) {
            if (courses[i].assignedTerm == -1 && !visited[i]) {
                if (tempInDegree[i] < minInDegree) {
                    minInDegree = tempInDegree[i];
                    bestU = static_cast<int>(i);
                }
            }
        }

        if (bestU != -1) {
            tempInDegree[bestU] = 0;
            tempQ.push(bestU);
            currentInDegree[bestU] = 0;
            scheduleWarnings.push_back(
                "[提示] 已自动解除课程[" + courses[bestU].name + "(" + courses[bestU].id + ")]的先修限制（用于打破循环依赖）"
            );
        }
        else {
            break;
        }
    }

    // 3. 正式拓扑排序排课
    queue<int> q;
    for (size_t i = 0; i < courses.size(); ++i) {
        if (courses[i].assignedTerm == -1 && currentInDegree[i] == 0) {
            q.push(static_cast<int>(i));
        }
    }

    int currentTerm = 1;

    while (!q.empty() && currentTerm <= maxTerms) {
        int levelSize = q.size();
        vector<int> deferred;

        for (int i = 0; i < levelSize; ++i) {
            int u = q.front(); q.pop();

            int termCourseCnt = 0, termCreditSum = 0, termPoliticsCnt = 0;
            for (auto& c : courses) {
                if (c.assignedTerm == currentTerm) {
                    termCourseCnt++;
                    termCreditSum += c.credit;
                    if (c.type == 1) termPoliticsCnt++;
                }
            }

            bool seasonOK = (courses[u].season != 1 && courses[u].season != 2) ||
                (courses[u].season == 1 && currentTerm % 2 != 0) ||
                (courses[u].season == 2 && currentTerm % 2 == 0);

            bool canPlace = seasonOK &&
                (termCourseCnt < maxCoursesPerTerm) &&
                (termCreditSum + courses[u].credit <= maxCreditsPerTerm) &&
                !(courses[u].type == 1 && termPoliticsCnt >= maxPoliticsPerTerm);

            if (canPlace) {
                courses[u].assignedTerm = currentTerm;
                for (int v : courses[u].nextCourses) {
                    if (courses[v].assignedTerm == -1) {
                        if (--currentInDegree[v] == 0) {
                            q.push(v);
                        }
                    }
                }
            }
            else {
                deferred.push_back(u);
            }
        }

        for (int u : deferred) {
            q.push(u);
        }
        currentTerm++;
    }

    // 第二阶段：补录分配
    vector<int> unassigned;
    for (size_t i = 0; i < courses.size(); ++i) {
        if (courses[i].assignedTerm == -1 && isRelevant(static_cast<int>(i))) {
            unassigned.push_back(static_cast<int>(i));
        }
    }

    for (int u : unassigned) {
        bool placed = false;
        for (int t = 1; t <= maxTerms; ++t) {
            bool prereqOK = true;
            for (int v = 0; v < static_cast<int>(courses.size()); ++v) {
                if (find(courses[v].nextCourses.begin(), courses[v].nextCourses.end(), u) != courses[v].nextCourses.end()) {
                    if (isRelevant(v)) {
                        if (courses[v].assignedTerm == -1 || courses[v].assignedTerm >= t) {
                            prereqOK = false;
                            break;
                        }
                    }
                }
            }
            if (!prereqOK) continue;

            int cnt = 0, cred = 0, pol = 0;
            for (auto& c : courses) {
                if (c.assignedTerm == t) {
                    cnt++;
                    cred += c.credit;
                    if (c.type == 1) pol++;
                }
            }

            bool seasonOK = (courses[u].season != 1 && courses[u].season != 2) ||
                (courses[u].season == 1 && t % 2 != 0) ||
                (courses[u].season == 2 && t % 2 == 0);

            if (seasonOK &&
                cnt < maxCoursesPerTerm &&
                cred + courses[u].credit <= maxCreditsPerTerm &&
                !(courses[u].type == 1 && pol >= maxPoliticsPerTerm)) {

                courses[u].assignedTerm = t;
                placed = true;
                break;
            }
        }

        if (!placed) {
            scheduleWarnings.push_back(
                "[严重] 课程[" + courses[u].name + "(" + courses[u].id + ")] 因硬性约束限制无法在前 " + to_string(maxTerms) + " 学期内安排"
            );
        }
    }

    return true;
}

void CourseSystem::displaySchedule(string targetMajor, string majorName) {
    if (!scheduleWarnings.empty()) {
        setColor(14);
        cout << "\n===== 排课约束与异常提示 =====\n";
        for (const auto& warn : scheduleWarnings) {
            cout << warn << endl;
        }
        setColor(7);
        cout << "==============================\n\n";
    }

    setColor(11);
    cout << "\n================ [" << majorName << " 专业] 培养方案 ================\n";
    setColor(7);

    vector<int> termCourses(maxTerms + 1, 0);
    vector<int> termCredits(maxTerms + 1, 0);
    vector<int> termPolitics(maxTerms + 1, 0);

    for (auto& c : courses) {
        if (c.assignedTerm >= 1 && c.assignedTerm <= maxTerms) {
            int t = c.assignedTerm;
            termCourses[t]++;
            termCredits[t] += c.credit;
            if (c.type == 1) termPolitics[t]++;
        }
    }

    int maxT = 0;
    for (auto& c : courses) {
        if (c.assignedTerm > maxT && c.assignedTerm <= maxTerms) {
            maxT = c.assignedTerm;
        }
    }

    bool has_unscheduled = false;
    for (auto& c : courses) {
        if (c.assignedTerm == -1 && (c.major == "ALL" || c.major.find(targetMajor) != string::npos)) {
            has_unscheduled = true;
            break;
        }
    }

    if (has_unscheduled) {
        setColor(12);
        cout << "[注意] 以下课程因约束或先修关系未能成功安排在 " << maxTerms << " 个学期内:\n";
        for (auto& c : courses) {
            if (c.assignedTerm == -1 && (c.major == "ALL" || c.major.find(targetMajor) != string::npos)) {
                cout << " - " << (c.type == 1 ? "[公]" : "[专]") << " " << c.name << "(" << c.credit << "学分)\n";
            }
        }
        setColor(7);
    }

    for (int t = 1; t <= maxT; ++t) {
        if (termCourses[t] == 0) continue;
        setColor(14);
        cout << "【第 " << t << " 学期】" << (t % 2 != 0 ? "(秋)" : "(春)")
            << " (" << termCourses[t] << "门, " << termCredits[t] << "学分)\n";
        setColor(7);
        for (auto& c : courses) {
            if (c.assignedTerm == t) {
                cout << " - " << (c.type == 1 ? "[公]" : "[专]") << " " << c.name << "(" << c.credit << "学分)\n";
            }
        }
    }
}

void CourseSystem::startMenu() {
    system("chcp 936 > nul");

    string currentFile = "";
    while (true) {
        system("cls");
        setColor(10);
        cout << ">>> 终极教务排课引擎 <<<\n\n";
        setColor(7);
        cout << "请输入数据文件名 (如: 2.txt): ";
        cin >> currentFile;

        if (loadFromFile(currentFile)) {
            setColor(10);
            cout << "\n[加载成功]\n";
            setColor(7);
            system("pause");
            break;
        }
        else {
            setColor(12);
            cout << "\n[错误] 文件加载失败！\n";
            setColor(7);
            system("pause");
        }
    }

    while (true) {
        system("cls");
        setColor(10);
        cout << ">>> 主菜单 <<<\n\n";
        setColor(7);
        for (size_t i = 0; i < majors.size(); ++i) {
            cout << i + 1 << ". 生成【" << majors[i].name << " (" << majors[i].code << ")】课表\n";
        }
        cout << majors.size() + 1 << ". 修改约束\n";
        cout << majors.size() + 2 << ". 切换文件\n";
        cout << "0. 退出\n";
        cout << "请选择: ";

        int choice;
        cin >> choice;

        if (choice >= 1 && choice <= static_cast<int>(majors.size())) {
            string mCode = majors[choice - 1].code;
            string mName = majors[choice - 1].name;

            generateSchedule(mCode);

            int scheduled = 0, total = 0;
            for (auto& c : courses) {
                if (c.major == "ALL" || c.major.find(mCode) != string::npos) {
                    total++;
                    if (c.assignedTerm != -1 && c.assignedTerm <= maxTerms) scheduled++;
                }
            }

            displaySchedule(mCode, mName);

            setColor(10);
            cout << "\n[排课统计] 成功安排 " << scheduled << "/" << total << " 门课程\n";
            setColor(7);
            system("pause");
        }
        else if (choice == static_cast<int>(majors.size()) + 1) {
            int c, cr;
            cout << "每学期最多课程数: "; cin >> c;
            cout << "每学期最大学分: "; cin >> cr;
            setConstraints(c, cr);
        }
        else if (choice == static_cast<int>(majors.size()) + 2) {
            string f; cin >> f;
            if (loadFromFile(f)) currentFile = f;
        }
        else if (choice == 0) break;
    }
}