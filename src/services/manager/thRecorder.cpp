#include "manager.hpp"

void startRecording(std::map<std::string, ArchiveParameters> *archivesToManage,
                    std::map<std::string, ArchiveParameters> *controlledArchives,
                    ArchiveManagerConfig *archiveManagerConfig,
                    std::mutex *archivesToManageMx)
{
    // Make a copy of archivesToManage with mutex protection
    std::map<std::string, ArchiveParameters> archivesToManageCopy;
    {
        std::lock_guard<std::mutex> lock(*archivesToManageMx);
        archivesToManageCopy = *archivesToManage;
    }
    // Mutex is now unlocked before fork()

    for (const auto &archiveEntry : archivesToManageCopy)
    {
        if (controlledArchives->find(archiveEntry.first) == controlledArchives->end())
        {
            std::string commandStr = archiveManagerConfig->archiveRecorderPath;
            std::string streamUUIDStr = archiveEntry.second.getStreamUUID();
            std::string configStr = "--config=" + archiveManagerConfig->configPath;

            // Create a vector of C-strings for execvp
            std::vector<char *> args;
            args.push_back(const_cast<char *>(commandStr.c_str()));
            args.push_back(const_cast<char *>(streamUUIDStr.c_str()));
            args.push_back(const_cast<char *>(configStr.c_str()));
            args.push_back(nullptr);

            pid_t pid = fork();

            switch (pid)
            {
            case -1:
                print(LogType::ERROR, "Error creating recorder process...");
                break;
            case 0:
                if (execvp(args[0], args.data()) == -1)
                {
                    print(LogType::ERROR, "Error executing recorder process... " + archiveEntry.second.getStreamUUID());
                    _exit(EXIT_FAILURE);
                }
                break;
            default:
                ArchiveParameters archiveWithPID = archiveEntry.second;
                archiveWithPID.setPID(pid);

                if (getDebug())
                    archiveWithPID.printInfo();

                controlledArchives->insert(std::pair<std::string, ArchiveParameters>(archiveWithPID.getStreamUUID(), archiveWithPID));

                break;
            }
        }
    }
}

void stopRecording(std::map<std::string, ArchiveParameters> *archivesToManage,
                   std::map<std::string, ArchiveParameters> *controlledArchives,
                   std::mutex *archivesToManageMx)
{
    // Make a copy of archivesToManage with mutex protection
    std::map<std::string, ArchiveParameters> archivesToManageCopy;
    {
        std::lock_guard<std::mutex> lock(*archivesToManageMx);
        archivesToManageCopy = *archivesToManage;
    }
    // Mutex is now unlocked

    for (auto it = controlledArchives->begin(); it != controlledArchives->end();)
    {
        if (it->second.getPID() && archivesToManageCopy.find(it->first) == archivesToManageCopy.end())
            kill(it->second.getPID(), SIGTERM);

        if (it->second.getPID() && waitpid(it->second.getPID(), nullptr, WNOHANG))
        {
            it = controlledArchives->erase(it);
            continue;
        }
        else
            it++;
    }
}

void recorderController(volatile sig_atomic_t *isInterrupted,
                        std::atomic<ServiceStatus> *serviceDigest,
                        ArchiveManagerConfig *archiveManagerConfig,
                        std::map<std::string, ArchiveParameters> *archivesToManage,
                        std::mutex *archivesToManageMx)
{

    print(LogType::DEBUGER, ">>> Start archive recorder controller thread");

    std::map<std::string, ArchiveParameters> controlledArchives;
    {
        std::lock_guard<std::mutex> lock(*archivesToManageMx);
        controlledArchives = *archivesToManage;
    }

    while (!*isInterrupted)
    {
        *serviceDigest = ServiceStatus::READY;

        stopRecording(archivesToManage, &controlledArchives, archivesToManageMx);
        startRecording(archivesToManage, &controlledArchives, archiveManagerConfig, archivesToManageMx);

        std::this_thread::sleep_for(std::chrono::milliseconds(ArchiveManagerConstants::DELAY_MS));
    }

    print(LogType::DEBUGER, "<<< Stop archive recorder controller thread");
}