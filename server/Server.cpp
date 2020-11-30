#include "thread.h"
#include "socketserver.h"
#include <stdlib.h>
#include <algorithm>
#include <time.h>
#include <list>
#include <vector>
#include <string>

using namespace Sync;

// data structure for auction items
struct auctionItem
{
    std::string name;
    double startingPrice;
};

struct player
{
    Socket& socketReference;
    std::string name;
    double money;
    std::vector<std::string> bag;
};

// this thread handles auction process
class RoomThread : public Thread
{
private:
    int roomNo;
    int capacity;
    std::vector<auctionItem> roomItems;
    // reference to players in the room
    std::vector<player> players;
    //if the game has started
    bool joinable = true;

public:
    bool IsJoinable()
    {
        return joinable;
    }

    void AddPlayer(player p)
    {
        players.push_back(p);
    }

    //add a destructor to close sockets in this room

    RoomThread(int roomNo, int capacity, std::vector<auctionItem> roomItems)
    {
        this->capacity = capacity;
        this->roomNo = roomNo;
        this->roomItems = roomItems;
    }

    virtual long ThreadMain()
    {
        //if we don't have enough players yet, don't start the game
        while (capacity != players.size())
        {
            sleep(1);
        }

        ByteArray data;
        std::string msg, userInput;

        //records which player is the last bid and the bid amount
        int lastBid;
        double bid;

        //prevent join if has enough players
        this->joinable = false;

        // welcome players
        for (auto player : players)
        {
            msg = "**Welcome " + player.name + "! The game will start now**";
            data = ByteArray(msg);
            player.socketReference.Write(data);
        }

        for (int i = 0; i<roomItems.size(); i++)
        {
            for (int j = 0; j<players.size(); j++)
            {
                msg = "For "+roomItems[i].name+", the starting price is "+std::to_string(roomItems[i].startingPrice);
                data = ByteArray(msg);
                players[j].socketReference.Write(data);

                //if the player doesn't have enough, pass to next player
                if (players[j].money<roomItems[i].startingPrice)
                {
                    msg += "**You don't have enough money to bid. Pass automatically**";
                    data = ByteArray(msg);
                    players[j].socketReference.Write(data);
                    continue;
                }
                else
                {
                    msg = "Would you like to bid or pass?";
                    data = ByteArray(msg);
                    players[j].socketReference.Write(data);
                }

                players[j].socketReference.Read(data);
                userInput = data.ToString();
                if (userInput == "pass") continue;
                else
                {
                    bid = std::stod(userInput);
                    // if the bid is valid
                    if (bid <= players[j].money && bid>=roomItems[i].startingPrice)
                    {
                        //update price and record the player
                        roomItems[i].startingPrice = bid;
                        lastBid = j;
                        msg = "Your bid is " + userInput + ". Waiting for other players to bid...";
                        data = ByteArray(msg);
                        players[j].socketReference.Write(data);
                    }
                    else
                    {
                        msg = "**Your bid is not valid. Pass automatically**";
                        data = ByteArray(msg);
                        players[j].socketReference.Write(data);
                    }
                }
            }

            players[lastBid].money -= roomItems[i].startingPrice;
            players[lastBid].bag.push_back(roomItems[i].name);

            //announce the result
            for (auto player : players)
            {
                msg = "\nCongratulate " + players[lastBid].name +" for getting " + roomItems[i].name + "!\n";
                player.socketReference.Write(msg);
            }
        }

        for (auto player : players)
        {
            msg = "Thank you for playing. You have "+std::to_string(player.money)+" left and you bag has ";
            // print items in the player's bag
            int count = 0;
            for(count; count < player.bag.size(); count++)
                msg += player.bag[count] + " ";
            // if the bag has no item in it
            if (count == 0)
                msg += "nothing in it.";
            player.socketReference.Write(msg);
        }

        return 0;
    }
};

// a global vector storing reference to room threads
std::vector<RoomThread*> roomThreads;

// This thread handles each socket connection
class SocketThread : public Thread
{
private:
    // Reference to the connected socket
    Socket& socketReference;

public:
    SocketThread(Socket& socketReference)
            : socketReference(socketReference)
    {}

    ~SocketThread()
    {}

    Socket& GetSocket()
    {
        return socketReference;
    }

