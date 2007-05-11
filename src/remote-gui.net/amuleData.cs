using System;
using System.Collections.Generic;
using System.Security.Cryptography;

namespace amule.net
{
    ///
    /// RLE implementation. I need only decoder part
    ///
    public class RLE_Data {
        bool m_use_diff;
        int m_len;
        byte[] m_enc_buff;
        byte[] m_buff;

        public RLE_Data(int len, bool use_diff)
        {
            m_len = len;
            m_use_diff = use_diff;
            // in worst case 2-byte sequence encoded as 3. So, data can grow at 1/3
            m_enc_buff = new byte[m_len*4/3 + 1];
            m_buff = new byte[m_len];
        }

        void Decode(byte [] buff)
        {
            int len = buff.Length;

            int i = 0, j = 0;
            while ( j != m_len ) {
                if ( i < (len -1) ) {
                    if (buff[i+1] == buff[i]) {
                        // this is sequence
                        //memset(m_enc_buff + j, buff[i], buff[i + 2]);
                        for(int k = 0; k < buff[i + 2]; k++) {
                            m_enc_buff[j + k] = buff[i];
                        }
                        j += buff[i + 2];
                        i += 3;
                    } else {
                        // this is single byte
                        m_enc_buff[j++] = buff[i++];
                    }
                } else {
                    // only 1 byte left in encoded data - it can't be sequence
                    m_enc_buff[j++] = buff[i++];
                    // if there's no more data, but buffer end is not reached,
                    // it must be error in some point
                    if ( j != m_len ) {
                        Console.WriteLine("RLE_Data: decoding error. {0} bytes decoded to {1} instead of {2}",
                            len, j, m_len);
                        throw new Exception("RLE_Data: decoding error");
                    }
                }
            }
            if ( m_use_diff ) {
                for (int k = 0; k < m_len; k++) {
                    m_buff[k] ^= m_enc_buff[k];
                }
            }
        }
    }


    //
    // I: ID for this kind of tag
    class amuleTag2ItemConnector<I> {
        virtual public I ID(ecProto.ecTag tag)
        {
            return default(I);
        }

        virtual public ecProto.ecTag CreateTag(I value)
        {
            return null;
        }
    }

    //
    // T: item in container
    // I: ID (key) for those items
    // G: EC tag used for xfer
    abstract class amuleGenericContainer<T, I, G> {
        private enum REQ_STATE { IDLE, REQ_SENT, FULL_REQ_SENT };
        REQ_STATE m_state = REQ_STATE.IDLE;

        LinkedList<T> m_items = new LinkedList<T>();
        //T[] m_items = new T[32];
        Dictionary<I, T> m_items_hash = new Dictionary<I, T>(32);

        bool m_inc_tags = false;

        ECOpCodes m_req_cmd;

        ECTagNames m_item_tagname;

        amuleTag2ItemConnector<I> m_tag2id;

        public amuleGenericContainer(ECOpCodes req_cmd, ECTagNames item_tagname, bool use_inc_tags,
            amuleTag2ItemConnector<I> tag2id)
        {
            m_req_cmd = req_cmd;
            m_item_tagname = item_tagname;
            m_tag2id = tag2id;
            m_inc_tags = use_inc_tags;
        }

        protected virtual bool Phase1Done()
        {
            return true;
        }

        public ecProto.ecPacket ReQuery()
        {
            // can not issue new query until previous one is replied
            if ( m_state != REQ_STATE.IDLE ) {
                return null;
            }

            ecProto.ecPacket request = new ecProto.ecPacket(m_req_cmd,
                m_inc_tags ? EC_DETAIL_LEVEL.EC_DETAIL_INC_UPDATE : EC_DETAIL_LEVEL.EC_DETAIL_UPDATE);
            m_state = REQ_STATE.REQ_SENT;

            return request;
        }

