// Include necessary headers
#include "socketserver.h"
#include "thread.h"
#include <iostream>
#include <vector>
#include <memory>
#include "Semaphore.h"
#include <algorithm>
#include <random>
#include "SharedObject.h"
#include <string>
#include <list>
#include <algorithm>
#include <cstdlib>
#include <ctime>

using namespace Sync;
// set a global value
int count = 0;

// std::atomic<int> count(0);

struct TableData
{
    std::vector<int> dealerHand = std::vector<int>(10);
    std::vector<int> playerHand = std::vector<int>(10);
};

struct MyShared
{
    std::vector<TableData> table = std::vector<TableData>(3);
    // std::vector<int> dealerHand1 = std::vector<int>(10);
    // std::vector<int> playerHand1 = std::vector<int>(10);
    // std::vector<int> dealerHand2 = std::vector<int>(10);
    // std::vector<int> playerHand2 = std::vector<int>(10);
    // std::vector<int> dealerHand3 = std::vector<int>(10);
    // std::vector<int> playerHand3 = std::vector<int>(10);
};

class Dealer
{
private:
    std::vector<int> hand = std::vector<int>(10);

    // Additional functionality for dealer actions
public:
    Dealer() {}

    ~Dealer() {}

    // send card
    void deal(Shared<MyShared> shared)
    {
        // this->hand = shared->dealerHand1;
    }

    // used for calculayte the hand card
    int calculateHandTotal()
    {
        int total = 0;
        for (int card : this->hand)
        {
            total += card;
        }
        return total;
    }
};

class Player
{
private:
    std::vector<int> playerHand = std::vector<int>(10);
    std::vector<int> dealerHand = std::vector<int>(10);
    Socket socket;
    bool hitFlag = false;
    bool explode = false;
    bool isContinue = true;
    int roomId;
    Semaphore write;
    Semaphore read;

    // Additional functionality for dealer actions
public:
    Player(Socket socket, int roomId, Semaphore write, Semaphore read) : socket(socket), roomId(roomId), write(write), read(read)
    {
        this->roomId = roomId;
        this->socket = socket;
        this->write = write;
        this->read = read;
    }

    ~Player() {}

    bool getHitFlag()
    {
        return hitFlag;
    }

    bool getExplode()
    {
        return explode;
    }

    void setExplode(bool explode)
    {
        this->explode = explode;
    }

    std::vector<int> getHand()
    {
        return this->playerHand;
    }

    void askHit()
    {
        ByteArray data("Do you want to hit or stand");
        // message type = 1 hit
        socket.Write(data);

        int receivedData = socket.Read(data);
        std::string received(data.v.begin(), data.v.end());

        if (received == "hit")
        {
            hitFlag = true;
        }
        else if (received == "stand")
        {
            hitFlag = false;
        }
    }

    bool getContinue()
    {

        return this->isContinue;
    }

    void deal(int card)
    {
        this->playerHand.push_back(card);
    }

    void readHand(Shared<MyShared> sharedmemory)
    {

        read.Wait();
        // playerHand = sharedmemory->playerHand1;
        // have not define the read and wirte semaphore
        write.Signal();
    }

    void send(Shared<MyShared> sharedMemory)
    {

        read.Wait();

        // dealerHand = sharedMemory->dealerHand1;
        // playerHand = sharedMemory->playerHand1;

        read.Signal();

        // avoid dead lock
        std::string sendData = "Player: ";
        for (int card : playerHand)
        {
            sendData += std::to_string(card) + " ";
        }
        sendData += "Dealer: ";
        for (int card : dealerHand)
        {
            sendData += std::to_string(card) + " ";
        }

        ByteArray data(sendData);
        socket.Write(data);
    }

    void askContinue()
    {
        ByteArray data("Do you want to continue playing? (yes or no)");
        // message type = 1 hit
        socket.Write(data);

        int receivedData = socket.Read(data);
        std::string received(data.v.begin(), data.v.end());

        if (received == "yes")
        {
            isContinue = true;
        }
        else if (received == "no")
        {
            isContinue = false;
        }
    }

    void sendWinner(std::string string)
    {

        socket.Write(string);
    }

    void closeConneton()
    {

        socket.Close();
    }

    // calculate the all hand card
    int calculateHandTotal()
    {
        int total = 0;
        for (int card : this->playerHand)
        {
            total += card;
        }
        return total;
    }
};

class Spectator
{
private:
    std::vector<int> playerHand = std::vector<int>(10);
    std::vector<int> dealerHand = std::vector<int>(10);
    Socket socket;
    int roomId;
    Semaphore read;
    // Additional functionality for dealer actions
public:
    Spectator(Socket socket, Semaphore read) : socket(socket), read(read)
    {
        this->socket = socket;
        this->read = read;
    }

    ~Spectator() {}

    Socket getSocket()
    {
        return this->socket;
    }

    int getRoomId()
    {
        return this->roomId;
    }

    void setSemaphore(Semaphore read)
    {
        this->read = read;
    }

