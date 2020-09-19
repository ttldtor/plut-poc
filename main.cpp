#include <atomic>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
//#include <syncstream>

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

void handlerTest() {
    Handler<void(int)> h{};

    auto id1 = h += [](int i) {std::cout << "tid:" << std::this_thread::get_id() << " " << i << "\n";};
    h += [](int i) {std::cout << "tid:" << std::this_thread::get_id() << " " << i * 2 << "\n";};
    h += [](int i) {std::cout << "tid:" << std::this_thread::get_id() << " " << i + 42 << "\n";};

    h(1);

    h -= id1;

    std::cout << "\n";

    h(2);

    std::atomic<bool> stop = false;
    //std::osyncstream scout(std::cout);

    auto t0 = std::thread([&h]{
      std::cout << "tid:" << std::this_thread::get_id() << "+++" << std::endl;

      for (int i = 1; i < 11; i++) {
          h += [i](int j) {std::cout << "tid:" << std::this_thread::get_id() << " " << i + j << "\n";};
      }
    });

    auto t1 = std::thread([&h, &stop]{
      while (!stop) {
          h(1);

          std::this_thread::sleep_for(std::chrono::milliseconds(17));
      }
    });

    auto t2 = std::thread([&h, &stop]{
      while (!stop) {
          h(2);

          std::this_thread::sleep_for(std::chrono::milliseconds(23));
      }
    });

    auto timeout = 30; //sec
    while (timeout --> 0) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    stop = true;

    t0.join();
    t1.join();
    t2.join();
}

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "test") {
        handlerTest();
    }

    return 0;
}
