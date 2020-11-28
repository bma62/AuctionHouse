// Name: Boyi Ma
// ID: 250892829
#include "thread.h"
#include "socket.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <string>

using namespace Sync;

int main(void)
{
	// Welcome the user 
	std::cout << "SE3313 Lab 3 Client" << std::endl;
    std::cout << "**Enter done to exit**" << std::endl;

    std::string userInput, serverResponse;
    ByteArray data;

	try
	{
        // Create our socket
        Socket socket("127.0.0.1", 3001);

        //open the socket
        socket.Open();

        while (true)
        {
            std::cout << "Please enter a string: " << std::endl;
            getline(std::cin, userInput);

            //if done, exit the loop to close socket
            if (userInput == "done")
                break;

            //populate ByteArray using userInput
            data = ByteArray(userInput);

            //check number of bytes successfully written
            if (socket.Write(data) != data.v.size())
            {
                std::cout << "Socket has been closed at the server side." << std::endl;
                break;
            }

            socket.Read(data);

            //check the result of read
            if (data.v.size() == 0) {
                std::cout << "Socket was gracefully closed at the server side." << std::endl;
                break;
            }

            else if (data.v.size() < 0) {
                std::cout << "Read error!" <<std::endl;
                break;
            }

            //read looks good, continue for next user input
            else {
                serverResponse = data.ToString();
                std::cout << serverResponse <<std::endl;
            }
        }

        socket.Close();
        return 0;
    }
    catch (std::string e)
    {
        std::cout << e << std::endl;
        return 1;
    }
}
