#pragma once

#include "Writer.h"
#include "MMF.h"
#include "AcqFrameCommon.h"

namespace HC
{

    template<class T>
    class AcqFrameWriter
    {
    public:
        AcqFrameWriter(void)
        {

        }
        ~AcqFrameWriter(void)
        {            
        }

        void Init(const wstring& name)
        {
            m_writer.Init(name);

            if(!RestoreFrameStreamHeader(name))
            {
                m_FrameStreamheader.headerFlag = HEADER_FLAG;
                m_FrameStreamheader.LastSequenceID = 1;
                m_FrameStreamheader.WriteEndCursor = sizeof(FrameStreamHeader);
                FlushStreamInfo();
            }

            m_writer.SetWriteCursor(m_FrameStreamheader.WriteEndCursor);
        }

        void UnInit()
        {
            m_writer.UnInit();
        }

        LONGLONG GetCursor()
        {
            return m_writer.GetWriteCursor();
        }

        void SetCursor(LONGLONG cursor)
        {
            m_writer.SetWriteCursor(cursor);
        }

        void SetCursorToEnd()
        {
            m_writer.SetWriteCursorToEnd();
        }

        bool Write(char* pBuffer, unsigned long bufferSize, unsigned long count)
        {
            m_frame.Create(pBuffer, count, MakeSequenceID());

            unsigned long bytesHaveWritten = 0;
            m_writer.Write(m_frame.FrameBytes(), m_frame.FrameLength(), &bytesHaveWritten);

            bool result = false;
            if(bytesHaveWritten == m_frame.FrameLength())
            {
                m_FrameStreamheader.WriteEndCursor = GetCursor();

                FlushStreamInfo();

                result = true;
            }

            m_frame.Destroy();

            return result;
        }

    private:
        bool RestoreFrameStreamHeader(const wstring& name )
        {
            CReader<CMMF> reader;
            reader.Init(name);
            reader.SetCursor(0);
            unsigned long length = 0;
            reader.Read((char*)&m_FrameStreamheader, sizeof(m_FrameStreamheader), &length);
            return length == sizeof(m_FrameStreamheader) && m_FrameStreamheader.IsValid();
        }

        unsigned long MakeSequenceID()
        {
            m_FrameStreamheader.LastSequenceID++;
            return m_FrameStreamheader.LastSequenceID;
        }

        void FlushStreamInfo()
        {
            SetCursor(0);

            unsigned long bytesHaveWritten = 0;
            m_writer.Write((char*)&m_FrameStreamheader, sizeof(m_FrameStreamheader), &bytesHaveWritten);

            SetCursor(m_FrameStreamheader.WriteEndCursor);
        }

    private:
        FrameStreamHeader m_FrameStreamheader;
        Writer<T> m_writer;
        AcqFrame m_frame;
    };

}