    void send(Shared<MyShared> sharedMemory)
    {

        read.Wait();

        // dealerHand = sharedMemory->dealerHand1;
        // playerHand = sharedMemory->playerHand1;

        read.Signal();
        // avoid dead lock
        std::string sendData = "Player: ";
        for (int card : playerHand)
        {
            sendData += std::to_string(card) + " ";
        }
        sendData += "Dealer: ";
        for (int card : dealerHand)
        {
            sendData += std::to_string(card) + " ";
        }

        ByteArray data(sendData);
        socket.Write(data);
    }

    void setRoomId(int roomId)
    {
        this->roomId = roomId;
        this->read = Semaphore("read" + roomId);
    }

    void askRoom()
    {
        // since when the room achieve 3, then it will assign the new comeer be the spectator
        ByteArray data("which rooom do you want to join in 1 or 2 or 3");
        socket.Write(data);
    }

    void sendWinner(std::string string)
    {
        socket.Write(string);
    }
};

// Define a simple GameRoom class inheriting from Thread
class GameRoom : public Thread
{
public:
    // main field
    int roomId;
    Dealer *gameDealer = new Dealer();
    Player *gamePlayer;
    bool continueFlag;
    std::vector<Spectator *> Spectatorlist;
    // define index = 0 for dealer index = 1 for player
    std::vector<std::vector<int>> deck;

    // create semaphore for read and write
    Semaphore write;
    Semaphore read;
    //

    GameRoom(int roomId, Player *player, Semaphore write, Semaphore read) : Thread(), gamePlayer(player), roomId(roomId), write(write), read(read)
    {
        this->gamePlayer = player;
        initializeDeck();
        this->roomId = roomId;
        this->write = write;
        this->read = read;
    }

    ~GameRoom() {}

    void addSpec(Spectator *newSpec)
    {
        Spectatorlist.push_back(newSpec);
    }

    // Initializes and shuffles the deck
    void initializeDeck()
    {
        deck.clear();
        deck.resize(4);
        for (int i = 0; i < 4; ++i)
        { // For each suit
            for (int j = 2; j <= 11; ++j)
            { // Add cards 2-10 and Ace as 11
                deck[i].push_back(j);
            }

            deck[i].push_back(10); // Queen
            deck[i].push_back(10); // King
            deck[i].push_back(10); // Jack
        }
        std::shuffle(deck.begin(), deck.end(), std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count()));
    }

    virtual long ThreadMain(void) override
    {
        // Game logic goes here. For now, just a placeholder.
        // Simulate game room activity
        std::cout << "2:::" << std::endl;
        try {
    Shared<MyShared> shared("sharedMemory");
    std::cout << "Accessed shared memory successfully." << std::endl;
} catch (const std::string& e) {
    std::cerr << "Exception caught: " << e << std::endl;
} catch (const std::exception& e) {
    std::cerr << "Standard exception caught: " << e.what() << std::endl;
} catch (...) {
    std::cerr << "Unknown exception caught" << std::endl;
}

        // try
        // {
        //     Shared<MyShared> shared("sharedMemory1");

        //     std::cout << "2" << std::endl;
        //     // used for a random generate a int num
        //     // srand(time(NULL));

        //     try
        //     {
        //         // initiali case
        //         continueFlag = true;
        //         while (continueFlag == true)
        //         {
        //             std::cout << "Starting a new game..." << std::endl;
        //             // initialize two card to the shared memory
        //             gamePlayer->deal(deck[0][5]);
        //             std::cout << "1" << std::endl;

        //             // shared->playerHand1 = this->gamePlayer->getHand();
        //             // std::cout << "1" << std::endl;
        //             // shared->playerHand1.push_back(deck[rand() % 4][rand() % 13]);
        //             // std::cout << "1" << std::endl;
        //             // shared->dealerHand1.push_back(deck[rand() % 4][rand() % 13]);
        //             // std::cout << "1" << std::endl;
        //             // shared->dealerHand1.push_back(deck[rand() % 4][rand() % 13]);
        //             // std::cout << "1" << std::endl;

        //             for (auto *spectator : Spectatorlist)
        //             {
        //                 spectator->send(shared);
        //             }

        //             gamePlayer->readHand(shared);

        //             // player hit the card
        //             while (true)
        //             {

        //                 gamePlayer->askHit();
        //                 if (gamePlayer->getHitFlag())
        //                 {
        //                     // shared->playerHand1.push_back(deck[rand() % 4][rand() % 13]);
        //                 }
        //                 else if (gamePlayer->getHitFlag() == false)
        //                 {
        //                     break;
        //                 };

        //                 if (gamePlayer->calculateHandTotal() > 21)
        //                 {
        //                     gamePlayer->setExplode(true);
        //                     break;
        //                 };

        //                 for (auto *spectator : Spectatorlist)
        //                 {
        //                     spectator->send(shared);
        //                 }
        //             }

        //             // dealer begin hit the card

        //             while ((!gamePlayer->getExplode() && gameDealer->calculateHandTotal() < 17) || (gameDealer->calculateHandTotal() < 21))
        //             {
        //                 // if dealer satisfy the condition then add card to shared memory
        //                 // shared->dealerHand1.push_back(deck[rand() % 4][rand() % 13]);
        //                 gameDealer->deal(shared);
        //                 // let spector get information
        //                 for (auto *spectator : Spectatorlist)
        //                 {
        //                     spectator->send(shared);
        //                 }
        //                 // since the player has finished his round and act as a spectator
        //                 gamePlayer->send(shared);
        //             }

        //             // if player exploded
        //             if (gamePlayer->getExplode())
        //             {
        //                 // dealer wins, notify player and all spectators
        //                 gamePlayer->sendWinner("Dealer wins!!");

        //                 for (auto *spectator : Spectatorlist)
        //                 {
        //                     spectator->sendWinner("Dealer wins!!");
        //                 }
        //             }

        //             // if dealer exploded
        //             if (gameDealer->calculateHandTotal() > 21)
        //             {
        //                 // Player wins, notify player and all spectators
        //                 gamePlayer->sendWinner("Player wins!!");
        //                 for (auto *spectator : Spectatorlist)
        //                 {
        //                     spectator->sendWinner("Player wins!!");
        //                 }
        //             }

        //             // compare score
        //             if (gameDealer->calculateHandTotal() > gamePlayer->calculateHandTotal())
        //             {
        //                 // dealer wins, notify player and all spectators
        //                 gamePlayer->sendWinner("Dealer wins!!");

        //                 for (auto *spectator : Spectatorlist)
        //                 {
        //                     spectator->sendWinner("Dealer wins!!");
        //                 }
        //             }
        //             else
        //             {
        //                 // Player wins, notify player and all spectators
        //                 gamePlayer->sendWinner("Player wins!!");
        //                 for (auto *spectator : Spectatorlist)
        //                 {
        //                     spectator->sendWinner("Player wins!!");
        //                 }
        //             }

        //             gamePlayer->askContinue();
        //             if (gamePlayer->getContinue())
        //             {
        //                 // nothing change
        //             }
        //             else
        //             {
        //                 // let the connetion close
        //                 gamePlayer->closeConneton();
        //                 // then set the first spectator as the player
        //                 // assign the member to here
        //                 gamePlayer = new Player(Spectatorlist[0]->getSocket(), Spectatorlist[0]->getRoomId(), this->write, this->read);
        //                 // reset the first place of vector list
        //                 // Removing the first element
        //                 Spectatorlist.erase(Spectatorlist.begin());
        //             }

        //             if (gamePlayer == nullptr && Spectatorlist.size() == 0)
        //             {

        //                 // terminate the thread if there is no player and the list is empty
        //                 // std::cout << "Thread has finished execution.\n";
        //                 // return 0;  then terminat
        //                 continueFlag = false;
        //             }
        //             else
        //             {
        //                 continueFlag == true;
        //             }
        //         }

        //         std::this_thread::sleep_for(std::chrono::seconds(10));
        //         std::cout << "Game room ending...\n";
        //         return 0;
        //     }
        //     catch (std::string &e)
        //     {

        //         // system output
        //         std::cout << "cannot create server" << std::endl;
        //     }
        // }
        // catch (std::string &e)
        // {
        //     // system output
        //     std::cout << e << std::endl;
        // }
    };
};

