#include "GameState.hpp"

void GameState::Initialize() {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        Logger::Error("Error Initializing SDL");
        return;
    }

    //SDL_DisplayMode displayMode;

    window = SDL_CreateWindow(nullptr, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_BORDERLESS);

    if (!window) {
        Logger::Error("Error creating SDL window");
        return;
    }

    // SDL_RENDERER_ACCELERATED  - manually instruct SDL to try to use accelerated GPU
    // SDL_RENDERER_PRESENTVSYNC - use VSync, i.e. sync frame rate with monitor's refresh rate. Enabling VSync will prevent some screen tearing artifacts 
    //                             when we display displaying our objects in our game loop, as it will try to synchronize the rendering of our frame with 
    //                             the refresh rate of the monitor.
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer) {
        Logger::Error("Error creating SDL renderer");
        return;
    }

    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

    isRunning = true;
}

void GameState::ProcessInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: 
                isRunning = false;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    isRunning = false;
                }
                break;
        }
    }
}

void GameState::Render() {
    SDL_SetRenderDrawColor(renderer, 21, 21, 21, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    // tank texture (PNG)
    auto tankSurface = IMG_Load("./assets/images/tank-tiger-right.png");
    auto tankTexture = SDL_CreateTextureFromSurface(renderer, tankSurface); 
    SDL_FreeSurface(tankSurface);

    SDL_Rect dstRect = {static_cast<int>(playerPos.x), static_cast<int>(playerPos.y), 32, 32};
    SDL_RenderCopy(renderer, tankTexture, nullptr, &dstRect);
    SDL_DestroyTexture(tankTexture);

    SDL_RenderPresent(renderer);
}

void GameState::Setup() {
    playerPos = glm::vec2(10.0, 20.0);
    playerVel = glm::vec2(100, 5);
}

void GameState::Update() {
    // wait until we reach MILLISECS_PER_FRAME
    // ticks = millisecond in SDL parlance
    // SDL_GetTicks() gets no. of millisecs from when the last time SDL_Init() was called

    // Dumb way(blocks other processes) - wait in this loop until current time > past frame draw time + millisecs per frame
    //while(!SDL_TICKS_PASSED(SDL_GetTicks(), milliSecsPrevFrame + MILLISECS_PER_FRAME));

    auto timeToWait = MILLISECS_PER_FRAME - (SDL_GetTicks() - milliSecsPrevFrame);
    if (timeToWait > 0 && timeToWait <= MILLISECS_PER_FRAME) {
        SDL_Delay(timeToWait);
    }

    // Time since last frame in seconds
    auto deltaTime = (SDL_GetTicks() - milliSecsPrevFrame) / 1000.0f;
    milliSecsPrevFrame = SDL_GetTicks();

    playerPos.x += playerVel.x * deltaTime;
    playerPos.y += playerVel.y * deltaTime;

    //Logger::Info({std::to_string(playerPos.x) + " , "  + std::to_string(playerPos.y)});
}

void GameState::Run() {
    Logger::Info("Game starting");
    Setup();
    while(isRunning) {
        ProcessInput();
        Update();
        Render();
    }
    Logger::Info("Game ended");
};

void GameState::Destroy() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}