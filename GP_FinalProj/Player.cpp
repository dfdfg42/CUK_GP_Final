#include "Player.h"
#include "globals.h"
#include <SDL_image.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>
#include "MovingMonster.h"
#include "RangedMonster.h"

bool Player::textures_loaded_ = false;  // 플래그 초기화
std::unordered_map<PlayerState, Animation> Player::shared_animations_;  // 공유 애니메이션 초기화

Player::Player()
    : is_parrying_(false),
    move_speed_(500.0f),
    parry_cooldown_(0.5f),
    parry_duration_(0.1f),
    parry_timer_(0.0f),
    direction_(0),
    state_(PlayerState::Idle),
    prev_state_(PlayerState::Idle),  // 초기 이전 상태
    current_frame_(0),
    frame_time_(0.0f),
    flip_(false),
    velocity_({ 0.0f, 0.0f }) {  // 속도 초기화
    if (!textures_loaded_) {
        LoadTextures();
        textures_loaded_ = true;
    }
    animations_ = shared_animations_;
    rect_ = { WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, 150, 150 }; // 캐릭터 크기 조정
}

void Player::LoadTextures() {
    LoadAnimation(PlayerState::Idle, "../../Resource/Character/player/Idle_", 8, 0.1f);
    LoadAnimation(PlayerState::Move, "../../Resource/Character/player/Move_", 5, 0.1f);
    LoadAnimation(PlayerState::Parry, "../../Resource/Character/player/Parry_", 6, 0.1f);
    LoadAnimation(PlayerState::Damage, "../../Resource/Character/player/Damage_", 8, 0.1f);
    LoadAnimation(PlayerState::Death, "../../Resource/Character/player/Death_", 6, 0.1f);
}

void Player::LoadAnimation(PlayerState state, const std::string& base_path, int frame_count, float frame_duration) {
    Animation animation;
    animation.frame_duration = frame_duration;
    for (int i = 1; i <= frame_count; ++i) {
        std::string path = base_path + std::to_string(i) + ".png";
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (!surface) {
            std::cerr << "Failed to load surface: " << path << " Error: " << IMG_GetError() << std::endl;
            continue;
        }
        SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        SDL_FreeSurface(surface);
        if (!texture) {
            std::cerr << "Failed to create texture: " << path << " Error: " << SDL_GetError() << std::endl;
            continue;
        }
        animation.frames.push_back(texture);
    }
    if (animation.frames.empty()) {
        std::cerr << "No frames loaded for state: " << static_cast<int>(state) << std::endl;
    }
    else {
        std::cout << "Loaded " << animation.frames.size() << " frames for state: " << static_cast<int>(state) << std::endl;
    }
    shared_animations_[state] = animation;
}

void Player::Update(float deltaTime) {
    float acceleration = 3000.0f;  // 가속도
    float deceleration = 3000.0f;  // 감속도
    float maxSpeed = 500.0f;  // 최대 속도
    bool isMoving = false;

    // Handle movement input with acceleration
    if (g_move_left) {
        velocity_.x -= acceleration * deltaTime;
        if (velocity_.x < -maxSpeed) velocity_.x = -maxSpeed;
        direction_ = 3; // left
        isMoving = true;
        flip_ = false;
    }
    if (g_move_right) {
        velocity_.x += acceleration * deltaTime;
        if (velocity_.x > maxSpeed) velocity_.x = maxSpeed;
        direction_ = 1; // right
        isMoving = true;
        flip_ = true;
    }
    if (g_move_up) {
        velocity_.y -= acceleration * deltaTime;
        if (velocity_.y < -maxSpeed) velocity_.y = -maxSpeed;
        direction_ = 0; // up
        isMoving = true;
    }
    if (g_move_down) {
        velocity_.y += acceleration * deltaTime;
        if (velocity_.y > maxSpeed) velocity_.y = maxSpeed;
        direction_ = 2; // down
        isMoving = true;
    }

    // Decelerate when no input is given
    if (!g_move_left && !g_move_right) {
        if (velocity_.x > 0) {
            velocity_.x -= deceleration * deltaTime;
            if (velocity_.x < 0) velocity_.x = 0;
        }
        else if (velocity_.x < 0) {
            velocity_.x += deceleration * deltaTime;
            if (velocity_.x > 0) velocity_.x = 0;
        }
    }

    if (!g_move_up && !g_move_down) {
        if (velocity_.y > 0) {
            velocity_.y -= deceleration * deltaTime;
            if (velocity_.y < 0) velocity_.y = 0;
        }
        else if (velocity_.y < 0) {
            velocity_.y += deceleration * deltaTime;
            if (velocity_.y > 0) velocity_.y = 0;
        }
    }

    // Update player position
    rect_.x += velocity_.x * deltaTime;
    rect_.y += velocity_.y * deltaTime;

    // Update state based on movement and parrying
    if (is_parrying_) {
        state_ = PlayerState::Parry;
    }
    else if (isMoving) {
        state_ = PlayerState::Move;
    }
    else {
        state_ = PlayerState::Idle;
    }

    // 상태가 변경되면 current_frame을 0으로 초기화
    if (state_ != prev_state_) {
        current_frame_ = 0;
        prev_state_ = state_;
    }

    // Update frame
    frame_time_ += deltaTime;
    if (!animations_[state_].frames.empty() && frame_time_ >= animations_[state_].frame_duration) {
        frame_time_ = 0.0f;
        current_frame_ = (current_frame_ + 1) % animations_[state_].frames.size();
    }

    // Update parry timer
    if (parry_timer_ > 0) {
        parry_timer_ -= deltaTime;
    }
    else {
        is_parrying_ = false;
    }

    // Keep player within window bounds
    rect_.x = std::max(0, std::min(WINDOW_WIDTH - rect_.w, rect_.x));
    rect_.y = std::max(0, std::min(WINDOW_HEIGHT - rect_.h, rect_.y));
}

