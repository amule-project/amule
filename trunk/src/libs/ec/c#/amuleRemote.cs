using System;
using System.Security;
using System.Security.Permissions;
using System.Threading;
using System.Collections.Generic;
using System.Text;
using System.Net;
using System.Net.Sockets;

namespace amule.net
{
    class amuleSocket {
        Socket sock;

        bool ReadNumber(ref UInt16 i)
        {
            return false;
        }

        public amuleSocket(Socket s)
        {
            sock = s;
        }
    }

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

    class amuleRemote {
        //
        // M$ way to resolve DNS
        //
        static ManualResetEvent resolveDone = new ManualResetEvent(false);
        static ManualResetEvent connectDone = new ManualResetEvent(false);

        // Record the IPs in the state object for later use.
        static void GetHostEntryCallback(IAsyncResult ar)
        {
            ResolveState ioContext = (ResolveState)ar.AsyncState;
            try {
                ioContext.IPs = Dns.EndGetHostEntry(ar);
            } catch (SocketException e) {
                ioContext.errorMsg = e.Message;
            }
            resolveDone.Set();
        }

        UInt32 flags;

        amuleSocket s;

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
                connectDone.Set();
            } catch (Exception e) {
                state.errorMsg = e.Message;
            }
        }

        [DnsPermission(SecurityAction.Demand, Unrestricted = true)]
        public bool ConnectToCore(string host, int port, string login, ref string error)
        {
            try {
                resolveDone.Reset();
                ResolveState resolveContext = new ResolveState();
                Dns.BeginGetHostEntry(host,
                    new AsyncCallback(GetHostEntryCallback), resolveContext);

                // Wait here until the resolve completes (the callback calls .Set())
                resolveDone.WaitOne();
                if ( resolveContext.IPs == null ) {
                    error = resolveContext.errorMsg;
                    return false;
                }
                Console.WriteLine("Resolved: '{0}' -> '{1}", host,resolveContext.IPs.AddressList[0]);

                IPEndPoint remoteEP = new IPEndPoint(resolveContext.IPs.AddressList[0], port);
                Socket s = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

                ConnectState connectContext = new ConnectState(s);
                s.BeginConnect(remoteEP, new AsyncCallback(ConnectCallback), connectContext);

                connectDone.WaitOne(1000,true);

                if ( connectContext.errorMsg != null) {
                    error = connectContext.errorMsg;
                    return false;
                }

            } catch (Exception e) {
                error = e.Message;
                return false;
            }
            return true;
        }

        public bool SendRequest()
        {
            return true;
        }
    }
}
