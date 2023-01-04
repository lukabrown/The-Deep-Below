//Personal Project by Luka Brown
//The Deep Below
//Mining Game
//In development, should work on both windows and linux
//Feel free to compile and playtest!


/* Gameplay:

Explore an (almost) endless mass of blocks and dig your way around!
The game starts with a grid output to a console with 'P' for player in the
middle. 

Go towards * if you see them, they're rare and sparkly
$ is a friendly shop!
+ is an enemy miner, beware!
Have fun!

Implemented features:
  - Main Menu
      Saving and Loading from a file

  - Rogue Enemy Miners to fight
  - Minibosses to fight
  - Boss to fight

  - Ending Score System

  - Shop system
      can sell ore
      can buy artifacts
      can trade artifacts for upgrades

  - Upgrades (5/7)
      Extra damage
      Extra health
      Sight (sees more of map at once)
      Clarity (sees all special blocks more often)
      Compass to guide player towards a secret of the mines

Agenda:
  - Upgrades (2/7)
      Mining width
      Mining depth
  - Encrypted save files (?)
*/

#include <vector>
#include <iostream>
#include <random>
#include <ctime>
#include <cstdlib>
#include <map>
#include <string>
#include <fstream>

#ifdef   _WIN32
#include <Windows.h>
#else    //linux
#include <unistd.h>
#endif


//enemy AI class
struct Rogue {
  int damage, coins, ore, artifacts, health, x, y, direction;
  bool moved;
};


//function prototypes
//intro/helper functions
static void Init();
static void Intro();
static void InputClear();
static void MySleep(double seconds);

//game functions
static int  GameInput();
static void Move(int x);
static bool CollectItem(int y, int x);
static void GameReport();
static bool TitleScreen();
static bool Miniboss();
static bool Boss();
static void Revive();

//map/grid functions
static void GenerateGrid();
static void PrintGrid();
static bool SaveGame(std::string name);
static bool LoadGame(std::string name);

//shop functions
static void CallShop();
static void SellOre();
static void BuyArtifacts();
static void Trade();
static void Upgrade(int x);

//miner functions
static void InitMiner(Rogue &miner, int y, int x);
static void MoveMiners();
static void MoveMiner(Rogue &miner);
static bool ProcessBlock(Rogue &miner, int y, int x);
static bool MinerFight(int y, int x);


//Constants
static const int GRID_UPPER = 2000; //2000x2000 grid, 4 million blocks
static const int MINERS = GRID_UPPER + GRID_UPPER / 2; //scales with grid size
static const int UPGRADE_UPPER = 7; //num of upgrades implemented

//"block" types
#define PLAYER   0
#define DIRT     1
#define MINED    2 //mined dirt
#define SHOP     3
#define ARTIFACT 4
#define ORE      5
#define MINER    6 //enemy
#define MINIBOSS 7
#define BOSS     8

//global vars
static bool game; //game on/off
static int  upgrades[UPGRADE_UPPER]; //stores levels of upgrades
static std::map<std::string, int> player; //dictionary of player items, defined in Init()
static std::vector<std::vector<int>> grid(GRID_UPPER, std::vector<int> (GRID_UPPER)); //map
static std::vector<Rogue> MinerList; //list of all enemy miners


int main() {
  Init(); //creates map and prints
  Intro(); //prints opening statement

  int action;
  bool update;

  while(game) {
    MySleep(.2);
    action = GameInput();

    switch (action) {
      case -1:
        std::cout << "Invalid Input\n";
        MySleep(2);
        update = false;
        break;
      case 0:
      case 1:
      case 2:
      case 3:
        Move(action);
        update = true;
        break;
      case 4: //hold your ground
        update = true;
        break;
      case 5:
        update = TitleScreen();
        break;
    } //end switch

    if (update)
      MoveMiners();

    if (player["health"] <= 0)
      game = false;
    
    if (player["died"] == 1)
      Revive();

    PrintGrid();
  }// end game while
  
  GameReport();
  return 0;
}
///////////////////////////////////////////////////////////////////////////////

