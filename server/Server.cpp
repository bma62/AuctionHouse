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
    int startingPrice;
};

// struct for each connecting player
struct player
{
    Socket& socketReference; // which socket this player is connecting from
    std::string name; //player name
    int money;   //player money
    std::vector<std::string> bag; //a vector holding action item names (string type) the player got
};

// this thread handles auction process, 1 thread per auction room created by the server
class RoomThread : public Thread
{
private:
    int roomNo; //an identifier
    int capacity; //how many players this room should take
    std::vector<auctionItem> roomItems; //items to be auctioned in this room

    // store data about players in the room and their information
    std::vector<player> players;
    //indicator for whether the game has started
    bool joinable = true;

    // a function to send messages to all players
    void SendPlayerBroadcast (std::string msg, std::vector<player> players)
    {
        for (auto player : players)
        {
            ByteArray data(msg); //convert string to ByteArray
            player.socketReference.Write(data); //get each player's socket& to send the data to them

            //TODO: add safety check here, if size<0 etc...
        }
    }

    // a function to send messages to one single player
    void SendToPlayer (std::string msg, player p)
    {
        ByteArray data(msg); //convert string to ByteArray
        p.socketReference.Write(data); //get the player's socket& to send the data to it

        //TODO: add safety check here, if size<0 etc...

    }

    // a function to receive input from player and convert to string
    std::string ReceiveFromPlayer (player p)
    {
        ByteArray data;
        p.socketReference.Read(data);
        std::string userInput = data.ToString();
        return userInput;
        //TODO: add safety check
    }

public:
    // get if the room is still joinable (hasn't started)
    bool IsJoinable()
    {
        return joinable;
    }

    // add player to the room's struct
    void AddPlayer(player p)
    {
        players.push_back(p);
    }

    //TODO: add a destructor to close sockets in this room

    // constructor that take roomNo, # players, and items to be auctioned
    RoomThread(int roomNo, int capacity, std::vector<auctionItem> roomItems)
    {
        this->roomNo = roomNo;
        this->capacity = capacity;
        this->roomItems = roomItems;
    }

    // the main loop for RoomThread
    virtual long ThreadMain()
    {
        //if we don't have enough players yet, stay here and don't start the game
        while (players.size() != capacity)
        {
            sleep(1);
        }

        //if we get here, there are enough players. So, prevent future joins
        this->joinable = false;

        ByteArray data;
        std::string msg, userInput; //msg will be sent to players, userInput will be received from players

        //records which player is the last bid and the bid amount
        int lastBid = -1;
        int bid = 0;

        // send welcome message to every player
        for (auto player : players)
        {
            msg = "Welcome " + player.name + "! The game will start now.";
            SendToPlayer(msg, player);
        }

        // for each item, repeat the auction process
        for (int i = 0; i<roomItems.size(); i++)
        {
            // for each item, broadcast starting price to all players
            std::string itemName = roomItems[i].name;
            int itemPrice = roomItems[i].startingPrice;
            msg = "For " + itemName + ", the starting price is " + std::to_string(itemPrice) + ".";
            SendPlayerBroadcast(msg, players);

            //reset this last bid for each item
            lastBid = -1;

            // loop among the players for bidding
            for (int j = 0; j<players.size(); j++)
            {
                // check if we are back at the last bidder's round, if yes, it means everybody has passed
                // and we can break out this item's loop
                if (j == lastBid) break;

                //if the player doesn't have enough to bid, pass to next player
                int playerMoney = players[j].money;
                if (playerMoney < itemPrice)
                {
                    msg = "You don't have enough money to bid - passed automatically.";
                    SendToPlayer(msg, players[j]);

                    // let other players know
                    msg = players[j].name + " passed.";
                    SendPlayerBroadcast(msg, players);
                    continue;
                }

                //if the player has enough to bid, ask if he wants to bid
                msg = "Enter a number to bid or enter pass: ";
                SendToPlayer(msg, players[j]);
                // wait for input
                userInput = ReceiveFromPlayer(players[j]);

                //if player wants to pass, skip rest and start next player's turn
                if (userInput == "pass")
                {
                    // let other players know
                    msg = players[j].name + " passed.";
                    SendPlayerBroadcast(msg, players);
                    continue;
                }

                //if not pass, convert input to int
                bid = std::stoi(userInput);

                // check if the bid is valid
                if (bid <= playerMoney && bid > itemPrice)
                {
                    //update price and record the player of a valid bid
                    itemPrice = bid;
                    lastBid = j;

                    //broadcast the new bid to everybody
                    msg = players[j].name + " bids for " + userInput + "!";
                    SendPlayerBroadcast(msg, players);

                    //let the player know what happens next
                    msg = "Waiting for other players to bid...";
                    SendToPlayer(msg, players[j]);
                }
                else
                {
                    msg = "Your bid is not valid - passed automatically.";
                    SendToPlayer(msg, players[j]);

                    // let other players know
                    msg = players[j].name + " passed.";
                    SendPlayerBroadcast(msg, players);
                }

                // if the for loop has reached the last player, reset it to start from 0 again
                // because we don't want to end this item's loop until everybody has passed after the last bidder's bid
                // so we may need multiple rounds for one same item.
                // decision whether to break out is at the top of this for loop
                if (j == players.size()-1) j=-1;
            }

            // after an item is auctioned, deduct money from the successful bidder and add item to his bag
            players[lastBid].money -= itemPrice;
            players[lastBid].bag.push_back(itemName);

            //broadcast the result to everybody
            msg = "Congratulations to " + players[lastBid].name +" for getting " + itemName + "!";
            SendPlayerBroadcast(msg, players);
        }

        // if we get here, all items have been auctioned
        for (auto player : players)
        {
            //construct end message
            msg = "Thank you for playing. You have "+std::to_string(player.money)+" left and you bag has ";
            // print items in the player's bag
            int count = 0;
            for(count; count < player.bag.size(); count++)
                msg += player.bag[count] + " ";

            // check if the bag has no item in it
            if (count == 0)
                msg += "nothing in it.";

            //send end message to each player
            SendToPlayer(msg, player);
        }

        return 0;
    }
};

// a global vector storing references to room threads
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
        int money;
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
    int userPrice;
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
