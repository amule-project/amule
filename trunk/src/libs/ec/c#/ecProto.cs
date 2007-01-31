using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace amule.net
{
    enum EcTagTypes {
        EC_TAGTYPE_UNKNOWN = 0,
        EC_TAGTYPE_CUSTOM = 1,
        EC_TAGTYPE_UINT8 = 2,
        EC_TAGTYPE_UINT16 = 3,
        EC_TAGTYPE_UINT32 = 4,
        EC_TAGTYPE_UINT64 = 5,
        EC_TAGTYPE_STRING = 6,
        EC_TAGTYPE_DOUBLE = 7,
        EC_TAGTYPE_IPV4 = 8,
        EC_TAGTYPE_HASH16 = 9
    };

    //
    // Flags sent on every packet
    //
    enum EcFlags {
        EC_FLAG_ZLIB = 0x00000001,
        EC_FLAG_UTF8_NUMBERS = 0x00000002,
        EC_FLAG_HAS_ID = 0x00000004,
        EC_FLAG_ACCEPTS = 0x00000010,
//        EC_FLAG_UNKNOWN_MASK = 0xff7f7f08
    };

    class ecProto {
        public class ecTag {
            protected int m_size;
            protected EcTagTypes m_type;
            LinkedList<ecTag> m_subtags;

            public ecTag(EcTagTypes t)
            {
                m_type = t;
                m_subtags = new LinkedList<ecTag>();
            }
            public ecTag()
            {
                m_subtags = new LinkedList<ecTag>();
            }

            public virtual void Write(BinaryWriter wr)
            {
            }

            public int Size()
            {
                int total_size = m_size;
                foreach (ecTag t in m_subtags) {
                    total_size += t.Size();
                }
                return total_size;
            }

            public void AddSubtag(ecTag t)
            {
                m_subtags.AddLast(t);
            }
        }

        public class ecTagInt : ecTag {
            private UInt32 m_val;
            public ecTagInt(UInt32 v)
                : base(EcTagTypes.EC_TAGTYPE_UINT32)
            {
                m_val = v;
                m_size = 4;
            }

            public override void Write(BinaryWriter wr)
            {
                byte b0 = (byte)(m_val & 0xff), b1 = (byte)((m_val >> 8 ) & 0xff),
                    b2 = (byte)((m_val >> 16) & 0xff), b3 = (byte)((m_val >> 24) & 0xff);
                
                switch ( m_size ){
                    case 4:
                        wr.Write(b2);
                        wr.Write(b3);
                        goto case 2;
                    case 2:
                        wr.Write(b1);
                        goto case 1;
                    case 1:
                        wr.Write(b0);
                        break;
                }

            }
        }

        public class ecPacket : ecTag {
            // since I have no zlib here, effectively disable compression
            const int MaxUncompressedPacket = 0x6666;

            //
            // Parsing ctor
            //
            private byte m_opcode;

            public ecPacket(byte [] rxBuffer, int len)
            {
                MemoryStream memStream = new MemoryStream(rxBuffer, 0, len);
                BinaryReader binReader = new BinaryReader(memStream);
            }

            //
            // Default ctor - for tx packets
            public ecPacket(byte cmd)
            {
                m_opcode = cmd;
            }

            public override void Write(BinaryWriter wr)
            {
                UInt32 flags = 0x20;
                int packet_size = Size();
                if ( packet_size > MaxUncompressedPacket ) {
                    flags |= (UInt32)EcFlags.EC_FLAG_ZLIB;
                }

                if ( (flags & (UInt32)EcFlags.EC_FLAG_ZLIB) != 0 ) {
                    throw new NotImplementedException("no zlib compression yet");
                }

                wr.Write(flags);
                wr.Write(packet_size);
                wr.Write(m_opcode);
            }


        }
    }
}
