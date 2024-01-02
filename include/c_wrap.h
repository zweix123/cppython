#pragma once

#include "utils.h"
#include <array>
#include <atomic>
#include <cassert>
#include <codecvt>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <variant>
#include <vector>

// _WIN32
// __linux__
// __APPLE__
//
#ifdef _WIN32
#include <Python.h>
const std::string LIB_PATH_SEP = ";";
#endif

#ifdef __linux__
#include <python3.10/Python.h>
const std::string LIB_PATH_SEP = ":";
#endif

#ifdef _WIN32
#define popen  _popen
#define pclose _pclose
#endif

namespace act {

static void python_path_init() {
    // 保证只进行一次
    static bool path_inited = false;
    if (path_inited) return;
    path_inited = true;
    // HACK: 如果下面的代码有BUG，即没有正确set path, 但是flag var却true了

    // 通过sys.path拿到解释器在导入模块时会搜索的路径
    const char *get_sys_path_cmd =
        "python3 -c \"import sys; _ = list(map(lambda p: print(p),sys.path))\"";
    std::string cmd_out;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(
        popen(get_sys_path_cmd, "r"), pclose);
    if (pipe) {
        std::array<char, 1024> buffer;
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
            cmd_out += buffer.data();
    } else {
        std::cerr << "Error: Failed to execute the command: "
                  << get_sys_path_cmd << std::endl;
        std::exit(1);
    }

    // 解析出sys中的路径
    std::vector<std::string> paths;
    {
        std::istringstream iss(cmd_out);
        std::string dummy_path;
        while (std::getline(iss, dummy_path, '\n')) {
            dummy_path.erase(
                dummy_path.find_last_not_of("\n") + 1); // 去掉末尾换行符
            paths.push_back(dummy_path);
        }
    }

    // 将项目中include这个路径也放入
    {
        std::filesystem::path pwd = std::filesystem::current_path();
        auto include_path = pwd / "include";
        paths.push_back(include_path.string());
    }

    // 使用C/Python API set path
    auto all_path_str = join(LIB_PATH_SEP, paths);
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wide_path_str = converter.from_bytes(all_path_str);
    Py_SetPath(wide_path_str.c_str());
}

class Interpreter {
  public:
    Interpreter() {
        python_path_init();
        Py_Initialize(); // TODO: 多次使用安全
    }
    ~Interpreter() {
        for (auto pObj : context_) {
            assert(pObj != nullptr);
            Py_DECREF(pObj);
        }
        Py_Finalize(); // TODO: 多次使用安全
    }

  public:
    void err() {
        PyErr_Print();
        exit(1);
    }
    PyObject *reg(PyObject *pObj) { // register注册
        if (pObj != nullptr) {
            context_.insert(pObj);
            return pObj;
        }
        err();
        return nullptr;
    }

  public:
    template<typename T>
    PyObject *wrap(const T &var) {
        if constexpr (std::is_same_v<T, bool>) {
            return reg(PyBool_FromLong(var));
        } else if constexpr (
            std::is_same_v<T, int> || std::is_same_v<T, long>) {
            return reg(PyLong_FromLong(var));
        } else if constexpr (
            std::is_same_v<T, float> || std::is_same_v<T, double>) {
            return reg(PyFloat_FromDouble(var));
        } else if constexpr (std::is_same_v<T, std::string>) {
            return PyUnicode_FromString(var.c_str());
        } else {
            throw std::runtime_error("Handle unsupported types");
            return nullptr;
        }
    }
    template<typename... Ts>
    PyObject *wrap(const std::variant<Ts...> &var) {
        return std::visit(
            [&](const auto &value) -> PyObject * { return wrap(value); }, var);
    }
    template<typename T>
    PyObject *wrap(const std::vector<T> &vec) { // list
        PyObject *pList = reg(PyList_New(0));
        for (auto value : vec) { PyList_Append(pList, wrap(value)); }
        return pList;
    }
    template<typename T, int N>
    PyObject *wrap(const std::array<T, N> &arr) { // tuple
        PyObject *pTuple = reg(PyTuple_New(N));
        for (int i = 0; i < N; ++i) {
            PyTuple_SetItem(pTuple, 0, wrap(arr[i]));
        }
        return pTuple;
    }
    template<typename T>
    PyObject *wrap(const std::set<T> &set) { // set
        PyObject *pSet = reg(PySet_New(nullptr));
        for (auto &value : set) { PySet_Add(pSet, wrap(value)); }
        return pSet;
    }
    template<typename Key, typename Value>
    PyObject *wrap(const std::map<Key, Value> &map) { // dict
        PyObject *pDict = reg(PyDict_New());
        for (auto &pair : map) {
            PyDict_SetItem(pDict, make(pair.first), wrap(pair.second));
        }
        return pDict;
    }

