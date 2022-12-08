//Personal Project by Luka Brown
//The Deep Below
//Mining Game
//In development, should work on both windows and linux
//Feel free to compile and playtest!


/* Gameplay:

Explore an (almost) endless mass of blocks and dig your way around!
The game starts with a grid output to a console with 'P' for player in the
middle. Use WASD to move. Go towards * if you see them. Press q to exit game.
Have fun!

Implemented features:
  Create and print grid
  Track player items
  Ore/Artifacts found randomly in dirt
  Sometimes hides ore from sight
  Movement
  Shop system
    can sell ore
    can buy artifacts
    can trade artifacts for upgrades
  Upgrades (2/5)
    Extra damage
    Extra sight
  End Score System

Agenda, in particular order:
  Encrypted Save System
  Upgrades (3/5)
    mining width
    mining depth
    clarity (sees all special blocks all the time)
  Rogue Enemy Miners to fight
  Minibosses to fight
  Boss to fight
  Main Menu
*/

#include <vector>
#include <iostream>
#include <random>
#include <ctime>
#include <cstdlib>
#include <map>
#include <string>
#include <fstream>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
using namespace std;

//enemy AI class
struct Rogue {
  int damage, coins, ore, artifacts, health, x, y, direction;
};

//function prototypes
static void GenerateMap();
static void PrintMap();
static int  GameLoop();
static void Move(int x);
static void CollectItem(int y, int x);
static void CallShop();
static void Init();
static void SellOre();
static void BuyArtifacts();
static void Trade();
static void Upgrade(int x);
static void GameReport();
static void Intro();
static void InitMiner(Rogue &miner, int y, int x);
static void MoveMiners(vector<Rogue>&);
static void MoveMiner(Rogue &miner);
static bool ProcessBlock(Rogue &miner, int y, int x);
static void SaveGrid();

//Constants
static const int MAP_UPPER = 2000; //2000x2000 grid, 4 million blocks
//block types
#define PLAYER   0
#define DIRT     1
#define MINED    2 //mined dirt
#define SHOP     3
#define ARTIFACT 4
#define ORE      5
#define MINER    6

//global vars
static bool game; //game on/off
static int upgrades[10]; //support for 10 upgrades, only 5 planned
static map<string, int> player; //dictionary of player items, defined in Init()
static vector<vector<int>> grid(MAP_UPPER, vector<int> (MAP_UPPER)); //map grid

int main() {
  Init(); //creates map and prints
  Intro(); //prints opening statement

  int action, x, y;
  vector<Rogue> MinerList;

  for (int i = 0; i < 30; i++) {
    y = rand() % 1500;
    if (y < 500) 
      y += 500;
    x = rand() % 1500;
    if (x < 500) 
      x += 500;

    Rogue miner;
    InitMiner(miner, y, x);
    MinerList.push_back(miner);
  }

  while(game) {
    #ifdef _WIN32
    Sleep(200); //only windows can wait between 0 and 1 seconds
    #endif

    action = GameLoop();

    switch (action) {
      case 0:
        game = false;
        break;
      case 1:
      case 2:
      case 3:
      case 4:
        Move(action);
        break;
      case 5:
        SaveGrid();
        break;
    } //end switch
    
    MoveMiners(MinerList);
    PrintMap();
  }// end game while
  
  SaveGrid();
  GameReport();
  return 0;
}
///////////////////////////////////////////////////////////////////////////////

//creates 2d vector of blocks
static void GenerateMap() {
  int y, x;
  int random;
  srand((unsigned int)time(NULL)); //seeds random

  for (y = 0; y < MAP_UPPER; y++) {
    for (x = 0; x < MAP_UPPER; x++) {
      random = rand() % 100;
      switch(random) { //1% chance each case
        case 0:
          random = rand() % 3; //33% ore 67% artifact
          if (random == 0)
            grid[y][x] = ORE;
          else
            grid[y][x] = ARTIFACT;
          break;
        case 1:
          random = rand() % 4; //25% shop 75% dirt
          if (random == 0)
            grid[y][x] = SHOP;
          else
            grid[y][x] = DIRT;
          break;
        case 2:
          grid[y][x] = ORE;
          break;
        default:
          grid[y][x] = DIRT;
          break;
      } //end switch
    } //end for x
  } //end for y
  grid[player["Y"]][player["X"]] = PLAYER; //sets player position
  return;
}
///////////////////////////////////////////////////////////////////////////////

