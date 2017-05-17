/*
I need to
- improve the selection of tiles (probably not going to touch that)
*/

#include <SFML/Graphics.hpp> //http://www.sfml-dev.org/tutorials/2.4
#include <cstring> //For memset
#include <thread> //For threads, and sleeping
#include <chrono> //For thread sleeping
#include <iostream> //For output to the terminal
typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned long ulong;

const byte pW = 12; //pile width
const byte pH = 8; //pile height
const byte pZ = pW / 2; //pile stack height
byte pile[pW][pH][pZ];
const uint sSS[2] = {34, 49}; //spriteSheetSize
const uint sSL[2] = {9, 4}; //spriteSheetLayout
const byte uniqueTiles = 34;
uint aPW = (pW + 1) * sSS[0]; //actual pile width
uint aPH = (pH + 1) * sSS[1]; //actual pile height
byte sel[3] = {0, 0, 0};
uint tiles = 0; //Amount of remaining tiles on the pile
uint initTiles; //The initial amount of tiles

uint gWidth = 500;
uint gHeight = 500;
uint xMargin = (gWidth / 2) - (aPW / 2);
uint yMargin = (gHeight / 2) - (aPH / 2);
const byte zOffset = 3;
const byte shadow = 5;
const byte grad = (55 / (pZ / 2)); //Gradient for tiles
byte tGrad; //For calculation
sf::RenderWindow window(sf::VideoMode(gWidth, gHeight), "Pahjong");
sf::Font arialFont; //For outputting text
sf::Texture tileTexture;
sf::RectangleShape displayTile(sf::Vector2f(sSS[0], sSS[1]));
sf::RectangleShape shadowTile(sf::Vector2f(sSS[0], sSS[1]));

sf::Clock gameClock;
uint pauseStart = 0;
uint pauseAdjust = 0;
uint penalties = 0;
std::string outText = ""; //Cache of text output
sf::Text displayText; //Display text
uint pairsFound = 0;
bool paused = false;
bool gameover = false;

//===================================
//For optimisation/approx/mathematical code
//===================================
uint g_seed = time(NULL);
inline uint frand()
{
  g_seed = (214013 * g_seed + 2531011);
  return (g_seed >> 16) & 0x7FFF;
}

inline bool rDo(byte chance)
{
    return !(frand() % chance);
}
//===================================

uint pz, px, py, t, xp, yp, p;

bool isFree(uint mx, uint my, uint mz)
{
    if (!pile[mx][my][mz]) { return false; }
    if (mx == 0 || mx == pW - 1) { return true; } //On the edges
    if (pile[mx - 1][my][mz] && pile[mx + 1][my][mz]) { return false; } //Long edges are both covered
    if (mz == pZ) { return true; } //On the top
    if (pile[mx][my][mz + 1]) { return false; } //Is covered
    return true;
}

bool freeTiles[pW][pH][pZ]; //Which tiles are selectable
uint freeTs = 0; //Amount of selectable tiles
bool showFree = false; //Hint flag
void findFreePairs()
{
    freeTs = 0;
    byte tiles[uniqueTiles] = {0};
    for (pz = 0; pz < pZ; ++pz)
    {
        for (px = 0; px < pW; ++px)
        {
            for (py = 0; py < pH; ++py)
            {
                freeTiles[px][py][pz] = false;
                if (isFree(px, py, pz))
                {
                    t = pile[px][py][pz];
                    if (t)
                    {
                        ++tiles[t];
                        if (tiles[t] == 2)
                        {
                            freeTiles[px][py][pz] = true;
                            ++freeTs;
                        }
                    }
                }
            }
        }
    }
}

