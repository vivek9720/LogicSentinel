#pragma once
#include <optional>
#include <string>
#include <vector>

namespace logicsentinel::audit {

struct OperationProfile {
    const char* operation;
    const char* domain;
    const char* asset;
    const char* risk;
    const char* statefulness;
    const char* required_control;
    const char* review_hint;
};

std::optional<OperationProfile> lookup_operation_profile(const std::string& operation);
std::vector<OperationProfile> operation_profiles_by_domain(const std::string& domain);
std::vector<OperationProfile> operation_profiles_by_risk(const std::string& risk);
std::vector<OperationProfile> operation_profiles_requiring_control(const std::string& control);
std::vector<OperationProfile> all_operation_profiles();
std::string operation_profile_summary(const OperationProfile& profile);

} // namespace logicsentinel::audit
