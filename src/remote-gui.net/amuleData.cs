using System;
using System.Collections.Generic;
using System.Security.Cryptography;

namespace amule.net
{
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

        ECOpCodes m_full_req_cmd;

        ECTagNames m_item_tagname;

        amuleTag2ItemConnector<I> m_tag2id;

        public amuleGenericContainer(ECOpCodes full_req_cmd, ECTagNames item_tagname, amuleTag2ItemConnector<I> tag2id)
        {
            m_full_req_cmd = full_req_cmd;
            m_item_tagname = item_tagname;
            m_tag2id = tag2id;
        }

        protected virtual bool Phase1Done()
        {
            return true;
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
                    ecProto.ecPacket full_request = new ecProto.ecPacket(m_full_req_cmd);
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
        Int64 m_filesize;

        public DownloadQueueItem(ecProto.ecMD5 id, string name, Int64 size)
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
        }
    }

    class DownloadQueueContainer : amuleGenericContainer<DownloadQueueItem, ecProto.ecMD5, ecProto.ecDownloadsInfoReq>
    {
        DownloadQueueContainer()
            : base(ECOpCodes.EC_OP_GET_DLOAD_QUEUE, ECTagNames.EC_TAG_PARTFILE,
            new DownloadQueueItem.DownloadTag2ItemConnector())
        {
        }

        override protected void ProcessItemUpdate(DownloadQueueItem item, ecProto.ecTag tag)
        {
            item.UpdateItem(tag);
        }

        override protected DownloadQueueItem CreateItem(ecProto.ecTag tag)
        {
            return null;
        }
    }

}