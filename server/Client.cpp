#include "thread.h"
#include "socket.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <thread>

using namespace Sync;

void threadRead(Socket& socketReference)
{
    ByteArray data;
    while (true)
    {
        socketReference.Read(data);
        std::cout << data.ToString() << std::endl;
    }
};

void threadWrite(Socket& socketReference)
{
    ByteArray data;
    std::string userInput;
    while (true)
    {
        getline(std::cin, userInput);
        data = ByteArray(userInput);
        socketReference.Write(data);
    }
};

int main(void)
{
	// Welcome the user 
	std::cout << "I am the auction house client" << std::endl;
//    std::cout << "**Enter done to exit**" << std::endl;

    std::string userInput, serverResponse;
    ByteArray data;
    Socket socket("127.0.0.1", 3001);

    //open the socket
    socket.Open();

    Socket& socketReference = socket;

    std::thread th1(threadRead, socket);
    std::thread th2(threadWrite, socket);

    th1.join();
    th2.join();
    return 0;


//	try
//	{
//        // Create our socket
//        Socket socket("127.0.0.1", 3001);
//
//        //open the socket
//        socket.Open();
//
//        while (true)
//        {
//            std::cout << "Please enter a string: " << std::endl;
//            getline(std::cin, userInput);
//
//            //if done, exit the loop to close socket
//            if (userInput == "done")
//                break;
//
//            //populate ByteArray using userInput
//            data = ByteArray(userInput);
//
//            //check number of bytes successfully written
//            if (socket.Write(data) != data.v.size())
//            {
//                std::cout << "Socket has been closed at the server side." << std::endl;
//                break;
//            }
//
//            socket.Read(data);
//
//            //check the result of read
//            if (data.v.size() == 0) {
//                std::cout << "Socket was gracefully closed at the server side." << std::endl;
//                break;
//            }
//
//            else if (data.v.size() < 0) {
//                std::cout << "Read error!" <<std::endl;
//                break;
//            }
//
//            //read looks good, continue for next user input
//            else {
//                serverResponse = data.ToString();
//                std::cout << serverResponse <<std::endl;
//            }
//        }
//
//        socket.Close();
//        return 0;
//    }
//    catch (std::string e)
//    {
//        std::cout << e << std::endl;
//        return 1;
//    }
}
