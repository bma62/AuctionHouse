// Name: Boyi Ma
// ID: 250892829
#include "thread.h"
#include "socketserver.h"
#include <stdlib.h>
#include <algorithm>
#include <time.h>
#include <list>
#include <vector>
#include <string>

using namespace Sync;

// This thread handles each socket connection
class SocketThread : public Thread
{
private:
    // Reference to the connected socket
    Socket& socketReference;

    // The data we are receiving
    ByteArray data;

    // Are we terminating?
    bool& terminate;
    
public:
    SocketThread(Socket& socketReference, bool& terminate)
    : socketReference(socketReference), terminate(terminate)
    {}

    ~SocketThread()
    {}

    Socket& GetSocket()
    {
        return socketReference;
    }

    virtual long ThreadMain()
    {
    	std::string userInput;
        // If terminate is ever flagged, we need to gracefully exit
        while(!terminate)
        {
            try
            {
                // Wait for data
                socketReference.Read(data);
                
                //check the result of read
                if (data.v.size() == 0)
                {
                    std::cout << "Socket has been closed at the client side." << std::endl;
                    break;
                }

                else if (data.v.size() < 0)
                {
                    std::cout << "Read error!" <<std::endl;
                    break;
                }

                //read looks good
                else
                {
                    userInput = data.ToString();
                }
                
                // convert every character of input to uppercase
                for (int i=0; i < userInput.length(); i++)
                {
                    userInput[i] = toupper(userInput[i]);
                }

                data = ByteArray(userInput);

                // Send it back and check number of bytes successfully written
                if (socketReference.Write(data) != data.v.size())
                {
                    std::cout << "Socket has been closed at the client side." << std::endl;
                    break;
                }
            }
            catch (...)
            {
                // We catch the exception, but there is nothing for us to do with it here. Close the thread.
            }
        }
        return 0;
    }
};

// This thread handles the server operations
class ServerThread : public Thread
{
private:
    //reference to the server
    SocketServer& server;

    //vector to store connected sockets
    std::vector<SocketThread*> socketThreads;

    bool terminate = false;

public:
    ServerThread(SocketServer& server)
    : server(server)
    {}

    ~ServerThread()
    {
        // Close the client sockets
        for (auto thread : socketThreads)
        {
            try
            {
                // Close the socket
                thread->GetSocket().Close();
            }
            catch (...)
            {
                // If already ended, this will cause an exception
            }
        }

        // Terminate the thread loops
        terminate = true;
    }

    virtual long ThreadMain()
    {
        while(true)
        {
            try
            {
                // Wait for a client socket connection
                Socket* newConnection = new Socket(server.Accept());

                // Pass a reference to this socket to create a new socket thread
                Socket& socketReference = *newConnection;
                socketThreads.push_back(new SocketThread(socketReference, terminate));
            }
            catch (TerminationException termE)
            { //if the server was shutdown
                return termE;
            }
            catch (std::string error)
            {
                std::cout << "[Error] " << error << std::endl;
                return 1;
            }
            catch (std::bad_alloc& ba)
            { //if new failed
                std::cout << "Thread creation failed: " << ba.what() << std::endl;
                return 1;
            }
        }
    }
};

int main(void)
{
    std::cout << "I am a server." << std::endl;
    std::cout << "**Enter ""done"" to terminate the server**" << std::endl;
    
    std::string userInput;
    
    try
    {
        // try to create our server
        SocketServer server(3001);

        // Need a thread to perform server operations
        ServerThread serverThread(server);

        while (true)
        {
            // This will wait for input to shutdown the server
//            FlexWait cinWaiter(1, stdin);
//            cinWaiter.Wait();
            
            getline(std::cin, userInput);

            if (userInput == "done")
            {
                break;
            }
        }

        // Shut down and clean up the server
        server.Shutdown();
        return 0;
    }
    catch (std::string e)
    {
        std::cout << e << std::endl;
        return 1;
    }
    
}
