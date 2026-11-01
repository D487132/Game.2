#include "monetization.hpp"

#include <utility>

namespace crumble {
namespace {

class UnavailableProvider final : public CommerceProvider {
public:
    bool showRewardedVideo(Completion) override { return false; }
    bool purchaseProduct(const std::string&, Completion) override { return false; }
};

} // namespace

Monetization::Monetization()
    : Monetization(std::make_unique<UnavailableProvider>()) {}

Monetization::Monetization(std::unique_ptr<CommerceProvider> provider)
    : provider_(std::move(provider)), inbox_(std::make_shared<Inbox>()) {
    if (!provider_) {
        provider_ = std::make_unique<UnavailableProvider>();
    }
}

bool Monetization::showRewardedVideo() {
    const auto inbox = inbox_;
    return provider_->showRewardedVideo([inbox](bool succeeded) {
        std::lock_guard<std::mutex> lock(inbox->mutex);
        inbox->events.push_back({CommerceKind::RewardedVideo, succeeded, {}});
    });
}

bool Monetization::purchaseProduct(const std::string& productId) {
    if (productId.empty()) {
        return false;
    }
    const auto inbox = inbox_;
    return provider_->purchaseProduct(productId, [inbox, productId](bool succeeded) {
        std::lock_guard<std::mutex> lock(inbox->mutex);
        inbox->events.push_back({CommerceKind::ProductPurchase, succeeded, productId});
    });
}

std::vector<CommerceEvent> Monetization::drainEvents() {
    std::vector<CommerceEvent> ready;
    std::lock_guard<std::mutex> lock(inbox_->mutex);
    ready.reserve(inbox_->events.size());
    while (!inbox_->events.empty()) {
        ready.push_back(std::move(inbox_->events.front()));
        inbox_->events.pop_front();
    }
    return ready;
}

} // namespace crumble
