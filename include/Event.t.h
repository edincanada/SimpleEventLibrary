
#pragma once
#ifndef SEL_EVENT_T_H_INCLUDED
#define SEL_EVENT_T_H_INCLUDED

#include <array>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>
#include <cstdint>

namespace sel
{
    template <std::size_t Capacity, typename... Args>
    class Event
    {
    public:
        class Handler;

    private:
        struct Slot
        {
            uint64_t id;
            std::function<void(Args..., Handler&)> callback;
        };

        std::array<std::optional<Slot>, Capacity> slots_{};

        std::size_t nextIndex_ = 0;   // logical append cursor
        uint64_t nextId_ = 1;
        bool invoking_ = false;

        // O(n) lookup (you accepted this tradeoff)
        std::optional<std::reference_wrapper<Slot>> findSlot(uint64_t id)
        {
            for (std::size_t ii = 0; ii < nextIndex_; ++ii)
            {
                if (slots_[ii].has_value() && slots_[ii]->id == id)
                    return slots_[ii].value();
            }

            return std::nullopt;
        }

        void compact()
        {
            std::size_t write = 0;

            for (std::size_t read = 0; read < nextIndex_; ++read)
            {
                if (!slots_[read].has_value())
                    continue;

                if (write != read)
                    slots_[write] = std::move(slots_[read]);

                ++write;
            }

            // clear remaining
            for (std::size_t ii = write; ii < nextIndex_; ++ii)
            {
                slots_[ii].reset();
            }

            nextIndex_ = write;
        }

        void ensureSpace()
        {
            if (nextIndex_ < Capacity)
                return;

            // no space at end → compact first
            compact();

            if (nextIndex_ >= Capacity)
                throw std::runtime_error("Event capacity exceeded");
        }

    public:
        class Handler
        {
            friend class Event;

            Event* event_ = nullptr;
            uint64_t id_ = 0;

            Handler(Event* e, uint64_t id)
                : event_(e), id_(id) {}

        public:
            Handler() = default;

            void unsubscribe()
            {
                if (!event_) return;

                for (std::size_t ii = 0; ii < event_->nextIndex_; ++ii)
                {
                    auto& slot = event_->slots_[ii];

                    if (slot.has_value() && slot->id == id_)
                    {
                        slot->callback = nullptr;
                        slot->id = 0;
                        slot.reset();
                        return;
                    }
                }
            }

            bool isSubscribed() const
            {
                if (!event_) return false;

                for (std::size_t i = 0; i < event_->nextIndex_; ++i)
                {
                    const auto& slot = event_->slots_[i];

                    if (slot.has_value() && slot->id == id_)
                        return true;
                }

                return false;
            }
        };

        Handler subscribe(std::function<void(Args..., Handler&)> callback)
        {
            ensureSpace();

            uint64_t id = nextId_++;

            slots_[nextIndex_] = Slot{
                .id = id,
                .callback = std::move(callback)
            };

            ++nextIndex_;

            return Handler(this, id);
        }

        void invoke(Args... args)
        {
            if (invoking_)
                throw std::runtime_error("Recursive invocation is not allowed");

            invoking_ = true;
            std::size_t write = 0;

            for (std::size_t read = 0; read < nextIndex_; ++read)
            {
                if (!slots_[read].has_value())
                    continue;

                // move forward if needed (compaction-in-place)
                if (write != read)
                    slots_[write] = std::move(slots_[read]);

                Handler h(this, slots_[write]->id);
                slots_[write]->callback(args..., h);

                ++write;
            }

            // clear trailing region
            for (std::size_t i = write; i < nextIndex_; ++i)
            {
                slots_[i].reset();
            }

            nextIndex_ = write;

            invoking_ = false;
        }
    };
} // namespace sel

#endif //SEL_EVENT_TDECL_H_INCLUDED
