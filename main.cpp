#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <optional>
#include <sstream>
#include <cmath>
using namespace std;

const int CELL_SIZE = 64;
const int ROWS = 6;
const int COLS = 6;

int bitVals[COLS] = {1, 2, 4, 8, 16, 32};

// Game states
enum GameState {
    MAIN_MENU,
    PLAYING,
    INSTRUCTIONS,
    GAME_OVER
};

GameState currentState = MAIN_MENU;
int menuSelection = 0;
const int MENU_OPTIONS = 3;

vector<vector<int>> grid(ROWS, vector<int>(COLS, 0));
sf::Vector2i playerPos(0, 0);

// Game state variables
int targetRow = 0;
int targetCol = 0;
bool gameWon = false;
bool showingResult = false;
bool hasAnswered = false;
int score = 0;
int highScore = 0;
sf::Clock resultTimer;
sf::Clock menuAnimationTimer;

void generateNewTarget() {
    targetRow = rand() % 64;  // 0-63 (6 bits)
    targetCol = rand() % 64;  // 0-63 (6 bits)
    gameWon = false;
    showingResult = false;
    hasAnswered = false;
}

void resetGame() {
    score = 0;
    playerPos = sf::Vector2i(0, 0);
    generateNewTarget();
    currentState = PLAYING;
}

void fillGrid() {
    for(int r = 0; r < ROWS; ++r){
        int rowBit = bitVals[ROWS - 1 - r];
        for(int c = 0; c < COLS; ++c){
            int colBit = bitVals[c];
            grid[r][c] = 1;
        }
    }
    
    // Override specific cells to create the pattern
    grid[0][0] = 0; grid[0][4] = 0; grid[0][5] = 0;
    grid[1][0] = 0; grid[1][4] = 0; grid[1][5] = 0;
    grid[2][0] = 0; grid[2][4] = 0; grid[2][5] = 0;
}

void drawMainMenu(sf::RenderWindow& window, sf::Font& font) {
    // Animated background effect
    float time = menuAnimationTimer.getElapsedTime().asSeconds();
    
    // Title
    sf::Text title(font, "BINARY GRID CHALLENGE", 48);
    title.setFillColor(sf::Color(255, 255, 255, 200 + 55 * std::sin(time * 2)));
    title.setPosition(sf::Vector2f(400 - title.getLocalBounds().size.x / 2, 100));
    window.draw(title);
    
    // Subtitle
    sf::Text subtitle(font, "Master the Art of Binary Logic", 20);
    subtitle.setFillColor(sf::Color::Cyan);
    subtitle.setPosition(sf::Vector2f(400 - subtitle.getLocalBounds().size.x / 2, 170));
    window.draw(subtitle);
    
    // Menu options
    vector<string> menuOptions = {"START GAME", "INSTRUCTIONS", "QUIT"};
    
    for(int i = 0; i < MENU_OPTIONS; i++) {
        sf::Text option(font, menuOptions[i], 28);
        
        if(i == menuSelection) {
            // Highlight selected option
            option.setFillColor(sf::Color::Yellow);
            option.setStyle(sf::Text::Bold);
            
            // Add animated arrow
            sf::Text arrow(font, ">>> ", 28);
            arrow.setFillColor(sf::Color::Yellow);
            arrow.setPosition(sf::Vector2f(250, 280 + i * 60));
            window.draw(arrow);
        } else {
            option.setFillColor(sf::Color::White);
        }
        
        option.setPosition(sf::Vector2f(300, 280 + i * 60));
        window.draw(option);
    }
    
    // High score display
    sf::Text highScoreText(font, "High Score: " + to_string(highScore), 18);
    highScoreText.setFillColor(sf::Color::Green);
    highScoreText.setPosition(sf::Vector2f(50, 500));
    window.draw(highScoreText);
    
    // Controls hint
    sf::Text controls(font, "Use UP/DOWN arrows to navigate, ENTER to select", 16);
    controls.setFillColor(sf::Color(150, 150, 150));
    controls.setPosition(sf::Vector2f(400 - controls.getLocalBounds().size.x / 2, 520));
    window.draw(controls);
}

