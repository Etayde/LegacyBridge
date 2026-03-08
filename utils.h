#pragma once

#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>

using namespace std;


struct OrderInfo {
    string timestamp;
    int table_id;
    string item_name;
    string category;
    float price;

    OrderInfo parseOrderInfo(const string& line);
};



class Buffer {
    queue<OrderInfo> q;
    mutex mtx;
    condition_variable cv;

public:
    void push(OrderInfo info);
    bool pop(OrderInfo& info);
};