  public:
    std::variant<std::monostate, bool, int, double, std::string>
    unwrap(PyObject *pObj) {
        if (PyBool_Check(pObj)) {
            bool value = (pObj == Py_True);
            return value;
        }
        if (PyLong_Check(pObj)) {
            int value = PyLong_AsLong(pObj);
            if (value == -1 && PyErr_Occurred()) { err(); }
            return value;
        }
        if (PyFloat_Check(pObj)) {
            double value = PyFloat_AsDouble(pObj);
            return value;
        }
        if (PyBytes_Check(pObj)) {
            const char *pBytes = PyBytes_AsString(pObj);
            if (!pBytes) { err(); }
            return std::string(pBytes);
        }
        if (PyUnicode_Check(pObj)) {
            PyObject *pBytes =
                reg(PyUnicode_AsEncodedString(pObj, "utf-8", "strict"));
            if (!pBytes) { err(); }
            return std::string(PyBytes_AsString(pBytes));
        }
        if (PyList_Check(pObj)) {
            Py_ssize_t size = PyList_Size(pObj);
            for (Py_ssize_t j = 0; j < size; j++) {
                PyObject *item = PyList_GetItem(pObj, j);
                // TODO
            }
            return std::monostate{};
        }
        if (PyDict_Check(pObj)) {
            // TODO
            return std::monostate{};
        }
        if (PyFunction_Check(pObj)) {
            // TODO
            return std::monostate{};
        }
        if (PyModule_Check(pObj)) {
            const char *name = PyModule_GetName(pObj);
            // TODO
            return std::monostate{};
        }
        return std::monostate{};
    }

  public:
    template<typename T = std::monostate, typename... Args>
    T call(
        const std::string &module_name,
        const std::string &function_name,
        Args &&...args) {
        PyObject *pModule = reg(PyImport_ImportModule(module_name.c_str()));
        PyObject *pFunc =
            reg(PyObject_GetAttrString(pModule, function_name.c_str()));

        const auto num_args = sizeof...(Args);
        auto pArgs = reg(PyTuple_New(num_args));

        int i = 0;
        ((PyTuple_SetItem(pArgs, i++, wrap(std::forward<Args>(args))), ...));

        PyObject *pResult = reg(PyObject_CallObject(pFunc, pArgs));
        /* 有三种C调用Python function的方式
        以
        ```python
        def add(x, y):
            return x + y
        ```
        为例
        1.
            ```
            PyObject *pArgs = PyTuple_New(2);
            PyTuple_SetItem(pArgs, 0, PyLong_FromLong(1));
            PyTuple_SetItem(pArgs, 1, PyLong_FromLong(2));
            PyObject *result = PyObject_CallObject(pFunc, pArgs);
            ```
        2.
            ```
            pArgs = Py_BuildValue("ii", 3, 4);
            result = PyObject_CallFunction(pFunc, "O", pArgs);
            ```
        3.
            ```
            result = PyObject_CallFunction(pFunc, "ii", 6, 7);
            ```
        */

        auto dummpy_result_var = unwrap(pResult);
        return std::get<T>(dummpy_result_var);
    }

  private:
    // static std::atomic<int> counter_;
    std::unordered_set<PyObject *> context_;
};

} // namespace act