//creates 2d vector of blocks
static void GenerateGrid() {
  int y, x, random;
  int inside = 0;
  srand((unsigned int)time(NULL)); //seeds random

  //initializes map with basic blocks
  for (y = 0; y < GRID_UPPER; y++) {
    for (x = 0; x < GRID_UPPER; x++) {
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
          random = rand() % 10; //30% shop 70% dirt
          if (random == 0 || random == 1 || random == 2)
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
  
  //generate minibosses in the inner chunk of the map
  for (int i = 0; i < GRID_UPPER/6; i++) {
    y = rand() % GRID_UPPER - GRID_UPPER / 4;
    x = rand() % GRID_UPPER - GRID_UPPER / 4;

    while (y <= GRID_UPPER / 4 || (y < GRID_UPPER/2 + GRID_UPPER/100 && y > GRID_UPPER/2 - GRID_UPPER/100))
      y = rand() % GRID_UPPER - GRID_UPPER / 4;

    while (x <= GRID_UPPER / 4 || (x < GRID_UPPER/2 + GRID_UPPER/100 && x > GRID_UPPER/2 - GRID_UPPER/100))
      x = rand() % GRID_UPPER - GRID_UPPER / 4;

    grid[y][x] = MINIBOSS;
  }

  //generate minibosses in the middle chunk of the map
  for (int i = 0; i < GRID_UPPER/6; i++) {
    y = rand() % GRID_UPPER - GRID_UPPER / 8;
    x = rand() % GRID_UPPER - GRID_UPPER / 8;

    while (!((y >= GRID_UPPER / 8 && y <= GRID_UPPER / 4) || y >= GRID_UPPER - GRID_UPPER / 4))
      y = rand() % GRID_UPPER - GRID_UPPER / 8;

    while (!((x >= GRID_UPPER / 8 && x <= GRID_UPPER / 4) || x >= GRID_UPPER - GRID_UPPER / 4))
      x = rand() % GRID_UPPER - GRID_UPPER / 8;

    grid[y][x] = MINIBOSS;
  }

  //generate minibosses in the outer chunk of the map
  for (int i = 0; i < GRID_UPPER/6; i++) {
    y = rand() % GRID_UPPER;
    x = rand() % GRID_UPPER;

    while (y <= GRID_UPPER / 4)
      y = rand() % GRID_UPPER - GRID_UPPER / 2;

    while (x <= GRID_UPPER / 4)
      x = rand() % GRID_UPPER - GRID_UPPER / 2;

    grid[y][x] = MINIBOSS;
  }

  //generates miners
  for (int i = 0; i < MINERS; i++) {
    Rogue miner;
    y = rand() % GRID_UPPER;
    x = rand() % GRID_UPPER;

    if ((y >= GRID_UPPER/4 && y <= GRID_UPPER - GRID_UPPER/4) && 
        (x >= GRID_UPPER/4 && x <= GRID_UPPER - GRID_UPPER/4))
      inside++;

    if (inside < GRID_UPPER / 7) {
      while (!((y >= GRID_UPPER/4 && y <= GRID_UPPER - GRID_UPPER/4) && 
               (x >= GRID_UPPER/4 && x <= GRID_UPPER - GRID_UPPER/4))) {
        y = rand() % GRID_UPPER;
        x = rand() % GRID_UPPER;
      }
    }

    InitMiner(miner, y, x);
    MinerList.push_back(miner);
  }

  //spawns boss
  y = rand() % GRID_UPPER;
  x = rand() % GRID_UPPER;
  while ((x < GRID_UPPER/2 + GRID_UPPER/8 && x > GRID_UPPER/2 - GRID_UPPER/8) ||
         (y < GRID_UPPER/2 + GRID_UPPER/8 && y > GRID_UPPER/2 - GRID_UPPER/8)) {
    y = rand() % GRID_UPPER;
    x = rand() % GRID_UPPER;
  }
  grid[y][x] = BOSS;
  player["bossY"] = y;
  player["bossX"] = x;
  
  grid[player["y"]][player["x"]] = PLAYER; //sets player position
  return;
}
///////////////////////////////////////////////////////////////////////////////

//prints blocks around the player
static void PrintGrid() {
  std::cout << "\n\n\n\n\n\n";
  int y, x, chance, sight;
  sight = upgrades[2] + 4;

  grid[player["y"]][player["x"]] = PLAYER;

  for (y = player["y"] - sight; y < player["y"] + sight + 1; y++) {
    if (y > GRID_UPPER-1)
      y = GRID_UPPER-1;
    else if (y < 0)
      y = 0;

    for (x = player["x"] - sight; x < player["x"] + sight + 1; x++) {
      if (x > GRID_UPPER-1)
        x = GRID_UPPER-1;
      else if (x < 0)
        x = 0;

      switch(grid[y][x]) {
        case PLAYER:
          std::cout << "P ";
          break;
        
        case MINER:
          std::cout << "+ ";
          break;

        case DIRT:
          std::cout << "# "; 
          break;

        case MINED:
          std::cout << "  ";
          break;

        case SHOP:
          std::cout << "$ ";
          break;

        case MINIBOSS:
          std::cout << "\" ";
          break;

        case BOSS:
          std::cout << "- ";
          break;

        case ARTIFACT: //ore and artifact have chance of not showing up
        case ORE:
          chance = rand() % (8 - (upgrades[4]*2)); //chance goes from 1/8 to 1/6 
          if (chance == 0)       //to 1/4 to 1/2 chance of showing up on the map
            std::cout << "* ";
          else
            std::cout << "# ";
          break;
      } //end switch
    } //end for x
    std::cout << '\n';
  } //end for y

  std::cout << "Ore: " << player["ore"] << " Artifacts: " << player["artifacts"];
  std::cout << " Coins: " << player["coins"] << " HP: " << player["health"] << '\n';

  if (upgrades[6] == 3) {
    bool left, right, down, up;
    left = right = down = up = false;
    std::string direction;
    
    if (player["x"] > player["bossX"])
      left = true;
    else if (player["x"] < player["bossX"])
      right = true;

    if (player["y"] > player["bossY"])
      up = true;
    else if (player["y"] < player["bossY"])
      down = true;

    if (up && right)
      direction = "North-East";
    else if (up && left)
      direction = "North-West";
    else if (down && left)
      direction = "South-West";
    else if (down && right)
      direction = "South-East";
    else if (up)
      direction = "North";
    else if (down)
      direction = "South";
    else if (right)
      direction = "East";
    else if (left)
      direction = "West";
    else
      direction = "Error";
    
    if (player["y"] >= player["bossY"])
      y = player["y"] - player["bossY"];
    else
      y = player["bossY"] - player["y"];

    if (player["x"] >= player["bossX"])
      x = player["x"] - player["bossX"];
    else
      x = player["bossX"] - player["x"];
    
    if (!(y < 7 && x < 7))
      std::cout << "Your cursed compass points " << direction << '\n';
    else
      std::cout << "Your cursed compass begins spinning in all directions\n";
  }
}
///////////////////////////////////////////////////////////////////////////////

//catches player movement and sends back
static int GameInput() {
  char input;
  InputClear();
  input = getchar();

  switch (input) {
    case 'w':
    case 'W':
      return 0;
    case 'a':
    case 'A':
      return 1;
    case 's':
    case 'S':
      return 2;
    case 'd':
    case 'D':
      return 3;
    case 'h':
    case 'H':
      return 4;
    case 't':
    case 'T':
      return 5;
    default:
      return -1;
  } //end switch
}
///////////////////////////////////////////////////////////////////////////////

//adjusts players position on map based on keypress
//parameter: int 1,2,3 or 4 of which direction to move player in
static void Move(int direction) {
  bool valid;
  switch (direction) {
    case 0: //w, up
      if (player["y"] > player["sight"]) { //makes sure it wont exceed map bounds
        valid = CollectItem(player["y"]-1,player["x"]); //processes block stepped on
        if (valid) {
          grid[player["y"]-1][player["x"]] = PLAYER;
          grid[player["y"]][player["x"]] = MINED;
          player["y"]--;
        }
      }
      break;
    case 1: //a, left
      if (player["x"] > player["sight"]) {
        valid = CollectItem(player["y"],player["x"]-1);
        if (valid) {
          grid[player["y"]][player["x"]-1] = PLAYER;
          grid[player["y"]][player["x"]] = MINED;
          player["x"]--;
        }
      }
      break;
    case 2: //s, down
      if (player["y"] < GRID_UPPER-player["sight"]-1) {
        valid = CollectItem(player["y"]+1,player["x"]);
        if (valid) {
          grid[player["y"]+1][player["x"]] = PLAYER;
          grid[player["y"]][player["x"]] = MINED;
          player["y"]++;
        }
      }
      break;
    case 3: //d, right
      if (player["x"] < GRID_UPPER-player["sight"]-1) {
        valid = CollectItem(player["y"],player["x"]+1);
        if (valid) {
          grid[player["y"]][player["x"]+1] = PLAYER;
          grid[player["y"]][player["x"]] = MINED;
          player["x"]++;
        }
      }
      break;
  }
}
///////////////////////////////////////////////////////////////////////////////

//processes block player stepped on
//parameters: YX co-ord. of the item to be collected by player
static bool CollectItem(int y, int x) {
  if (grid[y][x] == DIRT) {

    int z = rand() % 100;
    if (z == 0) { //1% chance artifact in dirt
      std::cout << "\nWhile digging, you found an ancient artifact!" << '\n';
      player["artifacts"]++;
      MySleep(2);
    } 
    else if (z == 1) { //1% chance ore in dirt
      std::cout << "\nWhile digging, you found a rare ore!" << '\n';
      player["ore"]++;
      MySleep(2);
    }
    player["dirt"]++;
  } 

  else if (grid[y][x] == SHOP) {
    CallShop();
  } 

  else if (grid[y][x] == ARTIFACT) {
    std::cout << "\nWhile digging, you found an ancient artifact!" << '\n';
    MySleep(2);
    player["artifacts"]++;
  }

  else if (grid[y][x] == ORE) {
    std::cout << "\nWhile digging, you found a rare ore!" << '\n';
    MySleep(2);
    player["ore"]++;
  } 

  else if (grid[y][x] == MINER) {
    return MinerFight(y,x); //valid only if miner dies
  }

  else if (grid[y][x] == MINIBOSS) {
    return Miniboss();
  }

  else if (grid[y][x] == BOSS) {
    return Boss();
  }

  return true;
}
///////////////////////////////////////////////////////////////////////////////

//calls shop when player steps into a shop
static void CallShop() {
  bool finish = false;
  bool asked = false;
  char x;

  std::cout << "You come across a small opening and see that there's a store inside.\n";
  std::cout << "Would you like to shop? I buy Ore! Enter Y if yes!\n";
  InputClear();
  std::cin >> x;
  

  if (!(x == 'y' || x == 'Y')) {
    std::cout << "This could be goodbye forever. Enter Y if you want to shop.\n";
    InputClear();
    std::cin >> x;
  }

  if (x == 'y' || x == 'Y') {
    //main store loop
    while (!finish) {
      std::cout << "\n\nYou take inventory: Ore: " << player["ore"] << " Artifacts: ";
      std::cout << player["artifacts"] << " Coins " << player["coins"] << '\n';
      
      std::cout << "The shop owner poins to a sign that reads:\n\nPick:";
      std::cout << "1. Sell ore (5/pc!)\n2. Buy artifacts (30/pc)\n";
      std::cout << "3. Deal of the Day\n4. Leave\n\nWhat would you like to do?\n";
      InputClear();
      std::cin >> x;

      if (x == '1' || x == '2' || x == '3' || x == '4' || x == 'q') {
        if (x == '1')
          SellOre();

        else if (x == '2')
          BuyArtifacts();

        else if (x == '3') {
          if (!asked) { //can only see upgrade offer once
            Trade();
            asked = true;

          } else {
            std::cout << "We've already discussed that. I'm not sayin it all again.\n\n";
            MySleep(3);
          }
          
        } else //q or 4
          finish = true;

      } else { //invalid input
        std::cout << "Sorry, I didn't get that. Which number did you want?\n\n";
        MySleep(3);
      } //end input if/else
    } //end store while loop
  } //end if x == y

  std::cout << "\nAlright, see you later. Oh, and grab some bread on your way out. Keeps ya hardy.\n";
  MySleep(3);

  player["health"] = player["maxHP"]; //heals player
  std::cout << "The bread looks delicious. You grab some and take a bite. On your way out you feel";
  std::cout << " refreshed. Ahh, bread.\n";

  MySleep(4);
  return;
}
///////////////////////////////////////////////////////////////////////////////

//initializes globals and calls GenMap
static void Init() {
  //set globals
  player["x"] = player["y"] = GRID_UPPER/2;//player starting in middle of the map
  player["damage"] = 10;
  player["ore"] = player["artifacts"] = player["dirt"] = player["coins"] = 0;
  player["kills"] = player["died"] = 0;
  player["health"] = player["maxHP"] = 35;

  game = true;
  for (int i = 0; i < UPGRADE_UPPER; i++) {
    upgrades[i] = 0;
  }

  //make and set map
  GenerateGrid();
}
///////////////////////////////////////////////////////////////////////////////

//shop helper if you want to sell ore
static void SellOre() {
  int temp;

  std::cout << "How much?\n";
  InputClear();
  std::cin >> temp;

  if (temp > player["ore"]) {
    std::cout << "You don't have enough ore! You only have " << player["ore"] << " ore\n\n";
    MySleep(2);
  } 
  else if (temp == 0) {
    std::cout << "Alright...\n";
    MySleep(2);
  }
  else {
    player["ore"] -= temp;
    player["coins"] += temp*5;
    std::cout << "Pleasure doing business with you!\n\n";
    MySleep(2);
  }
}
///////////////////////////////////////////////////////////////////////////////

//shop helper if you want to buy artifacts
static void BuyArtifacts() {
  int temp;

  std::cout << "How much?\n";
  InputClear();
  std::cin >> temp;

  if (temp*30 > player["coins"]) {
    std::cout << "You don't have enough coins! You only have " << player["coins"] << " coins\n\n";
    MySleep(2);
    return;
  } 
  else if (temp == 0) {
    std::cout << "Umm... Okay.\n";
    MySleep(2);
  } else {
    player["coins"] -= temp*30;
    player["artifacts"] += temp;
    std::cout << "Pleasure doing business with you!\n\n";
    MySleep(2);
  }
}
///////////////////////////////////////////////////////////////////////////////

//shop helper if you want to trade for an upgrade
static void Trade() {
  char input;

  //cost ranges from 15-35
  //nums 16-29 have a higher probability
  int cost = rand() % 35;
  if (cost < 15) { cost += 15; }

  std::cout << "\nOkay, I only have one fine deal for you.\n";
  std::cout << "If you have " << cost << " ancient artifacts then I may consider selling...\n";
  std::cout << "The only item that would help you is a magnificent Upgrade!\n\n";

  int random = rand() % UPGRADE_UPPER; //picks what upgrade the shop has

  if (upgrades[random] >= 3) { //3 is the max level an upgrade can achieve
    std::cout << "Oh... It looks like you already have the upgrade I was going to offer.\n";
    std::cout << "Looks like I have nothing special, sorry!\n";
    MySleep(4);
  } 
  else {
    switch(random) {
      case 0:
        std::cout << "This book enchants the sword of the user for extra damage.\n";
        break;
      case 1:
        std::cout << "This ring gives your eyes the ability to see much farther.\n";
        break;
      case 2:
        std::cout << "This enhancement will allow you to swing wider.\n";
        cost += 10; //upgrade costs more due to power
        break;
      case 3:
        std::cout << "This enhancement will allow you to dig deeper.\n";
        cost += 10; //upgrade costs more due to power
        break;
      case 4:
        std::cout << "Ever see some glints of ore in the mines? Carrying around ";
        std::cout << "this trinket will guide your eyes to the valuables.\n";
        break;
      case 5:
        std::cout << "This potion will make you much hardier. Trust me, looks like you need it.\n";
        break;
      case 6:
        std::cout << "This compass leads to a deep, dark, secret. I do not know where it\n";
        std::cout << "goes nor do I wish to. This is a cursed object... Beware.\n";
        break;
    } //end switch

    std::cout << "Do you want to buy it for " << cost << " artifacts? Enter Y if yes.\n";
    InputClear();
    std::cin >> input;
    if (input == 'y' || input == 'Y') {
      if (player["artifacts"] >= cost) {
        player["artifacts"] -= cost;
        Upgrade(random);
        std::cout << "Pleasure doing business with you!\n\n";
        MySleep(2);

      } else {
        std::cout << "Looks like you don't have enough artifacts for me. Figures...\n\n";
        MySleep(2);
      }
    } else {
      std::cout << "Right... You're missing out buddy.\n\n";
      MySleep(2);
    }
  }
}
///////////////////////////////////////////////////////////////////////////////

/* UPGRADE LIST
upgrades[0] = increases sword dmg
upgrades[1] = increases sight
upgrades[2] = increases mining depth //unimplemented
upgrades[3] = increases mining width //unimplemented
upgrades[4] = increases 'clarity', allowing for better sight on ore/artifacts
upgrades[5] = increases health
upgrades[6] = leads player to final boss
*/

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
      upgrades[2] += 3; //mining width
      std::cout << ("\nUpgrade not fully implemented.\n");
      MySleep(2);
      // TODO: implement mining width upgrade
      break;
    case 3:
      upgrades[3] += 3; //mining depth
      std::cout << ("\nUpgrade not fully implemented.\n");
      MySleep(2);
      // TODO: implement mining depth upgrade
      break;
    case 4:
      upgrades[4]++;
      break;
    case 5:
      upgrades[5]++;
      player["health"] += 15;
      player["maxHP"] += 15;
      break;
    case 6:
      upgrades[6] += 3;
      break;
  } //end switch
}
///////////////////////////////////////////////////////////////////////////////

