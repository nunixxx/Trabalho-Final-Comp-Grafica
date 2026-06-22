#pragma once

#include <string>
#include <vector>
#include <map>

struct AnimationClip {
    std::string name;
    std::vector<std::string> frameModels;
    float frameDuration = 0.1f;
    bool loop = true;
};

class Animator {
public:
    Animator();

    void addClip(const AnimationClip& clip);
    bool hasClip(const std::string& name) const;
    void removeClip(const std::string& name);

    void play(const std::string& name, bool reset = true);
    void stop();
    void update(float deltaTime);

    std::string getCurrentModel() const;
    bool isFinished() const;
    bool isPlaying() const { return playing_; }
    int getCurrentFrame() const { return currentFrame_; }
    int getFrameCount() const;

    void setSpeed(float speed) { speed_ = speed; }
    float getSpeed() const { return speed_; }
    const std::string& getCurrentClip() const { return currentClipName_; }

private:
    std::map<std::string, AnimationClip> clips_;
    std::string currentClipName_;
    int currentFrame_ = 0;
    float timer_ = 0.0f;
    float speed_ = 1.0f;
    bool playing_ = false;
    bool finished_ = false;
};
