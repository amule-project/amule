//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2007 lfroen ( lfroen@gmail.com / http://www.amule.org )
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
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace amule.net {
    public class amuleDownloadStatusList : amuleListView, IContainerUI {
        DownloadQueueContainer m_item_container;

        public amuleDownloadStatusList()
        {
            string[] columns = { "File name", "Status", "Size", "Completed", "Speed" };
            int[] width = { 300, 100, 100, 100, 100 };
            LoadColumns(columns, width);

            OwnerDraw = true;
            DoubleBuffered = true;
            DrawColumnHeader +=
                new DrawListViewColumnHeaderEventHandler(amuleDownloadStatusList_DrawColumnHeader);

            DrawSubItem += new DrawListViewSubItemEventHandler(amuleDownloadStatusList_DrawSubItem);
            DownloadQueueItem.InitDraw3DModifiers(FontHeight + 1);
        }

        public DownloadQueueContainer ItemContainer
        {
            get { return m_item_container; }
            set
            {
                m_item_container = value;
                m_item_container.NewItemStatusLineLength = Columns[1].Width;
            }
        }

        override protected void OnColumnWidthChanged(ColumnWidthChangedEventArgs e)
        {
            if ( e.ColumnIndex == 1 ) {
                int new_size = Columns[1].Width + 1;
                m_item_container.NewItemStatusLineLength = new_size;

                foreach ( ListViewItem i in Items ) {
                    DownloadQueueItem it = i.Tag as DownloadQueueItem;
                    it.AllocColorLine(new_size);
                    it.DrawLine();
                }
            }
        }

        unsafe void DrawStatusBar(DownloadQueueItem it, Graphics g, Rectangle posR)
        {
            //
            // Bitmap is created as 32bpp (rgb+alpha)
            //
            Bitmap status_bmp = new Bitmap(posR.Width, posR.Height,
                System.Drawing.Imaging.PixelFormat.Format32bppArgb);

            System.Drawing.Imaging.BitmapData bmd = status_bmp.LockBits(
                new Rectangle(0, 0, status_bmp.Width, status_bmp.Height),
                System.Drawing.Imaging.ImageLockMode.ReadWrite,
                System.Drawing.Imaging.PixelFormat.Format32bppArgb);

            RGB[] item_color_line = it.ColorLine;
            byte[] modifiers = DownloadQueueItem.Get_3D_Modifiers();

            for ( int y = 0; y < bmd.Height; y++ ) {
                byte* row = (byte*)bmd.Scan0 + (y * bmd.Stride);

                for ( int x = 0; x < bmd.Width; x++ ) {
                    UInt32* pixel_ptr = (UInt32*)(row + x * 4);
                    //row[x * 3 + 2] = 255;
                    //*pixel_ptr = 0xff0000; //RED
                    //*pixel_ptr = 0x00ff00; //GREEN
                    //*pixel_ptr = 0x1f0000ff; // BLUE
                    //*pixel_ptr = item_color_line[x] | (x << 24);
                    item_color_line[x].WriteToBuffWithModifier(pixel_ptr, modifiers[y]);
                }

            }

            status_bmp.UnlockBits(bmd);

            g.DrawImage(status_bmp, posR);
            status_bmp.Dispose();
        }

        void amuleDownloadStatusList_DrawSubItem(object sender, DrawListViewSubItemEventArgs e)
        {
            if ( e.ColumnIndex == 1 ) {
                // status is colored bar
                Rectangle r = e.Bounds;
                DownloadQueueItem it = e.Item.Tag as DownloadQueueItem;
                DrawStatusBar(it, e.Graphics, r);
                e.DrawDefault = false;
            }
            else {
                e.DrawBackground();
            }
            e.DrawText();
            if ( e.Item.Selected ) {
                e.DrawFocusRectangle(e.Bounds);
            }
        }

        void amuleDownloadStatusList_DrawColumnHeader(object sender, DrawListViewColumnHeaderEventArgs e)
        {
            e.DrawBackground();
            e.DrawText();
        }

        ///
        // interface to core
        ///
        void DoInsertItem(DownloadQueueItem i)
        {
            ListViewItem it = new ListViewItem(i.Name);

            it.SubItems.Add(new ListViewItem.ListViewSubItem(it, i.PercentDone));
            it.SubItems.Add(new ListViewItem.ListViewSubItem(it, i.Size));
            it.SubItems.Add(new ListViewItem.ListViewSubItem(it, i.SizeDone));
            it.SubItems.Add(new ListViewItem.ListViewSubItem(it, i.Speed));
            it.Tag = i;

            Items.Add(it);

            i.UiItem = it;
        }

        void DoUpdateItem(DownloadQueueItem i)
        {
            ListViewItem it = i.UiItem as ListViewItem;
            if ( it.SubItems[1].Text != i.PercentDone ) {
                it.SubItems[1].Text = i.PercentDone;
            }
            if ( it.SubItems[3].Text != i.SizeDone ) {
                it.SubItems[3].Text = i.SizeDone;
            }
            if ( it.SubItems[4].Text != i.Speed ) {
                it.SubItems[4].Text = i.Speed;
            }
            //Items
        }

        #region IContainerUI Members

        delegate void UpdateCallback();

        public void MyEndUpdate()
        {
            if ( InvokeRequired ) {
                UpdateCallback d = new UpdateCallback(EndUpdate);
                Invoke(d);
            }
            else {
                EndUpdate();
            }
        }

        public void MyBeginUpdate()
        {
            if ( InvokeRequired ) {
                UpdateCallback d = new UpdateCallback(BeginUpdate);
                Invoke(d);
            }
            else {
                BeginUpdate();
            }
        }

        delegate void ItemCallback(DownloadQueueItem i);

        public void InsertItem(object i)
        {
            DownloadQueueItem it = i as DownloadQueueItem;
            if ( InvokeRequired ) {
                ItemCallback d = new ItemCallback(DoInsertItem);
                Invoke(d, it);
            }
            else {
                DoInsertItem(it);
            }
        }

        public void UpdateItem(object i)
        {
            DownloadQueueItem it = i as DownloadQueueItem;
            if ( InvokeRequired ) {
                ItemCallback d = new ItemCallback(DoUpdateItem);
                Invoke(d, it);
            }
            else {
                DoUpdateItem(it);
            }
        }

        #endregion
    }

}