const byte uT = uniqueTiles + 1;
void randomisePile()
{
  //First, generate an array of pairs
    byte pairs[tiles] = {0};
    byte tilei = 1;
    for (p = 0; p < tiles; ++p) //Firstly, assign
    {
        pairs[p] = tilei;
        if (p + 1 < tiles) { pairs[p + 1] = tilei; ++p; }
        ++tilei;
//std::cout << std::to_string(pairs[p]) << std::endl;
        if (tilei > uT) { tilei = 1; }
    }
  //Randomise the pairs
    uint ri;
    uint temp;
    for (p = 0; p < tiles; ++p)
    {
        ri = frand() % tiles;
        temp = pairs[p];
        pairs[p] = pairs[ri];
        pairs[ri] = temp;
    }
  //Now assign tiles with a random pair
    uint ti = 0;
    for (pz = 0; pz < pZ; ++pz)
    {
        for (px = 0; px < pW; ++px)
        {
            for (py = 0; py < pH; ++py)
            {
                if (pile[px][py][pz])
                {
                    pile[px][py][pz] = pairs[++ti]; //(frand() % uniqueTiles) + 1;
//std::cout << std::to_string(pairs[++ti]) << std::endl;
                }
            }
        }
    }
    findFreePairs();
    if (!freeTs) { randomisePile(); } //Keep randomising until we have a good pile
}

std::string previousCompileTime;
std::string compileTime()
{
    if (!paused && !gameover)
    {
        uint now = ((int)gameClock.getElapsedTime().asSeconds() - pauseAdjust) + penalties;
        uint seconds = now % 60;
        uint minutes = now / 60;
        previousCompileTime = (minutes < 10 ? "0" : "") + std::to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + std::to_string(seconds);
    }
    return previousCompileTime;
}


void pollClose()
{
    sf::Event event;
    while (window.pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
            window.close();
    }
}

