#ifndef COURSEGRAPH_H
#define COURSEGRAPH_H

#include <vector>
#include <string>
#include <functional> // 支持 std::function

using namespace std;

struct Course {
    string id, name, major;
    int credit, type, season, assignedTerm;
    vector<int> nextCourses;
    int inDegree = 0, outDegree = 0;
    Course(string i = "", string n = "", int c = 0, int t = 0, int s = 0, string m = "")
        : id(i), name(n), credit(c), type(t), season(s), major(m) {
    }
};

struct MajorInfo {
    string code, name;
    MajorInfo(string c = "", string n = "") : code(c), name(n) {}
};

class CourseSystem {
private:
    vector<Course> courses;
    vector<MajorInfo> majors;
    // ✅ 新增：存储排课约束警告
    vector<string> scheduleWarnings;

public:
    int maxCoursesPerTerm, maxCreditsPerTerm, maxPoliticsPerTerm;

    CourseSystem();
    void setColor(int color);
    void setConstraints(int maxCourse, int maxCredit);
    void clearData();
    void clearScheduleWarnings();

    int findCourseIndex(string id);
    void addCourse(string id, string name, int credit, int type, int season, string major);
    void addPrerequisite(string preId, string targetId);

    bool loadFromFile(string filename);
    bool hasCycle(string targetMajor);

    bool generateSchedule(string targetMajor);
    void displaySchedule(string targetMajor, string majorName);
    void startMenu();
};

#endif