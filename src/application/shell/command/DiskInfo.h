
#ifndef __DiskInfo_include__
#define __DiskInfo_include__

#include "lib/stream/OutputStream.h"
#include "lib/string/String.h"
#include "kernel/service/TimeService.h"
#include "Command.h"

/**
 * Implementation of Command.
 * Shows information about a storage device.
 *
 * -h, --help: Show help message
 *
 */
class DiskInfo : public Command {

private:
    Kernel::StorageService *storageService = nullptr;
    Filesystem *fileSystem = nullptr;

public:
    /**
     * Default-constructor.
     */
    DiskInfo() = delete;

    /**
     * Copy-constructor.
     */
    DiskInfo(const DiskInfo &copy) = delete;

    /**
     * Constructor.
     *
     * @param shell The shell, that executes this command
     */
    explicit DiskInfo(Shell &shell);

    /**
     * Destructor.
     */
    ~DiskInfo() override = default;

    /**
     * Overriding function from Command.
     */
    void execute(Util::Array<String> &args) override;

    /**
     * Overriding function from Command.
     */
    const String getHelpText() override;
};

#endif
