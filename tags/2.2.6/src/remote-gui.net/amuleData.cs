//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2009 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2009 lfroen ( lfroen@gmail.com / http://www.amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//
using System;
using System.IO;
using System.Collections.Generic;
using System.Security.Cryptography;

namespace amule.net
{
    public class FileGap : IComparable {
        public Int64 m_start, m_end;

        public int CompareTo(object obj)
        {
            if ( obj is FileGap ) {
                FileGap temp = (FileGap)obj;
                return m_start.CompareTo(temp.m_start);
            }
            throw new ArgumentException("object is not a FileGap");
        }
    }

    public class RGB {
        public byte m_R, m_G, m_B;

        unsafe public void WriteToBuffAlpha(UInt32* ptr)
        {
            UInt32 val = (UInt32)((m_R << 16) | (m_G << 8) | m_B);
            val |= 0xff000000;
            *ptr = val;
        }

        unsafe public void WriteToBuffWithModifier(UInt32* ptr, byte modif)
        {
            byte R = (m_R > modif) ? (byte)(m_R - modif) : (byte)0;
            byte G = (m_G > modif) ? (byte)(m_G - modif) : (byte)0;
            byte B = (m_B > modif) ? (byte)(m_B - modif) : (byte)0;
            UInt32 val = (UInt32)((R << 16) | (G << 8) | B);
            val |= 0xff000000;
            *ptr = val;
        }

        public Int32 Color
        {
            get { return (Int32)((m_R << 16) | (m_G << 8) | m_B); }
            set 
            {
                m_R = (byte)(value >> 16);
                m_G = (byte)((value >> 8) & 0xff);
                m_B = (byte)(value & 0xff);
            }
        }

    }

    public class FileColoredGap : FileGap {
        public Int32 m_color;

    }

    public class GapBuffer {
        public FileGap[] m_buffer;

        public GapBuffer(byte[] raw_buffer, int size)
        {
            BinaryReader br = new BinaryReader(new MemoryStream(raw_buffer));
            int bufsize = size / (2 * sizeof(Int64));
            m_buffer = new FileGap[bufsize];
            for ( int i = 0; i < bufsize; i++ ) {
                m_buffer[i] = new FileGap();
                m_buffer[i].m_start = System.Net.IPAddress.NetworkToHostOrder(br.ReadInt64());
                m_buffer[i].m_end = System.Net.IPAddress.NetworkToHostOrder(br.ReadInt64());
            }
            System.Array.Sort(m_buffer);
        }
    }

    public class ColoredGapBuffer {
        public FileColoredGap[] m_buffer;

        public ColoredGapBuffer(int size)
        {
            m_buffer = new FileColoredGap[size];
            for ( int i = 0; i < size; i++ ) {
                m_buffer[i] = new FileColoredGap();
            }
            m_buffer[0].m_start = 0;
            m_buffer[0].m_end = 0;
            m_buffer[0].m_color = 0;
        }
    }

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

        public byte[] Buffer
        {
            get { return m_buff; }
        }