    virtual long ThreadMain()
    {
        std::string name;
        double money;
        int roomNo;
        ByteArray data;

        try
        {
            data = ByteArray("What is your name?");

            // Send msg to player and check number of bytes successfully written
            if (socketReference.Write(data) != data.v.size())
            {
                std::cout << "Socket has been closed at the client side before joining a room." << std::endl;
                return 0;
            }

            // Wait for data
            socketReference.Read(data);

            //check the result of read
            if (data.v.size() == 0)
            {
                std::cout << "Socket has been closed at the client side before joining a room." << std::endl;
                return 0;
            }

            else if (data.v.size() < 0)
            {
                std::cout << "Read error!" <<std::endl;
                return 0;
            }

            //read looks good
            else
            {
                name = data.ToString();
            }

            data = ByteArray("How much do you have?");

            // ask for money
            if (socketReference.Write(data) != data.v.size())
            {
                std::cout << "Socket has been closed at the client side before joining a room." << std::endl;
                return 0;
            }

            // Wait for data
            socketReference.Read(data);

            //check the result of read
            if (data.v.size() == 0)
            {
                std::cout << "Socket has been closed at the client side before joining a room." << std::endl;
                return 0;
            }

            else if (data.v.size() < 0)
            {
                std::cout << "Read error!" <<std::endl;
                return 0;
            }

            //read looks good, convert to money
            else
            {
                money = std::stod(data.ToString());
            }

            // try to join the player into a room
            while (true)
            {
                data = ByteArray("What is the room number you would like to join?");

                // Send msg to player and check number of bytes successfully written
                if (socketReference.Write(data) != data.v.size())
                {
                    std::cout << "Socket has been closed at the client side before joining a room." << std::endl;
                    return 0;
                }

                // Wait for data
                socketReference.Read(data);

                //check the result of read
                if (data.v.size() == 0)
                {
                    std::cout << "Socket has been closed at the client side before joining a room." << std::endl;
                    return 0;
                }

                else if (data.v.size() < 0)
                {
                    std::cout << "Read error!" <<std::endl;
                    return 0;
                }

                //read looks good, convert to integer
                else
                {
                    roomNo = std::stoi(data.ToString());
                    //check if we have a room the player wants to join
                    if (roomNo > roomThreads.size())
                    {
                        data = ByteArray("Room does not exist. Try again.");
                        socketReference.Write(data);
                        continue;
                    }

                    //check if room is still joinable
                    if (!roomThreads[roomNo-1]->IsJoinable())
                    {
                        data = ByteArray("Game has started. Try another room.");
                        socketReference.Write(data);
                        continue;
                    }

                    //put player into the room
                    player p = {socketReference, name, money};
                    roomThreads[roomNo-1]->AddPlayer(p);
                    data = ByteArray("You have been added to the room. Waiting for the game to start...");
                    socketReference.Write(data);
                    break;
                }
            }
        }
        catch (...)
        {
            // We catch the exception, but there is nothing for us to do with it here. Close the thread.
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
                socketThreads.push_back(new SocketThread(socketReference));
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
    std::cout << "I am the auction house server." << std::endl;
    std::cout << "******Enter ""exit"" to terminate the server******" << std::endl;
    
    std::string userInput;
    double userPrice;
    int capacity, roomNo = 0;
    std::vector<auctionItem> roomItems{};

    try
    {
        // try to create our server
        SocketServer server(3001);

        // Need a thread to perform server operations
        ServerThread serverThread(server);

        //ask whether to create new room
        while (true)
        {
            std::cout << "**Type y to create a new room**" << std::endl;
            getline(std::cin, userInput);

            // This will wait for input to shutdown the server
            if (userInput == "exit")
            {
                break;
            }

            if (userInput != "y" && userInput != "yes")
            {
                std::cout << "Invalid input! Try again." << std::endl;
                continue;
            }

            while (true)
            {
                std::cout << "How many players should this room take?" << std::endl;
                if (std::cin >> capacity) {// valid number
                    //ignore new line char for getline
                    std::cin.ignore(1000, '\n');
                    break;
                } else {// not a valid number
                    std::cout << "Invalid Input! Please input an integer." << std::endl;
                    //clear error on cin
                    std::cin.clear();
                    while (std::cin.get() != '\n') ; // empty loop
                }
            }

            //ask item names and prices
            while (true)
            {
                std::cout << "What item would you like to add? (Enter done to finish adding)" << std::endl;
                getline(std::cin, userInput);

                // if done
                if (userInput == "done")
                {
                    //check if we have items to add
                    if (roomItems.empty())
                    {
                        std::cout << "Add at least 1 item to continue..." <<std::endl;
                        continue;
                    }
                    //if we do, break out
                    break;
                }

                // ask price and check input
                while (true)
                {
                    std::cout << "How much should the starting price of " << userInput << " be?" << std::endl;
                    if (std::cin >> userPrice) {// valid number
                        //ignore new line char for getline
                        std::cin.ignore(1000, '\n');
                        break;
                    } else {// not a valid number
                        std::cout << "Invalid Input! Please input a numerical value." << std::endl;
                        //clear error on cin
                        std::cin.clear();
                        while (std::cin.get() != '\n') ; // empty loop
                    }
                }

                //if all checks passed, add this item
                auctionItem userItem = {userInput, userPrice};
                roomItems.push_back(userItem);
            }
            //create a new room thread and pass items
            ++roomNo;
            roomThreads.push_back(new RoomThread(roomNo,capacity,roomItems));

            // clear items to start again
            roomItems.clear();
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
