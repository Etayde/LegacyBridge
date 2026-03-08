#include "POSWatchdog.h"

OrderInfo POSWatcher::parseLine(const string& line) {
    stringstream ss(line);
    string token;
    OrderInfo info;

    getline(ss, info.timestamp, ',');
    
    getline(ss, token, ',');
    info.table_id = stoi(token);
    
    getline(ss, info.item_name, ',');
    getline(ss, info.category, ',');
    
    getline(ss, token, ',');
    info.price = stod(token);

    return info;
}

void POSWatcher::start() {
    cout << "Starting POS Watcher on: " << file_path << endl;
    
    while (true) {
        ifstream file(file_path);
        
        if (file.is_open()) {
            file.seekg(last_pos); 

            string line;
            while (getline(file, line)) {
                
                if (line.find("Timestamp") != string::npos) continue;
                if (line.empty()) continue; 
                OrderInfo info = parseLine(line);
                q.push(info);
                
                cout << "[Watcher] Queued new order: " << info.item_name 
                            << " (Table " << info.table_id << ")" << endl;
            }

            file.clear(); 
            last_pos = file.tellg();
            
            file.close();
        }
        
        this_thread::sleep_for(chrono::seconds(2));
    }
}