//processes final game statistics
static void GameReport() { 
  std::cout << "\n\nGame Over!\n";
  int upg = 0;
  int score = -140; //accounts for initial values player starts with

  for (int i = 0; i < UPGRADE_UPPER; i++) {
    if (upgrades[i] > 0) {
      score += upgrades[i]*100;
      upg += upgrades[i];
    }
  }

  score += player["dirt"];
  score += player["ore"]*5;
  score += player["artifacts"]*30;
  score += player["coins"]*3;
  score += player["damage"]*5;
  score += player["sight"]*5;
  score += player["maxHP"]*2;
  score += player["kills"]*50;

  std::cout << "Total Score:      " << score << "\n\n";
  MySleep(1);

  std::cout << "Dirt:             " << player["dirt"] << '\n';
  MySleep(1);

  std::cout << "Ore:              " << player["ore"] << '\n';
  MySleep(1);

  std::cout << "Artifacts:        " << player["artifacts"] << '\n';
  MySleep(1);

  std::cout << "Coins:            " << player["coins"] << '\n';
  MySleep(1);

  std::cout << "Upgrades aquired: " << upg << '\n';
  MySleep(1);

  std::cout << "Miners slayed:    " << player["kills"] << "\n\n";
  MySleep(1);
}
///////////////////////////////////////////////////////////////////////////////

