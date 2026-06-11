#pragma once
#include <string>
#include "MyVector.h"

using namespace std;

struct MajorNode {
    string code;
    string name;

    MajorNode() {}
    MajorNode(string c, string n) : code(c), name(n) {}
};

struct Course {
    string id;
    string name;
    int credit;
    int type;
    int season;
    string major;

    int inDegree;
    int outDegree;
    int assignedTerm;

    MyVector<int> nextCourses;

    Course() : credit(0), type(0), season(0), inDegree(0), outDegree(0), assignedTerm(-1) {}

    Course(string i, string n, int c, int t, int s, string m) : id(i), name(n), credit(c), type(t), season(s), major(m), inDegree(0), outDegree(0), assignedTerm(-1) {}

};

class CourseSystem {
private:
    MyVector<MajorNode> majorList;
    MyVector<Course> courses;
    int maxCoursesPerTerm;
    int maxCreditsPerTerm;
    int maxPoliticsPerTerm;

    int findCourseIndex(string id);
    void clearData();

public:
    CourseSystem();
    void setConstraints(int maxCourse, int maxCredit);
    void addCourse(string id, string name, int credit, int type, int season, string major);
    void addPrerequisite(string preId, string targetId);
    bool loadFromFile(string filename);
    bool generateSchedule(string targetMajor);
    void displaySchedule(string targetMajor, string majorName);
    bool exportAllSchedules(string outFilename);
    void startMenu();
};