#pragma once
// Recursion Tree Visualizer

#include "c_wrap.h"
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace act {

template<typename Node, typename ValueFunc, typename GetChilds>
void vis_tree_help(
    Node node,
    const ValueFunc &valueFunc,
    const GetChilds &getChilds,
    std::vector<std::variant<int, std::string>> &context) {
    context.push_back(valueFunc(node));
    auto childs = getChilds(node);
    context.push_back((int)childs.size());
    for (auto child : childs) {
        vis_tree_help(child, valueFunc, getChilds, context);
    }
}

template<typename Node, typename ValueFunc, typename GetChilds>
void vis_tree(
    Node node,
    const ValueFunc &valueFunc,
    const GetChilds &getChilds,
    const std::string &img_name = "recur_vis.png") {
    std::vector<std::variant<int, std::string>> tree_ser; // Tree Serialize

    vis_tree_help(node, valueFunc, getChilds, tree_ser);
    act::Interpreter interpreter;
    interpreter.call(
        "py_wrap_recur_vis", "recur_tree_viser", tree_ser, img_name);
}

struct RVCNode {
    std::string content{};
    std::vector<std::shared_ptr<RVCNode>> childs{};
    bool is_valid() const { return !content.empty(); }
    // bool has_son() const { return !childs.empty(); }
    std::vector<std::shared_ptr<RVCNode>> get_childs() const { return childs; }
};
static std::string get_content(std::shared_ptr<RVCNode> rvc_node) {
    return rvc_node->content;
}

static std::vector<std::shared_ptr<RVCNode>>
get_childs(std::shared_ptr<RVCNode> rvc_node) {
    return rvc_node->childs;
}

class RecurVisContext {
  public:
    void push() {
        stack_.push_back(std::make_shared<RVCNode>());
        if (stack_.size() == 1) root_ = stack_.back();
    }
    void pop() {
        auto top = stack_.back();

        stack_.pop_back();
        if (stack_.size() >= 1 && top->is_valid())
            stack_.back()->childs.push_back(top);
        if (stack_.size() == 0) {
            vis_tree(root_, get_content, get_childs, file_name);
            file_name = "recur_vis.png";
        }
    }
    template<typename T>
    RecurVisContext &operator<<(const T &v) {
        std::stringstream ss; // Trick
        ss << stack_.back()->content;
        ss << v;
        stack_.back()->content = ss.str();
        return *this;
    }
    void set_file_name(const std::string &str) { file_name = str; }
    void reset() { root_ = nullptr; }

  private:
    std::shared_ptr<RVCNode> root_{nullptr};
    std::vector<std::shared_ptr<RVCNode>> stack_{};
    std::string file_name{"recur_vis.png"};
};
static RecurVisContext rvc;

inline RecurVisContext &rv() {
    return rvc;
}

class RecurVisContextGuard {
  public:
    explicit RecurVisContextGuard(RecurVisContext &arg_rvc) : rvc_{arg_rvc} {
        rvc_.push();
    }
    ~RecurVisContextGuard() { rvc_.pop(); }

    RecurVisContextGuard(const RecurVisContextGuard &other) = delete;
    RecurVisContextGuard(RecurVisContextGuard &&other) = delete;
    RecurVisContextGuard &operator=(const RecurVisContextGuard &other) = delete;
    RecurVisContextGuard &operator=(RecurVisContextGuard &&other) = delete;

  private:
    RecurVisContext &rvc_;
};

inline std::unique_ptr<RecurVisContextGuard> guard() {
    return std::move(std::make_unique<RecurVisContextGuard>(rvc));
}

// #define RV act::RecurVisContextGuard rvcg(act::rvc);
#define RV auto rvcg = act::guard();
// rvc.set_file_name(file_name);
#define rv_out (act::rv())

} // namespace act