void drawInstructions(sf::RenderWindow& window, sf::Font& font) {
    // Title
    sf::Text title(font, "HOW TO PLAY", 36);
    title.setFillColor(sf::Color::Yellow);
    title.setPosition(sf::Vector2f(400 - title.getLocalBounds().size.x / 2, 50));
    window.draw(title);
    
    // Instructions text
    vector<string> instructions = {
        "1. You'll see target ROW and COLUMN numbers (0-63)",
        "2. Convert these numbers to binary in your head",
        "3. Find where the row and column bits intersect",
        "4. Use arrow keys to move your cursor",
        "5. Press SPACE to submit your answer",
        "6. Blue squares show correct intersections",
        "7. Red squares = 1 bits, Green squares = 0 bits",
        "",
        "EXAMPLE:",
        "Target Row: 5 (binary: 000101) = bits 1 + 4",
        "Target Col: 3 (binary: 000011) = bits 1 + 2", 
        "Intersection: Where both have bit 1 set",
        "",
        "SCORING:",
        "Correct answer: +10 points",
        "Wrong answer: Try again or press N for new target"
    };
    
    for(int i = 0; i < instructions.size(); i++) {
        sf::Text line(font, instructions[i], 16);
        if(i < 7) {
            line.setFillColor(sf::Color::White);
        } else if(i >= 8 && i <= 12) {
            line.setFillColor(sf::Color::Cyan);
        } else {
            line.setFillColor(sf::Color::Green);
        }
        line.setPosition(sf::Vector2f(50, 120 + i * 22));
        window.draw(line);
    }
    
    // Back instruction
    sf::Text backText(font, "Press ESC to return to main menu", 18);
    backText.setFillColor(sf::Color::Yellow);
    backText.setPosition(sf::Vector2f(400 - backText.getLocalBounds().size.x / 2, 550));
    window.draw(backText);
}

void drawGrid(sf::RenderWindow& window, sf::Font& font){
    for(int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            sf::RectangleShape cell(sf::Vector2f(CELL_SIZE - 2, CELL_SIZE - 2));
            cell.setPosition(sf::Vector2f(c * CELL_SIZE + 100, r * CELL_SIZE + 100));
            
            if (!hasAnswered) {
                cell.setFillColor(sf::Color(128, 128, 128));
            } else {
                bool rowBitSet = (targetRow & bitVals[ROWS - 1 - r]) != 0;
                bool colBitSet = (targetCol & bitVals[c]) != 0;
                
                if (rowBitSet && colBitSet) {
                    cell.setFillColor(sf::Color::Blue);
                } else if (grid[r][c] == 1){
                    cell.setFillColor(sf::Color::Red);
                } else {
                    cell.setFillColor(sf::Color::Green);
                }
            }

            if (playerPos.x == c && playerPos.y == r){
                cell.setOutlineColor(sf::Color::Yellow);
                cell.setOutlineThickness(4);
            } else {
                cell.setOutlineColor(sf::Color::Black);
                cell.setOutlineThickness(2);
            }

            window.draw(cell);
            
            sf::Text bitText(font, to_string(bitVals[c]), 12);
            bitText.setFillColor(sf::Color::White);
            bitText.setPosition(sf::Vector2f(c * CELL_SIZE + 105, r * CELL_SIZE + 105));
            window.draw(bitText);
        }
    }
    
    for(int r = 0; r < ROWS; r++) {
        int rowBitValue = bitVals[ROWS - 1 - r];
        sf::Text rowText(font, to_string(rowBitValue), 16);
        rowText.setFillColor(sf::Color::White);
        rowText.setPosition(sf::Vector2f(50, r * CELL_SIZE + 115));
        window.draw(rowText);
    }
    
    for(int c = 0; c < COLS; c++) {
        sf::Text colText(font, to_string(bitVals[c]), 16);
        colText.setFillColor(sf::Color::White);
        colText.setPosition(sf::Vector2f(c * CELL_SIZE + 115, 50));
        window.draw(colText);
    }
}

