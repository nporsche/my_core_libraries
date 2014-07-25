#pragma once

#include "Reader.h"
#include "MMF.h"
#include "AcqFrameCommon.h"

namespace HC
{
    template<class T>
    class AcqFrameReader
    {
    public:
        AcqFrameReader()
        {

        }

        ~AcqFrameReader()
        {
            
        }

        void Init(wstring& name)
        {
            m_reader.Init(name);
            SetCursor(sizeof(FrameStreamHeader));
        }

        void UnInit()
        {
            m_reader.Uninit();
        }

        LONGLONG GetCursor()
        {
            return m_reader.GetCursor();
        }

        void SetCursor(LONGLONG cursor)
        {
            m_reader.SetCursor(cursor);
        }

        void SetCursorToEnd()
        {
            m_reader.SetCursorToEnd();
        }

        bool Read(char* pBuffer, unsigned long bufferSize,  unsigned long count)
        {
            if(bufferSize < count)
                throw exception("buffer size is not enough");

            if(!LoadStreamInfo())
                return false;

            if(m_frameStreamHeader.WriteEndCursor < GetCursor() + count)
                return false;

            unsigned long FrameSize = AcqFrame::Header::Length() + count;
            if(FrameSize > m_buffer.size)
                m_buffer.Grow(FrameSize);

            unsigned long bytesHaveRead = 0;
            m_reader.Read(m_buffer.p, FrameSize, &bytesHaveRead);

            if(bytesHaveRead != FrameSize)
                return false;

            AcqFrame frame;

            frame.Attach(m_buffer.p, bytesHaveRead);

            memcpy(pBuffer, frame.PayloadBytes(), count);

            frame.Detech();

            return true;
        }

        bool Peek(char* pBuffer, unsigned long bufferSize,  unsigned long count)
        {
            if(bufferSize < count)
                throw exception("buffer size is not enough");

            if(!LoadStreamInfo())
                return false;

            if(m_frameStreamHeader.WriteEndCursor < GetCursor() + count)
                return false;

            unsigned long byteHavePeeked = 0;
            m_reader.Peek(pBuffer, count, &byteHavePeeked);

            return byteHavePeeked == count;
        }

        unsigned long NextFrameLength()
        {
            unsigned long frameLength = 0;
            if(!Peek((char*)&frameLength, sizeof(frameLength),sizeof(frameLength)))
                return 0;

            return frameLength - AcqFrame::Header::Length();
        }

    private:
        bool LoadStreamInfo()
        {
            LONGLONG cur = m_reader.GetCursor(); 
            m_reader.SetCursor(0);

            unsigned long byteHaveRead = 0;
            m_reader.Read((char*)&m_frameStreamHeader, sizeof(m_frameStreamHeader), &byteHaveRead);

            m_reader.SetCursor(cur);

            return byteHaveRead == sizeof(m_frameStreamHeader) && m_frameStreamHeader.IsValid();
        }

    private:
        Reader<T> m_reader;
        FrameStreamHeader m_frameStreamHeader;
        AcqFrame::Buffer m_buffer;
    };
}