//introduces game mechanics
static void Intro() {
  int x;
  std::cout << "\nHello... You're finally awake.\nI have kept you safe this long but ";
  std::cout << "you must continue this journey on your own.\n\n";
  std::cout << "The Deep Below is endless, so mine to your heart's content.\n";
  std::cout << "And beware of others... you aren't alone down here.\n\n";
  std::cout << "Press any key to begin.\n";
  std::cin >> x;

  std::cout << "\n\nControls: \n   Enter WASD to move\n";
  MySleep(1);
  std::cout << "   Enter H to hold your ground\n";
  MySleep(1);
  std::cout << "   Enter T to go to the title screen\n";
  MySleep(1);
  std::cout << "\nGood Luck Mining!";
  MySleep(2);

  PrintGrid();
}
///////////////////////////////////////////////////////////////////////////////

//creates initial values for the enemy miners
//parameters: miner to be initalized
static void InitMiner(Rogue &miner, int y, int x) {
  miner.artifacts = 0;
  miner.coins = 25;
  miner.damage = 8;
  miner.ore = 0;
  miner.health = 30;
  miner.y = y;
  miner.x = x;
  miner.direction = rand() % 4;
  if (miner.x % 2 == 0)
    miner.moved = false;
  else
    miner.moved = true;
  grid[miner.y][miner.x] = MINER;
}
///////////////////////////////////////////////////////////////////////////////