        void ProcessUpdate(ecProto.ecPacket packet, ecProto.ecPacket full_req)
        {
            LinkedList<ecProto.ecTag>.Enumerator i = packet.GetTagIterator();
            while (i.MoveNext())
            {
                ecProto.ecTag t = i.Current;
                // sometimes reply contains additional tags
                if ( t.Name() != m_item_tagname ) {
                    continue;
                }
                I item_id = m_tag2id.ID(t);
                if ( m_items_hash.ContainsKey(item_id) ) {
                    ProcessItemUpdate(m_items_hash[item_id], t);
                } else {
                    if ( m_inc_tags ) {
                        T item = CreateItem(t);
                        m_items.AddLast(item);
                        m_items_hash[item_id] = item;// CreateItem(t);
                    } else {
                        full_req.AddSubtag(m_tag2id.CreateTag(item_id));
                    }
                }
                //
                // now process item deletion
                //
                // TODO
            }
        }

        //
        // derived class must provide
        //
        abstract protected void ProcessItemUpdate(T item, ecProto.ecTag tag);

        abstract protected T CreateItem(ecProto.ecTag tag);

        public ecProto.ecPacket HandlePacket(ecProto.ecPacket p)
        {
            ecProto.ecPacket reply = null;
            switch (m_state) {
                case REQ_STATE.IDLE:
                    throw new Exception("Should not get packet in IDLE state");
                case REQ_STATE.REQ_SENT:
                    m_state = REQ_STATE.IDLE;
                    if ( !Phase1Done() ) {
                        break;
                    }
                    ecProto.ecPacket full_request = new ecProto.ecPacket(m_req_cmd);
                    ProcessUpdate(p, full_request);

                    // // Phase 3: request full info about files we don't have yet
                    if ( !m_inc_tags && (full_request.SubtagCount() != 0)) {
                        reply = full_request;
                        m_state = REQ_STATE.FULL_REQ_SENT;
                    }
                    
                    break;
                case REQ_STATE.FULL_REQ_SENT:
                    m_state = REQ_STATE.IDLE;
                    break;
            }
            return reply;
        }
    }

    class DownloadQueueItem {
        ecProto.ecMD5 m_id;
        string m_filename;
        UInt64 m_filesize, m_size_xfered, m_size_done;

        public DownloadQueueItem(ecProto.ecMD5 id, string name, UInt64 size)
        {
            m_id = id;
            m_filename = name;
            m_filesize = size;
        }

        public class DownloadTag2ItemConnector : amuleTag2ItemConnector<ecProto.ecMD5>
        {
            public override ecProto.ecMD5 ID(ecProto.ecTag tag)
            {
                ecProto.ecTagMD5 md5_tag = (ecProto.ecTagMD5)tag;
                return md5_tag.ValueMD5();
            }

            public override ecProto.ecTag CreateTag(ecProto.ecMD5 value)
            {
                return new ecProto.ecTagMD5(ECTagNames.EC_TAG_PARTFILE, value);
            }
        }

        public void UpdateItem(ecProto.ecTag tag)
        {
            m_size_done = ((ecProto.ecTagInt)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_SIZE_DONE)).Value64();
            m_size_xfered = ((ecProto.ecTagInt)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_SIZE_XFER)).Value64();
        }
    }

    class DownloadQueueContainer : amuleGenericContainer<DownloadQueueItem, ecProto.ecMD5, ecProto.ecDownloadsInfoReq>
    {
        public DownloadQueueContainer()
            : base(ECOpCodes.EC_OP_GET_DLOAD_QUEUE, ECTagNames.EC_TAG_PARTFILE, true,
            new DownloadQueueItem.DownloadTag2ItemConnector())
        {
        }

        override protected void ProcessItemUpdate(DownloadQueueItem item, ecProto.ecTag tag)
        {
            item.UpdateItem(tag);
        }

        override protected DownloadQueueItem CreateItem(ecProto.ecTag tag)
        {
            ecProto.ecMD5 id = ((ecProto.ecTagMD5)tag).ValueMD5();
            string filename = ((ecProto.ecTagString)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_NAME)).StringValue();
            UInt64 filesize = ((ecProto.ecTagInt)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_SIZE_FULL)).Value64();
            DownloadQueueItem i = new DownloadQueueItem(id, filename, filesize);
            i.UpdateItem(tag);
            return i;
        }
    }

}