int main()
{
    SocketServer server(12345); // Listen on port 12345
    std::vector<GameRoom *> gameRooms;

    std::cout << "Server started. Listening for connections...\n";

    // Accept connections and create game rooms
    try
    {
        while (true)
        {
            Socket newConnection = server.Accept();
            count++;
            std::cout << "Accepted new connection. Starting a game room...\n";
            Shared<MyShared> sharedMemory("sharedMemory", true);
            if (count < 3)
            {
                Semaphore write = Semaphore("write" + count, 1, true);
                Semaphore read = Semaphore("read" + count, 0, true);
                GameRoom *game = new GameRoom(count, new Player(newConnection, count, write, read), write, read);
                // Start a new game room for each connection
                gameRooms.push_back(game);
            }
            else
            {

                // send message to client tell him the room is room full
                // which room you want to join

                // wait response
                //  if (out of range ---- retry) inside the range put to the spectator list

                // send current situation of current rooom
                Semaphore read = Semaphore("read" + count);
                Spectator *newSpec = new Spectator(newConnection, read);
                newSpec->askRoom();
                ByteArray response;
                newConnection.Read(response);
                std::string receive(response.v.begin(), response.v.end());
                if (receive.length() == 0)
                {
                    break;
                }
                read = Semaphore("read" + receive);
                if (receive == "1")
                {
                    newSpec->setSemaphore(read);
                    newSpec->setRoomId(1);
                    gameRooms[0]->addSpec(newSpec);
                }
                else if (receive == "2")
                {
                    newSpec->setSemaphore(read);
                    newSpec->setRoomId(2);
                    gameRooms[1]->addSpec(newSpec);
                }
                else if (receive == "3")
                {
                    // in room 3
                    newSpec->setSemaphore(read);
                    newSpec->setRoomId(3);
                    gameRooms[2]->addSpec(newSpec);
                }
                else
                {
                    break;
                }
            }
        }
    }
    catch (const std::string &e)
    {
        std::cerr << "Server exception: " << e << std::endl;
        server.Shutdown();
    }

    return 0;
}