//prints blocks around the player
static void PrintMap() {
  cout << "\n\n\n\n\n\n";
  int y, x, a;

  for (y = player["Y"]-player["sight"]; y < player["Y"]+player["sight"]+1; y++) {
    if (y > MAP_UPPER-1)
      y = MAP_UPPER-1;
    else if (y < 0)
      y = 0;

    for (x = player["X"]-player["sight"]; x < player["X"]+player["sight"]+1; x++) {
      if (x > MAP_UPPER-1)
        x = MAP_UPPER-1;
      else if (x < 0)
        x = 0;

      switch(grid[y][x]) {
        case PLAYER:
          cout << "P ";
          break;
        case DIRT:
          cout << "# "; //□_
          break;
        case MINED:
          cout << "  ";
          break;
        case ARTIFACT: //ore and artifact have chance of not showing up
        case ORE:
          a = rand() % 8;
          if (a == 0) //12.5% chance of showing up in sight
            cout << "* ";
          else
            cout << "# ";
          break;
        case SHOP:
        case MINER:
          cout << "* ";
          break;
      } //end switch
    } //end for x
    cout << '\n';
  } //end for y

  cout << "Ore: " << player["ore"] << " Artifacts: " << player["artifacts"];
  cout << " Coins: " << player["coins"] << '\n';
  return;
}
///////////////////////////////////////////////////////////////////////////////

//catches player movement and sends back
static int GameLoop() {
  char input;
  fflush(stdin);
  input = getchar();
  switch (input) {
    case 'q':
    case '0':
    case 'Q':
      return 0;
    case 'w':
    case 'W':
      return 1;
    case 'a':
    case 'A':
      return 2;
    case 's':
    case 'S':
      return 3;
    case 'd':
    case 'D':
      return 4;
    case 'O':
    case 'o':
      return 5;
    default:
      return -1;
  } //end switch
}
///////////////////////////////////////////////////////////////////////////////

//adjusts players position on map based on keypress
static void Move(int x) {
  switch (x) {
    case 1: //w, up
      if (player["Y"] > player["sight"]) {
        CollectItem(player["Y"]-1,player["X"]);
        grid[player["Y"]-1][player["X"]] = PLAYER;
        grid[player["Y"]][player["X"]] = MINED;
        player["Y"]--;
      }
      break;
    case 2: //a, left
      if (player["X"] > player["sight"]) {
        CollectItem(player["Y"],player["X"]-1);
        grid[player["Y"]][player["X"]-1] = PLAYER;
        grid[player["Y"]][player["X"]] = MINED;
        player["X"]--;
      }
      break;
    case 3: //s, down
      if (player["Y"] < MAP_UPPER-player["sight"]-1) {
        CollectItem(player["Y"]+1,player["X"]);
        grid[player["Y"]+1][player["X"]] = PLAYER;
        grid[player["Y"]][player["X"]] = MINED;
        player["Y"]++;
      }
      break;
    case 4: //d, right
      if (player["X"] < MAP_UPPER-player["sight"]-1) {
        CollectItem(player["Y"],player["X"]+1);
        grid[player["Y"]][player["X"]+1] = PLAYER;
        grid[player["Y"]][player["X"]] = MINED;
        player["X"]++;
      }
      break;
  }
}
///////////////////////////////////////////////////////////////////////////////

