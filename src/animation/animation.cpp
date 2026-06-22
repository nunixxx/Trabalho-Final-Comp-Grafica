#include "animation.h"

Animator::Animator()
    : currentFrame_(0)
    , timer_(0.0f)
    , speed_(1.0f)
    , playing_(false)
    , finished_(false)
{}

void Animator::addClip(const AnimationClip& clip)
{
    clips_[clip.name] = clip;
}

bool Animator::hasClip(const std::string& name) const
{
    return clips_.find(name) != clips_.end();
}

void Animator::removeClip(const std::string& name)
{
    auto it = clips_.find(name);
    if (it != clips_.end())
        clips_.erase(it);
}

void Animator::play(const std::string& name, bool reset)
{
    auto it = clips_.find(name);
    if (it == clips_.end()) return;

    if (reset || name != currentClipName_)
    {
        currentClipName_ = name;
        currentFrame_ = 0;
        timer_ = 0.0f;
        finished_ = false;
    }

    playing_ = true;
}

void Animator::stop()
{
    playing_ = false;
    finished_ = true;
}

void Animator::update(float deltaTime)
{
    if (!playing_ || finished_) return;

    auto it = clips_.find(currentClipName_);
    if (it == clips_.end()) return;

    const AnimationClip& clip = it->second;

    timer_ += deltaTime * speed_;

    while (timer_ >= clip.frameDuration)
    {
        timer_ -= clip.frameDuration;
        currentFrame_++;

        if (currentFrame_ >= (int)clip.frameModels.size())
        {
            if (clip.loop)
            {
                currentFrame_ = 0;
            }
            else
            {
                currentFrame_ = (int)clip.frameModels.size() - 1;
                finished_ = true;
                playing_ = false;
                break;
            }
        }
    }
}

std::string Animator::getCurrentModel() const
{
    auto it = clips_.find(currentClipName_);
    if (it == clips_.end() || it->second.frameModels.empty())
        return "";

    if (currentFrame_ >= (int)it->second.frameModels.size())
        return it->second.frameModels.back();

    return it->second.frameModels[currentFrame_];
}

bool Animator::isFinished() const
{
    return finished_;
}

int Animator::getFrameCount() const
{
    auto it = clips_.find(currentClipName_);
    if (it == clips_.end())
        return 0;
    return (int)it->second.frameModels.size();
}
