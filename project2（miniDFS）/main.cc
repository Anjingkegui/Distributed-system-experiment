#include <iostream>
#include <thread>
#include <fstream>


#include "socket.h"
#include "miniDFS.h"
#include "nameserver.h"
#include "dataserver.h"

using namespace std;

#define NS_PORT 2333
#define LOCAL_HOST "127.0.0.1"

const int ds_port[] = { 6666, 6667, 6668, 6669, 6670, 6671, 6672, 6673,  6674, 6675 };
thread thread_pool[10];

void ns_thread()
{
    nameserver(NS_PORT).running();
}
void ds_thread(int i)
{
    dataserver(LOCAL_HOST, ds_port[i], LOCAL_HOST, NS_PORT).running();
}

int file_size(string file)
{
    struct stat st;
    stat(file.data(), &st);
    return st.st_size;
}
void client(string file = "test.txt")
{
    minidfs dfs;
    char *p = new char[file_size(file)];

    dfs.Connect(LOCAL_HOST, NS_PORT);

    int id = dfs.UploadFile(file);
    cout << file << "id " << id << endl;
    dfs.Read(id, 0, file_size(file),  p);

	ofstream ofs(file + ".copy");
	ofs.write(p, file_size(file));
	ofs.close();
    system(("diff -s " + file + " " + file + ".copy").c_str());
    cout << "client finish. CTRC-C to kill all threads" << endl;

    delete[] p;
    dfs.Disconnect();
}

int main(int argc, char *argv[])
{
    string file("test.txt");
    if (argc > 1)
        file = string(argv[1]);

    thread t1(ns_thread);
    sleep(1);
    for (int i = 0; i < sizeof(ds_port)/sizeof(ds_port[0]);  ++ i)
        thread_pool[i] = thread(ds_thread, i);
    sleep(3);
    thread t3(client, file);
    t3.join();
    t1.join();
    for (int i = 0; i < sizeof(ds_port)/sizeof(ds_port[0]);  ++ i)
        thread_pool[i].join(); 
    
    return 0;
}
