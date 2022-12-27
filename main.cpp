//Personal Project by Luka Brown
//The Deep Below
//Mining Game
//In development, should work on both windows and linux
//Feel free to compile and playtest!


/* Gameplay:

Explore an (almost) endless mass of blocks and dig your way around!
The game starts with a grid output to a console with 'P' for player in the
middle. 

Controls:
  Use WASD to move.
  Press q to exit game.
  Press o to output to grid.txt (prints map, ints based on block types)

Go towards * if you see them, they're rare and sparkly
$ is a friendly shop!
+ is an enemy miner, beware!
Have fun!

Implemented features:
  Output grid to file
  Rogue Enemy Miners to fight
  Shop system
    can sell ore
    can buy artifacts
    can trade artifacts for upgrades
  Upgrades (3/5)
    Extra damage
    Extra sight
    clarity (sees all special blocks more often)
  End Score System  
  Main Menu
  Saving and Loading from a file

Agenda, in particular order:
  Encrypted files
  Upgrades (2/5)
    mining width
    mining depth
  Minibosses to fight
  Boss to fight
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
static bool SaveGame(std::string name);
static bool LoadGame(std::string name);
//game functions
static int  GameLoop();
static void Move(int x);
static bool CollectItem(int y, int x);
static void GameReport();
static void TitleScreen();
//map/grid functions
static void GenerateGrid();
static void PrintGrid();
static void SaveGrid();
//shop functions
static void CallShop();
static void SellOre();
static void BuyArtifacts();
static void Trade();
static void Upgrade(int x);
//enemy miner functions
static void InitMiner(Rogue &miner);
static void MoveMiners();
static void MoveMiner(Rogue &miner);
static bool ProcessBlock(Rogue &miner, int y, int x);
static bool MinerFight(int y, int x);

//Constants
static const int GRID_UPPER = 2000; //2000x2000 grid, 4 million blocks
static const int MINERS = 2000; //num of enemy miners in the map
static const int UPGRADE_UPPER = 5; //num of upgrades implemented

//"block" types
#define PLAYER   0
#define DIRT     1
#define MINED    2 //mined dirt
#define SHOP     3
#define ARTIFACT 4
#define ORE      5
#define MINER    6 //enemy

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

  //creates vector of all the enemy miners
  for (int i = 0; i < MINERS; i++) {
    Rogue miner;
    InitMiner(miner);
    MinerList.push_back(miner);
  }

  while(game) {
    MySleep(.2);
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
      case 6:
        TitleScreen();
        break;
    } //end switch
    
    MoveMiners();
    PrintGrid();
  }// end game while
  
  SaveGrid();
  GameReport();
  return 0;
}
///////////////////////////////////////////////////////////////////////////////

//creates 2d vector of blocks
static void GenerateGrid() {
  int y, x;
  int random;
  srand((unsigned int)time(NULL)); //seeds random

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
  grid[player["y"]][player["x"]] = PLAYER; //sets player position
  return;
}
///////////////////////////////////////////////////////////////////////////////

//prints blocks around the player
static void PrintGrid() {
  std::cout << "\n\n\n\n\n\n";
  int y, x, chance;

  grid[player["y"]][player["x"]] = PLAYER;

  for (y = player["y"]-player["sight"]; y < player["y"]+player["sight"]+1; y++) {
    if (y > GRID_UPPER-1)
      y = GRID_UPPER-1;
    else if (y < 0)
      y = 0;

    for (x = player["x"]-player["sight"]; x < player["x"]+player["sight"]+1; x++) {
      if (x > GRID_UPPER-1)
        x = GRID_UPPER-1;
      else if (x < 0)
        x = 0;

      switch(grid[y][x]) {
        case PLAYER:
          std::cout << "P ";
          break;
        case DIRT:
          std::cout << "# "; //□_
          break;
        case MINED:
          std::cout << "  ";
          break;
        case ARTIFACT: //ore and artifact have chance of not showing up
        case ORE:
          chance = rand() % (8 - (upgrades[4]*2)); //chance goes from 1/8 to 1/6 
          if (chance == 0)       //to 1/4 to 1/2 chance of showing up on the map
            std::cout << "* ";
          else
            std::cout << "# ";
          break;
        case SHOP:
          std::cout << "$ ";
          break;
        case MINER:
          std::cout << "+ ";
          break;
      } //end switch
    } //end for x
    std::cout << '\n';
  } //end for y

  std::cout << "Ore: " << player["ore"] << " Artifacts: " << player["artifacts"];
  std::cout << " Coins: " << player["coins"] << " HP: " << player["health"] << '\n';
  return;
}
///////////////////////////////////////////////////////////////////////////////

//catches player movement and sends back
static int GameLoop() {
  char input;
  InputClear();
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
    case 't':
    case 'T':
      return 6;
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
    case 1: //w, up
      if (player["y"] > player["sight"]) { //makes sure it wont exceed map bounds
        valid = CollectItem(player["y"]-1,player["x"]); //processes block stepped on
        if (valid) {
          grid[player["y"]-1][player["x"]] = PLAYER;
          grid[player["y"]][player["x"]] = MINED;
          player["y"]--;
        }
      }
      break;
    case 2: //a, left
      if (player["x"] > player["sight"]) {
        valid = CollectItem(player["y"],player["x"]-1);
        if (valid) {
          grid[player["y"]][player["x"]-1] = PLAYER;
          grid[player["y"]][player["x"]] = MINED;
          player["x"]--;
        }
      }
      break;
    case 3: //s, down
      if (player["y"] < GRID_UPPER-player["sight"]-1) {
        valid = CollectItem(player["y"]+1,player["x"]);
        if (valid) {
          grid[player["y"]+1][player["x"]] = PLAYER;
          grid[player["y"]][player["x"]] = MINED;
          player["y"]++;
        }
      }
      break;
    case 4: //d, right
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
      MySleep(1.3);
    } 
    else if (z == 1) { //1% chance ore in dirt
      std::cout << "\nWhile digging, you found a rare ore!" << '\n';
      player["ore"]++;
      MySleep(1.3);
    }
      
    //98% chance dirt
    player["dirt"]++;
  } 

  else if (grid[y][x] == SHOP) {
    CallShop();
  } 

  else if (grid[y][x] == ARTIFACT) {
    std::cout << "\nWhile digging, you found an ancient artifact!" << '\n';
    MySleep(1);
    player["artifacts"]++;
  }

  else if (grid[y][x] == ORE) {
    std::cout << "\nWhile digging, you found a rare ore!" << '\n';
    MySleep(1);
    player["ore"]++;
  } 

  else if (grid[y][x] == MINER) {
    return MinerFight(y,x); //valid only if miner dies
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
      MySleep(2);
      
      std::cout << "\nThe shop owner poins to a sign that reads: Pick:\n";
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
            MySleep(2);
          }
          
        } else //q or 4
          finish = true;

      } else { //invalid input
        std::cout << "Sorry, I didn't get that. Which number did you want?\n\n";
        MySleep(2);
      } //end input if/else
    } //end store while loop
  } //end if x == y

  std::cout << "Alright, see you later. Oh, and grab some bread on your way out. Keeps ya hardy.\n";
  MySleep(2);

  player["health"] = player["maxHP"]; //heals player
  std::cout << "The bread looks delicious. You grab some and take a bite. On your way out you feel";
  std::cout << " refreshed. Ahh, bread.\n";

  MySleep(2);
  return;
}
///////////////////////////////////////////////////////////////////////////////

//initializes globals and calls GenMap
static void Init() {
  //set globals
  player["x"] = player["y"] = GRID_UPPER/2;//player starting in middle of the map
  player["sight"] = 4; //sees 4 blocks in any direction
  player["damage"] = 10;
  player["ore"] = player["artifacts"] = player["dirt"] = player["coins"] = 0;
  player["kills"] = 0;
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
  int implementedUpgrades = 5; //# of finished upgrades. max 10

  //cost ranges from 15-35
  //nums 16-29 have a higher probability
  int cost = rand() % 35;
  if (cost < 15) { cost += 15; }

  std::cout << "\nOkay, I only have one fine deal for you.\n";
  std::cout << "If you have " << cost << " ancient artifacts then I may consider selling...\n";
  std::cout << "The only item that would help you is a magnificent Upgrade!\n\n";

  int random = rand() % implementedUpgrades; //0-3, picks what upgrade the shop has

  if (upgrades[random] >= 3) { //3 is the max level an upgrade can achieve
    std::cout << "Oh... It looks like you already have the upgrade I was going to offer.\n";
    std::cout << "Looks like I have nothing special, sorry!\n";
    MySleep(2);
  } 
  else {
    switch(random) {
      case 0:
        std::cout << "This enhancement will augment your weapon to be far superior.\n";
        break;
      case 1:
        std::cout << "This enhancement will give you extra sight in the mines.\n";
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
        std::cout << "This enhancement will allow you to see more ore.\n";
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
  MySleep(2);
  std::cout << "   Enter T to go to the title screen\n";
  MySleep(2);
  std::cout << "   Enter Q to quit and get a final score\n";
  MySleep(2);
  std::cout << "\nGood Luck Mining!";
  MySleep(2);

  PrintGrid();
}
///////////////////////////////////////////////////////////////////////////////

//creates initial values for the enemy miners
//parameters: miner to be initalized
static void InitMiner(Rogue &miner) {
  miner.artifacts = 0;
  miner.coins = 25;
  miner.damage = 8;
  miner.ore = 0;
  miner.health = 30;
  miner.x = rand() % 2000;
  miner.y = rand() % 2000;
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
      MySleep(1);
      std::cout << "They don't seem to notice you and continue swinging their pickaxe";
      std::cout << " even though you are in their way.\n";
      MySleep(1);
      std::cout << "You take " << miner.damage << " damage from the miner.\n\n";
      MySleep(3);
      player["health"] -= miner.damage;

      //player death
      if (player["health"] <= 0) {
        game = false;
        player["health"] = 0;
        std::cout << "You've taken too much damage and the miner is merciless.\n";
        std::cout << "You fall to the ground and the miner continues on their way\n";
        MySleep(3);
      }
      return false;
    case MINER: //doesn't allow miner overlap
      return false;
    default:
      break;
  }
  return true;
}
///////////////////////////////////////////////////////////////////////////////

//outputs the current grid to a file
static void SaveGrid() {
  std::ofstream MyFile("grid.txt");
  for (int y = 0; y < GRID_UPPER; y++) {
    for (int x = 0; x < GRID_UPPER; x++) {
      MyFile << grid[y][x];
    }
    MyFile << '\n';
  }
  MyFile.close();
}
///////////////////////////////////////////////////////////////////////////////

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
    MySleep(2);
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
  MySleep(2);
  MinerList[enemyIndex].health -= damage;

  if (MinerList[enemyIndex].health <= 0) { //if miner dies
    std::cout << "AAAGH... the miner lets out a last scream before falling down.\n";
    std::cout << "They won't be getting back up from that.\n";
    MySleep(2);

    MinerList[enemyIndex].health = 0;
    MinerList[enemyIndex].x = -1;
    MinerList[enemyIndex].y = -1;
    player["kills"]++;

    std::cout << "You gain " << MinerList[enemyIndex].coins << " coins.\n";
    MySleep(2);

    player["coins"] += MinerList[enemyIndex].coins;
    return true;
  } 
  else { //miner lives and retaliates
    damage = rand() % 4;
    damage += 6;

    std::cout << "The miner swings their pick back and dealt " << damage;
    std::cout << " damage!\n";
    MySleep(2);

    player["health"] -= damage;
    if (player["health"] <= 0) {
      game = false;

      std::cout << "The pick impales you and leaves you with a gash too wide to mend.";
      MySleep(2);

      std::cout << "\nYou fall down on the hard rocks and reflect as you die.\n";
      MySleep(3);
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
static void TitleScreen() {
  char input;
  int upg = 0;

  std::cout << "\n\n\n\n\n\n\n\nThe Deep Below\n\nTitle Screen:\n";
  std::cout << "1. Resume Game\n";
  std::cout << "2. Save Game\n";
  std::cout << "3. Load Game\n";
  std::cout << "4. Output Map\n";
  std::cout << "5. View Stats\n";
  std::cout << "6. Exit Game\n";
  std::cout << "What would you like to do? Enter the number.\n";

  InputClear();
  std::cin >> input;
  input -= 1;

  switch (input) {
    case '0':
      break;
    case '1':
      std::cout << "Saving...\n";
      MySleep(1);
      SaveGame("save.txt");
      break;
    case '2':
      std::cout << "Loading...\n";
      MySleep(1);
      LoadGame("save.txt");
      break;
    case '3':
      std::cout << "Ouputting Map to grid.txt\n";
      MySleep(2);
      SaveGrid();
      break;
    case '4':
      for (int i = 0; i < UPGRADE_UPPER; i++) {
        if (upgrades[i] > 0) {
          upg += upgrades[i];
        }
      }

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
      break;
    case '5':
      game = false;
      break;
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
    MyFile << player["x"] << ',' << player["y"] << ',' << player["sight"] << ',';
    MyFile << player["damage"] << ',' << player["ore"] << ',' << player["dirt"];
    MyFile << ',' << player["artifacts"] << ',' << player["coins"] << ',';
    MyFile << player["kills"] << ','<< player["health"] << ',' << player["maxHP"] << '\n';

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
    std::cout << "Save Successful!";
    MySleep(2);
    return true;
  }

  catch (std::ofstream::failure e) {
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
      
    for (int j = 0; j < 11; j++) {
      num = 0;
      position = line.find(delimiter);
      item = line.substr(0, position);

      for (int i = 0; item[i] != '\0'; i++) {
        num = num * 10 + item[i] - '0';
      }

      switch (j) {
        case 0:
          player["x"] = num;
          break;
        case 1:
          player["y"] = num;
          break;
        case 2:
          player["sight"] = num;
          break;
        case 3:
          player["damage"] = num;
          break;
        case 4:
          player["ore"] = num;
          break;
        case 5:
          player["dirt"] = num;
          break;
        case 6:
          player["artifacts"] = num;
          break;
        case 7:
          player["coins"] = num;
          break;
        case 8:
          player["kills"] = num;
          break;
        case 9:
          player["health"] = num;
          break;
        case 10:
          player["maxHP"] = num;
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
    std::cout << "Load Successful!";
    MySleep(2);
    return true;
  }
  
  catch (std::ifstream::failure e) {
    std::cerr << "Error opening/reading/closing file\n";
    MySleep(2);
    return false;
  }
}
///////////////////////////////////////////////////////////////////////////////