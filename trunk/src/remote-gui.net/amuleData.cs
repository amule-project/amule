using System;
using System.IO;
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

        public void Realloc(int size)
        {
	        if ( size == m_len ) {
		        return;
	        }

            if ( (size > m_len) && (size > m_buff.Length) ) {
                m_buff = new byte[size];
                m_enc_buff = new byte[size * 4 / 3 + 1];
            }
            m_len = size;
        }

        public void Decode(byte [] buff, int start_offset)
        {
            int len = buff.Length;

            int i = start_offset, j = 0;
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

    class PartFileEncoderData {
        public RLE_Data m_part_status;
        public RLE_Data m_gap_status;

        public PartFileEncoderData(int partcount, int gapcount)
        {
            m_part_status = new RLE_Data(partcount+1, true);
            m_gap_status = new RLE_Data(gapcount*sizeof(Int64)+1, true);
        }

        public void Decode(byte [] gapdata, byte [] partdata)
        {
            m_part_status.Decode(partdata, 0);

            // in a first dword - real size
            //uint32 gapsize = ENDIAN_NTOHL(RawPeekUInt32(gapdata));
            //gapdata += sizeof(uint32);
            //m_gap_status.Realloc(gapsize * 2 * sizeof(uint64));
            Int32 gapsize = System.Net.IPAddress.NetworkToHostOrder(
                (Int32)gapdata[0] | ((Int32)gapdata[1] << 8) |
                ((Int32)gapdata[2] << 16) | ((Int32)gapdata[3] << 24));

            m_gap_status.Realloc(gapsize*2*sizeof(Int64));
            m_gap_status.Decode(gapdata, 4);
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

    public interface IContainerUI {
        void MyEndUpdate();
        void MyBeginUpdate();

        void InsertItem(object i);
        void UpdateItem(object i);
    }

    //
    // T: item in container
    abstract class amuleGenericContainer<T> {
        private enum REQ_STATE { IDLE, REQ_SENT, FULL_REQ_SENT };
        REQ_STATE m_state = REQ_STATE.IDLE;

        LinkedList<T> m_items = new LinkedList<T>();
        Dictionary<ecProto.ecMD5, T> m_items_hash = new Dictionary<ecProto.ecMD5, T>(32);

        ECOpCodes m_req_cmd;

        ECTagNames m_item_tagname;

        bool m_inc_tags = true;

        protected IContainerUI m_owner;

        public amuleGenericContainer(ECOpCodes req_cmd, ECTagNames item_tagname, IContainerUI owner)
        {
            m_owner = owner;
            m_req_cmd = req_cmd;
            m_item_tagname = item_tagname;
        }

        protected virtual bool Phase1Done()
        {
            return true;
        }

        protected virtual ecProto.ecTag CreateItemTag(ecProto.ecMD5 id)
        {
            return null;
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
            m_owner.MyBeginUpdate();
            LinkedList<ecProto.ecTag>.Enumerator i = packet.GetTagIterator();
            while (i.MoveNext())
            {
                ecProto.ecTag t = i.Current;
                // sometimes reply contains additional tags
                if ( t.Name() != m_item_tagname ) {
                    continue;
                }
                ecProto.ecMD5 item_id = ((ecProto.ecTagMD5)t).ValueMD5();
                if ( m_items_hash.ContainsKey(item_id) ) {
                    T item = m_items_hash[item_id];
                    ProcessItemUpdate(item, t);

                    if ( m_owner != null ) {
                        m_owner.UpdateItem(item);
                    }
                } else {
                    if ( m_inc_tags ) {
                        T item = CreateItem(t);
                        m_items.AddLast(item);
                        m_items_hash[item_id] = item;

                        if (m_owner != null) {
                            m_owner.InsertItem(item);
                        }

                    } else {
                        full_req.AddSubtag(CreateItemTag(item_id));
                    }
                }
                //
                // now process item deletion
                //
                // TODO
            }
            m_owner.MyEndUpdate();
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

    public class amuleFileItem {
        protected ecProto.ecMD5 m_id;

        protected string m_filename;
        protected Int64 m_filesize;

        object m_ui_item;

        public string ValueToPrefix(Int64 value)
        {
            if (value < 1024)
            {
                return string.Format("{0} bytes", value);
            }
            else if (value < 1048576)
            {
                return string.Format("{0:f} Kb", ((float)value) / 1024);
            }
            else if (value < 1073741824)
            {
                return string.Format("{0:f} Mb", ((float)value) / 1048576);
            }
            else
            {
                return string.Format("{0:f} Gb", ((float)value) / 1073741824);
            }
        }

        public string Name
        {
            get { return m_filename; }
        }
	
        public string Size
        {
            get { return ValueToPrefix(m_filesize); }
        }
        public object UiItem
        {
            get { return m_ui_item; }
            set { m_ui_item = value; }
        }
    }

    public class DownloadQueueItem : amuleFileItem {
        Int64 m_size_xfered, m_size_done;
        Int32 m_speed;


        public DownloadQueueItem(ecProto.ecMD5 id, string name, Int64 size)
        {
            m_id = id;
            m_filename = name;
            m_filesize = size;
        }

        public void UpdateItem(ecProto.ecTag tag)
        {
            ecProto.ecTagInt itag = (ecProto.ecTagInt)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_SIZE_DONE);
            if ( itag != null ) {
                m_size_done = itag.Value64();
            }
            itag = (ecProto.ecTagInt)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_SIZE_XFER);
            if ( itag != null ) {
                m_size_xfered = itag.Value64();
            }

            itag = (ecProto.ecTagInt)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_SPEED);
            if ( itag != null ) {
                m_speed = (Int32)itag.Value64();
            }
        }
	
        public string SizeDone
        {
            get { return ValueToPrefix(m_size_done); }
        }
        public string Speed
        {
            get { return (m_speed == 0) ? "" : (ValueToPrefix(m_speed) + "/s"); }
        }
        public string PercentDone
        {
            get { return String.Format("{0} %", m_size_done * 100 / m_filesize); }
        }
    }

    class DownloadQueueContainer : amuleGenericContainer<DownloadQueueItem> {
        Dictionary<ecProto.ecMD5, PartFileEncoderData> m_enc_map;

        public DownloadQueueContainer(IContainerUI owner)
            : base(ECOpCodes.EC_OP_GET_DLOAD_QUEUE, ECTagNames.EC_TAG_PARTFILE, owner)
        {
            m_enc_map = new Dictionary<ecProto.ecMD5, PartFileEncoderData>();
        }

        override protected void ProcessItemUpdate(DownloadQueueItem item, ecProto.ecTag tag)
        {
            item.UpdateItem(tag);

            ecProto.ecMD5 id = ((ecProto.ecTagMD5)tag).ValueMD5();
            if ( !m_enc_map.ContainsKey(id) ) {
                throw new Exception("No RLE decoder for download queue item");
            }
            PartFileEncoderData decoder = m_enc_map[id];
            ecProto.ecTagCustom gapstat = (ecProto.ecTagCustom)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_GAP_STATUS);
            ecProto.ecTagCustom partstat = (ecProto.ecTagCustom)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_PART_STATUS);
            decoder.Decode(gapstat.Value(), partstat.Value());

            ecProto.ecTagCustom reqstat = (ecProto.ecTagCustom)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_REQ_STATUS);
            BinaryReader br = new BinaryReader(new MemoryStream(reqstat.Value()));
            // TODO: add colored status bar code
        }

        override protected DownloadQueueItem CreateItem(ecProto.ecTag tag)
        {
            ecProto.ecMD5 id = ((ecProto.ecTagMD5)tag).ValueMD5();
            string filename = ((ecProto.ecTagString)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_NAME)).StringValue();
            Int64 filesize = (Int64)((ecProto.ecTagInt)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_SIZE_FULL)).Value64();
            DownloadQueueItem i = new DownloadQueueItem(id, filename, filesize);

            m_enc_map[id] = new PartFileEncoderData((int)(filesize / 9728000), 10);

            i.UpdateItem(tag);

            return i;
        }
    }

    public class SharedFileItem : amuleFileItem {
    }

    class SharedFileListContainer : amuleGenericContainer<SharedFileItem>
    {
        public SharedFileListContainer(IContainerUI owner)
            : base(ECOpCodes.EC_OP_GET_SHARED_FILES, ECTagNames.EC_TAG_PARTFILE, owner)
        {
        }

        override protected void ProcessItemUpdate(SharedFileItem item, ecProto.ecTag tag)
        {
        }

        override protected SharedFileItem CreateItem(ecProto.ecTag tag)
        {
            return null;
        }
    }

}