//processes block player stepped on
static void CollectItem(int y, int x) {
  if (grid[y][x] == DIRT) {

    int z = rand() % 100;
    if (z == 0 || z == 1) { //1% chance artifact, 1% chance ore
      if (z == 0) {
        cout << "\nWhile digging, you found an ancient artifact!" << '\n';
        player["artifacts"]++;
      } else {
        cout << "\nWhile digging, you found a rare ore!" << '\n';
        player["ore"]++;
      }
      #ifdef _WIN32
      Sleep(1300);
      #else
      sleep(1);
      #endif
    } else { //98% chance dirt
      player["dirt"]++;
    }

  } else if (grid[y][x] == SHOP) {
    CallShop();

  } else if (grid[y][x] == ARTIFACT) {
    cout << "\nWhile digging, you found an ancient artifact!" << '\n';
    #ifdef _WIN32
    Sleep(1000);
    #else
    sleep(1);
    #endif
    player["artifacts"]++;

  } else if (grid[y][x] == ORE) {
    cout << "\nWhile digging, you found a rare ore!" << '\n';
    #ifdef _WIN32
    Sleep(1000);
    #else
    sleep(1);
    #endif
    player["ore"]++;
  }
}
///////////////////////////////////////////////////////////////////////////////

//calls shop when player steps into a shop
void CallShop() {
  bool finish = false;
  bool asked = false;
  char x;

  cin.clear(); //ensures player actually gets asked the questions
  cout << "You come across a small opening and see that there's a store inside.\n";
  cout << "Would you like to shop? I buy Ore! Enter Y if yes!\n";
  cin >> x;

  if (!(x == 'y' || x == 'Y')) {
    cout << "This could be goodbye forever. Enter Y if you want to shop.\n";
    cin >> x;
  }

  if (x == 'y' || x == 'Y') {
    while (!finish) {
      cout << "\n\nYou take inventory: Ore: " << player["ore"] << " Artifacts: ";
      cout << player["artifacts"] << " Coins " << player["coins"] << '\n';
      cout << "The shop owner poins to a sign that reads:\n";
      cout << "1. Sell ore\n2. Buy artifacts\n";
      cout << "3. Ask for any special offers\n4. Leave\n\nWhat would you like to do?\n";
      cin >> x;
      if (x == '1' || x == '2' || x == '3' || x == '4') {
        if (x == '1')
          SellOre();
        else if (x == '2')
          BuyArtifacts();
        else if (x == '3') {
          if (!asked) { //can only see upgrade offer once
            Trade();
            asked = true;
          }
          else
            cout << "We've already discussed that.\n\n";
        } else
          finish = true;
      } else {
        if (x == 'q')
          finish = true;
        else
          cout << "Sorry, I didn't get that. Which number did you want?\n\n";
      }
    } //end while
  } //end if x == y

  cout << "Alright, see you later.\n";
  return;
}
///////////////////////////////////////////////////////////////////////////////

//initializes globals and calls GenMap
static void Init() {
  //set globals
  player["X"] = player["Y"] = MAP_UPPER/2;//player starting in middle of the map
  player["sight"] = 4; //sees 4 blocks in any direction
  player["damage"] = 10;
  player["ore"] = player["artifacts"] = player["dirt"] = player["coins"] = 0;
  game = true;
  for (int i = 0; i < 10; i++) {
    upgrades[i] = 0;
  }

  //make and set map
  GenerateMap();
}
///////////////////////////////////////////////////////////////////////////////

//shop helper if you want to sell ore
static void SellOre() {
  bool finish = false;
  char x;

  if (player["ore"] == 0)
    cout << "Oh... it doesn't look like you're carrying any ore to sell.\n";
  else {
    while (!finish && player["ore"] > 0) {
      cout << "Would you like to sell any ore? 5 coins per piece! Enter Y if yes.\n";
      cin >> x;
      if (x == 'y' || x == 'Y') {
        cout << "How much?\n";
        int z;
        cin >> z;
        if (z > player["ore"]) {
          cout << "You don't have enough ore! You only have " << player["ore"] << " ore\n\n";
        } else {
          player["ore"] -= z;
          player["coins"] += z*5;
          cout << "Pleasure doing business with you!\n\n";
        }
      } else {
        cout << "Alright, no ore.\n\n";
        finish = true;
      }
    } //end while
  } //end else
}
///////////////////////////////////////////////////////////////////////////////

