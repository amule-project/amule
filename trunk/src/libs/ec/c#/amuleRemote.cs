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

        static ManualResetEvent m_socket_op_Done = new ManualResetEvent(false);

        // Record the IPs in the state object for later use.
        static void GetHostEntryCallback(IAsyncResult ar)
        {
            ResolveState ioContext = (ResolveState)ar.AsyncState;
            try {
                ioContext.IPs = Dns.EndGetHostEntry(ar);
            } catch (SocketException e) {
                ioContext.errorMsg = e.Message;
            }
            m_socket_op_Done.Set();
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

                // Signal that the connection has been made.
                m_socket_op_Done.Set();
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
                m_socket_op_Done.Set();
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
                }
            } else {
                if ( o.m_rx_remaining_count == 0 ) {
                    //
                    // Packet received - call handler
                    //
                    if ( o.m_handler != null ) {
                        o.m_rx_mem_stream.Seek(0, SeekOrigin.Begin);
                        o.m_handler.HandlePacket(new ecProto.ecPacket(o.m_sock_reader));
                    }
                    m_socket_op_Done.Set();
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
            m_socket_op_Done.Set();
        }

        public bool SendPacket(ecProto.ecPacket packet)
        {
            m_tx_mem_stream.Seek(0, SeekOrigin.Begin);
            packet.Write(m_sock_writer);

            m_socket_op_Done.Reset();
            m_s.BeginSend(m_tx_buffer, 0, packet.PacketSize(),
                SocketFlags.None, new AsyncCallback(TxCallback), this);

            return true;
        }
        public void StartReceive()
        {
            m_socket_op_Done.Reset();
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
                m_socket_op_Done.Reset();
                ResolveState resolveContext = new ResolveState();
                Dns.BeginGetHostEntry(host,
                    new AsyncCallback(GetHostEntryCallback), resolveContext);

                // Wait here until the resolve completes (the callback calls .Set())
                m_socket_op_Done.WaitOne();
                if ( resolveContext.IPs == null ) {
                    error = resolveContext.errorMsg;
                    return false;
                }
                Console.WriteLine("Resolved: '{0}' -> '{1}", host,resolveContext.IPs.AddressList[0]);
                Console.WriteLine("Connecting to {0}:{1}", resolveContext.IPs.AddressList[0], port);
                IPEndPoint remoteEP = new IPEndPoint(resolveContext.IPs.AddressList[0], port);
                m_s = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

                m_socket_op_Done.Reset();
                ConnectState connectContext = new ConnectState(m_s);
                m_s.BeginConnect(remoteEP, new AsyncCallback(ConnectCallback), connectContext);
                m_socket_op_Done.WaitOne(1000,true);

                if ( connectContext.errorMsg != null) {
                    error = connectContext.errorMsg;
                    return false;
                }

                m_tx_mem_stream = new MemoryStream(m_tx_buffer);
                m_sock_writer = new BinaryWriter(m_tx_mem_stream);
                m_rx_mem_stream = new MemoryStream(m_rx_buffer);
                m_sock_reader = new BinaryReader(m_rx_mem_stream);

                ecProto.ecLoginPacket p = new ecProto.ecLoginPacket("amule.net", "0.0.1", pass);
                SendPacket(p);

                if (!m_socket_op_Done.WaitOne(1000, true)) {
                    // Was unable to send login request for 1sec. Line must be really slow
                    return false;
                }

                m_handler = new amuleLogicHandler(this);
                StartReceive();

                // FIXME: must be able to cancel this read.
                m_socket_op_Done.WaitOne();
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