void drawGameUI(sf::RenderWindow& window, sf::Font& font) {
    sf::Text targetText(font, "Target Row: " + to_string(targetRow) + " | Target Col: " + to_string(targetCol), 24);
    targetText.setFillColor(sf::Color::White);
    targetText.setPosition(sf::Vector2f(100, 10));
    window.draw(targetText);
    
    sf::Text scoreText(font, "Score: " + to_string(score), 20);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(sf::Vector2f(600, 10));
    window.draw(scoreText);
    
    sf::Text instructText(font, "Find intersection! SPACE=check, N=new target, ESC=menu", 16);
    instructText.setFillColor(sf::Color::Cyan);
    instructText.setPosition(sf::Vector2f(100, 500));
    window.draw(instructText);
    
    int currentRowBit = bitVals[ROWS - 1 - playerPos.y];
    int currentColBit = bitVals[playerPos.x];
    
    sf::Text posText(font, "Current Position: Row bit " + to_string(currentRowBit) + 
                     ", Col bit " + to_string(currentColBit), 16);
    posText.setFillColor(sf::Color::Yellow);
    posText.setPosition(sf::Vector2f(100, 520));
    window.draw(posText);
    
    if (showingResult) {
        sf::Text resultText(font);
        if (gameWon) {
            resultText.setString("CORRECT! Well done! +10 points");
            resultText.setFillColor(sf::Color::Green);
        } else {
            resultText.setString("Wrong position! Try again. Press N for new target.");
            resultText.setFillColor(sf::Color::Red);
        }
        resultText.setCharacterSize(20);
        resultText.setPosition(sf::Vector2f(100, 450));
        window.draw(resultText);
    }
}

void checkAnswer() {
    hasAnswered = true;
    
    int currentRowBit = bitVals[ROWS - 1 - playerPos.y];
    int currentColBit = bitVals[playerPos.x];
    
    bool rowBitSet = (targetRow & currentRowBit) != 0;
    bool colBitSet = (targetCol & currentColBit) != 0;
    
    if (rowBitSet && colBitSet) {
        gameWon = true;
        score += 10;
        if (score > highScore) {
            highScore = score;
        }
        showingResult = true;
        resultTimer.restart();
    } else {
        gameWon = false;
        showingResult = true;
        resultTimer.restart();
    }
}

void handleMenuInput(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::Key::Up) {
        menuSelection = (menuSelection - 1 + MENU_OPTIONS) % MENU_OPTIONS;
    }
    if (key == sf::Keyboard::Key::Down) {
        menuSelection = (menuSelection + 1) % MENU_OPTIONS;
    }
    if (key == sf::Keyboard::Key::Enter) {
        switch(menuSelection) {
            case 0: // START GAME
                resetGame();
                break;
            case 1: // INSTRUCTIONS
                currentState = INSTRUCTIONS;
                break;
            case 2: // QUIT
                exit(0);
                break;
        }
    }
}

void handleGameInput(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::Key::Escape) {
        currentState = MAIN_MENU;
        return;
    }
    
    if (!showingResult) {
        if (key == sf::Keyboard::Key::Left && playerPos.x > 0) playerPos.x--;
        if (key == sf::Keyboard::Key::Right && playerPos.x < COLS - 1) playerPos.x++;
        if (key == sf::Keyboard::Key::Up && playerPos.y > 0) playerPos.y--;
        if (key == sf::Keyboard::Key::Down && playerPos.y < ROWS - 1) playerPos.y++;
        
        if (key == sf::Keyboard::Key::Space) {
            checkAnswer();
        }
    }
    
    if (key == sf::Keyboard::Key::N) {
        generateNewTarget();
    }
}

void handleInstructionsInput(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::Key::Escape) {
        currentState = MAIN_MENU;
    }
}

int main() {
    srand(time(nullptr));
    
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Binary Grid Challenge");
    fillGrid();
    
    sf::Font font;
    if (!font.openFromFile("arial.ttf")) {
        return -1;
    }
    
    menuAnimationTimer.restart();
    
    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                switch(currentState) {
                    case MAIN_MENU:
                        handleMenuInput(keyPressed->code);
                        break;
                    case PLAYING:
                        handleGameInput(keyPressed->code);
                        break;
                    case INSTRUCTIONS:
                        handleInstructionsInput(keyPressed->code);
                        break;
                }
            }
        }
        
        // Auto-generate new target after showing result for 3 seconds (only if correct)
        if (currentState == PLAYING && showingResult && gameWon && 
            resultTimer.getElapsedTime().asSeconds() > 3.0f) {
            generateNewTarget();
        }
        
        window.clear(sf::Color::Black);
        
        // Render based on current state
        switch(currentState) {
            case MAIN_MENU:
                drawMainMenu(window, font);
                break;
            case PLAYING:
                drawGrid(window, font);
                drawGameUI(window, font);
                break;
            case INSTRUCTIONS:
                drawInstructions(window, font);
                break;
        }
        
        window.display();
    }
    
    return 0;
}