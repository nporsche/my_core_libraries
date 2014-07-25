#pragma once

namespace HC
{
    class RWLock
    {
    public:
        explicit RWLock( const wstring& name ):
        m_hMapFile(NULL),
            m_name(name),
            m_hMutex(NULL)
        {
            Init();        
        }

        ~RWLock()
        {
            if(m_hSemaphore[writers])
                CloseHandle(m_hSemaphore[writers]);

            if(m_hSemaphore[readers])
                CloseHandle(m_hSemaphore[readers]);

            if(m_hMutex)
                CloseHandle(m_hMutex);

            if(m_pState)
                UnmapViewOfFile(m_pState);

            if(m_hMapFile)
                CloseHandle(m_hMapFile);   
        }

        void LockExclusive()
        {
            WaitForSingleObject(m_hMutex,INFINITE);

            bool bFreeResource = (m_pState->active == 0);

            if(bFreeResource)
            {
                m_pState->active = -1;
            }
            else
            {
                ++m_pState->writeWaitCount;
            }

            ReleaseMutex(m_hMutex);

            if(!bFreeResource)
            {
                WaitForSingleObject(m_hSemaphore[writers], INFINITE);
            }
        }

        void LockShared()
        {
            WaitForSingleObject(m_hMutex, INFINITE);

            bool bPendingWrite = (m_pState->active < 0) || (m_pState->writeWaitCount > 0);  //check if writer is pending

            if(bPendingWrite)
            {
                ++m_pState->readWaitCount;
            }
            else
            {
                ++m_pState->active;
            }

            ReleaseMutex(m_hMutex);

            if(bPendingWrite)
            {
                WaitForSingleObject(m_hSemaphore[readers], INFINITE);
            }
        }

        void ReleaseLock()
        {
            WaitForSingleObject(m_hMutex, INFINITE);

            if(m_pState->active == 0)   //Something wrong here
                return; 

            if(m_pState->active > 0)    //Reader release
            {
                --m_pState->active;
            }
            else    //Writer release
            {
                ++m_pState->active;
            }    

            //once the lock is free, writer has the higher priority to get the lock.
            HANDLE hSem = NULL;
            unsigned long resourceCount = 1;
            if(m_pState->active == 0)
            {
                if(m_pState->writeWaitCount > 0)
                {
                    hSem = m_hSemaphore[writers];
                    resourceCount = 1;
                    --m_pState->writeWaitCount;
                    m_pState->active = -1;
                }
                else if(m_pState->readWaitCount > 0)
                {
                    hSem = m_hSemaphore[readers];
                    resourceCount = m_pState->readWaitCount;            
                    m_pState->active = resourceCount;
                    m_pState->readWaitCount = 0;
                }
            }

            ReleaseMutex(m_hMutex);

            if(hSem != NULL)
            {
                ReleaseSemaphore(hSem, resourceCount, NULL);
            }
        }


    private:    
        RWLock(const RWLock&){}
        RWLock& operator = (const RWLock&){}

        void Init()
        {
            m_hMapFile = CreateFileMappingW(
                INVALID_HANDLE_VALUE,   
                NULL,                   
                PAGE_READWRITE,         
                0,                      
                sizeof(State),          
                (m_name + L"MMF").c_str());

            if(m_hMapFile == NULL)
                throw exception("CreateFileMapping failed");

            m_pState = (State*) MapViewOfFile(m_hMapFile,
                FILE_MAP_ALL_ACCESS,
                0,                   
                0,                   
                sizeof(State));

            if(m_pState == NULL)
                throw exception("MapViewOfFile failed");

            m_hMutex = CreateMutexW(NULL, FALSE, (m_name + L"Mutex").c_str());
            if(ERROR_SUCCESS == GetLastError())
            {
                memset(m_pState, 0, sizeof(State));
            }

            if(m_hMutex == NULL)
                throw exception("CreateMutex failed");

            m_hSemaphore[readers] = CreateSemaphoreW(NULL,0, LONG_MAX, (m_name + L"Semaphore_reader").c_str());
            if(m_hSemaphore[readers] == NULL)
                throw exception("CreateSemaphore for reader failed");

            m_hSemaphore[writers] = CreateSemaphoreW(NULL,0, LONG_MAX, (m_name + L"Semaphore_writer").c_str());
            if(m_hSemaphore[writers] == NULL)
                throw exception("CreateSemaphore for writer failed");
        }

    private:
        HANDLE m_hSemaphore[2];
        HANDLE m_hMutex;
        HANDLE m_hMapFile;
        wstring m_name;
        enum semaphoreEnum
        {
            readers,
            writers
        };

        struct State
        {
            long active;    //how many threads are accessing if active > 0 readers, if active < 0 writers.
            unsigned long readWaitCount;    //how many readers are waiting for lock
            unsigned long writeWaitCount;   //how many writers are waiting for lock
        }* m_pState;    
    };
}