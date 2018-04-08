#include <iostream>
#include <thread>

#include "socket.h"
#include "miniDFS.h"

using namespace std;

#define NS_PORT 2333
#define LOCAL_HOST "127.0.0.1"


int main(int argc, char *argv[])
{
    minidfs dfs;

    dfs.Connect(LOCAL_HOST, NS_PORT);
    int id = dfs.UploadFile(string(argv[1]));
    cout << string(argv[1]) << "id " << id << endl;
    dfs.Disconnect();

    return 0;
}