//shop helper if you want to buy artifacts
static void BuyArtifacts() {
  bool finish = false;
  bool first = true;
  char x;
  int z;

  while (!finish && (player["coins"] >= 30 || first)) {
    if (first) first = false;
    cout << "Price is 30 coins per artifact. Do you want to buy any? Enter Y if yes.\n";
    cin >> x;
    if (x == 'y' || x == 'Y') {
      cout << "How much?\n";
      cin >> z;
      if (z*30 > player["coins"]) {
        cout << "You don't have enough coins! You only have " << player["coins"] << " coins\n\n";
      } else {
        player["coins"] -= z*30;
        player["artifacts"] += z;
        cout << "Pleasure doing business with you!\n\n";
      }
    } else {
      cout << "Okay.\n\n";
      finish = true;
    } //end else
  } //end while
}
///////////////////////////////////////////////////////////////////////////////

//shop helper if you want to trade for an upgrade
static void Trade() {
  char x;

  int z = 4; //z is finished upgrades. max 10
  int cost = rand() % 50;
  if (cost < 20) { cost += 12; }

  cout << "\nOkay, I only have one fine deal for you.\n";
  cout << "If you have " << cost << " ancient artifacts then I may consider selling...\n";
  cout << "The only item that would help you is a magnificent Upgrade!\n\n";
  int random = rand() % z; //0-3
  if (upgrades[random] > 3) {
    cout << "Oh... It looks like you already have the upgrade I was going to offer.\n";
    cout << "Looks like I have nothing special, sorry!\n";
  } else {
    switch(random) {
      case 0:
        cout << "This enhancement will augment your weapon to be far superior.\n";
        break;
      case 1:
        cout << "This enhancement will give you extra sight in the mines.\n";
        break;
      case 2:
        cout << "This enhancement will allow you to swing wider.\n";
        break;
      case 3:
        cout << "This enhancement will allow you to dig deeper.\n";
        break;
    } //end switch

    cout << "Do you want to buy it for " << cost << " artifacts? Enter Y if yes.\n";
    cin >> x;
    if (x == 'y' || x == 'Y') {
      if (player["artifacts"] >= cost) {
        player["artifacts"] -= cost;
        Upgrade(random);
        cout << "Pleasure doing business with you!\n\n";
      } else {
        cout << "Looks like you don't have enough artifacts for me. Figures...\n\n";
      }
    } else
      cout << "Right... You're missing out buddy.\n\n";
  }
}
///////////////////////////////////////////////////////////////////////////////

/* UPGRADE LIST
UPGRADES[0] = increases sword dmg
UPGRADES[1] = increases sight
UPGRADES[2] = increases mining depth
UPGRADES[3] = increases mining width */

//processes which upgrade to give player
static void Upgrade(int x) {

  switch (x) {
    case 0:
      upgrades[0] += 1; //sword dmg
      player["damage"] += 5;
      break;
    case 1:
      upgrades[1] += 1; //sight
      player["sight"] += 1;
      break;
    case 2:
      upgrades[2] += 1; //mining width
      cout << ("\nUpgrade not fully implemented.\n");
      // TODO: implement mining width upgrade
      break;
    case 3:
      upgrades[3] += 1; //mining depth
      cout << ("\nUpgrade not fully implemented.\n");
      // TODO: implement mining depth upgrade
      break;
  } //end switch
}
///////////////////////////////////////////////////////////////////////////////

//processes final game statistics
static void GameReport() { 
  cout << "\n\nGame Over!\n";
  int upg = 0;
  int score = 0;

  for (int i = 0; i < 10; i++) {
    if (upgrades[i] > 0) {
      score += upgrades[i]*50;
      upg += upgrades[i];
    }
  }

  score += player["dirt"];
  score += player["ore"]*5;
  score += player["artifacts"]*30;
  score += player["coins"]*3;
  score += player["damage"]*5;
  score += player["sight"]*5;

  cout << "Total Score: " << score << "\n\n";
  #ifdef _WIN32
  Sleep(500);
  #else
  sleep(1);
  #endif

  cout << "Dirt: " << player["dirt"] << '\n';
  #ifdef _WIN32
  Sleep(500);
  #else
  sleep(1);
  #endif

  cout << "Ore: " << player["ore"] << '\n';
  #ifdef _WIN32
  Sleep(500);
  #else
  sleep(1);
  #endif

  cout << "Artifacts: " << player["artifacts"] << '\n';
  #ifdef _WIN32
  Sleep(500);
  #else
  sleep(1);
  #endif

  cout << "Coins: " << player["coins"] << '\n';
  #ifdef _WIN32
  Sleep(500);
  #else
  sleep(1);
  #endif

  cout << "Upgrades aquired: " << upg << "\n\n";
  #ifdef _WIN32
  Sleep(500);
  #else
  sleep(1);
  #endif
}
///////////////////////////////////////////////////////////////////////////////

