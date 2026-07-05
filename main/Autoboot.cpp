#include "Autoboot.h"
#include <string.h>
#include <cstdlib>

static char* autobootPath = NULL;

namespace Autoboot
{
    void setPath(const char* path)
    {
        if (!path) {
			autobootPath = NULL;
            return;
		}

        if (strncmp(path, "sd:/", 4) != 0 &&
            strncmp(path, "usb:/", 5) != 0)
        {
			return;
        }
		size_t len = strlen(path);
		autobootPath = (char*)malloc(len + 1);

		if (!autobootPath)
			return;
		memcpy(autobootPath, path, len + 1);
    }

    const char* getPath()
    {
        return autobootPath;
    }

    bool hasPath()
    {
        return autobootPath != NULL;
    }
}