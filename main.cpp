#include <atomic>
#include <functional>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <vector>

template<typename T>
struct IdGenerator {
    using EntityType = T;

    static std::atomic<std::uint64_t> id;

    /**
	 * @return new id for object of the entity type @p T
	 */
    static constexpr std::uint64_t get() noexcept {
        return id++;
    }
};

template<typename T>
std::atomic<std::uint64_t> IdGenerator<T>::id{0};

struct CharType {
    char16_t c;
    unsigned attributes;
};

template<typename>
struct Handler;

template<typename... Args>
struct Handler<void(Args...)> {
    std::unordered_map<std::uint64_t, std::function<void(Args...)>> functions{};
    std::mutex mutex{};

    template<typename F>
    std::uint64_t operator+=(F &&f) {
        std::lock_guard lock(mutex);

        auto id = IdGenerator<Handler<void(Args...)>>::get();

        functions[id] = f;

        return id;
    }

    void operator-=(std::uint64_t id) {
        std::lock_guard lock(mutex);

        functions.erase(id);
    }

    void operator()(Args &&... args) {
        std::lock_guard lock(mutex);

        for (auto && [id, f] : functions) {
            f(std::forward<Args>(args)...);
        }
    }
};


struct Console {
    std::vector<CharType> buffer;
};

int main() {
    Handler<void(int)> h{};

    auto id1 = h += [](int i) {std::cout << i << "\n";};
    h += [](int i) {std::cout << i * 2 << "\n";};
    h += [](int i) {std::cout << i + 42 << "\n";};

    std::cout << "Hello, World!" << std::endl;

    h(1);

    h -= id1;

    std::cout << "\n";

    h(2);

    return 0;
}
