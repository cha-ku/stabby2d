#include "GameState.hpp"
#include "MovementSystem.hpp"
#include "RenderSystem.hpp"
#include "RigidBodyComponent.hpp"
#include "SpriteComponent.hpp"
#include "TransformComponent.hpp"
#include <fstream>
#include <sstream>
void GameState::Initialize() {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        Logger::Error("Error Initializing SDL");
        return;
    }

    //SDL_DisplayMode displayMode;
    window = SDL_CreateWindow(nullptr, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, SDL_WINDOW_BORDERLESS);
    if (window == nullptr) {
        Logger::Error("Error creating SDL window");
        return;
    }

    // SDL_RENDERER_ACCELERATED  - manually instruct SDL to try to use accelerated GPU
    // SDL_RENDERER_PRESENTVSYNC - use VSync, i.e. sync frame rate with monitor's refresh rate. Enabling VSync will prevent some screen tearing artifacts
    //                             when we display displaying our objects in our game loop, as it will try to synchronize the rendering of our frame with
    //                             the refresh rate of the monitor.
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (renderer == nullptr) {
        Logger::Error("Error creating SDL renderer");
        return;
    }

    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

    isRunning = true;
}

void GameState::Setup() {
  registry->AddSystem<MovementSystem>();
  registry->AddSystem<RenderSystem>();

  assetStore->AddTexture(std::string("tank-right"),std::string("./assets/images/tank-panther-right.png"), renderer);
  assetStore->AddTexture("tilemap", "./assets/tilemaps/jungle.png", renderer);

  constexpr int width{32};
  constexpr int height{32};

  // Create panther tank
  auto tankRight = registry->CreateEntity();
  tankRight.AddComponent<TransformComponent>(Position(10.0F, 30.0F), Scale(1.0F, 1.0F), Rotation(0.0));
  tankRight.AddComponent<RigidBodyComponent>(Velocity(10.0F, 0.0F));
  tankRight.AddComponent<SpriteComponent>("tank-right", width, height, SDL_Rect(0, 0, width, height));

  // create tilemap
  auto ConstructTileMap = [&, this]() {
    const char delim{','};
    const std::string fileName{"./assets/tilemaps/jungle.map"};
    std::ifstream mapFile;
    mapFile.open(fileName);
    if (mapFile.fail()) {
      mapFile.close();
      Logger::Error("Could not open " + fileName);
      return;
    }

    // hardcode number of rows and number of columns for now
    constexpr uint8_t tileSize{32};

    uint8_t yPos = 0;
    for(std::string line; std::getline(mapFile, line); ++yPos) {
      std::string numStr;
      std::stringstream ssLine(line);
      uint8_t xPos = 0;
      while(std::getline(ssLine, numStr, delim)) {
        //Logger::Info(numStr);
        const auto tileMapVal = std::stoi(numStr);
        const auto xVal = (tileMapVal / 10) * tileSize;
        const auto yVal = (tileMapVal % 10) * tileSize;
        auto tile = registry->CreateEntity();
        tile.AddComponent<TransformComponent>(Position(xPos * width * height, yPos * width * height), Scale(1.0F, 1.0F), Rotation(0.0F));
        tile.AddComponent<SpriteComponent>("tilemap", width, height, SDL_Rect(xVal, yVal, width, height));
        ++xPos;
      }
    }
    mapFile.close();
  };
  ConstructTileMap();
}

void GameState::ProcessInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
            case SDL_QUIT:
                isRunning = false;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    isRunning = false;
                }
                break;
            default:
              Logger::Error("Received bad event");
              break;
        }
    }
}

void GameState::Render() {
  SDL_SetRenderDrawColor(renderer, 21, 21, 21, 255);
  SDL_RenderClear(renderer);

  registry->GetSystem<RenderSystem>().Update(renderer, assetStore);
  SDL_RenderPresent(renderer);
}


void GameState::Update() {
  // wait until we reach MILLISECS_PER_FRAME
  // ticks = millisecond in SDL parlance
  // SDL_GetTicks() gets no. of millisecs from when the last time SDL_Init() was called

  // Dumb way(blocks other processes) - wait in this loop until current time > past frame draw time + millisecs per frame
  //while(!SDL_TICKS_PASSED(SDL_GetTicks(), milliSecsPrevFrame + MILLISECS_PER_FRAME));

  auto timeToWait = MILLISECS_PER_FRAME - (SDL_GetTicks64() - milliSecsPrevFrame);
  if (timeToWait > 0 && timeToWait <= MILLISECS_PER_FRAME) {
      SDL_Delay(timeToWait);
  }

  constexpr double updateInterval = 1000;
  // Time since last frame in seconds
  auto deltaTime = static_cast<double>((SDL_GetTicks64() - milliSecsPrevFrame)) / updateInterval;
  milliSecsPrevFrame = SDL_GetTicks64();
  registry->Update();
  registry->GetSystem<MovementSystem>().Update(deltaTime);
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
