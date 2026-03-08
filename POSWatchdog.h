#pragma once 

#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include "utils.h"

using namespace std;

class POSWatcher {
private:
    string file_path;
    Buffer& q;
    streampos last_pos;

    OrderInfo parseLine(const string& line);

public:
    POSWatcher(const string& path, Buffer& q) 
        : file_path(path), q(q), last_pos(0) {}

    void start();
};