//iterates through all miners to move them
static void MoveMiners() {
  for (int i = 0; i < MINERS; i++) {
    if (MinerList[i].health != 0) {
      if (MinerList[i].moved) { //moves every other time
        MoveMiner(MinerList[i]);
        MinerList[i].moved = !MinerList[i].moved;
      }
      else
        MinerList[i].moved = !MinerList[i].moved;
    }
  }
}
///////////////////////////////////////////////////////////////////////////////

//moves a miner on the map
//parameter: miner to be moved
static void MoveMiner(Rogue &miner) {
  int change = rand() % 10;
  if (change == 0)
    miner.direction = rand() % 4;

  bool temp;
  switch (miner.direction) {
    case 0: //up
      if (miner.y > 0) {
        temp = ProcessBlock(miner, miner.y-1, miner.x);
        if (temp) {
          grid[miner.y-1][miner.x] = MINER;
          if (grid[miner.y][miner.x] != PLAYER)
            grid[miner.y][miner.x] = MINED;
          else
            grid[miner.y][miner.x] = PLAYER;
          miner.y--;
        } else {
          grid[miner.y][miner.x] = MINER;
        }
      }
      break;
    case 1: //left
      if (miner.x > 0) {
        temp = ProcessBlock(miner, miner.y, miner.x-1);
        if (temp) {
          grid[miner.y][miner.x-1] = MINER;
          if (grid[miner.y][miner.x] != PLAYER)
            grid[miner.y][miner.x] = MINED;
          else
            grid[miner.y][miner.x] = PLAYER;
          miner.x--;
        } else {
          grid[miner.y][miner.x] = MINER;
        }
      }
      break;
    case 2: //down
      if (miner.y < GRID_UPPER-1) {
        temp = ProcessBlock(miner, miner.y+1, miner.x);
        if (temp) {
          grid[miner.y+1][miner.x] = MINER;
          if (grid[miner.y][miner.x] != PLAYER)
            grid[miner.y][miner.x] = MINED;
          else
            grid[miner.y][miner.x] = PLAYER;
          miner.y++;
        } else {
          grid[miner.y][miner.x] = MINER;
        }
      }
      break;
    case 3: //right
      if (miner.x < GRID_UPPER-1) {
        temp = ProcessBlock(miner, miner.y, miner.x+1);
        if (temp) {
          grid[miner.y][miner.x+1] = MINER;
          if (grid[miner.y][miner.x] != PLAYER)
            grid[miner.y][miner.x] = MINED;
          else
            grid[miner.y][miner.x] = PLAYER;
          miner.x++;
        } else {
          grid[miner.y][miner.x] = MINER;
        }
      }
      break;
  }
}
///////////////////////////////////////////////////////////////////////////////

