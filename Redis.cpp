/**
 * It's a simple TDD for a Redis Server 
*/

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <unistd.h>
#include <thread>
#include <mutex>
using namespace std;


enum class ResponseState{
    ERROR = -1,
    OK,
    NONE
};


class StorageManager{
    std::mutex storageMutex;
    static StorageManager* m_storageMgr;
    StorageManager(){};
    map<string, string> storageMap;

    public:
    
    static StorageManager* getInstance(){
        if(!m_storageMgr){
            m_storageMgr = new StorageManager();
        }
        
        return m_storageMgr;
    }

    void set(string key, string value, ResponseState& state){
        storageMutex.lock();
        storageMap[key] = value;
        state = ResponseState::OK;
        storageMutex.unlock();
    }

    string get(string key, ResponseState& state){
        if(storageMap.find(key) == storageMap.end()){
            state = ResponseState::ERROR;
            return "-1";
        }
        state = ResponseState::OK;
        return storageMap[key];
    }

};

StorageManager* StorageManager::m_storageMgr = nullptr;

class CommandHandler{
    public:

    void HandleCommands(vector<string> argList, string& result, ResponseState& state){
        if(argList.size()==0){
            state = ResponseState::ERROR;
            return; 
        }
        if(argList[0]=="set"){
            StorageManager::getInstance()->set(argList[1], argList[2], state);
            result = "OK";
        }
        else if(argList[0]=="get"){
            result = StorageManager::getInstance()->get(argList[1], state);
        }
        else if(argList[0]=="echo"){
            result = argList[1];
            state = ResponseState::OK;
        }
        else if(argList[0]=="ping"){
            result = "pong";
            state = ResponseState::OK;
        }
        else{
            state = ResponseState::ERROR;
        }
    }

};

class DataHandler{
    public : 

    void SerializeData(string& input, ResponseState state){
        string respData = "";
        switch(state){
            case ResponseState::OK :
            {
                respData+= '+';
                break;
            }
            case ResponseState::ERROR :
            {
                respData+= '-';
                break;
            }
            default :
            {
                respData+= '0';
                break;
            }
        }
        respData+= input + "\r\n";
        input = respData;
    }

    void DeserializeData(vector<string>& argList, 
                        string inputs){
        int noOfBulkString = inputs[2];
        int ind = 4;
        while(ind < inputs.length()){
            switch (inputs[ind]){
                case '$':
                    {
                        ind++;
                        int length = 0;
                        while(ind < inputs.length() && inputs[ind]!='\r'){
                            length = length*10 + inputs[ind]-'0';
                            ind++;
                        }
                        ind+=2;
                        string value = "";
                        for(int j = 0; j<length; j++){
                            value += inputs[ind];
                            ind++;
                        }
                        ind+=2;
                        argList.push_back(value);
                        break;
                    }
                default:
                    ind++;
                    break;

            }
        }
    }
};

class RedisController{
    DataHandler* dataHandler;
    CommandHandler* cmdHandler;
    string results;

    void Handle(string inputs, ResponseState& state){
        vector<string> argList;
        dataHandler->DeserializeData(argList, inputs);
        cmdHandler->HandleCommands(argList, results, state);
    }
    public:

    RedisController(){
        dataHandler = new DataHandler();
        cmdHandler = new CommandHandler();
    }

    string perFormOperation(string input){
        ResponseState state = ResponseState::NONE;
        Handle(input, state);
        dataHandler->SerializeData(results, state);
        return results;
    }
};


class Test{
    RedisController* server;
    public:

    Test(){
        server = new RedisController();
    }

    void test001(){
        RedisController redis;
        cout << "SERVER Response " << server->perFormOperation("*2\r\n$4\r\necho\r\n$11\r\nhello world\r\n");
        cout << "SERVER Response " << server->perFormOperation("*2\r\n$3\r\nget\r\n$1\r\n2\r\n");
        cout << "SERVER Response " << server->perFormOperation("*1\r\n$4\r\nping\r\n");
        cout << "SERVER Response " << server->perFormOperation("*2\r\n$3\r\nset\r\n$2\r\n20\r\n$3\r\n200");
        cout << "SERVER Response " << server->perFormOperation("*2\r\n$3\r\nget\r\n$2\r\n20\r\n");
    }

};


int main(){
    Test tester;
    tester.test001();
}