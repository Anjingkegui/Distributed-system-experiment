#include <iostream>
#include <thread>
#include <fstream>

#include "socket.h"
#include "miniDFS.h"

using namespace std;

#define NS_PORT 2333
#define LOCAL_HOST "127.0.0.1"

int file_size(string file)
{
    struct stat st;
    stat(file.data(), &st);
    return st.st_size;
}

int main(int argc, char *argv[])
{
    minidfs dfs;
    int id  = atoi(argv[1]);
    string file(argv[2]);
    char *p = new char[file_size(file)];

    dfs.Connect(LOCAL_HOST, NS_PORT);
    dfs.Read(id, 0, file_size(file),  p);
	ofstream ofs(file + ".copy");
	ofs.write(p, file_size(file));
	ofs.close();

    system(("diff -s " + file + " " + file + ".copy").c_str());
    dfs.Disconnect();

    return 0;
}
