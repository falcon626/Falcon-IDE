#define NOMINMAX
#include <windows.h>
#include <rpc.h>
#include <crtdbg.h>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>
#include "../../Src/Framework/System/GUID/FlGUID.h"

std::map<std::string, std::string> FlGuid::m_replacedGuids;
#define CHECK(x) do { if (!(x)) { std::cerr << "GUID check failed at " << __LINE__ << '\n'; std::exit(1); } } while (false)

int main()
{
    FlGuid id;
    const auto before = id.ToString();
    CHECK(before.size() == 36);
    id.NewGuid();
    CHECK(id.ToString() != before);
    CHECK(FlGuid::GetReplacedGuid(before) == id.ToString());
    CHECK(FlGuid::GetReplacedGuid("unknown") == "unknown");
    const auto valid = id.ToString();
    CHECK(!id.FromString("not-a-guid") && id.ToString() == valid);
    CHECK(!id.FromString("") && id.ToString() == valid);
    CHECK(!id.FromString(valid + std::string("\0extra", 6)) && id.ToString() == valid);
    CHECK(id.FromString(before) && id.ToString() == before);
    std::atomic<unsigned> count{};
    std::vector<std::thread> workers;
    for (unsigned worker = 0; worker < 4; ++worker) {
        workers.emplace_back([&] {
            for (unsigned index = 0; index < 500; ++index) {
                FlGuid value;
                const auto old = value.ToString();
                value.NewGuid();
                CHECK(FlGuid::GetReplacedGuid(old) == value.ToString());
                ++count;
            }
        });
    }
    for (auto& worker : workers) worker.join();
    CHECK(count == 2000);
    std::cout << "GUID smoke PASS\n";
}
