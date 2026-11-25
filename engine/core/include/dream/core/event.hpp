#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace dream::core {

class EventConnection final {
public:
    EventConnection() = default;
    EventConnection(const EventConnection&) = delete;
    EventConnection& operator=(const EventConnection&) = delete;

    EventConnection(EventConnection&& other) noexcept
        : disconnect_(std::move(other.disconnect_)) {}

    EventConnection& operator=(EventConnection&& other) noexcept {
        if (this != &other) {
            disconnect();
            disconnect_ = std::move(other.disconnect_);
        }
        return *this;
    }

    ~EventConnection() { disconnect(); }

    void disconnect() noexcept {
        if (disconnect_) {
            auto action = std::move(disconnect_);
            action();
        }
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return static_cast<bool>(disconnect_);
    }

private:
    template <typename> friend class EventDispatcherAccess;
    friend class EventDispatcher;

    explicit EventConnection(std::function<void()> disconnect)
        : disconnect_(std::move(disconnect)) {}

    std::function<void()> disconnect_;
};

struct EventDispatchReport final {
    std::size_t invoked{0};
    std::size_t failed{0};
};

class EventDispatcher final {
private:
    struct Slot final {
        std::uint64_t id;
        int priority;
        std::uint64_t order;
        std::atomic<bool> active{true};
        std::function<void(const void*)> callback;
    };

    struct State final {
        std::mutex mutex;
        std::unordered_map<std::type_index, std::vector<std::shared_ptr<Slot>>> slots;
        std::uint64_t next_id{1};
        std::uint64_t next_order{0};
    };

public:
    EventDispatcher() : state_(std::make_shared<State>()) {}
    EventDispatcher(const EventDispatcher&) = delete;
    EventDispatcher& operator=(const EventDispatcher&) = delete;

    template <typename Event, typename Callback>
    [[nodiscard]] EventConnection subscribe(Callback&& callback, int priority = 0) {
        auto state = state_;
        auto slot = std::make_shared<Slot>();
        slot->priority = priority;
        slot->callback = [fn = std::forward<Callback>(callback)](const void* event) mutable {
            std::invoke(fn, *static_cast<const Event*>(event));
        };

        {
            std::lock_guard lock(state->mutex);
            slot->id = state->next_id++;
            slot->order = state->next_order++;
            state->slots[std::type_index(typeid(Event))].push_back(slot);
        }

        return EventConnection([weak = std::weak_ptr<State>(state),
                                type = std::type_index(typeid(Event)),
                                weak_slot = std::weak_ptr<Slot>(slot)] {
            if (auto target = weak_slot.lock()) {
                target->active.store(false, std::memory_order_release);
            }
            if (auto owner = weak.lock()) {
                std::lock_guard lock(owner->mutex);
                auto found = owner->slots.find(type);
                if (found != owner->slots.end()) {
                    auto& entries = found->second;
                    entries.erase(std::remove_if(entries.begin(), entries.end(),
                        [](const auto& entry) {
                            return !entry->active.load(std::memory_order_acquire);
                        }), entries.end());
                    if (entries.empty()) owner->slots.erase(found);
                }
            }
        });
    }

    template <typename Event>
    [[nodiscard]] EventDispatchReport dispatch(const Event& event) const {
        std::vector<std::shared_ptr<Slot>> snapshot;
        {
            std::lock_guard lock(state_->mutex);
            const auto found = state_->slots.find(std::type_index(typeid(Event)));
            if (found == state_->slots.end()) return {};
            snapshot = found->second;
        }

        std::stable_sort(snapshot.begin(), snapshot.end(),
            [](const auto& left, const auto& right) {
                if (left->priority != right->priority)
                    return left->priority > right->priority;
                return left->order < right->order;
            });

        EventDispatchReport report;
        for (const auto& slot : snapshot) {
            if (!slot->active.load(std::memory_order_acquire)) continue;
            try {
                slot->callback(&event);
                ++report.invoked;
            } catch (...) {
                ++report.failed;
            }
        }
        return report;
    }

private:
    std::shared_ptr<State> state_;
};

} // namespace dream::core
