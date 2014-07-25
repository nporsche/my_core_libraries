#pragma once
#include "AcqFrameCommon.h"
#include "RWLock.h"

namespace HC
{
    class CMMF
    {
    public:
        CMMF():
              m_hMMF(NULL),
              m_hFile(NULL),
              m_WriteOffset(0),
              m_ReadOffset(0),
              m_rwLock(NULL),
              m_dwAllocationGranularity(0)
          {}

          ~CMMF(void)
          {
              Uninit();
          }

          void Init(const wstring& name, Accessiblity access)
          {    
              SYSTEM_INFO info;
              GetSystemInfo(&info);
              m_dwAllocationGranularity = info.dwAllocationGranularity;

              m_name = name;
              m_FileMappingObjectName = RetrieveName(L"_MMF");

              m_rwLock = new RWLock(RetrieveName(L"_RWLock"));

              try
              {
                  unsigned long fileAccess = GENERIC_READ;
                  unsigned long dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
                  unsigned long dwCreationMode = OPEN_EXISTING;
                  if(access.Writeable())
                  {
                      dwCreationMode = OPEN_ALWAYS;
                      fileAccess |= GENERIC_WRITE;
                      dwShareMode ^= FILE_SHARE_WRITE; //clear share write flag
                  }

                  m_hFile = CreateFileW(m_name.c_str(), fileAccess, dwShareMode, NULL, dwCreationMode, FILE_ATTRIBUTE_NORMAL, NULL);
                  if(m_hFile == INVALID_HANDLE_VALUE)
                      throw exception("Cannot create or open file");   
              }
              catch(exception e)
              {
                  Uninit();
              }
          }

          void Uninit()
          {
              if(m_rwLock)
              {
                  delete m_rwLock;
                  m_rwLock = NULL;
              }

              if(m_hMMF)
              {
                  CloseHandle(m_hMMF);
                  m_hMMF = NULL;
              }
              if(m_hFile)
              {
                  CloseHandle(m_hFile);
                  m_hFile = NULL;
              }

              m_WriteOffset = 0;
              m_ReadOffset = 0;
          }

          void SetReadCursor(LONGLONG index )
          {
              m_ReadOffset = index;
          }

          void SetWriteCursor( LONGLONG index )
          {
              m_WriteOffset = index;
          }

          void SetWriteCursorToEnd()
          {
              LONGLONG size = FileSize();
              SetWriteCursor(size);
          } 

          void SetReadCursorToEnd()
          {
              LONGLONG size = FileSize();
              SetReadCursor(size);
          }

          LONGLONG GetReadCursor()
          {
              return m_ReadOffset;
          }

          LONGLONG GetWriteCursor()
          {
              return m_WriteOffset;
          }

          void Write( char* p, unsigned long count, unsigned long* pBytesHaveWrite )
          {    
              *pBytesHaveWrite = 0;
              if(p == NULL || count == 0)
                  return;   

              LONGLONG mappingSize = m_WriteOffset + count;
              LONGLONG fileSize = FileSize();

              if (fileSize < mappingSize)
              {
                  GrowFileSize(mappingSize);
              }

              else if(!IsMappingFileCreated())
              {
                  CreateMappingFile(fileSize);
              }

              LONGLONG baseGranularityAddress = m_WriteOffset - (m_WriteOffset % m_dwAllocationGranularity);
              LONGLONG distanceBetweenBaseToCursor = m_WriteOffset - baseGranularityAddress;

              char* pBuf = (char*)MapViewOfFile(m_hMMF, FILE_MAP_READ|FILE_MAP_WRITE, HIDWORD(baseGranularityAddress), LODWORD(baseGranularityAddress), (SIZE_T)(distanceBetweenBaseToCursor + count));

              if(pBuf != NULL)
              {
                  memcpy(pBuf + distanceBetweenBaseToCursor, p, count);
                  *pBytesHaveWrite = count;
                  m_WriteOffset += *pBytesHaveWrite;   
              }
              else
              {       
                  throw exception("Failed at MapViewOfFile");
              }

              UnmapViewOfFile(pBuf);
          }

