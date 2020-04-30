#include "catch2/catch.hpp"
#include "utils/future.hpp"
#include "utils/promise_storage.hpp"
#include <queue>

using namespace erwin;

class DeferredProducer
{
private:
    struct GetPixelsCommand
    {
        size_t size;
        size_t promise_token;
    };

    std::queue<GetPixelsCommand> commands_;
    PromiseStorage<uint8_t*> promises_;

public:
    std::future<uint8_t*> get_pixels(size_t size)
    {
        // Save promise and enqueue command
        auto&& [token, fut] = promises_.future_operation();
        commands_.push({size, token});

        return std::move(fut);
    }

    void flush()
    {
        while(!commands_.empty())
        {
            auto&& command = commands_.front();

            // Set value in associated promise
            uint8_t* buffer = new uint8_t[command.size];
            for(size_t ii=0; ii<command.size; ++ii)
                buffer[ii] = uint8_t(ii%255);

            promises_.fulfill(command.promise_token, buffer);

            commands_.pop();
        }
    }
};


class PromiseStorageFixture
{
public:
    bool handle(std::future<uint8_t*>& fut, size_t size, const std::string& name)
    {
        if(is_ready(fut))
        {
            uint8_t* buffer = fut.get();
            delete[] buffer;
            return true;
        }
        else
        	return false;
    }

    DeferredProducer producer;
};

TEST_CASE_METHOD(PromiseStorageFixture, "Promise Storage: deferred execution test", "[fut]")
{
    auto fut0 = producer.get_pixels(8);
    auto fut1 = producer.get_pixels(16);

    bool handled0 = handle(fut0, 8,  "fut0");
    bool handled1 = handle(fut1, 16, "fut1");

    REQUIRE(!handled0);
    REQUIRE(!handled1);

    producer.flush();

    handled0 = handle(fut0, 8,  "fut0");
    handled1 = handle(fut1, 16, "fut1");

    REQUIRE(handled0);
    REQUIRE(handled1);
}