// vim:set sw=2 ts=2 sts et:

#include <optional>
#include <string>

struct SplitUrl {
  std::string schemehost;
  std::string portpath;
};

std::optional<SplitUrl> splitUrl(const std::string &url, std::string *error = nullptr);
