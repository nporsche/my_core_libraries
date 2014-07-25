#pragma once

namespace HC
{
    template <class T>
    class Writer
    {
    public:
        Writer(void)
        {

        }
        ~Writer(void)
        {

        }

        void Init(const wstring& name)
        {
            Accessiblity access;
            access.AccessWrite();
            m_T.Init(name, access);
        }

        void UnInit()
        {
            m_T.UnInit();
        }

        LONGLONG GetWriteCursor() 
        {
            return m_T.GetWriteCursor();
        }

        void SetWriteCursor(LONGLONG index)
        {
            m_T.SetWriteCursor(index);
        }

        void SetWriteCursorToEnd()
        {
            m_T.SetWriteCursorToEnd();
        }

        void Write(char* p, unsigned long dwCount, unsigned long* pBytesHaveWritten)
        {
            m_T.Write(p, dwCount, pBytesHaveWritten);
        }

    private:
        T m_T;
    };

}
