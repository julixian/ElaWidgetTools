#pragma once

import std;
import mcpp;
import mcpp.rules.qt;

namespace ela_build {
inline void configure_qt(const char* source_directory, const std::filesystem::path& resource_file,
                         std::span<const std::string> include_directories, bool library) {
    namespace fs = std::filesystem;
    // 环境变量优先；未设置时读取 Ela 根目录的路径文件第一行，再沿用插件自动查找。
    mcpp::rerun_if_env_changed("QT_ROOT_DIR");
    fs::path qt;
    if (const std::string_view environment_root = mcpp::env_or("QT_ROOT_DIR"); !environment_root.empty()) {
        qt = environment_root;
    } else {
        const auto configuration = fs::path(mcpp::manifest_dir()).parent_path() / "QT_ROOT_DIR.txt";
        mcpp::rerun_if_changed(configuration.generic_string().c_str());
        std::ifstream input(configuration);
        std::string root;
        std::getline(input, root);
        if (!root.empty() && root.back() == '\r') root.pop_back();
        qt = root;
        if (qt.empty()) qt = mcpp::rules::qt::root();
    }
    if (qt.empty()) throw std::runtime_error("请通过 QT_ROOT_DIR 或根目录 QT_ROOT_DIR.txt 指定 Qt SDK 根目录");
    const bool windows = std::string_view(mcpp::target_os()) == "windows";
    const bool macos = std::string_view(mcpp::target_os()) == "macos";
    // 对齐原 CMake Release 的 NDEBUG，以及 Qt targets 对非 Debug 配置提供的 QT_NO_DEBUG。
    // release/fast-release 即使保留调试符号，也关闭标准 assert 和 Qt 断言。
    const std::string_view profile = mcpp::profile();
    if (profile != "dev" && profile != "debug") {
        mcpp::define("NDEBUG");
        mcpp::define("QT_NO_DEBUG");
    }
    if (windows && std::string_view(mcpp::target_arch()) == "x86_64") mcpp::define("WIN64");
    const auto generated = fs::path(mcpp::out_dir()) / "qt";
    // 仅复用插件的 SDK 定位，不调用 compile：库构建无需复制 Qt DLL/插件。
    if (macos) mcpp::cxxflag(("-F" + (qt / "lib").generic_string()).c_str());
    else mcpp::include_dir((qt / "include").generic_string().c_str());
    for (const std::string module : {"Core", "Gui", "Widgets"}) {
        const auto headers = macos ? qt / "lib" / ("Qt" + module + ".framework") / "Headers" :
            qt / "include" / ("Qt" + module);
        const auto library = windows ? qt / "lib" / ("Qt6" + module + ".lib") :
            macos ? qt / "lib" / ("Qt" + module + ".framework") / ("Qt" + module) :
            qt / "lib" / ("libQt6" + module + ".so");
        mcpp::include_dir(headers.generic_string().c_str());
        mcpp::link_flag(library.generic_string().c_str());
        mcpp::define(("QT_" + mcpp::rules::qt::detail::upper(module) + "_LIB").c_str());
        const auto private_headers = mcpp::rules::qt::detail::private_dir(qt, module);
        mcpp::include_dir(private_headers.generic_string().c_str());
        mcpp::include_dir((private_headers / ("Qt" + module)).generic_string().c_str());
    }
    if (std::string_view(mcpp::compiler()) == "msvc") {
        mcpp::cxxflag("/Zc:__cplusplus");
        mcpp::cxxflag("/permissive-");
    }
    if (!windows) {
        if (!macos) mcpp::cxxflag("-fPIC");
        mcpp::runtime_search_dir((qt / "lib").generic_string().c_str());
    }
    const std::vector<fs::path> qt_roots = {qt};
    const auto moc = mcpp::rules::qt::detail::tool(qt_roots, "moc");
    // moc 必须解析 Ela 的属性宏及平台条件，传入库头文件目录、Qt 头文件目录和宏。
    mcpp::rerun_if_changed(__FILE__);
    mcpp::rerun_if_changed_glob((std::string(source_directory) + "/**/*.h").c_str());
    for (auto it = fs::recursive_directory_iterator(source_directory); it != fs::recursive_directory_iterator(); ++it) {
        const auto& entry = *it;
        if (entry.is_directory() && (entry.path().filename() == "target" || entry.path().filename() == ".git")) {
            it.disable_recursion_pending();
            continue;
        }
        if (!entry.is_regular_file() || entry.path().extension() != ".h") continue;
        const auto header = fs::absolute(entry.path()).generic_string();
        mcpp::rerun_if_changed(header.c_str());
        std::ifstream input(entry.path(), std::ios::binary);
        const std::string text{std::istreambuf_iterator<char>(input), {}};
        if (text.find("Q_OBJECT") == std::string::npos && text.find("Q_GADGET") == std::string::npos &&
            text.find("Q_NAMESPACE") == std::string::npos) continue;
        const auto filename = "moc_" + entry.path().stem().string() + ".cpp";
        const auto output = (generated / filename).generic_string();
        const auto dependency = output + ".d";
        const auto id = "ela-moc:" + filename;
        mcpp::action action;
        action.id = id.c_str();
        action.role = mcpp::roles::source;
        action.depfile = dependency.c_str();
        action.arg(moc.c_str()).arg(header.c_str()).arg("-o").arg(output.c_str())
            .arg("--output-dep-file").arg("--dep-file-path").arg(dependency.c_str());
        if (library) action.arg("-DELAWIDGETTOOLS_LIBRARY");
        if (std::string_view(mcpp::target_os()) == "windows") {
            action.arg("-D_WIN32");
            if (std::string_view(mcpp::target_arch()) == "x86_64") action.arg("-D_WIN64");
        }
        for (const auto& directory : include_directories)
            action.arg(("-I" + (fs::path(mcpp::manifest_dir()) / directory).generic_string()).c_str());
        action.arg(("-I" + (qt / "include").generic_string()).c_str());
        for (const char* module : {"QtCore", "QtGui", "QtWidgets"})
            action.arg(("-I" + (qt / "include" / module).generic_string()).c_str());
        action.input(header.c_str()).input(moc.c_str()).output(output.c_str()).submit();
    }

    const auto qrc = fs::absolute(resource_file);
    const auto resource_name = qrc.stem().string();
    const auto rcc = mcpp::rules::qt::detail::tool(qt_roots, "rcc");
    const auto resource_output = (generated / ("qrc_" + resource_name + ".cpp")).generic_string();
    mcpp::rerun_if_changed(qrc.generic_string().c_str());
    mcpp::action resource;
    resource.id = "ela-rcc";
    resource.role = mcpp::roles::source;
    resource.arg(rcc.c_str()).arg("--name").arg(resource_name.c_str()).arg(qrc.generic_string().c_str())
        .arg("-o").arg(resource_output.c_str()).input(qrc.generic_string().c_str()).input(rcc.c_str());
    for (const auto& file : mcpp::rules::qt::detail::qrc_files(qrc)) {
        mcpp::rerun_if_changed(file.c_str());
        resource.input(file.c_str());
    }
    resource.output(resource_output.c_str()).submit();

}
} // namespace ela_build