//returns true if valid move, false if invalid
//special interaction on Shop blocks
//parameters: miner to be moved and the YX co-ord. of where they want to go
static bool ProcessBlock(Rogue &miner, int y, int x) {
  switch (grid[y][x]) {
    case ORE: //adds ore to sell at shops
      miner.ore++;
      break;

    case SHOP: //makes miner more valuable to fight over time
      //sells all ore and adds to miner coins
      for (int z = 0; z < miner.ore; z++) {
        miner.ore--;
        miner.coins += 5;
      }
      //upgrades miner if they have enough artifacts
      if (miner.artifacts >= 10) {
        miner.artifacts -= 10;
        miner.damage += 5;
      }
      //changes miner direction so they leave the shop and dont idle
      miner.direction = rand() % 4;
      return false;

    case ARTIFACT: //adds artifacts to upgrade dmg at shops
      miner.artifacts++;
      break;

    case PLAYER: //damages player if theyre in the way
      std::cout << "You spot a miner coming toward you and see a haze in their eyes.\n";
      MySleep(2);
      std::cout << "\nThey don't seem to notice you and continue swinging their pickaxe";
      std::cout << " even though you are in their way.\n";
      MySleep(2);
      std::cout << "You brace and take " << miner.damage/2 << " damage from the miner.\n\n";
      MySleep(3);
      player["health"] -= miner.damage/2;

      //player death
      if (player["health"] <= 0) {
        game = false;
        player["died"]++;
        player["health"] = 0;
        std::cout << "You've taken too much damage and the miner is merciless.\n";
        std::cout << "You fall to the ground and the miner continues on their way.\n";
        MySleep(5);
      }
      return false;

    case MINER: //doesn't allow overlap
    case MINIBOSS:
    case BOSS:
      return false;
  }
  return true;
}
//////////////////////////////////////////////////////////////////////////////

//processes player fighting with enemy miner
//parameters: YX co-ord. of the miner to be faught
static bool MinerFight(int y, int x) {
  std::cout << "You have come across another miner. They are competition.\n";
  std::cout << "Do you swing? Enter Y for yes.\n";

  char in;
  InputClear();
  std::cin >> in;

  if (!(in == 'Y' || in == 'y')) { //player doesnt want to fight
    std::cout << "He raises his pick to swing at you but doesn't. You brush past each other.\n";
    MySleep(3);
    return false;
  }

  //player fights
  int enemyIndex; //index of miner in MinerList to fight
  int deviation = rand() % 5; //random chance to change the dmg
  int damage; 

  //computes index
  for (long long unsigned int i = 0; i < MinerList.size(); i++) {
    if (MinerList[i].y == y && MinerList[i].x == x) {
      enemyIndex = i;
      break;
    }
  }

  //computes damage
  if (deviation == 0) //no change on dmg
    damage = player["damage"];
  else if (deviation == 1 || deviation == 2) //negative 1 or 2 from base dmg
    damage = player["damage"] - deviation;
  else //positive 1 or 2 from base dmg
    damage = player["damage"] + (deviation-2);

  //effects
  std::cout << "You approach the miner and swing with all your might.\n";
  std::cout << "You dealt " << damage << " damage to the miner.\n";
  MySleep(3);
  MinerList[enemyIndex].health -= damage;

  if (MinerList[enemyIndex].health <= 0) { //if miner dies
    std::cout << "AAAGH... the miner lets out a last scream before falling down.\n";
    std::cout << "They won't be getting back up from that.\n";
    MySleep(3);

    MinerList[enemyIndex].health = 0;
    MinerList[enemyIndex].x = -1;
    MinerList[enemyIndex].y = -1;
    player["kills"]++;

    std::cout << "You gain " << MinerList[enemyIndex].coins << " coins.\n";
    MySleep(3);

    player["coins"] += MinerList[enemyIndex].coins;
    return true;
  } 
  else { //miner lives and retaliates
    damage = rand() % 4;
    damage += 6;

    std::cout << "The miner swings their pick back and dealt " << damage;
    std::cout << " damage!\n";
    MySleep(3);

    player["health"] -= damage;
    if (player["health"] <= 0) {
      game = false;
      player["died"]++;

      std::cout << "The pick impales you and leaves you with a gash too wide to mend.";
      MySleep(2);

      std::cout << "\nYou fall down on the hard rocks and reflect as you die.\n";
      MySleep(4);
      player["health"] = 0;
    }
    return false;
  } 
}
///////////////////////////////////////////////////////////////////////////////

