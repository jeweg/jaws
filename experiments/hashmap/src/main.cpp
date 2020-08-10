#include "jaws/jaws.hpp"
#include "jaws/assume.hpp"
#include <unordered_map>
#include <iostream>

jaws::LoggerPtr logger;

//#define HAS_COPY_SEMANTICS
#define HAS_MOVE_SEMANTICS

struct A
{
    int state;
    static int instance_counter;

    A(int state) : state(state)
    {
        logger->info("    A ctor ({})", state);
        ++instance_counter;
    }
    ~A()
    {
        logger->info("    A dtor");
        --instance_counter;
    }

#ifdef HAS_COPY_SEMANTICS
    A(const A& other) : state(other.state)
    {
        logger->info("    A copy ctor");
        ++instance_counter;
    }
    A& operator=(const A& other)
    {
        state = other.state;
        logger->info("    A copy assignment op");
    }
#else
    A(const A&) = delete;
    A& operator=(const A&) = delete;
#endif

#ifdef HAS_MOVE_SEMANTICS
    A(A&& other) : state(other.state)
    {
        logger->info("    A move ctor");
        ++instance_counter;
    }
    A& operator=(A&& other)
    {
        state = other.state;
        logger->info("    A move assignment op");
    }
#else
    A(A&&) = delete;
    A& operator=(A&&) = delete;
#endif


    friend std::ostream& operator<<(std::ostream& os, const A& a)
    {
        os << "A(" << a.state << ")";
        return os;
    }
    friend bool operator==(const A& a1, const A& a2)
    {
        return a1.state == a2.state;
    }
};

int A::instance_counter = 0;

namespace std {

template<> struct hash<A>
{
    std::size_t operator()(const A& a) const noexcept
    {
        logger->info("      (compute hash value)");
        return std::hash<int>{}(a.state);
    }
};


} // namespace std


int main(int argc, char** argv)
{
    logger = jaws::Jaws::instance()->get_logger_ptr(jaws::Category::General);
    logger->set_pattern("[%n] [%l] %v");

    logger->info("create hashmap:");
    std::unordered_map<A, int> hashmap;

    logger->info("reserve:");
    hashmap.reserve(100);

    logger->info("insert:");
    hashmap.insert(std::make_pair(A{0}, 0));

    logger->info("iterate {} elem{}:", hashmap.size(), hashmap.size() != 1 ? "s" : "");
    for (auto& p : hashmap) { logger->info("    {} => {}", p.first, p.second); }

    logger->info("find:");
    auto iter = hashmap.find(A(1));
    iter = hashmap.find(A(0));
    JAWS_ASSUME(iter != hashmap.end());

    // This is not allowed because the key is const.
    // A my_a = std::move(iter->first);

    logger->info("destroy hashmap:");
}
