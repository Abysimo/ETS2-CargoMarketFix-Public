#include "build_info.h"
#include <cstdio>
#include <cstring>

int main() {
    using namespace cmf::fix_builds;
    static_assert(supported.size() == 1);
    unsigned cases = 0;
    const auto check = [&](bool ok, const char* name) {
        if (!ok) { std::printf("FAIL %s\n", name); return false; }
        ++cases; std::printf("PASS %s\n", name); return true;
    };
    for (const auto* d : {&ets157, &ets158, &ets159, &ets160}) {
        const bool intended = d == supported[0];
        if (!check((identify(d->sha256) == d) == intended, d->version)) return 1;
        if (!check(known(d) == intended, "descriptor ownership scope")) return 1;
    }
    if (!check(!identify("") && !identify("0000000000000000000000000000000000000000000000000000000000000000"), "unknown rejected")) return 1;
    if (!check(std::strcmp(cmf::kTargetEts2Version, supported[0]->version) == 0 &&
        std::strcmp(cmf::kExpectedExecutableSha256, supported[0]->sha256) == 0 &&
        std::strstr(cmf::kPluginVersion, supported[0]->version), "runtime metadata")) return 1;
    std::printf("TOTAL %u scope cases\n", cases);
}
