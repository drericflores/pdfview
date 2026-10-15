// tool_registry.h - the list of tools features have contributed.
#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "view/tool.h"

namespace efpdf {

class ToolRegistry {
public:
    template <typename T, typename... Args>
    T &emplace(Args &&...args) {
        auto tool = std::make_unique<T>(std::forward<Args>(args)...);
        T &ref = *tool;
        tools_.push_back(std::move(tool));
        return ref;
    }

    const std::vector<std::unique_ptr<Tool>> &all() const { return tools_; }

private:
    std::vector<std::unique_ptr<Tool>> tools_;
};

}  // namespace efpdf
