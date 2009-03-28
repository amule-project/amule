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
using System.Security;
using System.Security.Permissions;
using System.Threading;
using System.Collections.Generic;
using System.Text;
using System.Net;
using System.Net.Sockets;

namespace amule.net
{

    // Define the state object for the callback. 
    // Use hostName to correlate calls with the proper result.
    public class ResolveState {
        IPHostEntry resolvedIPs;

        public string errorMsg;

        public IPHostEntry IPs
        {
            get { return resolvedIPs; }
            set { resolvedIPs = value; }
        }
    }

    public class ConnectState {
        public Socket sock;
        public string errorMsg;

        public ConnectState(Socket s)
        {
            sock = s;
        }
    }

    public abstract class amuleECHandler {
        public amuleECHandler()
        {
        }

        public abstract void HandlePacket(ecProto.ecPacket packet);
    }

    class amuleLogicHandler : amuleECHandler {
        amuleRemote m_owner;
        bool m_auth_result = false;

        public amuleLogicHandler(amuleRemote o)
        {
            m_owner = o;
        }

        public override void HandlePacket(ecProto.ecPacket packet)
        {
            if ( packet.Opcode() == ECOpCodes.EC_OP_AUTH_OK ) {
                Console.WriteLine("amuleLogicHandler : Authenticated OK");
                m_auth_result = true;
                m_owner.m_packet_op_Done.Set();
            } else {
                Console.WriteLine("amuleLogicHandler : Authentication failed. Core reply was {0}", packet.Opcode());
            }
        }

        public bool AuthResult()
        {
            return m_auth_result;
        }
    }

    class amuleRemote {
        amuleECHandler m_handler = null;

        public ManualResetEvent m_packet_op_Done = new ManualResetEvent(false);

        // Record the IPs in the state object for later use.
        static void GetHostEntryCallback(IAsyncResult ar)
        {
            ResolveState ioContext = (ResolveState)ar.AsyncState;
            try {
                ioContext.IPs = Dns.EndGetHostEntry(ar);
            } catch (SocketException e) {
                ioContext.errorMsg = e.Message;
            }
        }

        Socket m_s;

        static void ConnectCallback(IAsyncResult ar)
        {
            // Retrieve the socket from the state object.
            ConnectState state = (ConnectState)ar.AsyncState;
            try {
                // Complete the connection.
                state.sock.EndConnect(ar);

                Console.WriteLine("Socket connected to {0}",
                    state.sock.RemoteEndPoint.ToString());

            } catch (Exception e) {
                state.errorMsg = e.Message;
            }
        }

        byte[] m_rx_buffer = new byte[32 * 1024];
        MemoryStream m_rx_mem_stream = null;
        int m_rx_byte_count = 0;
        int m_rx_remaining_count = 0;

        byte[] m_tx_buffer = new byte[32 * 1024];
        MemoryStream m_tx_mem_stream = null;

        //LinkedList<byte[]> m_tx_queue;
        BinaryReader m_sock_reader = null;
        BinaryWriter m_sock_writer = null;

        static void RxCallback(IAsyncResult ar)
        {
            amuleRemote o = (amuleRemote)ar.AsyncState;
            Console.WriteLine("RxCallback signalled, calling EndReceive");
            int bytesRead = o.m_s.EndReceive(ar);
            if ( bytesRead == 0 ) {
                // remote side closed connection. 
                // indicate error to caller
                o.m_rx_byte_count = -1;
                return;
            }
            o.m_rx_remaining_count -= bytesRead;
            Console.WriteLine("RxCallback: got {0} bytes, waiting for {1}",
                bytesRead, o.m_rx_remaining_count);
            // are we still waiting for flags and size?
            if (o.m_rx_byte_count < 8) {
                if ((o.m_rx_byte_count + bytesRead) >= 8) {
                    // got flags and packet size - may proceed.
                    Int32 flags = o.m_sock_reader.ReadInt32();
                    Int32 val32 = o.m_sock_reader.ReadInt32();

                    o.m_rx_remaining_count = (int)IPAddress.NetworkToHostOrder(val32) - (bytesRead - 8);
                    Console.WriteLine("RxCallback: expecting packet size={0}", o.m_rx_remaining_count);
                    if ( o.m_rx_buffer.Length <= (o.m_rx_remaining_count+o.m_rx_byte_count) ) {
                        byte [] new_buffer = new byte[o.m_rx_remaining_count + o.m_rx_buffer.Length + 1];
                        o.m_rx_buffer.CopyTo(new_buffer, 0);
                        o.m_rx_buffer = new_buffer;
                        //
                        // update stream reader with new buffer
                        //
                        o.m_rx_mem_stream = new MemoryStream(o.m_rx_buffer);
                        o.m_sock_reader = new BinaryReader(o.m_rx_mem_stream);
                    }
                }
            } else {
                if ( o.m_rx_remaining_count == 0 ) {
                    //
                    // Packet received - call handler
                    //
                    if ( o.m_handler != null ) {
                        o.m_rx_mem_stream.Seek(0, SeekOrigin.Begin);
                        Console.WriteLine("Packet received - call handler");
                        ecProto.ecPacket p = new ecProto.ecPacket(o.m_sock_reader);
                        //m_packet_op_Done.Set();
                        o.m_handler.HandlePacket(p);
                        Console.WriteLine("Handler done");
                    }
                    Console.WriteLine("Signalling event");
                    //m_packet_op_Done.Set();
                    //
                    // Keep waiting for more packets
                    //
                    o.StartReceive();
                    return;                
                }
            }
            o.m_rx_byte_count += bytesRead;

            // not just yet - keep waiting
            o.m_s.BeginReceive(o.m_rx_buffer, o.m_rx_byte_count, o.m_rx_remaining_count,
                SocketFlags.None, new AsyncCallback(RxCallback), o);
        }