void Player::Render() {
    // Get current texture based on state and frame
    if (animations_.find(state_) == animations_.end() || animations_[state_].frames.empty()) {
        std::cerr << "Error: no frames available for state " << static_cast<int>(state_) << std::endl;
        return;
    }

    // Validate current frame index
    if (current_frame_ >= animations_[state_].frames.size()) {
        std::cerr << "Error: current_frame_ " << current_frame_ << " is out of bounds for state " << static_cast<int>(state_) << std::endl;
        return;
    }

    SDL_Texture* current_texture = animations_[state_].frames[current_frame_];

    // Render texture if valid
    if (current_texture == nullptr) {
        std::cerr << "Error: current_texture is nullptr in state " << static_cast<int>(state_) << ", frame " << current_frame_ << std::endl;
        return;
    }

    SDL_RendererFlip flip = flip_ ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderCopyEx(g_renderer, current_texture, NULL, &rect_, 0, NULL, flip);
}

void Player::HandleEvents(const SDL_Event& event, std::vector<Monster*>& monsters) {
    switch (event.type) {
    case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_LEFT:
            g_move_left = true;
            break;
        case SDLK_RIGHT:
            g_move_right = true;
            break;
        case SDLK_UP:
            g_move_up = true;
            break;
        case SDLK_DOWN:
            g_move_down = true;
            break;
        case SDLK_SPACE:
            PerformParry(monsters); // Call PerformParry with monster list
            break;
        }
        break;

    case SDL_KEYUP:
        switch (event.key.keysym.sym) {
        case SDLK_LEFT:
            g_move_left = false;
            break;
        case SDLK_RIGHT:
            g_move_right = false;
            break;
        case SDLK_UP:
            g_move_up = false;
            break;
        case SDLK_DOWN:
            g_move_down = false;
            break;
        }
        break;
    }
}

void Player::PerformParry(std::vector<Monster*>& monsters) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> disX(0, WINDOW_WIDTH - 128);
    std::uniform_int_distribution<> disY(0, WINDOW_HEIGHT - 128);
    if (parry_timer_ <= 0) {
        Monster* closestMonster = nullptr;
        float closestDistance = std::numeric_limits<float>::max(); // 매우 큰 값으로 초기화
        float parryDistance = 1000.0f; // 패링 범위

        // 가장 가까운 몬스터를 찾음
        for (auto& monster : monsters) {
            int deltaX = monster->getX() - rect_.x;
            int deltaY = monster->getY() - rect_.y;
            float distance = sqrt(deltaX * deltaX + deltaY * deltaY);
            if (distance <= parryDistance && distance < closestDistance) {
                closestDistance = distance;
                closestMonster = monster;
            }
        }

        // 가장 가까운 몬스터가 있는 경우 패링을 적용
        if (closestMonster != nullptr) {
            is_parrying_ = true;
            parry_timer_ = parry_cooldown_ + parry_duration_;
            float parry_distance = 100.0f; // 패링 시 이동 거리

            int currentHealth = closestMonster->GetHealth();
            closestMonster->SetHealth(currentHealth - 1); // 몬스터의 체력을 1 감소시킴
            if (closestMonster->IsDead()) {
                // 체력이 0이 되면 몬스터를 제거
                auto it = std::find(monsters.begin(), monsters.end(), closestMonster);
                if (it != monsters.end()) {
                    monsters.erase(it);
                    g_kill_count++; // 몬스터 삭제 시 킬 수 증가
                    delete closestMonster; // 몬스터가 동적으로 할당된 경우 메모리 해제
                    if (monsters.size() < 7) {
                        int x = disX(gen);
                        int y = disY(gen);
                        if (monsters.size() % 2 == 0) {
                            monsters.push_back(new MovingMonster(x, y));
                        }
                        else {
                            monsters.push_back(new RangedMonster(x, y));
                        }
                    }
                }
            }

            // 플레이어 이동 처리
            switch (direction_) {
            case 0: rect_.y -= parry_distance; break; // 위
            case 1: rect_.x += parry_distance; break; // 오른쪽
            case 2: rect_.y += parry_distance; break; // 아래
            case 3: rect_.x -= parry_distance; break; // 왼쪽
            }
        }
    }
}

void Player::SetParrying(bool parrying) {
    is_parrying_ = parrying;
}

bool Player::IsParrying() const {
    return is_parrying_;
}

SDL_Rect Player::GetRect() const {
    // 충돌 범위를 줄이기 위해 크기를 줄여서 반환
    SDL_Rect smaller_rect = rect_;
    smaller_rect.w *= 0.5;  // 너비를 50%로 줄임
    smaller_rect.h *= 0.6;  // 높이를 60%로 줄임
    return smaller_rect;
}

void Player::SetPosition(int x, int y) {
    rect_.x = x;
    rect_.y = y;
}