ulong timer = 0;
bool redraw = true;
int main()
{
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    window.setPosition(sf::Vector2i((desktop.width / 2) - (gWidth / 2), (desktop.height / 2) - (gHeight / 2)));
  //Load font
    if (!arialFont.loadFromFile("arial.ttf"))
    {
        std::cout << "Couldn't load font" << std::endl;
    }
    displayText.setFont(arialFont);
    displayText.setFillColor(sf::Color::Black);
    displayText.setCharacterSize(24);
  //Load sprites & shadows
    if (!tileTexture.loadFromFile("sprites.png"))
    {
        std::cout << "Couldn't load sprites" << std::endl;
    }
    shadowTile.setFillColor(sf::Color(0, 0, 0, 100));
    displayTile.setOutlineThickness(1);
    displayTile.setOutlineColor(sf::Color(0, 0, 0));
    displayTile.setTexture(&tileTexture);
  //Prepare pile for generation
    for (pz = 0; pz < pZ; ++pz)
    {
        for (px = 0; px < pW; ++px)
        {
            for (py = 0; py < pH; ++py)
            {
                if (px > pz && px < pW - pz && py > pz && py < pH - pz)
                {
                    pile[px][py][pz] = 1;
                    ++tiles;
                } else {
                    pile[px][py][pz] = 0;
                }
            }
        }
    }
    initTiles = tiles;
  //Randomise pile
    randomisePile();

    while (window.isOpen())
    {
        pollClose();
      //Monitor keyboard
        /*if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && timer > 3) //Reshuffle?
        {
            randomisePile();
            timer = 0;
            redraw = true;
        }*/
      //Monitor mouse
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && timer > 10) //LEFT CLICK
        {
            showFree = false;
          //Get position
            sf::Vector2i localPosition = sf::Mouse::getPosition(window);
            uint mx = localPosition.x;
            uint my = localPosition.y;
            uint mz;
            if (mx > xMargin && mx < xMargin + aPW && my > yMargin && my < yMargin + aPH)
            {
              //Find which tile they're selecting
                mx -= xMargin;
                my -= yMargin;
                mx /= sSS[0];
                my /= sSS[1];
              //Find the z
                for (pz = pZ - 1; pz >= 0; pz--)
                {
                    if (pz == -1) { break; }
                    if (pile[mx][my][pz] != 0)
                    {
                        mz = pz;
                        break;
                    }
                }
                bool match = false;
                bool selectable = isFree(mx, my, mz);
                if (!(sel[0] == mx && sel[1] == my && sel[2] == mz)) //Is this not the same tile as already selected?
                {
                    if (pile[sel[0]][sel[1]][sel[2]] == pile[mx][my][mz]) //Did the user find a match?
                    {
                        if (selectable) //Is this tile available for selection?
                        {
                            pile[sel[0]][sel[1]][sel[2]] = 0;
                            pile[mx][my][mz] = 0;
                            match = true;
                            ++pairsFound;
                            tiles -= 2;
                            if (!tiles) //Have we completed the game?
                            {
                                gameover = true;
                            } else {
                                findFreePairs();
                                if (!freeTs)
                                {
                                    randomisePile();
                                    timer = 0;
                                    redraw = true;
                                }
                            }
                        }
                    }
                }
                if (!match && selectable)
                {
                    sel[0] = mx;
                    sel[1] = my;
                    sel[2] = mz;
                }
            }
            timer = 0;
            redraw = true;
        }
        if (sf::Mouse::isButtonPressed(sf::Mouse::Right) && timer > 15) //RIGHT CLICK
        {
            showFree = !showFree;
            timer = 0;
            redraw = true;
            if (showFree) { penalties += 5; }
        }
      //Draw the scene
        if (timer > 100) { redraw = true; timer = 0; } //Redraw for the clock?
        if (redraw)
        {
            redraw = false;
            if (!window.hasFocus() && !paused) //Should we enter pause (we've lost focus, and we're not yet paused)?
            {
                paused = true;
                pauseStart = (int)gameClock.getElapsedTime().asSeconds();
            } else if (paused && window.hasFocus()) { //Should we exit pause? (we're paused, but we've gained focus)
                paused = false;
              //Calculate the time between now and the pause start
                pauseAdjust += (int)gameClock.getElapsedTime().asSeconds() - pauseStart;
            }
            window.clear(paused ? sf::Color(128, 128, 128) : sf::Color(255, 255, 255));
            for (pz = 0; pz < pZ; ++pz)
            {
                for (py = 0; py < pH; ++py)
                {
                    for (px = 0; px < pW; ++px)
                    {
                        t = pile[px][py][pz];
                        if (t != 0)
                        {
                            t--;
                          //Calculate gradient
                            tGrad = 200 + (grad * pz);
                            displayTile.setFillColor(sf::Color(tGrad, tGrad, tGrad));
                          //Colour if selected in some way
                            if (sel[0] == px && sel[1] == py && sel[2] == pz) { displayTile.setFillColor(sf::Color(255, 200, 255)); }
                            if (showFree && freeTiles[px][py][pz]) { displayTile.setFillColor(sf::Color(200, 255, 200)); }
                          //Select the correct texture
                            if (!paused)
                            {
                                displayTile.setTextureRect(sf::IntRect(sSS[0] * (t % sSL[0]), sSS[1] * (t / sSL[0]), sSS[0], sSS[1]));
                            } else {
                                displayTile.setTextureRect(sf::IntRect(sSS[0] * sSL[0], sSS[1] * sSL[0], sSS[0], sSS[1]));
                            }
                          //Calculate position, and draw
                            xp = xMargin + (px * sSS[0]) - (pz * zOffset);
                            yp = yMargin + (py * sSS[1]) - (pz * zOffset);
                            displayTile.setPosition(xp, yp);
                            shadowTile.setPosition(xp + shadow, yp + shadow);
                            window.draw(shadowTile);
                            window.draw(displayTile);
                        }
                    }
                }
            }

          //Compile info text
            outText = compileTime() + "   " + std::to_string(pairsFound) + "/" + std::to_string(initTiles / 2) + " found   " + std::to_string(freeTs) + " free";
            displayText.setString(outText);
            window.draw(displayText);

            window.display();
        }
        sf::sleep(sf::milliseconds(10));
        ++timer;
        while (gameover)
        {
            window.clear(sf::Color(255, 255, 255));
            displayText.setString(compileTime());
            window.draw(displayText);
            window.display();
            sf::sleep(sf::milliseconds(100));
            pollClose();
        }
    }

    return 0;
}
