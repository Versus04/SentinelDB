#include <iostream>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

class SentinelDB{
    std::unordered_map<std::string,std::string> mpp;
    std::ofstream logfile;
    public:
        SentinelDB(const std::string& filename = "data.log") {
        logfile.open(filename, std::ios::app);
        load(filename);
    }
    void load(const std::string& filename)
    {
        std::ifstream in(filename);
        std::string cmd,key,value;
        while(in>>cmd)
        {
            if(cmd =="SET")
            {
                in>>key>>value;
                mpp[key]=value;
            }
            else if(cmd=="DEL")
            {
                in>>key;
                mpp.erase(key);
            }
        }
    }
    void set(const std::string& key ,const std::string& value)
    {
        mpp[key]=value;
        logfile<<"SET "<<key<<" "<<value<<"\n";
        logfile.flush();
    }
    void del(const std::string& key)
    {
        mpp.erase(key);
        logfile<<"DEL "<<key<<"\n";
        logfile.flush();
    }
    std::string get(const std::string& key)
    {
        if(mpp.find(key)!=mpp.end())return mpp[key];
        return "(nil)";
    }
    void save()
    {
        std::ofstream snapshot("snapshot.db");
        for(auto it:mpp)
        {
            snapshot<<it.first<<" "<<it.second<<"\n"; 
        }
        snapshot.close();
         std::cout << "Snapshot saved.\n";
    }

};
int main()
{
    SentinelDB db;
   WSADATA wsaData;
   WSAStartup(MAKEWORD(2,2), &wsaData);  // Initialize Winsock library

   int sockfd;
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (sockfd == INVALID_SOCKET) {
    std::cout << "Failed to create socket" << std::endl;
     WSACleanup();
    return 1;
  }
  std::cout << "Socket created successfully" << std::endl;
  sockaddr_in seraddress;
  seraddress.sin_family=AF_INET;
  seraddress.sin_port=htons(8080);
  seraddress.sin_addr.s_addr=INADDR_ANY;
  if(bind(sockfd,(sockaddr*)&seraddress,sizeof(seraddress))==SOCKET_ERROR)
  {
    std::cout<<"Binding Failed"<<std::endl;
    closesocket(sockfd);
    WSACleanup();
    return 1;
  }
    std::cout << "Server listening on port 8080..." << std::endl;
    listen(sockfd, SOMAXCONN);
    sockaddr_in clientAddr;
    int clientsize = sizeof(clientAddr);
    SOCKET clisocket = accept(sockfd,(sockaddr*)&clientAddr,&clientsize);
    if(clisocket==INVALID_SOCKET)
    {
        std::cout<<"Accept Failed"<<std::endl;
        closesocket(sockfd);
        WSACleanup();
        return 1;

    }
    std::cout << "✅ Client connected!" << std::endl;
    char buffer[1024];
    std::string commandBuffer;
    while(true)
    {
        std::cout << "📡 Waiting for client input..." << std::endl;

        int byterec =   recv(clisocket,buffer,sizeof(buffer)-1,0);
        if(byterec<=0)
        {
            std::cout<<"Client Disconnected from the server"<<std::endl;
            break;
        }
        buffer[byterec]='\0';
        commandBuffer += buffer;
        size_t pos;
        while((pos = commandBuffer.find_first_of("\r\n")) != std::string::npos){

            std::string command = commandBuffer.substr(0, pos);
        commandBuffer.erase(0, pos + 1);
        while (!commandBuffer.empty() && (commandBuffer[0] == '\r' || commandBuffer[0] == '\n'))
    commandBuffer.erase(0, 1);

        if (command.empty()){
            

            continue;}
           
           std::cout << "Client Says: " << command << std::endl;

        std::istringstream iss(command);
        std::string cmd, key, value;
        iss >> cmd;
        std::string reply;
        if(cmd=="SET")
        {
            iss>>key;
            iss>>value;
            db.set(key,value);
            reply = "+OK\r\n";
        }
        else if(cmd=="GET")
        {
            iss>>key;
            //std::cout<<db.get(key)<<"\n";
            reply = db.get(key) + "\r\n";
        }
        else if(cmd=="DEL")
        {
            iss>>key;
            db.del(key);
             reply = "+OK\r\n";
        }
        else if(cmd=="SAVE")
        {
            db.save();
            
            reply = "+Snapshot saved\r\n";
        }
        else if(cmd=="EXIT"){
            reply = "BYE\r\n";
        send(clisocket, reply.c_str(), reply.size(), 0);
        std::cout << "Client requested exit.\n";
        goto cleanup; 
            break;
        }
        else{
             reply = "-ERR unknown command\r\n";
        } 
        send(clisocket, reply.c_str(), reply.size(), 0);
    }
    
    }
        
  cleanup:
  closesocket(clisocket);
  closesocket(sockfd);
  WSACleanup();

}