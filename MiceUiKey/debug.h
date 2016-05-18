
#ifndef _DEBUG_H_
#define _DEBUG_H_
HANDLE StartSaveFile(char *szFileName);
void StopSaveFile(HANDLE hFile);
BOOL SaveLineToFile(HANDLE hFile,  char *pData, int len);
#endif //_DEBUG_H_
