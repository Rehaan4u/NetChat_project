#ifndef CLIENT_H
#define CLIENT_H

#include<string>
#include<cstring>

class Client {
    public:
        int id;
        char name[50];

        Client(){
            id = 0;
            std::memset(name,0,sizeof(name));
        }

        Client(int id,char *name){
            this->id = id;
            std::strcpy(this->name,name);
        }

        void serialize(char *buffer) const {
            std::memcpy(buffer,&id,sizeof(id));
            std::memcpy(buffer + sizeof(id),name,sizeof(name));
        }

        void deserialize(const char* buffer){
            std::memcpy(&id,buffer,sizeof(id));
            std::memcpy(name,buffer + sizeof(id),sizeof(name));
        }

        static constexpr size_t serializedSize(){
            return sizeof(id) + sizeof(name);
        }
};

#endif