//introduces game mechanics
static void Intro() {
  int x;
  cin.clear();
  cout << "\nHello... You're finally awake.\nI have kept you safe this long but ";
  cout << "you must continue this journey on your own.\n\nUse WASD to move. ";
  cout << "The Deep Below is endless, so mine to your heart's content.\n";
  cout << "Once you are finished, enter q to get a final score. Press any key to begin.\n";
  cin >> x;
  PrintMap();
}
///////////////////////////////////////////////////////////////////////////////

//creates initial values for the enemy miners
static void InitMiner(Rogue &miner, int y, int x) {
  miner.artifacts = 0;
  miner.coins = 25;
  miner.damage = 10;
  miner.ore = 0;
  miner.health = 30;
  miner.x = x;
  miner.y = y;
  miner.direction = rand() % 4;
  grid[y][x] = MINER;
}
///////////////////////////////////////////////////////////////////////////////

//processes miner actions after player action
static void MoveMiners(vector<Rogue> &MinerList) {
  int y;
  for (int x = 0; x < MinerList.size(); x++) {
    y = rand() % 2;
    if (y == 0)
      MoveMiner(MinerList[x]);
  }
}
///////////////////////////////////////////////////////////////////////////////

//moves a miner on the map
static void MoveMiner(Rogue &miner) {
  int change = rand() % 10;
  if (change == 0)
    miner.direction = rand() % 4;
  bool temp;
  switch (miner.direction) {
    case 0: //w, up
      if (miner.y > 0) {
        temp = ProcessBlock(miner, miner.y-1, miner.x);
        if (temp) {
          grid[miner.y-1][miner.x] = MINER;
          grid[miner.y][miner.x] = MINED;
          miner.y--;
        }
      }
      break;
    case 1: //a, left
      if (miner.x > 0) {
        temp = ProcessBlock(miner, miner.y, miner.x-1);
        if (temp) {
          grid[miner.y][miner.x-1] = MINER;
          grid[miner.y][miner.x] = MINED;
          miner.x--;
        }
      }
      break;
    case 2: //s, down
      if (miner.y < MAP_UPPER-1) {
        temp = ProcessBlock(miner, miner.y+1, miner.x);
        if (temp) {
          grid[miner.y+1][miner.x] = MINER;
          grid[miner.y][miner.x] = MINED;
          miner.y++;
        }
      }
      break;
    case 3: //d, right
      if (miner.x < MAP_UPPER-1) {
        temp = ProcessBlock(miner, miner.y, miner.x+1);
        if (temp) {
          grid[miner.y][miner.x+1] = MINER;
          grid[miner.y][miner.x] = MINED;
          miner.x++;
        }
      }
      break;
  }
}
///////////////////////////////////////////////////////////////////////////////

//returns true if valid move, false if invalid
static bool ProcessBlock(Rogue &miner, int y, int x) {
  switch (grid[y][x]) {
    case ORE:
      miner.ore++;
      break;
    case SHOP:
      for (int z = 0; z < miner.ore; z++) {
        miner.ore--;
        miner.coins += 5;
      }
      if (miner.artifacts >= 10) {
        miner.artifacts -= 10;
        miner.damage += 5;
      }
      return false;
    case ARTIFACT:
      miner.artifacts++;
      break;
    case PLAYER:
    case MINER:
      return false;
    default:
      break;
  }
  return true;
}

static void SaveGrid() {
  static ofstream MyFile("grid.txt");
  for (int y = 0; y < MAP_UPPER; y++) {
    for (int x = 0; x < MAP_UPPER; x++) {
      MyFile << grid[y][x];
    }
    MyFile << '\n';
  }
  MyFile.close();
}
