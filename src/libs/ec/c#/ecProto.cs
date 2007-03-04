using System;
using System.IO;
using System.Security.Cryptography;
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

    class ecProto {

        public class ecTag {
            protected int m_size;
            protected EcTagTypes m_type;
            protected ECTagNames m_name;
            protected LinkedList<ecTag> m_subtags;

            public ecTag(ECTagNames n, EcTagTypes t)
            {
                m_name = n;
                m_type = t;
                m_subtags = new LinkedList<ecTag>();
            }
            public ecTag()
            {
                m_subtags = new LinkedList<ecTag>();
            }

            protected void WriteSubtags(BinaryWriter wr)
            {
                Int16 count16 = (Int16)m_subtags.Count;
                if (count16 != 0) {
                    wr.Write(System.Net.IPAddress.HostToNetworkOrder(count16));
                    foreach (ecTag t in m_subtags)
                    {
                        t.Write(wr);
                    }
                }
            }

            public virtual void Write(BinaryWriter wr)
            {
                Int16 name16 = (Int16)(m_name);
                name16 <<= 1;
                byte type8 = (byte)m_type;
                Int32 size32 = (Int32)Size();
                if (m_subtags.Count != 0) {
                    name16 |= 1;
                }
                wr.Write(System.Net.IPAddress.HostToNetworkOrder(name16));
                wr.Write(type8);
                wr.Write(System.Net.IPAddress.HostToNetworkOrder(size32));

                WriteSubtags(wr);
                //
                // here derived class will put actual data
                //
            }

            public int Size()
            {
                int total_size = m_size;
                foreach (ecTag t in m_subtags) {
                    total_size += t.Size();
                    // name + type + size for each tag
                    total_size += (2 + 1 + 4);
                    if (t.HaveSubtags()) {
                        total_size += 2;
                    }
                }
                return total_size;
            }

            public void AddSubtag(ecTag t)
            {
                m_subtags.AddLast(t);
            }

            bool HaveSubtags()
            {
                return (m_subtags.Count != 0);
            }
        }

        public class ecTagInt : ecTag {
            private UInt64 m_val;
            public ecTagInt(ECTagNames n, UInt32 v)
                : base(n, EcTagTypes.EC_TAGTYPE_UINT32)
            {
                m_val = v;
                m_size = 4;
            }

            public ecTagInt(ECTagNames n, UInt64 v)
                : base(n, EcTagTypes.EC_TAGTYPE_UINT64)
            {
                m_val = v;
                m_size = 8;
            }

            public override void Write(BinaryWriter wr)
            {
                base.Write(wr);
                
                switch ( m_size ) {
                    case 8:
                        Int32 val32 = (Int32)(m_val >> 32);
                        wr.Write(System.Net.IPAddress.HostToNetworkOrder(val32));

                        val32 = (Int32)(m_val & 0xffffffff);
                        wr.Write(System.Net.IPAddress.HostToNetworkOrder(val32));
                        break;
                    case 4:
                        val32 = (Int32)(m_val & 0xffffffff);
                        wr.Write(System.Net.IPAddress.HostToNetworkOrder(val32));
                        break;
                    case 2:
                        Int16 val16 = (Int16)(m_val & 0xffff);
                        wr.Write(System.Net.IPAddress.HostToNetworkOrder(val16));
                        break;
                    case 1:
                        wr.Write((byte)(m_val & 0xff));
                        break;
                }

            }
        }

        public class ecTagMD5 : ecTag {
            byte[] m_val;
            public ecTagMD5(ECTagNames n, string s, bool string_is_hash)
                : base(n, EcTagTypes.EC_TAGTYPE_HASH16)
            {
                if ( string_is_hash ) {
                    // in this case hash is passed as hex string
                    if ( s.Length != 16*2 ) {
                        throw new Exception("md5 hash of proto version have incorrect length");
                    }
                    //byte[] hash_str = s.ToCharArray();
                    for (int i = 0; i < 16; i++) {
                        string v = s.Substring(i * 2, 2);
                        
                    }
                    m_val = new byte[16];
                } else {
                    MD5CryptoServiceProvider p = new MD5CryptoServiceProvider();
                    byte[] bs = System.Text.Encoding.UTF8.GetBytes(s);
                    m_val = p.ComputeHash(bs);
                }
                m_size = 16;
            }
            public override void Write(BinaryWriter wr)
            {
                base.Write(wr);
                wr.Write(m_val);
            }
        }

        public class ecTagString : ecTag {
            byte[] m_val;
            public ecTagString(ECTagNames n, string s)
                : base(n, EcTagTypes.EC_TAGTYPE_STRING)
            {
                m_val = System.Text.Encoding.UTF8.GetBytes(s);
                m_size = m_val.GetLength(0) + 1;
            }
            public override void Write(BinaryWriter wr)
            {
                base.Write(wr);
                wr.Write(m_val);
                byte zero_byte = 0;
                wr.Write(zero_byte);
            }
        }

        public class ecPacket : ecTag {
            // since I have no zlib here, effectively disable compression
            const int MaxUncompressedPacket = 0x6666;

            //
            // Parsing ctor
            //
            private ECOpCodes m_opcode;

            public ecPacket(byte [] rxBuffer, int len)
            {
                MemoryStream memStream = new MemoryStream(rxBuffer, 0, len);
                BinaryReader binReader = new BinaryReader(memStream);
            }

            //
            // Default ctor - for tx packets
            public ecPacket(ECOpCodes cmd)
            {
                m_opcode = cmd;
            }

            public int PacketSize()
            {
                int packet_size = Size();
                // 1 (command) + 2 (tag count)
                return packet_size + 1 + 2;
            }

            public override void Write(BinaryWriter wr)
            {
                UInt32 flags = 0x20;
                int packet_size = PacketSize();
                if ( packet_size > MaxUncompressedPacket ) {
                    flags |= (UInt32)ECFlags.EC_FLAG_ZLIB;
                }

                if ((flags & (UInt32)ECFlags.EC_FLAG_ZLIB) != 0) {
                    throw new NotImplementedException("no zlib compression yet");
                }

                wr.Write(System.Net.IPAddress.HostToNetworkOrder((Int32)(flags)));
                wr.Write(System.Net.IPAddress.HostToNetworkOrder((Int32)(packet_size)));
                wr.Write((byte)m_opcode);
                WriteSubtags(wr);
            }


        }
        public class ecLoginPacket : ecPacket {
            public ecLoginPacket(string client_name, string version, string pass)
                : base(ECOpCodes.EC_OP_AUTH_REQ)
            {
                AddSubtag(new ecTagString(ECTagNames.EC_TAG_CLIENT_NAME, client_name));
                AddSubtag(new ecTagString(ECTagNames.EC_TAG_CLIENT_VERSION, version));
                AddSubtag(new ecTagInt(ECTagNames.EC_TAG_PROTOCOL_VERSION,
                    (UInt64)ProtocolVersion.EC_CURRENT_PROTOCOL_VERSION));

                AddSubtag(new ecTagMD5(ECTagNames.EC_TAG_PASSWD_HASH, pass, false));

                // discussion is ongoing
                //AddSubtag(new ecTagMD5(ECTagNames.EC_TAG_VERSION_ID, EC_VERSION_ID, true));
            }
        }

    }
}
