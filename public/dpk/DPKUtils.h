#pragma once

// Fixes slashes in the directory name
int	 DPK_FilenameHash(const char* filename, int version);

// use to append paths
int	 DPK_FilenameHashAppend(const char* filename, int startHash);

