#pragma once
#include "jaws/core.hpp"
#include "jaws/assume.hpp"
#include "fmt/format.h"
#include <cinttypes>
#include <atomic>
#include <iostream>

// TODO: this is not thread-safe and not even thread-compatible.
namespace jaws::util {

template <typename CRTP, bool enable_log_lifetime_events = true, bool enable_log_count_changes = true>
class InstanceCounter
{
protected: // TODO: questionable
    uint64_t _InstanceCounter_this_id = 0;
    bool _InstanceCounter_in_moved_from_state = false;
    static uint64_t &NextId()
    {
        static uint64_t next_id = 0;
        return next_id;
    }
    static int &moved_from_instance_count()
    {
        static int count = 0;
        return count;
    }
    static int &alive_instance_count()
    {
        static int count = 0;
        return count;
    }
    void log_event(const std::string &msg)
    {
        if (enable_log_lifetime_events) {
            std::cerr << fmt::format(
                "[{}]({}{}) {}\n",
                get_rtti_type_name<CRTP>(),
                _InstanceCounter_this_id,
                _InstanceCounter_in_moved_from_state ? "+" : "",
                msg);
        }
    }
    void log_count_changes()
    {
        if (enable_log_count_changes) {
            std::cerr << fmt::format(
                "[{}] Now {} alive instance{} + {} moved-from instance{}\n",
                get_rtti_type_name<CRTP>(),
                alive_instance_count(),
                alive_instance_count() != 1 ? "s" : "",
                moved_from_instance_count(),
                moved_from_instance_count() != 1 ? "s" : "");
        }
    }

protected:
    InstanceCounter() : _InstanceCounter_this_id(NextId()++)
    {
        ++alive_instance_count();
        log_event("C'tor");
        log_count_changes();
    }
    InstanceCounter(const InstanceCounter &other) : _InstanceCounter_this_id(NextId()++)
    {
        JAWS_ASSUME(!other._InstanceCounter_in_moved_from_state);
        ++alive_instance_count();
        log_event(fmt::format("Copy c'tor <-[{}]", other._InstanceCounter_this_id));
        log_count_changes();
    }
    InstanceCounter &operator=(const InstanceCounter &other)
    {
        JAWS_ASSUME(!other._InstanceCounter_in_moved_from_state);
        log_event(fmt::format("Copy assignment op <-[{}]", other._InstanceCounter_this_id));
    }
    InstanceCounter(InstanceCounter &&other) noexcept : _InstanceCounter_this_id(NextId()++)
    {
        JAWS_ASSUME(!other._InstanceCounter_in_moved_from_state);
        other._InstanceCounter_in_moved_from_state = true;
        ++moved_from_instance_count();
        ++alive_instance_count();
        log_event(fmt::format("Move c'tor <-[{}]", other._InstanceCounter_this_id));
        log_count_changes();
    }
    InstanceCounter &operator=(InstanceCounter &&other) noexcept
    {
        JAWS_ASSUME(!other._InstanceCounter_in_moved_from_state);
        other._InstanceCounter_in_moved_from_state = true;
        ++moved_from_instance_count();
        log_event(fmt::format("Move assignment op <-[{}]", other._InstanceCounter_this_id));
    }
    virtual ~InstanceCounter()
    {
        --alive_instance_count();
        if (_InstanceCounter_in_moved_from_state) { --moved_from_instance_count(); }
        log_event("~D'tor");
        log_count_changes();
    }

public:
    [[nodiscard]] constexpr bool is_moved_from() const noexcept { return _InstanceCounter_in_moved_from_state; }
    [[nodiscard]] constexpr uint64_t get_instance_id() const noexcept { return _InstanceCounter_this_id; }
    static int get_moved_from_instance_count() noexcept { return moved_from_instance_count(); }
    static int get_alive_instance_count() noexcept { return alive_instance_count(); }
};

}
