#pragma once

namespace HC
{
    template<class T>
    class CReader 
    {
    public:
        CReader() {}

        ~CReader(void) 
        {            
        }

        void Init(const wstring& name)
        {
            Accessiblity access;
            access.AccessRead();
            m_T.Init(name,access);
        }

        void Uninit()
        {
            m_T.Uninit();
        }

        void SetCursor(LONGLONG index)
        {
            m_T.SetReadCursor(index);
        }

        void SetCursorToEnd()
        {
            m_T.SetReadCursorToEnd();
        }

        LONGLONG GetCursor() 
        {
            return m_T.GetReadCursor();
        }

        void Read(char* p, unsigned long count, unsigned long* pBytesHaveRead)
        {
            m_T.Read(p, count, pBytesHaveRead);
        }

        void Peek(char* p, unsigned long count, unsigned long* pBytesHaveRead)
        {
            m_T.Peek(p, count, pBytesHaveRead);
        }

    private:
        T m_T;
    };
}