#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace crumble {

enum class CommerceKind { RewardedVideo, ProductPurchase };

struct CommerceEvent {
    CommerceKind kind{};
    bool succeeded{};
    std::string productId;
};

class CommerceProvider {
public:
    using Completion = std::function<void(bool)>;
    virtual ~CommerceProvider() = default;
    virtual bool showRewardedVideo(Completion completion) = 0;
    virtual bool purchaseProduct(const std::string& productId, Completion completion) = 0;
};

class Monetization {
public:
    Monetization();
    explicit Monetization(std::unique_ptr<CommerceProvider> provider);
    bool showRewardedVideo();
    bool purchaseProduct(const std::string& productId);
    std::vector<CommerceEvent> drainEvents();

private:
    struct Inbox {
        std::mutex mutex;
        std::deque<CommerceEvent> events;
    };

    std::unique_ptr<CommerceProvider> provider_;
    std::shared_ptr<Inbox> inbox_;
};

} // namespace crumble
