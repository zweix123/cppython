#!/usr/bin/env python3

import argparse
import os
import re
import subprocess
import sys
import sysconfig


class MultiOS:
    @staticmethod
    def example():
        if sys.platform.startswith("win"):
            print("当前操作系统为 Windows")
        elif sys.platform.startswith("linux"):
            print("当前操作系统为 Linux")
        elif sys.platform.startswith("darwin"):
            print("当前操作系统为 macOS")
        else:
            print("未知的操作系统类型")

    @staticmethod
    def get_exe_file_suf():
        if sys.platform.startswith("win"):
            return ".exe"
        elif sys.platform.startswith("linux"):
            return ".out"
        else:
            assert False, "not support."

    @staticmethod
    def get_compile_opt():
        if sys.platform.startswith("win"):
            py_path = sys.prefix
            py_version = sysconfig.get_python_version()
            py_version = py_version.replace(".", "")
            return "-I {} -L {} {}".format(
                os.path.join(py_path, "include"),
                os.path.join(py_path, "libs"),
                os.path.join(py_path, "libs", f"python{py_version}.lib"),
            )
        elif sys.platform.startswith("linux"):
            py_version = sysconfig.get_python_version()
            return f"-lpython{py_version}"
        else:
            assert False, "not support."


def is_include_py(code_file_path: str):
    # 递归查找code file path文件是否include了Python.h(简单的文本比对)
    pattern = re.compile(r'#include\s*"([^"]+)"')
    with open(code_file_path, "r", encoding="UTF-8") as f:
        for line in f:
            if "Python.h" in line:  # check
                return True
            match = pattern.search(line)
            if match:
                include_code_file_name = match.group(1)
                if (
                    is_include_py(os.path.join("include", include_code_file_name))
                    is True
                ):
                    return True
    return False


def get_input_and_output_from_code_file(code_file_path: str):
    # 从代码文件中按照规定的格式拿到输入输出测试样例
    state: int = 0  # 0 默认状态, 1 input，2 output
    content_cache: list[str] = ["", "", ""]
    result: list[(str, str)] = list()

    with open(code_file_path, "r", encoding="UTF-8") as f:
        for line in f:
            if line.startswith("//>"):
                state = 1
                content_cache[state] += line[3:]
            elif line.startswith("//<"):
                state = 2
                content_cache[state] += line[3:]
            else:
                if state != 0:
                    # flash
                    result.append((content_cache[1], content_cache[2]))
                    content_cache: list[str] = ["", "", ""]
                state = 0
    if state != 0:
        result.append((content_cache[1], content_cache[2]))
        content_cache: list[str] = ["", "", ""]
        state = 0
    return result


def cmp_output(output: str, ans_text: str):
    output = output.strip()
    ans_text = ans_text.strip()
    output_lines = output.split("\n")
    ans_text_lines = ans_text.split("\n")
    if len(output_lines) != len(ans_text_lines):
        return False
    for output_line, ans_text_line in zip(output_lines, ans_text_lines):
        if output_line.strip() != ans_text_line.strip():
            return False
    return True


def compile_and_exe_file(code_file_path: str):
    # 编译并运行代码文件,
    # 可执行文件在构建目录下,
    # 如果代码有写在代码文本中的输入输出样例则用其测试
    # 否则则直接运行(可能代码使用断言测试或者真的是什么都没有)
    assert os.path.splitext(code_file_path)[1] in (
        ".py",
        ".c",
        ".cpp",
    ), "Only support c, cpp, py file."
    if code_file_path.endswith(".py"):  # python code file
        os.system(f"python3 {code_file_path}")
        return
    # other situations is c or cpp code file.

    exe_file_path = os.path.join(
        "build", os.path.splitext(code_file_path)[0] + MultiOS.get_exe_file_suf()
    )
    compile_cmd = f"g++ -std=c++2a {code_file_path} -o {exe_file_path}"
    compile_cmd += " -I " + os.path.join(os.getcwd(), "include")
    if is_include_py(code_file_path) is True:
        compile_cmd += " " + MultiOS.get_compile_opt()
    print(compile_cmd)
    complie_result = os.system(compile_cmd)
    if complie_result != 0:
        print(f"\033[31m编译错误\033[0m")
        sys.exit(1)
    else:
        print("编译成功")

    examples = get_input_and_output_from_code_file(code_file_path)
    if len(examples) == 0:  # not input and output
        result = subprocess.run([exe_file_path])
        if result.returncode != 0:
            print(f"\033[31m可执行文件出错, 错误码: {result.returncode}\033[0m")
            exit(1)
        else:
            print("运行完毕")
    else:
        for i, example in enumerate(examples):
            input_content, output_content = example
            output_content = output_content.strip()
            result = subprocess.run(
                [exe_file_path], input=input_content, text=True, capture_output=True
            )
            if result.returncode != 0:
                print(f"\033[31m可执行文件出错, 错误码: {result.returncode}\033[0m")
                exit(1)
            else:
                output_result = result.stdout.strip()
                if cmp_output(output_result, output_content):
                    print(f"\033[32m测试{i + 1}通过\033[0m")
                else:
                    print(f"\033[31m测试{i + 1}未通过\033[0m")
                    print("预期输出")
                    print(output_content)
                    print("实际输出")
                    print(output_result)


def compile_and_exe_all_file():
    import os

    # 定义需要查找的目录
    valid_folders = [
        os.path.join(".", valid_folder) for valid_folder in ["src", "test"]
    ]

    # 定义需要查找的文件后缀名
    extensions = (".c", ".py", ".cpp")

    # 获取当前脚本的绝对路径
    script_path = os.path.realpath(__file__)

    # 获取当前目录和递归目录下所有符合条件的文件路径
    file_paths = []
    for root, dirs, files in os.walk("."):
        if root in valid_folders:
            for filename in files:
                if filename.endswith(extensions):
                    file_path = os.path.join(root, filename)
                    if os.path.realpath(file_path) != script_path:
                        file_paths.append(file_path)

    for i, file_path in enumerate(file_paths):
        print(f"{i + 1}/{len(file_paths)}", file_path)
        compile_and_exe_file(file_path)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="(compile and) run c/cpp/py file.")
    parser.add_argument("file", nargs="?", help="A single c/cpp/py file.")
    parser.add_argument("--all", action="store_true", help="compile and run all file.")

    args = parser.parse_args()
    if args.all:
        compile_and_exe_all_file()
    elif args.file:
        file_path = args.file
        compile_and_exe_file(file_path)
    else:
        print("No file specified. Use either --all or provide a file path.")