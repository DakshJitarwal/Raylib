#include "raylib.h"
#include <math.h>

#define MAX_PLATFORMS 5

//linkedlist containing platforms
typedef struct PlatformNode {
    Rectangle rect;
    struct PlatformNode* next;
} PlatformNode;

// Create 5 static nodes
PlatformNode nodes[5]; //this is an array containing 5 nodes
PlatformNode* head = &nodes[0]; //initialize a pointer to first node
PlatformNode* nextToSpawn = &nodes[0]; //Pointer to the next plattform to display

typedef struct {
    Vector2 pos;
    Vector2 velocity;
    float rotation;
    bool active;
    bool hasTargeted; // Tracks if he already changed his angle
    float width;
    float height;
} Boss;

Boss dineshBoss = { {0, 0}, {0, 0}, 0.0f, false, false, 180, 135 };    


float spawnTimer = 0.0f;
float spawnInterval = 5.0f; //interval bw 2 plattforms

typedef enum { STATE_OFFICE, STATE_MIDDLE, STATE_FALLING, STATE_LANDED, STATE_RUNNING, STATE_CUTSCENE, STATE_INTRO, STATE_GLIDING, STATE_GAMEOVER} GameState;

int main(void) {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Ava's Birthday Escape - Fixed Animation");

    float globalScale = 0.6f;
    
    float standardHeight = 200.0f * globalScale; //Variable to set height of all character images as same
    float standardWidth = standardHeight * 0.8f;

    // ASSET LOADING 
    Image img1 = LoadImage("office.png"); //This stores image in RAM
    int officeH = (int)(140 * globalScale);
    ImageResize(&img1, (img1.width * officeH / img1.height), officeH); //This function operates on image in RAM, since office is static image
    Texture2D texOffice = LoadTextureFromImage(img1); //This loads the image from RAM to GPU
    UnloadImage(img1);      //This unloads the image from RAM, freeing up its space

    Image desk = LoadImage("desk.png");
    int deskH = (int)(220 * globalScale);
    ImageResize(&desk, (desk.width * deskH / desk.height), deskH);
    Texture2D textDesk = LoadTextureFromImage(desk);
    UnloadImage(desk);

    
    Image standing = LoadImage("standing.png");
    float aspect = (float)standing.width / (float)standing.height;
    ImageResize(&standing, (int)((standardHeight+20)* aspect), (int)(standardHeight+10));
    //ImageResize(&standing, (standing.width * standingH / standing.height), standingH);
    Texture2D textStanding = LoadTextureFromImage(standing);
    UnloadImage(standing);

    Image imgFall = LoadImage("falling.png");
    Texture2D texFall = LoadTextureFromImage(imgFall);
    UnloadImage(imgFall);

    Image imgRun = LoadImage("running.png");
    Texture2D texRun = LoadTextureFromImage(imgRun);
    UnloadImage(imgRun);

    Image imgIntro = LoadImage("dinesh.png");
    // Scale it similar to Game Over image
    int targetIntroH = 300;
    ImageResize(&imgIntro, (imgIntro.width * targetIntroH / imgIntro.height), targetIntroH);
    Texture2D texIntro = LoadTextureFromImage(imgIntro);
    UnloadImage(imgIntro);

    Image imgGlide = LoadImage("glide.png");
    Texture2D textGlide = LoadTextureFromImage(imgGlide);
    UnloadImage(imgGlide);

    Image imgBoss = LoadImage("dinesh.png");
    ImageResize(&imgBoss, 120, 100); 
    Texture2D texDinesh = LoadTextureFromImage(imgBoss);
    UnloadImage(imgBoss);

    Image imgGameOver = LoadImage("game_over.png");
    int targetGOH = 300;
    ImageResize(&imgGameOver, (imgGameOver.width * targetGOH / imgGameOver.height), targetGOH);
    Texture2D texGameOver = LoadTextureFromImage(imgGameOver);
    UnloadImage(imgGameOver);

    // VARIABLES 
    GameState currentState = STATE_OFFICE; //Initialize the state of the game

    float scrollSpeed = 300.0f; // world speed
    float backgroundOffset = 0.0f; //this changes with scrollspeed, and decreases with every iteration of the loop, so that things appear to be moving backwards
    
    //coordinates of position of the office desk image and clock
    Vector2 deskPos = { 200, 20 }; 
    Vector2 clockCenter = { 200, 60 * globalScale + 10 }; 
    float currentHour = 9.0f;
    float elapsedOfficeTime = 0.0f;

    float middleTimer = 0.0f; //Need to keep the middle state frame for 2 seconds
    Vector2 tablePos = {150,7};  //position of the desk in the middle state
    Vector2 standingPos = {80, 7};

    //coordinates of falling prajakta
    Vector2 avaPos = { 300 * globalScale, 20 }; 
    float velocityY = 0.0f;
    float gravity = 1800.0f;
    float groundLevel = 360.0f;
    float feetPadding = 30.0f * globalScale;
    
    bool hasTeleported = false;
    float cutsceneTimer = 0.0f;

    float glidingGravity = 600.0f; 

    int animFrame = 0;
    float animTimer = 0.0f;

    int flightCount = 0; // Tracks how many times Ava has jumped off of a plattform, to make the game harder over time

    //game over variables
    int currentSentence = 0;      // 0 to 3
    int charsToPrint = 0;
    float typewriterTimer = 0.0f;
    bool allCharsDisplayed = false;

    //Dinesh Intro text
    const char* introSentence = "Where are you going deweeb? Come back! I'll chase you to the end of the world!!";

    const char* sentences[4];
    sentences[0] = "Too bad, you finally got caught";
    sentences[1] = "It was an impressive flight though, I gotta give you that";
    sentences[2] = TextFormat("I'm impressed! Here is your raise - %d%%", flightCount); // Example: 5% per flight
    sentences[3] = "You have to work overtime to earn it though, of course *wink wink*";

    //linking the platform nodes in a circular fashion
    for (int i = 0; i < 5; i++) {
        // Park them way off-screen at -2000 so they aren't visible. This is to initialize rectangle objects before drawing them
        nodes[i].rect = (Rectangle){ -2000, 0, 1000, 10 };
        
        // Circular linking
        if (i < 4) nodes[i].next = &nodes[i + 1];
        else nodes[i].next = &nodes[0]; 
    }

    PlatformNode* curr = head; //Pointer to current plattform that is being displayed


    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        switch (currentState) {
            case STATE_OFFICE:
                elapsedOfficeTime += dt;
                currentHour = 9.0f + (elapsedOfficeTime / 5.0f) * 8.0f; 
                if (currentHour >= 17.0f) {
                    currentState = STATE_MIDDLE;
                    // Position her where she sat
                    avaPos = (Vector2){ 100 * globalScale + 30, 80 }; 
                    animFrame = 0;
                    animTimer = 0;
                    hasTeleported = false;
                }
                break;

            case STATE_MIDDLE:
                middleTimer += dt;
                if(middleTimer >= 2.0f){
                    currentState = STATE_FALLING;

                }
                break;

            case STATE_FALLING:
                velocityY += gravity * dt;
                avaPos.y += velocityY * dt;

                // STEP 1: If she hasn't teleported yet and hits the bottom
                if (!hasTeleported && avaPos.y > screenHeight + 50) {
                    currentState = STATE_INTRO;
                    //cutsceneTimer = 0.0f;
                }

                // STEP 3: Normal landing logic (only active after she emerges from top)
                if (hasTeleported && avaPos.y >= groundLevel) {
                    avaPos.y = groundLevel;
                    velocityY = 0;
                    currentState = STATE_LANDED;
                    animFrame = 2;
                }
                break;

            case STATE_CUTSCENE:
                cutsceneTimer += dt;
                // After 1 second 
                if (cutsceneTimer >= 3.0f) {
                    currentState = STATE_FALLING;
                    hasTeleported = true; // Flag her to emerge from top
                    avaPos.y = -standardHeight; // Snap to top
                    velocityY = 300.0f; // Give her some speed coming in, so that the warp isnt instantaneous
                }
                break;

            case STATE_LANDED:
                // Stand up sequence (Frames 2 -> 3 -> 4)
                animTimer += dt;
                if (animFrame < 4) {
                    if (animTimer > 0.12f) {
                        animFrame++;
                        animTimer = 0;
                    }
                }
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    currentState = STATE_RUNNING;
                    animFrame = 0;
                }
                break;

            case STATE_RUNNING:
                animTimer += dt;
                if (animTimer > 0.08f) {
                    animTimer = 0;
                    animFrame = (animFrame + 1) % 8;
                }
                backgroundOffset -= scrollSpeed * dt;
                //check if she has launched herself
                if(curr->rect.x + curr->rect.width>0){
                    curr->rect.x -= scrollSpeed * dt;
                    if((curr->rect.x + curr->rect.width) < avaPos.x){
                        currentState = STATE_GLIDING;
                        velocityY = -100.0f; // Give her a tiny little "hop" as she starts gliding

                        flightCount++;

                        //initiate the Dinesh spawn
                        dineshBoss.active = true;
                        dineshBoss.hasTargeted = false; //when he spawns, he has not yet changed his trajectory
                        dineshBoss.pos.x = (float)screenWidth + 150; // Start off-screen
                        dineshBoss.pos.y = (float)GetRandomValue(100, 600); // Random height
                        dineshBoss.velocity = (Vector2){ -350.0f, 0.0f };   // Initial horizontal speed
                    }
                    break;
                }
                else{
                    if ((screenWidth/3 + backgroundOffset) < avaPos.x) {
                        currentState = STATE_GLIDING;
                        backgroundOffset = 0.0f;
                        velocityY = -100.0f; // Give her a tiny little "hop" as she starts gliding
                    }

                    break;
                }
            
            case STATE_INTRO:
                typewriterTimer += dt;
                if (typewriterTimer > 0.05f) {
                    if (charsToPrint < (int)TextLength(introSentence)) {
                        charsToPrint++;
                    } else {
                        allCharsDisplayed = true;
                    }
                    typewriterTimer = 0.0f;
                }

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (!allCharsDisplayed) {
                        charsToPrint = (int)TextLength(introSentence);
                        allCharsDisplayed = true;
                    } else {
                        // TRANSITION TO cutscene
                        currentState = STATE_CUTSCENE;
                        cutsceneTimer = 0.0f;
                        //velocityY = -100.0f; 
                    }
                }
                break;

            case STATE_GAMEOVER:
                typewriterTimer += dt;
                if (typewriterTimer > 0.05f) { // Speed of typing
                    if (charsToPrint < (int)TextLength(sentences[currentSentence])) {
                        charsToPrint++;
                    } else {
                        allCharsDisplayed = true;
                    }
                    typewriterTimer = 0.0f;
                }

                // Handle Clicking
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (!allCharsDisplayed) {
                        // Skip to end of current sentence
                        charsToPrint = (int)TextLength(sentences[currentSentence]);
                        allCharsDisplayed = true;
                    } else {
                        // Go to next sentence
                        if (currentSentence < 3) {
                            currentSentence++;
                            charsToPrint = 0;
                            allCharsDisplayed = false;
                            // Update text for sentence 2 specifically to catch the latest flightCount
                            if (currentSentence == 2) 
                                sentences[2] = TextFormat("I'm impressed! Here is your raise - %d%%", flightCount);
                        } else {
                            // RESET AND RESTART
                            currentState = STATE_OFFICE;

                            // 1. Story & Time Variables
                            elapsedOfficeTime = 0.0f;
                            currentHour = 9.0f;
                            middleTimer = 0.0f;
                            cutsceneTimer = 0.0f;
                            hasTeleported = false;

                            // Gameplay & Difficulty Variables
                            flightCount = 0;
                            backgroundOffset = 0.0f;
                            spawnTimer = 0.0f;

                            // animation and physics reset
                            avaPos = (Vector2){ 300 * globalScale, 20 }; 
                            velocityY = 0.0f;
                            animFrame = 0;
                            animTimer = 0.0f;
                            // Boss Reset
                            dineshBoss.active = false;
                            dineshBoss.hasTargeted = false;
                            dineshBoss.velocity = (Vector2){ 0, 0 };

                            //reset the game over text
                            currentSentence = 0;
                            charsToPrint = 0;
                            allCharsDisplayed = false;

                            // Re-park platforms...
                            for (int i = 0; i < MAX_PLATFORMS; i++) {
                                nodes[i].rect.x = -2000; // Park them off-screen
                            }
                            curr = head;
                            nextToSpawn = head;
                        }
                    }
                }
                break;

            case STATE_GLIDING:
                
                // Gliding physics:
                velocityY *= 0.98f; //damp the vertical velocity per frame
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        velocityY -= 300.0f;
                    }
                velocityY += glidingGravity * dt;
                avaPos.y += velocityY * dt;
                
                // Constant forward scroll
                //backgroundOffset -= scrollSpeed * dt;
                
                //Ava's hitbox rectangle
                Rectangle avaHitbox = { 
                avaPos.x - (standardWidth / 2), 
                avaPos.y - standardHeight, 
                standardWidth, 
                standardHeight 
                };

                if (dineshBoss.active) {
                    // 1. THE PIVOT: Change angle once when he is close-ish
                    if (!dineshBoss.hasTargeted && dineshBoss.pos.x < screenWidth - 200 && dineshBoss.pos.x > avaPos.x + 200) {
                        dineshBoss.hasTargeted = true;

                        // Target Ava's current position
                        Vector2 targetDir = { avaPos.x - dineshBoss.pos.x, avaPos.y - dineshBoss.pos.y };
                        float distance = sqrtf(targetDir.x * targetDir.x + targetDir.y * targetDir.y);
                        
                        // Speed Boost toward her
                        float boostSpeed = 750.0f + (flightCount * 50.0f);
    
                        // Cap the speed so it doesn't become impossible
                        if (boostSpeed > 2000.0f) boostSpeed = 2000.0f;
                        dineshBoss.velocity.x = (targetDir.x / distance) * boostSpeed;
                        dineshBoss.velocity.y = (targetDir.y / distance) * boostSpeed;
                    }

                    // 2. MOVE DINESH
                    dineshBoss.pos.x += dineshBoss.velocity.x * dt;
                    dineshBoss.pos.y += dineshBoss.velocity.y * dt;
                    dineshBoss.rotation = (atan2f(dineshBoss.velocity.y, dineshBoss.velocity.x) * RAD2DEG) + 180.0f;

                    // 3. COLLISION
                    Rectangle dineshHitbox = { dineshBoss.pos.x - (dineshBoss.width / 2), dineshBoss.pos.y - (dineshBoss.height / 2), dineshBoss.width, dineshBoss.height };
                    if (CheckCollisionRecs(avaHitbox, dineshHitbox)) {
                        dineshBoss.active = false;
                        currentState = STATE_GAMEOVER; // Overtime!
                    }

                    // 4. DEACTIVATE if he misses and goes off-screen
                    if (dineshBoss.pos.x < -200) dineshBoss.active = false;

                }

                spawnTimer += dt;
                if(spawnTimer>=spawnInterval){
                    spawnTimer = 0;
                    (nextToSpawn->rect).x = screenWidth;
                    (nextToSpawn->rect).y = (float)GetRandomValue(200, 720);

                    //move the nextToSpawn to point to the next plattform
                    nextToSpawn = nextToSpawn->next;
                }
                //check if collision has occured with the current plattform
                if(curr->rect.x + curr->rect.width >= 0){
                    curr->rect.x -= scrollSpeed * dt; //move the rectangle to the left
                    if(velocityY >= 0 && CheckCollisionPointRec((Vector2){avaPos.x, avaPos.y}, curr->rect)){
                        velocityY = 0;
                        currentState = STATE_RUNNING;
                        avaPos.y = curr->rect.y;
                    }
                }
                //if plattform has exited the screen, park it again at -2000, and point head at the next rectangle in circular queue
                if(curr->rect.x + curr->rect.width < 0){
                    curr->rect.x = -2000;
                    curr = nextToSpawn;
                }

                break;
            }

        BeginDrawing();
            ClearBackground(RAYWHITE);

            if (currentState == STATE_OFFICE) {
                DrawTextureV(texOffice, deskPos, WHITE);
                float rad = 40.0f * globalScale;
                DrawCircleV(clockCenter, rad, LIGHTGRAY);
                DrawCircleLines(clockCenter.x, clockCenter.y, rad, BLACK);
                float hAngle = (currentHour * 30.0f - 90.0f) * DEG2RAD;
                DrawLineEx(clockCenter, (Vector2){clockCenter.x + cosf(hAngle)*(rad*0.6f), clockCenter.y + sinf(hAngle)*(rad*0.6f)}, 3, BLACK);
            }
            
            if (currentState == STATE_MIDDLE){
                DrawTextureV(textDesk, tablePos, WHITE);
                DrawTextureV(textStanding, standingPos, WHITE);
            }

            // Draw Ava for Falling or Landed states
            if (currentState == STATE_FALLING || currentState == STATE_LANDED) {
                float fW = (float)texFall.width / 5.0f;
                Rectangle source = { (float)animFrame * fW, 0, fW, (float)texFall.height };
                Rectangle dest = { avaPos.x, avaPos.y + feetPadding, standardWidth, standardHeight - 20 };
                // Using bottom-center origin for consistent landing
                DrawTexturePro(texFall, source, dest, (Vector2){ standardWidth/2, standardHeight }, 0, WHITE);
                if(hasTeleported){
                    DrawLineEx( (Vector2){ 30, groundLevel }, (Vector2){screenWidth/3, groundLevel }, 10.0f, BLACK);
                }
            }

            if (currentState == STATE_CUTSCENE) {
                // Draw a black overlay or a specific background
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.8f));
                
                // Draw your cutscene text
                DrawText("20 MINUTES LATER...", screenWidth/2 - 150, screenHeight/2, 30, RAYWHITE);
            }

            if (currentState == STATE_RUNNING) {
                float rW = (float)texRun.width / 8.0f;
                Rectangle source = { (float)animFrame * rW, 0, rW, (float)texRun.height };
                Rectangle dest = { avaPos.x, avaPos.y, (standardWidth), standardHeight };
                DrawTexturePro(texRun, source, dest, (Vector2){ (standardHeight/2), (standardHeight) }, 0, WHITE);
                if(curr->rect.x + curr->rect.width >0){
                    DrawRectangleRec(curr->rect, BLACK);
                    DrawRectangle(curr->rect.x, curr->rect.y, curr->rect.width, 4, BLACK);
                }
                else{
                    DrawLineEx( (Vector2){ 30 + backgroundOffset, groundLevel }, (Vector2){ screenWidth/3 + backgroundOffset, groundLevel }, 10.0f, BLACK);
                }
            }

            if (currentState == STATE_LANDED && animFrame == 4) 
                DrawText("TAP TO RUN!", screenWidth/2 - 80, screenHeight/2, 25, BLACK);

            if(currentState == STATE_GLIDING){
                Rectangle source = {0,0, (float)textGlide.width, (float)textGlide.height };
                Rectangle dest = {avaPos.x, avaPos.y, standardWidth*1.3f, standardHeight*1.3f};
                DrawTexturePro(textGlide, source, dest, (Vector2){standardWidth/2, standardHeight }, 0.0f, WHITE);
                if(curr->rect.x + curr->rect.width>0){
                    DrawRectangleRec(curr->rect, BLACK);
                    DrawRectangle(curr->rect.x, curr->rect.y, curr->rect.width, 4, BLACK);
                }
                if (dineshBoss.active) {
                    Rectangle source = { 0, 0, (float)texDinesh.width, (float)texDinesh.height };
                    Rectangle dest = { dineshBoss.pos.x, dineshBoss.pos.y, dineshBoss.width, dineshBoss.height };
                    // Origin is middle of the texture for clean rotation
                    Vector2 origin = { dineshBoss.width / 2, dineshBoss.height / 2 };

                    DrawTexturePro(texDinesh, source, dest, origin, dineshBoss.rotation, WHITE);

                    // Optional: Laser sight warning when he targets her
                    if (dineshBoss.hasTargeted) {
                        DrawCircle(dineshBoss.pos.x - 40, dineshBoss.pos.y, 5, RED); 
                    }
                }
            }

            if (currentState == STATE_INTRO) {
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(WHITE, 0.8f));

                // Draw Intro Image
                DrawTexture(texIntro, screenWidth/2 - texIntro.width/2, screenHeight/2 - 150, WHITE);

                // Draw Typewriter Text
                const char* partialText = TextSubtext(introSentence, 0, charsToPrint);
                int textWidth = MeasureText(partialText, 20);
                
                // Dinesh's intro dialogue
                DrawText(partialText, screenWidth/2 - textWidth/2, screenHeight/2 + 150, 20, ORANGE);

                if (allCharsDisplayed) {
                    DrawText("[ Click to continue]", screenWidth/2 - 60, screenHeight/2 + 200, 15, GRAY);
                }
            }

            if (currentState == STATE_GAMEOVER) {
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(WHITE, 0.8f));

                // 1. Draw Image in Center
                DrawTexture(texGameOver, screenWidth/2 - texGameOver.width/2, screenHeight/2 - 150, WHITE);

                // 2. Draw Typewriter Text
                // Subtext creates a string only up to 'charsToPrint' length
                const char* partialText = TextSubtext(sentences[currentSentence], 0, charsToPrint);
                
                int textWidth = MeasureText(partialText, 20);
                DrawText(partialText, screenWidth/2 - textWidth/2, screenHeight/2 + 150, 20, ORANGE);

                // 3. Prompt to click
                if (allCharsDisplayed) {
                    const char* prompt = (currentSentence < 3) ? "[ Click for more ]" : "[ Click to go to Office (only option) ]";
                    DrawText(prompt, screenWidth/2 - MeasureText(prompt, 15)/2, screenHeight/2 + 200, 15, GRAY);
                }
            }

        EndDrawing();
    }

    UnloadTexture(texOffice);
    UnloadTexture(texFall);
    UnloadTexture(texRun);
    CloseWindow();
    return 0;
}