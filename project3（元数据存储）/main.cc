#include <iostream>
#include <stdio.h>
#include <thread>
#include <fstream>

#include <sys/types.h>
#include <signal.h>

#include "socket.h"
#include "miniDFS.h"
#include "nameserver.h"
#include "dataserver.h"

using namespace std;

#define NS_PORT 2333
#define LOCAL_HOST "127.0.0.1"

const int ds_port[] = { 6666, 6667, 6668, 6669, 6670, 6671, 6672, 6673, 6674, 6675 };
thread ds_thread_pool[10];

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

void client()
{
    minidfs dfs;
    dfs.Connect(LOCAL_HOST, NS_PORT);

    printf("welcome!\n>");

    char cmd[256]="";
    //while(EOF != scanf("%s", cmd))
    string scmd = cmd;
    while (getline(cin, scmd))
    {
        if (scmd == string("bye") || scmd == "")
            break;
        // process the cmd
        strcpy(cmd, scmd.c_str());
    

        string subcmd;
        

        int cmdpos=scmd.find_first_of(' ');
        if(cmdpos != string::npos)
            subcmd = scmd.substr(0, cmdpos);
        else
            subcmd = scmd;

        //if(subcmd.c_str() == "mkdir" || subcmd.c_str() == "create")
        if(subcmd == "touch")
        {
            system(cmd);

            int namepos = scmd.find_last_of(' ');
            if(namepos != string::npos)
            {
                string filename = scmd.substr(namepos+1, scmd.length()-namepos-1);

                // get stat to file and upload to server
                string statFile = filename+".statdata";
                string tmpcmd = "stat "+filename+" >"+statFile;
                system(tmpcmd.data());

                dfs.UploadFile(statFile);

                // delete
                remove(statFile.c_str());
            }
        }

        else if(subcmd == "rm")
        {
            system(cmd);

            int namepos = scmd.find_last_of(' ');
            if(namepos != string::npos)
            {
                string filename = scmd.substr(namepos+1, scmd.length()-namepos-1);

                // delete the file on server
                string statFile = filename+".statdata";
                dfs.DeleteFile(statFile);
            }
        }

        else if(subcmd == "stat")
        {
            system(cmd);

            int namepos = scmd.find_last_of(' ');
            if(namepos != string::npos)
            {
                string filename = scmd.substr(namepos+1, scmd.length()-namepos-1);

                // get the stat from the server
                string statFile = filename+".statdata";
                //int ReadFile(std::string file, char **buff);
                char *buff = NULL;
                dfs.ReadFile(statFile, &buff);
                printf("%s\n", buff);

                if(buff)
                    delete [] buff;

            }
        }

        else if(subcmd == "cluster")
        {
            // get all cluster info from the server
            if(dfs.GetClusterInfo())
            {
                // clusterData.txt
                FILE *tmpf;  
                int c;  
              
                tmpf = fopen( "clusterData.txt", "r");  
              
                while ( (c = fgetc(tmpf)) != EOF){  
                    printf ("%c", c);  
                }  
              
                fclose(tmpf);  

                // delete
                remove((char *)"clusterData.txt");
            }
        }

        else
        {
            //printf("meta doesn't support `%s`\n", cmd);
            system(cmd);
        }

        printf(">");
    }
    dfs.Disconnect();
}

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    
    thread t_ns(ns_thread);
    sleep(1);
    for (int i = 0; i < sizeof(ds_port)/sizeof(ds_port[0]);  ++i)
        ds_thread_pool[i] = thread(ds_thread, i);
    sleep(3);
    thread t_cl(client);

    t_cl.join();
    kill(0, SIGINT);
    
    return 0;
}