        static void TxCallback(IAsyncResult ar)
        {
            amuleRemote o = (amuleRemote)ar.AsyncState;
            Console.WriteLine("TxCallback signalled, calling EndWrite");
            o.m_s.EndSend(ar);
        }

        public IAsyncResult SendPacket(ecProto.ecPacket packet)
        {
            m_tx_mem_stream.Seek(0, SeekOrigin.Begin);
            packet.Write(m_sock_writer);

            return m_s.BeginSend(m_tx_buffer, 0, packet.PacketSize(),
                SocketFlags.None, new AsyncCallback(TxCallback), this);
        }

        public void StartReceive()
        {
            // reply packet is supposed to have at least 8 bytes
            m_rx_remaining_count = 8;
            m_rx_byte_count = 0;
            m_rx_mem_stream.Seek(0, SeekOrigin.Begin);
            m_s.BeginReceive(m_rx_buffer, 0, 8, SocketFlags.None, new AsyncCallback(RxCallback), this);
        }

        [DnsPermission(SecurityAction.Demand, Unrestricted = true)]
        public bool ConnectToCore(string host, int port, string pass, ref string error)
        {
            try {
                IAsyncResult async_result;

                ResolveState resolveContext = new ResolveState();
                async_result = Dns.BeginGetHostEntry(host,
                    new AsyncCallback(GetHostEntryCallback), resolveContext);

                async_result.AsyncWaitHandle.WaitOne();
                if ( resolveContext.IPs == null ) {
                    error = resolveContext.errorMsg;
                    return false;
                }
                Console.WriteLine("Resolved: '{0}' -> '{1}", host,resolveContext.IPs.AddressList[0]);
                Console.WriteLine("Connecting to {0}:{1}", resolveContext.IPs.AddressList[0], port);
                IPEndPoint remoteEP = new IPEndPoint(resolveContext.IPs.AddressList[0], port);
                m_s = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

                ConnectState connectContext = new ConnectState(m_s);
                async_result = m_s.BeginConnect(remoteEP, new AsyncCallback(ConnectCallback), connectContext);
                if ( !async_result.AsyncWaitHandle.WaitOne(10000, true) ) {
                    error = "Timeout during connection. Possible firewall";
                    return false;
                }

                if ( connectContext.errorMsg != null) {
                    error = connectContext.errorMsg;
                    return false;
                }

                m_tx_mem_stream = new MemoryStream(m_tx_buffer);
                m_sock_writer = new BinaryWriter(m_tx_mem_stream);
                m_rx_mem_stream = new MemoryStream(m_rx_buffer);
                m_sock_reader = new BinaryReader(m_rx_mem_stream);

                ecProto.ecLoginPacket p = new ecProto.ecLoginPacket("amule.net", "0.0.1", pass);
                async_result = SendPacket(p);

                if (!async_result.AsyncWaitHandle.WaitOne()) {
                    // Was unable to send login request for 1sec. Line must be really slow
                    return false;
                }

                m_handler = new amuleLogicHandler(this);
                StartReceive();
                Console.WriteLine("Waiting for auth done");
                // FIXME: must be able to cancel this read.
                m_packet_op_Done.WaitOne();
                if ( m_rx_byte_count == -1 ) {
                    // remote side terminated connection
                    Console.WriteLine("Connection terminated on remote side");
                }
                Console.WriteLine("Connect done");
                bool result = ((amuleLogicHandler)m_handler).AuthResult();
                m_handler = null;
                return result;
            } catch (Exception e) {
                error = e.Message;
                return false;
            }
        }

        public void SetECHandler(amuleECHandler h)
        {
            m_handler = h;
        }
    }
}