//saves a lot of typing the ifdef else every time
static void MySleep(double seconds) {
  #ifdef _WIN32
  seconds *= 1000;
  Sleep(seconds);

  #else
  seconds = (int)seconds;
  sleep(second);
  #endif
}
///////////////////////////////////////////////////////////////////////////////

//allows the player to explore options outside of the game
static bool TitleScreen() {
  char input;
  int upg = 0;

  std::cout << "\n\n\n\n\n\n\n\nThe Deep Below\n\nTitle Screen:\n";
  std::cout << "1. Resume Game\n";
  std::cout << "2. Save Game\n";
  std::cout << "3. Load Game\n";
  std::cout << "4. View Stats\n";
  std::cout << "5. Exit Game\n";
  std::cout << "6. View Credits\n";
  std::cout << "What would you like to do? Enter the number.\n";

  InputClear();
  std::cin >> input;
  input -= 1;

  switch (input) {
    case '0': //resume game
      return true;
    case '1': //save game
      std::cout << "Saving...\n";
      MySleep(1);
      SaveGame("save.txt");
      return false;
    case '2': //load game
      std::cout << "After loading, press any key to begin.\n";
      MySleep(1);
      LoadGame("save.txt");
      return false;
    case '3': //view stats
      //print upgrades
      for (int i = 0; i < UPGRADE_UPPER; i++) {
        if (upgrades[i] > 0) {
          upg += upgrades[i];
        }
      }

      //print stats
      std::cout << "Dirt:             " << player["dirt"] << '\n';
      MySleep(1);
      std::cout << "Ore:              " << player["ore"] << '\n';
      MySleep(1);
      std::cout << "Artifacts:        " << player["artifacts"] << '\n';
      MySleep(1);
      std::cout << "Coins:            " << player["coins"] << '\n';
      MySleep(1);
      std::cout << "Upgrades aquired: " << upg << '\n';
      MySleep(1);
      std::cout << "Miners slayed:    " << player["kills"] << "\n\n";
      MySleep(1);
      return false;
    case '4': //exit game
      game = false;
      return false;
    case '5': //view credits
      std::cout << "Well my, my, my, thank you for asking!\n\n";
      MySleep(1);
      std::cout << "This was developed solely by Luka Brown!\n\n";
      MySleep(2);
      std::cout << "They are a software developer currently based in Texas and";
      std::cout << " working on finishing a Computer Science degree in '23!\n";
      MySleep(7);
      return false;
    default:
      std::cout << "Invalid Input\n";
      MySleep(2);
      return false;
  }
}
///////////////////////////////////////////////////////////////////////////////

