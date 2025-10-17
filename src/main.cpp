#include <iostream>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <vector>
#include <algorithm>
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
std::mutex db_mutex;
void handleclient(SOCKET clisocket , SentinelDB& db)
{
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
        commandBuffer.erase(
    std::remove_if(commandBuffer.begin(), commandBuffer.end(),
        [](unsigned char c){ return c == 255; }),
    commandBuffer.end()
);
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
            {
                std::lock_guard<std::mutex> lock(db_mutex);
                db.set(key,value);}
            reply = "+OK\r\n";
        }
        else if(cmd=="GET")
        {
            iss>>key;
            //std::cout<<db.get(key)<<"\n";
           {
            std::lock_guard<std::mutex> lock(db_mutex);
             reply = db.get(key) + "\r\n";}
        }
        else if(cmd=="DEL")
        {
            iss>>key;
            {std::lock_guard<std::mutex> lock(db_mutex);
                db.del(key);}
            reply = "+OK\r\n";
        }
        else if(cmd=="SAVE")
        {
            {std::lock_guard<std::mutex> lock(db_mutex);
                    db.save();}
            
            reply = "+Snapshot saved\r\n";
        }
        else if(cmd=="EXIT"){
            reply = "BYE\r\n";
        send(clisocket, reply.c_str(), reply.size(), 0);
        std::cout << "Client requested exit.\n";
            
            break;
        }
        else{
             reply = "-ERR unknown command\r\n";
        } 
        send(clisocket, reply.c_str(), reply.size(), 0);
    }
    
    }
        
  
  closesocket(clisocket);
  std::cout << "🧵 Client thread ending.\n";
}
void autosave(SentinelDB& db , int sleeptime)
{
    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(sleeptime));
        {
        std::lock_guard<std::mutex>lock(db_mutex);
        db.save();
    }
    std::time_t now = std::time(nullptr); 
         std::cout << "[AUTO] Snapshot saved in background.\n"<< std::ctime(&now);
}

}

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
  BOOL opt = TRUE;
setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

  if(bind(sockfd,(sockaddr*)&seraddress,sizeof(seraddress))==SOCKET_ERROR)
  {
    std::cout<<"Binding Failed"<<std::endl;
    closesocket(sockfd);
    WSACleanup();
    return 1;
  }
    std::cout << "Server listening on port 8080..." << std::endl;
    if (listen(sockfd, SOMAXCONN) == SOCKET_ERROR) {
    std::cout << "Listen failed. Error: " << WSAGetLastError() << std::endl;
    closesocket(sockfd);
    WSACleanup();
    return 1;
}

    int clientCounter = 0;
    sockaddr_in clientAddr;
    int clientsize = sizeof(clientAddr);
     std::thread(autosave,std::ref(db),10).detach();
    while(true)
    {
       
    SOCKET clisocket = accept(sockfd,(sockaddr*)&clientAddr,&clientsize);
    if(clisocket==INVALID_SOCKET)
    {
        std::cout << "Accept Failed: " << WSAGetLastError() << std::endl;
        continue;
        

    }
     clientCounter++;
      std::thread(handleclient, clisocket, std::ref(db)).detach();
    }
   
  closesocket(sockfd);
  WSACleanup();

}