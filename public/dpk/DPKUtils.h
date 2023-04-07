#pragma once

// Fixes slashes in the directory name
void DPK_RebuildFilePath(const char* str, char* newstr);
void DPK_FixSlashes(EqString& str);