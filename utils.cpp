#include "utils.h"
#include <sstream>

OrderInfo parseOrderInfo(const string& line) {
    OrderInfo order;
    stringstream ss(line);
    string token;

    getline(ss, order.timestamp, ',');
    
    getline(ss, token, ',');
    order.table_id = stoi(token);
    
    getline(ss, order.item_name, ',');
    getline(ss, order.category, ',');
    
    getline(ss, token, ',');
    order.price = stof(token);

    return order;
}

void Buffer::push(OrderInfo info) {
        
    std::lock_guard<std::mutex> lock(mtx);
   
    q.push(std::move(info));
    cv.notify_one();
}

bool Buffer::pop(OrderInfo& info) {
    
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this] { return !q.empty(); });
    
    info = std::move(q.front());
    q.pop();
    
    return true;
}