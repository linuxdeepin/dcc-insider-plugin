// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

namespace {

constexpr char kSystemctlPath[] = "/usr/bin/systemctl";
constexpr char kConfigDir[] = "/etc/X11";
constexpr char kConfigFile[] = "default-display-manager";

struct DisplayManagerConfig
{
    const char *id;
    const char *serviceToEnable;
    const char *serviceToDisable;
    const char *displayManagerPath;
};

constexpr std::array<DisplayManagerConfig, 2> kAllowedDisplayManagers = {{
    { "lightdm", "lightdm.service", "ddm.service", "/usr/sbin/lightdm" },
    { "ddm", "ddm.service", "lightdm.service", "/usr/bin/ddm" },
}};

void logError(const char *message)
{
    std::cerr << message << ": " << std::strerror(errno) << '\n';
}

const DisplayManagerConfig *displayManagerConfig(std::string_view id)
{
    for (const auto &config : kAllowedDisplayManagers) {
        if (config.id == id) {
            return &config;
        }
    }

    return nullptr;
}

int runSystemctl(const char *command, const char *service, bool force = false)
{
    pid_t pid = fork();
    if (pid < 0) {
        logError("fork failed");
        return 1;
    }

    if (pid == 0) {
        if (force) {
            const char *argv[] = {
                kSystemctlPath,
                command,
                "--force",
                service,
                nullptr,
            };
            execv(kSystemctlPath, const_cast<char *const *>(argv));
        } else {
            const char *argv[] = {
                kSystemctlPath,
                command,
                service,
                nullptr,
            };
            execv(kSystemctlPath, const_cast<char *const *>(argv));
        }

        _exit(127);
    }

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) {
            continue;
        }

        logError("waitpid failed");
        return 1;
    }

    if (!WIFEXITED(status)) {
        std::cerr << kSystemctlPath << ' ' << command << ' ' << service << " did not exit normally\n";
        return 1;
    }

    return WEXITSTATUS(status);
}

void rollbackDisplayManagerSwitch(const DisplayManagerConfig &config)
{
    std::cerr << "rolling back display-manager service switch\n";

    const int enableStatus = runSystemctl("enable", config.serviceToDisable, true);
    if (enableStatus != 0) {
        std::cerr << "rollback failed to enable " << config.serviceToDisable << ": " << enableStatus << '\n';
    }

    const int disableStatus = runSystemctl("disable", config.serviceToEnable);
    if (disableStatus != 0) {
        std::cerr << "rollback failed to disable " << config.serviceToEnable << ": " << disableStatus << '\n';
    }
}

bool writeAll(int fd, std::string_view data)
{
    while (!data.empty()) {
        const ssize_t written = write(fd, data.data(), data.size());
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }

            logError("write failed");
            return false;
        }

        data.remove_prefix(static_cast<size_t>(written));
    }

    return true;
}

class PreparedDisplayManagerFile
{
public:
    explicit PreparedDisplayManagerFile(std::string_view displayManagerPath)
        : m_dirFd(open(kConfigDir, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW))
    {
        if (m_dirFd < 0) {
            logError("open /etc/X11 failed");
            return;
        }

        for (int i = 0; i < 128; ++i) {
            m_tempFileName = ".default-display-manager.tmp." + std::to_string(getpid()) + "." + std::to_string(i);
            m_fileFd = openat(m_dirFd,
                              m_tempFileName.c_str(),
                              O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
                              0644);
            if (m_fileFd >= 0) {
                break;
            }

            if (errno != EEXIST) {
                logError("open temporary display-manager file failed");
                return;
            }
        }

        if (m_fileFd < 0) {
            std::cerr << "failed to create a unique temporary display-manager file\n";
            return;
        }

        const std::string content = std::string(displayManagerPath) + '\n';
        if (!writeAll(m_fileFd, content)) {
            return;
        }

        if (fchmod(m_fileFd, 0644) < 0) {
            logError("fchmod failed");
            return;
        }

        if (fsync(m_fileFd) < 0) {
            logError("fsync temporary display-manager file failed");
            return;
        }

        if (close(m_fileFd) < 0) {
            m_fileFd = -1;
            logError("close temporary display-manager file failed");
            return;
        }
        m_fileFd = -1;
        m_ready = true;
    }

    ~PreparedDisplayManagerFile()
    {
        if (m_fileFd >= 0) {
            close(m_fileFd);
        }

        if (!m_committed && m_dirFd >= 0 && !m_tempFileName.empty()) {
            unlinkat(m_dirFd, m_tempFileName.c_str(), 0);
        }

        if (m_dirFd >= 0) {
            close(m_dirFd);
        }
    }

    bool isReady() const
    {
        return m_ready;
    }

    bool commit()
    {
        if (!m_ready) {
            return false;
        }

        if (renameat(m_dirFd, m_tempFileName.c_str(), m_dirFd, kConfigFile) < 0) {
            logError("rename display-manager file failed");
            return false;
        }

        m_committed = true;

        if (fsync(m_dirFd) < 0) {
            logError("fsync /etc/X11 failed");
        }

        return true;
    }

private:
    int m_dirFd = -1;
    int m_fileFd = -1;
    std::string m_tempFileName;
    bool m_ready = false;
    bool m_committed = false;
};

} // namespace

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "usage: dcc-insider-dm-switch <lightdm|ddm>\n";
        return 2;
    }

    const DisplayManagerConfig *config = displayManagerConfig(argv[1]);
    if (!config) {
        std::cerr << "invalid display manager\n";
        return 2;
    }

    PreparedDisplayManagerFile displayManagerFile(config->displayManagerPath);
    if (!displayManagerFile.isReady()) {
        return 1;
    }

    int status = runSystemctl("enable", config->serviceToEnable, true);
    if (status != 0) {
        return status;
    }

    status = runSystemctl("disable", config->serviceToDisable);
    if (status != 0) {
        rollbackDisplayManagerSwitch(*config);
        return status;
    }

    if (!displayManagerFile.commit()) {
        rollbackDisplayManagerSwitch(*config);
        return 1;
    }

    return 0;
}
