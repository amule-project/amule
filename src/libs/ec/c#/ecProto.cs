using System;
using System.IO;
using System.Security.Cryptography;
using System.Collections.Generic;
using System.Text;

namespace amule.net
{
    public enum EcTagTypes {
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

    public class ecProto {

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

            protected int Size()
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

            public ecTag SubTag(ECTagNames name)
            {
                foreach (ecTag t in m_subtags) {
                    if (t.m_name == name) {
                        return t;
                    }
                }
                return null;
            }
        }

        public class ecTagInt : ecTag {
            private UInt64 m_val;
            public ecTagInt(ECTagNames n, byte v)
                : base(n, EcTagTypes.EC_TAGTYPE_UINT8)
            {
                m_val = v;
                m_size = 1;
            }

            public ecTagInt(ECTagNames n, UInt16 v)
                : base(n, EcTagTypes.EC_TAGTYPE_UINT16)
            {
                m_val = v;
                m_size = 2;
            }

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

            public ecTagInt(ECTagNames n, Int32 tag_size, BinaryReader br)
                : base(n, EcTagTypes.EC_TAGTYPE_UINT8)
            {
                m_size = tag_size;
                switch ( m_size ) {
                    case 8:
                        m_type = EcTagTypes.EC_TAGTYPE_UINT64;
                        m_val = (UInt32)System.Net.IPAddress.NetworkToHostOrder(br.ReadInt32());
                        m_val <<= 32;
                        m_val |= ((UInt32)System.Net.IPAddress.NetworkToHostOrder(br.ReadInt32()));
                        break;
                    case 4:
                        m_type = EcTagTypes.EC_TAGTYPE_UINT32;
                        m_val = (UInt64)System.Net.IPAddress.NetworkToHostOrder(br.ReadInt32());
                        break;
                    case 2:
                        m_type = EcTagTypes.EC_TAGTYPE_UINT16;
                        m_val = (UInt64)System.Net.IPAddress.NetworkToHostOrder(br.ReadInt16());
                        break;
                    case 1:
                        m_type = EcTagTypes.EC_TAGTYPE_UINT8;
                        m_val = (UInt64)br.ReadByte();
                        break;
                    default:
                        throw new Exception("Unexpected size of data in integer tag");
                }

            }

            public int ValueInt()
            {
                return (int)m_val;
            }