          void Read( char* p, unsigned long count, unsigned long* pBytesHaveRead )
          {
              Peek(p,count,pBytesHaveRead);

              if(*pBytesHaveRead == count)
                  m_ReadOffset += *pBytesHaveRead;
          }


          void Peek( char* p, unsigned long count, unsigned long* pBytesHaveRead )
          {
              *pBytesHaveRead = 0;

              LONGLONG fileSize = FileSize();

              if(fileSize == 0)
                  return;

              if(fileSize < m_ReadOffset + count)
                  return;

              m_rwLock->LockShared();

              m_hMMF = OpenFileMappingW(FILE_MAP_READ, FALSE, m_FileMappingObjectName.c_str());
              if(NULL == m_hMMF)
              {
                  OutputDebugStringW(L"[MMF][Read][Warning]OpenFileMapping failed, will try CreateFileMapping with NULL name");
                  m_hMMF = CreateFileMappingW(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL);
                  if(NULL == m_hMMF)
                  {
                      OutputDebugStringW(L"[MMF][Read][Error]CreateFileMapping failed, throw exception");
                      throw exception("Cannot map the file");
                  }
              }        

              LONGLONG baseGranularityAddress = m_ReadOffset - (m_ReadOffset % m_dwAllocationGranularity);
              LONGLONG distanceBetweenBaseToCursor = m_ReadOffset - baseGranularityAddress;

              char* pBuf = (char*)MapViewOfFile(m_hMMF, FILE_MAP_READ, HIDWORD(baseGranularityAddress), LODWORD(baseGranularityAddress),(SIZE_T)(count + distanceBetweenBaseToCursor));

              if(pBuf != NULL)
              {
                  memcpy(p, pBuf + distanceBetweenBaseToCursor, count);
                  *pBytesHaveRead = count;
              }    
              else
              {
                  throw exception("Failed at MapViewOfFile");
              }
              UnmapViewOfFile(pBuf);
              CloseHandle(m_hMMF);
              m_hMMF = NULL;

              m_rwLock->ReleaseLock();
          }


    private:
        LONGLONG FileSize()
        {
            LARGE_INTEGER l;
            if(!GetFileSizeEx(m_hFile, &l))
                throw exception("Cannot get file size");

            return l.QuadPart;
        }

        wstring RetrieveName(const wstring& postFix)
        {
            long pos1 = m_name.find_last_of(L"\\");
            long pos2 = m_name.find_first_of(L".");
            wstring str = m_name.substr(pos1 + 1, pos2 - pos1 - 1);
            str += postFix;
            return str;
        }

        void GrowFileSize(LONGLONG minRequiredSize)
        {
            LONGLONG bytesPerKB = 1024;
            LONGLONG growChunk = 32 * bytesPerKB;

            LONGLONG newFileSize = (minRequiredSize / growChunk + 1) * growChunk;

            m_rwLock->LockExclusive();

            CloseHandle(m_hMMF);

            m_hMMF = CreateFileMappingW(m_hFile, NULL, PAGE_READWRITE, HIDWORD(newFileSize), LODWORD(newFileSize), m_FileMappingObjectName.c_str());
            if(NULL == m_hMMF)
                throw exception("Cannot map the file");

            if(GetLastError() == ERROR_ALREADY_EXISTS)
            {
                OutputDebugStringW(L"[MMF][Write][ERROR] CreateFileMapping with ERROR_ALREADY_EXISTS");
            }

            m_rwLock->ReleaseLock();
        }

        bool IsMappingFileCreated()
        {
            return m_hMMF != NULL;
        }

        void CreateMappingFile( LONGLONG mappingSize )
        {
            m_hMMF = CreateFileMappingW(m_hFile, NULL, PAGE_READWRITE, HIDWORD(mappingSize), LODWORD(mappingSize), m_FileMappingObjectName.c_str());
            if(NULL == m_hMMF)
                throw exception("Cannot map the file");
        }

    private:
        LONGLONG m_ReadOffset;
        LONGLONG m_WriteOffset;
        wstring m_name;
        wstring m_FileMappingObjectName;
        HANDLE m_hFile;
        HANDLE m_hMMF;
        unsigned long m_dwAllocationGranularity;
        RWLock* m_rwLock;
    };
}
