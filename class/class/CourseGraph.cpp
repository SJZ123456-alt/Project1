#include "CourseGraph.h"
#include <iostream>
#include <fstream>
#include <windows.h>

using namespace std;

string UTF8ToGBK(const string& str_utf8) {
    int len = MultiByteToWideChar(CP_UTF8, 0, str_utf8.c_str(), -1, NULL, 0);
    if (len <= 0) return str_utf8;
    wchar_t* wszGBK = new wchar_t[len + 1];
    memset(wszGBK, 0, len * 2 + 2);
    MultiByteToWideChar(CP_UTF8, 0, str_utf8.c_str(), -1, wszGBK, len);
    len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
    char* szGBK = new char[len + 1];
    memset(szGBK, 0, len + 1);
    WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
    string strTemp(szGBK);
    delete[] wszGBK;
    delete[] szGBK;
    return strTemp;
}

CourseSystem::CourseSystem() : maxCoursesPerTerm(6), maxCreditsPerTerm(16), maxPoliticsPerTerm(1) {}

void CourseSystem::setConstraints(int maxCourse, int maxCredit) {
    maxCoursesPerTerm = maxCourse;
    maxCreditsPerTerm = maxCredit;
}

void CourseSystem::clearData() {
    courses.clear();
    majorList.clear();
}

int CourseSystem::findCourseIndex(string id) {
    for (int i = 0; i < courses.size(); i++)
    {
        if (courses[i].id == id)
        {
            return i;
        }
    }
    return -1;
}

void CourseSystem::addCourse(string id, string name, int credit, int type, int season, string major)
{
    if (findCourseIndex(id) == -1) courses.push_back(Course(id, name, credit, type, season, major));
}