            public UInt64 Value64()
            {
                return m_val;
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

            public ecTagMD5(ECTagNames name, byte[] hash_data)
                : base(name, EcTagTypes.EC_TAGTYPE_HASH16)
            {
                m_val = hash_data;
                m_size = 16;
            }

            public ecTagMD5(ECTagNames name, BinaryReader br)
                : base(name, EcTagTypes.EC_TAGTYPE_HASH16)
            {
                m_size = 16;
                m_val = br.ReadBytes(16);
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

            public ecTagString(ECTagNames n, Int32 tag_size, BinaryReader br)
                : base(n, EcTagTypes.EC_TAGTYPE_STRING)
            {
                byte[] buf = br.ReadBytes(tag_size);
                m_size = tag_size;
                m_val = buf;
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

            private ECOpCodes m_opcode;
            protected Int32 m_flags;

            //
            // Parsing ctor
            //
            ecTag ReadTag(BinaryReader br)
            {
                ecTag t = null;
                Int16 tag_name16 = System.Net.IPAddress.NetworkToHostOrder(br.ReadInt16());
                bool have_subtags = ((tag_name16 & 1) != 0);
                ECTagNames tag_name = (ECTagNames)(tag_name16 >> 1);

                byte tag_type8 = br.ReadByte();
                Int32 tag_size32 = System.Net.IPAddress.NetworkToHostOrder(br.ReadInt32());

                EcTagTypes tag_type = (EcTagTypes)tag_type8;
                switch (tag_type) {
                    case EcTagTypes.EC_TAGTYPE_UNKNOWN:
                        break;
                    case EcTagTypes.EC_TAGTYPE_CUSTOM:
                        break;

                    case EcTagTypes.EC_TAGTYPE_UINT8:
                        goto case EcTagTypes.EC_TAGTYPE_UINT64;
                    case EcTagTypes.EC_TAGTYPE_UINT16:
                        goto case EcTagTypes.EC_TAGTYPE_UINT64;
                    case EcTagTypes.EC_TAGTYPE_UINT32:
                        goto case EcTagTypes.EC_TAGTYPE_UINT64;
                    case EcTagTypes.EC_TAGTYPE_UINT64:
                        t = new ecTagInt(tag_name, tag_size32, br);
                        break;

                    case EcTagTypes.EC_TAGTYPE_STRING:
                        break;
                    case EcTagTypes.EC_TAGTYPE_DOUBLE:
                        break;
                    case EcTagTypes.EC_TAGTYPE_IPV4:
                        break;
                    case EcTagTypes.EC_TAGTYPE_HASH16:
                        break;
                    default:
                        break;
                }
                return t;
            }
 
            public ecPacket()
            {
                m_flags = 0x20;
            }

            public ecPacket(BinaryReader br)
            {
                m_flags = System.Net.IPAddress.NetworkToHostOrder(br.ReadInt32());
                Int32 packet_size = System.Net.IPAddress.NetworkToHostOrder(br.ReadInt32());
                m_opcode = (ECOpCodes)br.ReadByte();

                Int16 tags_count = System.Net.IPAddress.NetworkToHostOrder(br.ReadInt16());

                if ( tags_count != 0 ) {
                    for (int i = 0; i < tags_count; i++) {
                        AddSubtag(ReadTag(br));
                    }
                }
            }

            //
            // Default ctor - for tx packets
            public ecPacket(ECOpCodes cmd)
            {
                m_flags = 0x20;
                m_opcode = cmd;
            }

            //
            // Size of data for TX, not of payload
            //
            public int PacketSize()
            {
                int packet_size = Size();
                if ((m_flags & (UInt32)ECFlags.EC_FLAG_ACCEPTS) != 0) {
                    packet_size += 4;
                }
                // 1 (command) + 2 (tag count) + 4 (flags) + 4 (total size)
                return packet_size + 1 + 2 + 4 + 4;
            }

            public ECOpCodes Opcode()
            {
                return m_opcode;
            }

            public override void Write(BinaryWriter wr)
            {
                // 1 (command) + 2 (tag count)
                int packet_size = Size() + 1 + 2;
                if ( packet_size > MaxUncompressedPacket ) {
                    m_flags |= (Int32)ECFlags.EC_FLAG_ZLIB;
                }

                if ((m_flags & (UInt32)ECFlags.EC_FLAG_ZLIB) != 0) {
                    throw new NotImplementedException("no zlib compression yet");
                }


                wr.Write(System.Net.IPAddress.HostToNetworkOrder((Int32)(m_flags)));
                if ((m_flags & (UInt32)ECFlags.EC_FLAG_ACCEPTS) != 0) {
                    wr.Write(System.Net.IPAddress.HostToNetworkOrder((Int32)(m_flags)));
                }

                wr.Write(System.Net.IPAddress.HostToNetworkOrder((Int32)(packet_size)));
                wr.Write((byte)m_opcode);
                if ( m_subtags.Count != 0 ) {
                    WriteSubtags(wr);
                } else {
                    wr.Write((Int16)(0));
                }
            }


        }
        public class ecLoginPacket : ecPacket {
            public ecLoginPacket(string client_name, string version, string pass)
                : base(ECOpCodes.EC_OP_AUTH_REQ)
            {
                m_flags |= 0x20 | (Int32)ECFlags.EC_FLAG_ACCEPTS;

                AddSubtag(new ecTagString(ECTagNames.EC_TAG_CLIENT_NAME, client_name));
                AddSubtag(new ecTagString(ECTagNames.EC_TAG_CLIENT_VERSION, version));
                AddSubtag(new ecTagInt(ECTagNames.EC_TAG_PROTOCOL_VERSION,
                    (UInt64)ProtocolVersion.EC_CURRENT_PROTOCOL_VERSION));

                AddSubtag(new ecTagMD5(ECTagNames.EC_TAG_PASSWD_HASH, pass, false));

                // discussion is ongoing
                //AddSubtag(new ecTagMD5(ECTagNames.EC_TAG_VERSION_ID, EC_VERSION_ID, true));
            }
        }

        public class ecDownloadsInfoReq : ecPacket {
            public ecDownloadsInfoReq() : base(ECOpCodes.EC_OP_GET_DLOAD_QUEUE)
            {
            }
        }

    }
}
