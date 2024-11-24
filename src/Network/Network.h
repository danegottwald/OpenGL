#pragma once

class Network
{
public:
    // Empty Impl
    Network()
    {}

    // Get Singleton
    static Network& GetInstance();

    // Delete Copy and Assignment
    Network(Network const&) = delete;
    void operator=(Network const&) = delete;

private:
    static Network* s_Instance;

};