void CourseSystem::addPrerequisite(string preId, string targetId)
{
    int u = findCourseIndex(preId);
    int v = findCourseIndex(targetId);
    if (u != -1 && v != -1)
    {
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

    for (int i = 0; i < mCount; i++) {
        string code, name;
        file >> code >> name;
        name = UTF8ToGBK(name);
        majorList.push_back(MajorNode(code, name));
    }

    for (int i = 0; i < cCount; i++) {
        string id, name, major; int c, t, s;
        file >> id >> name >> c >> t >> s >> major;
        name = UTF8ToGBK(name);
        addCourse(id, name, c, t, s, major);
    }

    for (int i = 0; i < eCount; i++) {
        string p, t; file >> p >> t;
        addPrerequisite(p, t);
    }
    file.close();
    return true;
}

bool CourseSystem::generateSchedule(string targetMajor) {
    MyVector<int> currentInDegree;
    for (int i = 0; i < courses.size(); i++) {
        currentInDegree.push_back(0);
    }

    int validCount = 0;

    for (int i = 0; i < courses.size(); i++)
    {
        if (courses[i].major == "ALL" || courses[i].major.find(targetMajor) != string::npos)
        {
            courses[i].assignedTerm = -1;
            validCount++;
        }
        else
        {
            courses[i].assignedTerm = -99;
        }
    }
    for (int i = 0; i < courses.size(); i++)
    {
        if (courses[i].assignedTerm == -1)
        {
            for (int j = 0; j < courses[i].nextCourses.size(); j++)
            {
                int v = courses[i].nextCourses[j];
                if (courses[v].assignedTerm == -1)
                {
                    currentInDegree[v]++;
                }
            }
        }
    }

    int term = 1, scheduled = 0;

    while (scheduled < validCount && term <= 8) {
        MyVector<int> readyQueue;

        int season = (term % 2 != 0) ? 1 : 2;
        bool waitSeason = false;

        for (int i = 0; i < courses.size(); i++)
        {
            if (courses[i].assignedTerm == -1 && currentInDegree[i] == 0) {
                if (courses[i].season == 0 || courses[i].season == season) readyQueue.push_back(i);
                else waitSeason = true;
            }
        }

        if (readyQueue.size() == 0)
        {
            if (waitSeason)
            {
                term++; continue;
            }
            else break;
        }

        for (int i = 0; i < readyQueue.size() - 1; i++) {
            for (int j = 0; j < readyQueue.size() - 1 - i; j++) {
                int a = readyQueue[j];
                int b = readyQueue[j + 1];
                bool swapNeeded = false;

                if (courses[a].type != courses[b].type) {
                    swapNeeded = courses[a].type > courses[b].type;
                }
                else if (courses[a].outDegree != courses[b].outDegree) {
                    swapNeeded = courses[a].outDegree < courses[b].outDegree;
                }
                else {
                    swapNeeded = courses[a].credit < courses[b].credit;
                }

                if (swapNeeded) {
                    int temp = readyQueue[j];
                    readyQueue[j] = readyQueue[j + 1];
                    readyQueue[j + 1] = temp;
                }
            }
        }

        int tC = 0, tCr = 0, tP = 0;
        MyVector<int> justScheduled;

        for (int i = 0; i < readyQueue.size(); i++) {
            int idx = readyQueue[i];
            if (tC >= maxCoursesPerTerm || tCr + courses[idx].credit > maxCreditsPerTerm || (courses[idx].type == 1 && tP >= maxPoliticsPerTerm)) continue;
            courses[idx].assignedTerm = term;
            justScheduled.push_back(idx);
            tC++;
            tCr += courses[idx].credit;
            if (courses[idx].type == 1) tP++;
        }

        if (justScheduled.size() == 0)
        {
            term++;
            continue;
        }

        for (int i = 0; i < justScheduled.size(); i++)
        {
            int u = justScheduled[i];
            for (int j = 0; j < courses[u].nextCourses.size(); j++)
            {
                int v = courses[u].nextCourses[j];
                if (courses[v].assignedTerm == -1) currentInDegree[v]--;
            }
        }

        scheduled += justScheduled.size();
        term++;
    }
    return scheduled == validCount;
}

void CourseSystem::displaySchedule(string targetMajor, string majorName) {
    cout << "\n[" << majorName << " 专业] 培养方案\n";
    int maxT = 0;
    for (int i = 0; i < courses.size(); i++) {
        if (courses[i].assignedTerm > maxT) maxT = courses[i].assignedTerm;
    }
    for (int t = 1; t <= maxT; t++) {
        cout << "【第 " << t << " 学期】" << (t % 2 != 0 ? "(秋季)" : "(春季)") << "\n";
        for (int i = 0; i < courses.size(); i++) {
            if (courses[i].assignedTerm == t) {
                cout << " - " << (courses[i].type == 1 ? "[公]" : "[专]") << " " << courses[i].name << "(" << courses[i].credit << "学分)\n";
            }
        }
    }
}

bool CourseSystem::exportAllSchedules(string outFilename) {
    ofstream outFile(outFilename);
    if (!outFile.is_open()) return false;

    outFile << ">>> 全校各专业培养方案汇总 <<<\n";
    outFile << "当前约束: 每学期最多 " << maxCoursesPerTerm << " 课, " << maxCreditsPerTerm << " 学分\n\n";

    for (int i = 0; i < majorList.size(); i++) {
        string mCode = majorList[i].code;
        string mName = majorList[i].name;

        if (generateSchedule(mCode)) {
            outFile << "================ [" << mName << " 专业] 培养方案 ================\n";
            int maxT = 0;
            for (int j = 0; j < courses.size(); j++) {
                if (courses[j].assignedTerm > maxT) maxT = courses[j].assignedTerm;
            }
            for (int t = 1; t <= maxT; t++) {
                outFile << "【第 " << t << " 学期】" << (t % 2 != 0 ? "(秋季)" : "(春季)") << "\n";
                for (int j = 0; j < courses.size(); j++) {
                    if (courses[j].assignedTerm == t) {
                        outFile << " - " << (courses[j].type == 1 ? "[公]" : "[专]") << " " << courses[j].name << "(" << courses[j].credit << "学分)\n";
                    }
                }
            }
            outFile << "\n";
        }
        else {
            outFile << "================ [" << mName << " 专业] 排课失败 ================\n";
            outFile << "原因：该专业内部的有向图存在回路，或排课约束过度严苛。\n\n";
        }
    }
    outFile.close();
    return true;
}

void CourseSystem::startMenu() {
    system("chcp 936 > nul");
    string currentFile = "";

    while (true) {
        system("cls");
        cout << "排课系统\n\n";
        cout << "请输入包含全校数据的文件名 (例如: courses.txt): ";
        cin >> currentFile;

        if (loadFromFile(currentFile)) {
            cout << "\n成功读取数据\n";
            system("pause");
            break;
        }
        else {
            cout << "\n找不到文件 [" << currentFile << "] 或数据格式不匹配\n";
            system("pause");
        }
    }

    while (true) {
        system("cls");
        cout << "排课系统\n\n";
        cout << "当前使用数据库: " << currentFile << "\n";
        cout << "已从文件中解析到以下专业信息：\n\n";

        for (int i = 0; i < majorList.size(); i++)
        {
            cout << i + 1 << ". 生成【" << majorList[i].name << " (" << majorList[i].code << ")】课表\n";
        }

        int constraintOption = majorList.size() + 1;
        int switchFileOption = majorList.size() + 2;
        int exportOption = majorList.size() + 3;

        cout << "\n--- 系统设置 ---\n";
        cout << constraintOption << ". 修改排课约束 (当前: 每学期最多" << maxCoursesPerTerm << "课, " << maxCreditsPerTerm << "学分)\n";
        cout << switchFileOption << ". 切换其他文件\n";
        cout << exportOption << ". 一键导出所有专业课表\n";
        cout << "0. 退出系统\n\n请选择操作: ";

        int choice;
        cin >> choice;

        if (choice >= 1 && choice <= majorList.size()) {
            string mCode = majorList[choice - 1].code;
            string mName = majorList[choice - 1].name;
            if (generateSchedule(mCode)) {
                displaySchedule(mCode, mName);
            }
            else {
                cout << "\n排课失败\n原因：该专业内部的有向图存在回路，或排课约束过度严苛。\n\n";
            }
            system("pause");
        }
        else if (choice == constraintOption) {
            int cCount, cCredit;
            cout << "输入每学期最大排课门数(当前: 每学期最多" << maxCoursesPerTerm << "课):";
            cin >> cCount;
            cout << "输入每学期最大学分上限(当前: 每学期最多" << maxCreditsPerTerm << "学分):";
            cin >> cCredit;
            if (cCount > 0 && cCredit > 0) setConstraints(cCount, cCredit);
        }
        else if (choice == switchFileOption) {
            string newFile;
            cout << "\n请输入新的文件名 (如 courses.txt): ";
            cin >> newFile;

            if (loadFromFile(newFile)) {
                currentFile = newFile;
                cout << "\n切换成功\n";
            }
            else {
                cout << "\n切换失败，系统将继续使用原数据 [" << currentFile << "]\n";
                loadFromFile(currentFile);
            }
            system("pause");
        }
        else if (choice == exportOption) {
            string outFilename = currentFile;
            size_t dotPos = outFilename.find_last_of('.');
            outFilename.insert(dotPos, "_success");

            if (exportAllSchedules(outFilename)) {
                cout << "\n导出成功！所有专业的课表已保存至 [" << outFilename << "] 文件中\n";
            }
            else {
                cout << "\n导出失败！\n";
            }
            system("pause");
        }
        else if (choice == 0) {
            break;
        }
    }
}