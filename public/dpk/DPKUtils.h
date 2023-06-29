#pragma once

// Fixes slashes in the directory name
void DPK_RebuildFilePath(const char* str, char* newstr);
void DPK_FixSlashes(EqString& str);
int	 DPK_FilenameHash(const char* filename, int version);

// use to append paths
int	 DPK_FilenameHashAppend(const char* filename, int startHash);