//clears input, is called before any cin
static void InputClear() {
  std::cin.clear();
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
///////////////////////////////////////////////////////////////////////////////

//saves the state of the current game to save.txt
static bool SaveGame(std::string name) {
  try {
    std::ofstream MyFile(name);

    //save grid
    for (int y = 0; y < GRID_UPPER; y++) {
      for (int x = 0; x < GRID_UPPER; x++) {
        MyFile << grid[y][x];
      }
      MyFile << '\n';
    }

    //save player
    MyFile << player["y"] << ',' << player["x"] << ',' << player["damage"];
    MyFile << ',' << player["ore"] << ',' << player["dirt"];
    MyFile << ',' << player["artifacts"] << ',' << player["coins"];
    MyFile << ','<< player["kills"] << ','<< player["health"];
    MyFile << ',' << player["maxHP"]<< ',' << player["died"];
    MyFile << ',' << player["bossY"]<< ',' << player["bossX"] << '\n';

    //save upgrades
    for (int i = 0; i < UPGRADE_UPPER; i++) {
      MyFile << upgrades[i] << ',';
    }
    MyFile << '\n';

    //save miners
    for (int i = 0; i < MINERS; i++) {
      MyFile << MinerList[i].damage << ',' << MinerList[i].coins << ',';
      MyFile << MinerList[i].artifacts << ',' << MinerList[i].health << ',';
      MyFile << MinerList[i].x << ',' << MinerList[i].y << ',';
      MyFile << MinerList[i].direction << ',' << MinerList[i].moved << '\n';
    }

    MyFile.close();
    std::cout << "Save Successful!\n";
    MySleep(2);
    return true;
  }

  catch (std::ofstream::failure const &e) {
    std::cerr << "Error opening/writing/closing file\n";
    MySleep(2);
    return false;
  }
}
///////////////////////////////////////////////////////////////////////////////

//loads the state of save.txt into the current game
static bool LoadGame(std::string name) {
  InputClear();
  try {
    std::ifstream MyFile(name);
    std::string line;
    std::string item;
    std::string delimiter = ",";
    size_t position = 0;
    char temp;
    int num;

    //load grid
    for (int y = 0; y < GRID_UPPER; y++) {
      for (int x = 0; x < GRID_UPPER; x++) {
        temp = MyFile.get();
        grid[y][x] = temp - '0';
      }
      temp = MyFile.get();
    }

    //load player
    std::getline(MyFile, line);
      
    for (int j = 0; j < 13; j++) {
      num = 0;
      position = line.find(delimiter);
      item = line.substr(0, position);

      for (int i = 0; item[i] != '\0'; i++) {
        num = num * 10 + item[i] - '0';
      }

      switch (j) {
        case 0:
          player["y"] = num;
          break;
        case 1:
          player["x"] = num;
          break;
        case 2:
          player["damage"] = num;
          break;
        case 3:
          player["ore"] = num;
          break;
        case 4:
          player["dirt"] = num;
          break;
        case 5:
          player["artifacts"] = num;
          break;
        case 6:
          player["coins"] = num;
          break;
        case 7:
          player["kills"] = num;
          break;
        case 8:
          player["health"] = num;
          break;
        case 9:
          player["maxHP"] = num;
          break;
        case 10:
          player["died"] = num;
          break;
        case 11:
          player["bossY"] = num;
          break;
        case 12:
          player["bossX"] = num;
          break;
      }
    line.erase(0, position + delimiter.length());
    }

    //load upgrades
    std::getline(MyFile, line);
    for (int i = 0; i < UPGRADE_UPPER; i++) {
      num = 0;
      position = line.find(delimiter);
      item = line.substr(0, position);
      for (int z = 0; item[z] != '\0'; z++) {
        num = num * 10 + item[z] - '0';
      }
      upgrades[i] = num;
      line.erase(0, position + delimiter.length());
    }

    //load miners
    for (int i = 0; i < MINERS; i++) {
      std::getline(MyFile, line);

      for (int j = 0; j < 8; j++) {
        num = 0;
        position = line.find(delimiter);
        item = line.substr(0, position);

        for (int z = 0; item[z] != '\0'; z++) {
          num = num * 10 + item[z] - '0';
        }

        switch (j) {
          case 0:
            MinerList[i].damage = num;
            break;
          case 1:
            MinerList[i].coins = num;
            break;
          case 2:
            MinerList[i].artifacts = num;
            break;
          case 3:
            MinerList[i].health = num;
            break;
          case 4:
            MinerList[i].x = num;
            break;
          case 5:
            MinerList[i].y = num;
            break;
          case 6:
            MinerList[i].direction = num;
            break;
          case 7:
            MinerList[i].moved = num;
            break;
        }

        line.erase(0, position + delimiter.length());
      }
    }

    //load game
    game = true;
    
    MyFile.close();
    std::cout << "Load Successful!\n";
    MySleep(2);
    return true;
  }
  
  catch (std::ifstream::failure const &e) {
    std::cerr << "Error opening/reading/closing file\n";
    MySleep(2);
    return false;
  }
}
///////////////////////////////////////////////////////////////////////////////

//engages miniboss fight
static bool Miniboss() {
  int health = 65 + upgrades[5] * 5;
  int damage = 15 + upgrades[0] * 5;
  std::cout << "Not fully implemented.\n";
  MySleep(3);
  return true;
}
///////////////////////////////////////////////////////////////////////////////

//engages boss fight
static bool Boss() {
  int health = 100 + upgrades[5] * 10;
  //int damage = 25 + upgrades[0] * 10;
  char input;

  std::cout << "Before you appears a sudden and bright cave opening.\n";
  std::cout << "You get an overwhelming sense of fear and can hear a faint ";
  std::cout << "humming coming from the entrance.\n\n";
  MySleep(3);
  std::cout << "You get the feeling that this might be something you won't";
  std::cout << " come out of.\nDo you want to continue? Enter Y if yes.\n";
  InputClear();
  std::cin >> input;

  if (!(input == 'y' || input == 'Y'))
    return false;

  std::cout << "You enter and there is a large being made of pure crystal.\n";
  std::cout << "It begins to move and its eyes begin to glow red.\n";
  std::cout << "You prepare to fight!\n";

  while (health != 0 && player["health"] != 0) {
    std::cout << "Unimplemented boss minigame.\n";
    health = 0;
  }

  game = false;
  player["died"] = 2;

  if (player["health"] <= 0)
    return false;
  
  return true;
}
///////////////////////////////////////////////////////////////////////////////

//player gets one get out of jail free card
static void Revive() {
  game = true;
  player["died"] = 2;
  player["health"] = player["maxHP"];

  std::cout << "\n\nYou slowly come to conciousness... You can tell you are ";
  std::cout << "in a lot of pain.\nYou start to open your eyes...\n\n";
  MySleep(4);

  std::cout << "You see a bright light and begin to hear a voice.\n";
  std::cout << "\"I cannot save you again... please stay safe.\"\n";
  std::cout << "With that, the light fades and you get back up again.\n";
  MySleep(4);

  grid[player["y"]][player["x"]] = MINED;
  player["x"] = GRID_UPPER/2;
  player["y"] = GRID_UPPER/2;
  grid[player["y"]][player["x"]] = PLAYER;
}
///////////////////////////////////////////////////////////////////////////////