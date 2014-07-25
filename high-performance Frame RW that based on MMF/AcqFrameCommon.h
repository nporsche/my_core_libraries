#pragma once

#include <assert.h>
#define HEADER_FLAG 0xffffffff
#define LODWORD(_qw)    ((unsigned long)(_qw))
#define HIDWORD(_qw)    ((unsigned long)(((_qw) >> 32) & 0xffffffff))

namespace HC
{
    struct FrameStreamHeader
    {
        LONGLONG WriteEndCursor;
        unsigned long LastSequenceID;
        unsigned long headerFlag;
        bool IsValid()
        {
            return HEADER_FLAG == headerFlag;
        }
    };

    /*|----|----|----------|*/
    /*|len |seqid|payload|*/
    class AcqFrame
    {
    public:
        struct Header
        {
            unsigned long m_FrameLength;
            unsigned long m_seq;     
            static unsigned long Length()
            {
                return sizeof(Header);
            }
        };

        struct Buffer
        {
            Buffer(unsigned long s = 32)
            {
                Init(s);
            }

            ~Buffer()
            {
                Destroy();
            }

            void Init( unsigned long s )
            {
                size = s;
                p = new char[size];
            }

            void Destroy()
            {
                delete[] p;
                p = NULL;
                size = 0;
            }

            void Attach(char* pBuffer, unsigned long s)
            {
                assert(pBuffer != NULL && s > 0);
                if(pBuffer != NULL && s > 0)
                {
                    Destroy();
                    p = pBuffer;
                    size = s;
                }
            }

            void Detach()
            {
                p = NULL;
                size = 0;
            }

            void Grow(unsigned long expectedCapacity)
            {
                if(size >= expectedCapacity)
                    return;

                while (size < expectedCapacity)
                    size <<= 1;

                delete[] p;
                p = new char[size];
            }

            unsigned long size;
            char* p;
        };

    public:
        AcqFrame()
        {
            memset(&header, 0, Header::Length());
        }
        ~AcqFrame()
        {

        }

        void Create(char* pPayLoad, unsigned long payloadLen, unsigned long seqID)
        {        
            header.m_FrameLength = payloadLen + Header::Length();
            header.m_seq = seqID;

            if(buffer.size < header.m_FrameLength)
            {
                buffer.Grow(header.m_FrameLength);          
            }
            memcpy(buffer.p, &header, Header::Length());
            memcpy(buffer.p + Header::Length(), pPayLoad, payloadLen);
        }

        void Destroy()
        {
            memset(&header, 0, Header::Length());
        }

        void Attach(char* p, unsigned long length)
        {
            assert(p != NULL && length > 0);       

            if(p != NULL && length > 0)
            {
                buffer.Attach(p,length);
                memcpy((char*)&header, buffer.p, Header::Length());
            }
        }

        void Detech()
        {
            buffer.Detach();
            memset(&header, 0, Header::Length());
        }

        char* FrameBytes()
        {
            return buffer.p;
        }

        //including len itself
        unsigned long FrameLength()
        {
            return header.m_FrameLength;
        }

        char* PayloadBytes()
        {
            return buffer.p + Header::Length();
        }

        unsigned long PayloadLength()
        {
            return header.m_FrameLength - Header::Length();
        }

        unsigned long SeqID()
        {
            return header.m_seq;
        }

    private:
        Header header;
        Buffer buffer;
    };

    class Accessiblity
    {
    public:
        Accessiblity():
          m(0)
          {

          }
          ~Accessiblity()
          {

          }

          void AccessRead()
          {
              m |= READACCESS;
          }

          void AccessWrite()
          {
              m |= WRITEACCESS;
          }

          bool Readable()
          {
              return (m & READACCESS) > 0;
          }

          bool Writeable()
          {
              return (m & WRITEACCESS) > 0;
          }
    private:

        enum ACCESS
        {
            READACCESS = 0x00000001,
            WRITEACCESS = 0x00000002
        }; 

        unsigned int m;
    };

}





