#include "utils.h"
#include "POSWatchdog.h"
#include "CloudUploader.h"

using namespace std;

int main() {
    Buffer order_buffer;
    POSWatcher watcher("/Users/itaide-beer/LegacyBridge/mock_pos.csv", order_buffer);

    string supabase_domain = "lichtpsvufgmanyqerpe.supabase.co"; 
    string supabase_key = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImxpY2h0cHN2dWZnbWFueXFlcnBlIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzI3ODM2NzYsImV4cCI6MjA4ODM1OTY3Nn0.JPpCyg1edPkymD7hiMfxzkT-PER5kVetdOg3Zztw2uE";
    CloudUploader uploader(order_buffer, supabase_domain, supabase_key);

    cout << "Starting LegacyBridge Service..." << endl;
    
    thread watcher_thread(&POSWatcher::start, &watcher);
    
    thread uploader_thread(&CloudUploader::start, &uploader);

    watcher_thread.join();
    uploader_thread.join();
    return 0;
}