        public int Length
        {
            get { return m_len; }
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

    public class PartFileEncoderData {
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
    abstract public class amuleGenericContainer<T> {
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

        public LinkedList<T> Items
        {
            get { return m_items; }
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

        protected const Int32 FILE_PARTSIZE = 9728000;
        object m_ui_item;

        public amuleFileItem(ecProto.ecMD5 id, string name, Int64 size)
        {
            m_id = id;
            m_filename = name;
            m_filesize = size;
        }

        public ecProto.ecMD5 ID
        {
            get { return m_id; }
        }

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

        Int32 m_src_count, m_non_current_src_count, m_xfer_src_count, m_a4af_src_count;

        PartFileEncoderData m_decoder;

        //
        // Used for colored status
        //
        ColoredGapBuffer m_color_gap_buff;
        GapBuffer m_req_parts;

        //
        // Format24BppRgb or similar
        //
        RGB[] m_color_line;

        public static byte[] m_modifiers;

        public RGB [] ColorLine
        {
            get { return m_color_line; }
        }
	
        public static byte[] Get_3D_Modifiers()
        {
            return m_modifiers;
        }

        public DownloadQueueItem(ecProto.ecMD5 id, string name, Int64 size, PartFileEncoderData encoder)
            : base(id, name, size)
        {
            m_decoder = encoder;
            m_color_gap_buff = new ColoredGapBuffer((Int32)(size / FILE_PARTSIZE) + 1 + 1);
        }

        public static void InitDraw3DModifiers(int height)
        {
            if ( m_modifiers != null && m_modifiers.Length == height ) {
                return;
            }
            m_modifiers = new byte[height];
            for ( int i = 0; i < height; i++ ) {
                double curr_mod = 30 * (1 + System.Math.Cos((2*System.Math.PI)*(height-(((double)i)/height))));
                m_modifiers[i] = (byte)System.Math.Floor(curr_mod);
            }
        }

        public void AllocColorLine(int size)
        {
            m_color_line = new RGB[size];
            for ( int i = 0; i < m_color_line.Length; i++ ) {
                m_color_line[i] = new RGB();
            }
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

            itag = (ecProto.ecTagInt)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_SOURCE_COUNT);
            if ( itag != null ) {
                m_src_count = (Int32)itag.Value64();
            }

            itag = (ecProto.ecTagInt)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_SOURCE_COUNT_A4AF);
            if ( itag != null ) {
                m_a4af_src_count = (Int32)itag.Value64();
            }

            itag = (ecProto.ecTagInt)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_SOURCE_COUNT_NOT_CURRENT);
            if ( itag != null ) {
                m_non_current_src_count = (Int32)itag.Value64();
            }

            itag = (ecProto.ecTagInt)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_SOURCE_COUNT_XFER);
            if ( itag != null ) {
                m_xfer_src_count = (Int32)itag.Value64();
            }

            ecProto.ecTagCustom gapstat = (ecProto.ecTagCustom)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_GAP_STATUS);
            ecProto.ecTagCustom partstat = (ecProto.ecTagCustom)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_PART_STATUS);
            m_decoder.Decode(gapstat.Value(), partstat.Value());

            ecProto.ecTagCustom reqstat = (ecProto.ecTagCustom)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_REQ_STATUS);
            BinaryReader br = new BinaryReader(new MemoryStream(reqstat.Value()));

            m_req_parts = new GapBuffer(reqstat.Value(), reqstat.Value().Length);
            DrawLine();
        }

        public void DrawLine()
        {
            GapBuffer status_gaps = new GapBuffer(m_decoder.m_gap_status.Buffer, m_decoder.m_gap_status.Length);
            byte[] part_info = m_decoder.m_part_status.Buffer;

            int colored_gaps_size = 0;
            for ( int j = 0; j < status_gaps.m_buffer.Length; j++ ) {
                Int64 gap_start = status_gaps.m_buffer[j].m_start;
                Int64 gap_end = status_gaps.m_buffer[j].m_end;
                Int64 start = gap_start / FILE_PARTSIZE;
                Int64 end = (gap_end / FILE_PARTSIZE) + 1;

                //
                // Order is RGB
                //
                Int32 color = 0xff0000;
                for ( Int64 i = start; i < end; i++ ) {
                    if ( part_info[i] != 0 ) {
                        int blue = 210 - (22 * (part_info[i] - 1));
                        if ( blue < 0 ) { blue = 0; }
                        color = (blue << 8) | 255;
                    }
                    Int64 fill_gap_begin = ((i == start) ? gap_start : FILE_PARTSIZE * i);
                    Int64 fill_gap_end = ((i == (end - 1)) ? gap_end : FILE_PARTSIZE * (i + 1));

                    if ( (m_color_gap_buff.m_buffer[colored_gaps_size].m_end == fill_gap_begin) &&
                        (m_color_gap_buff.m_buffer[colored_gaps_size].m_color == color) ) {
                        m_color_gap_buff.m_buffer[colored_gaps_size].m_end = fill_gap_end;
                    } else {
                        colored_gaps_size++;
                        m_color_gap_buff.m_buffer[colored_gaps_size].m_start = fill_gap_begin;
                        m_color_gap_buff.m_buffer[colored_gaps_size].m_end = fill_gap_end;
                        m_color_gap_buff.m_buffer[colored_gaps_size].m_color = color;
                    }
                }
            }
            //
            // Now actual drawing
            //
            int width = m_color_line.Length;
            for ( int i = 0; i < width; i++ ) {
                m_color_line[i].Color = 0x7f7f7f;
            }
            if ( m_filesize < width ) {
                //
                // if file is that small, draw it in single step
                //
                if ( m_req_parts.m_buffer.Length != 0 ) {
                    for ( int i = 0; i < width; i++ ) {
                        // yellow
                        m_color_line[i].Color = 0xffd000;
                    }
                } else if ( m_color_gap_buff.m_buffer.Length != 0 ) {
                    for ( int i = 0; i < width; i++ ) {
                        m_color_line[i].Color = m_color_gap_buff.m_buffer[i].m_color;
                    }
                }
            } else {
                Int32 factor = (Int32)(m_filesize / width);
                for ( int i = 1; i <= colored_gaps_size; i++ ) {
                    Int32 start = (Int32)(m_color_gap_buff.m_buffer[i].m_start / factor);
                    Int32 end = (Int32)(m_color_gap_buff.m_buffer[i].m_end / factor);
                    for ( Int32 j = start; j < end; j++ ) {
                        m_color_line[j].Color = m_color_gap_buff.m_buffer[i].m_color;
                    }
                }
                foreach ( FileGap g in m_req_parts.m_buffer ) {
                    Int32 start = (Int32)(g.m_start / factor);
                    Int32 end = (Int32)(g.m_end / factor);
                    for ( Int32 j = start; j < end; j++ ) {
                        m_color_line[j].Color = 0xffd000;
                    }
                }
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
        public string Sources
        {
            get 
            {
                string result;
                if (m_non_current_src_count != 0) {
                    result = String.Format("{0} / {1}",
                        m_src_count - m_non_current_src_count, m_src_count);
                } else {
                    result = String.Format("{0}", m_src_count);
                }
                if (m_a4af_src_count != 0) {
                    result += String.Format(" +{0}", m_a4af_src_count);
                }
                if ( m_xfer_src_count != 0 ) {
                    result += String.Format(" ({0})", m_xfer_src_count);
                }
                return result; 
            }
        }
    }

    public class DownloadQueueContainer : amuleGenericContainer<DownloadQueueItem> {
        Dictionary<ecProto.ecMD5, PartFileEncoderData> m_enc_map;

        private int m_new_item_status_length;

        public int NewItemStatusLineLength
        {
            get { return m_new_item_status_length; }
            set { m_new_item_status_length = value; }
        }

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
        }

        override protected DownloadQueueItem CreateItem(ecProto.ecTag tag)
        {
            ecProto.ecMD5 id = ((ecProto.ecTagMD5)tag).ValueMD5();
            string filename = ((ecProto.ecTagString)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_NAME)).StringValue();
            Int64 filesize = (Int64)((ecProto.ecTagInt)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_SIZE_FULL)).Value64();

            PartFileEncoderData e = new PartFileEncoderData((int)(filesize / 9728000), 10);
            m_enc_map[id] = e;

            DownloadQueueItem i = new DownloadQueueItem(id, filename, filesize, e);

            i.AllocColorLine(m_new_item_status_length);

            i.UpdateItem(tag);

            return i;
        }
    }

    public class SharedFileItem : amuleFileItem {
        public SharedFileItem(ecProto.ecMD5 id, string name, Int64 size)
            : base(id, name, size)
        {
        }
    }

    class SharedFileListContainer : amuleGenericContainer<SharedFileItem>
    {
        public SharedFileListContainer(IContainerUI owner)
            : base(ECOpCodes.EC_OP_GET_SHARED_FILES, ECTagNames.EC_TAG_KNOWNFILE, owner)
        {
        }

        override protected void ProcessItemUpdate(SharedFileItem item, ecProto.ecTag tag)
        {
        }

        override protected SharedFileItem CreateItem(ecProto.ecTag tag)
        {
            ecProto.ecMD5 id = ((ecProto.ecTagMD5)tag).ValueMD5();
            string filename = ((ecProto.ecTagString)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_NAME)).StringValue();
            Int64 filesize = (Int64)((ecProto.ecTagInt)tag.SubTag(ECTagNames.EC_TAG_PARTFILE_SIZE_FULL)).Value64();
            SharedFileItem i = new SharedFileItem(id, filename, filesize);

            //i.UpdateItem(tag);

            return i;
        }
    }

}