using System;
using Microsoft.SPOT;

namespace MFConsoleApplication1
{
    using System.IO;
    //using System.Security.Cryptography.X509Certificates;
    using System.Text;
    using System.Threading;
    using System.Net;

    internal class MyHttpClient
    {
        private string url = null;

        public string Url
        {
            get
            {
                return this.url;
            }
            set
            {
                this.url = value;
            }
        }

        public  void Run()
        {
            Thread.Sleep(5000);
           // PrintHttpData("http://www.google.de");
            // Test SSL connection with no certificate verification
            int counter = 0;
            while (true)
            {


                try
                {
                    if (url == null)
                    {
                        Debug.Print(counter.ToString());
                        PrintHttpData("https://www.google.de");
                        //PrintHttpData("https://banking.dkb.de/dkb/-?$part=Welcome.login");
                        //PrintHttpData("https://epetitionen.bundestag.de/");
                        //PrintHttpData("https://www.elster.de/eon_home.php");
                    }
                    else
                    {
                        Debug.Print(url + ": counter: " + counter.ToString());
                        PrintHttpData(url);
                    }
                    counter++;
                }
                catch (Exception exception)
                {

                    throw;
                }
                Thread.Sleep(500);
            }
        }

        public static void PrintHttpData(string url)
        {
            // Create an HTTP Web request.
            HttpWebRequest request = HttpWebRequest.Create(url) as HttpWebRequest;

            // Assign the certificates. The value must not be null if the
            // connection is HTTPS.
            //request.HttpsAuthentCerts = caCerts;

            // Set request.KeepAlive to use a persistent connection. 
            request.KeepAlive = true;

            // Get a response from the server.
            WebResponse resp = null;

            try
            {
                resp = request.GetResponse();
            }
            catch (Exception e)
            {
                Debug.Print("Exception in HttpWebRequest.GetResponse(): " + e.ToString());
            }

            // Get the network response stream to read the page data.
            if (resp != null)
            {
                Stream respStream = resp.GetResponseStream();
                string page = null;
                byte[] byteData = new byte[8192];
                char[] charData = new char[8192];
                int bytesRead = 0;
                Decoder UTF8decoder = System.Text.Encoding.UTF8.GetDecoder();
                int totalBytes = 0;

                // allow 5 seconds for reading the stream
                respStream.ReadTimeout = 5000;

                // If we know the content length, read exactly that amount of 
                // data; otherwise, read until there is nothing left to read.
                if (resp.ContentLength != -1)
                {
                    for (int dataRem = (int)resp.ContentLength; dataRem > 0; )
                    {
                        Thread.Sleep(500);
                        bytesRead = respStream.Read(byteData, 0, byteData.Length);
                        if (bytesRead == 0)
                        {
                            Debug.Print(
                                "Error: Received " + (resp.ContentLength - dataRem) + " Out of " + resp.ContentLength);
                            break;
                        }
                        dataRem -= bytesRead;

                        // Convert from bytes to chars, and add to the page 
                        // string.
                        int byteUsed, charUsed;
                        bool completed = false;
                        totalBytes += bytesRead;
                        UTF8decoder.Convert(
                            byteData,
                            0,
                            bytesRead,
                            charData,
                            0,
                            bytesRead,
                            true,
                            out byteUsed,
                            out charUsed,
                            out completed);
                        page = page + new String(charData, 0, charUsed);

                        // Display the page download status.
                        Debug.Print("Bytes Read Now: " + bytesRead + " Total: " + totalBytes);
                    }

                    page = new String(System.Text.Encoding.UTF8.GetChars(byteData));
                }
                else
                {
                    // Read until the end of the data is reached.
                    while (true)
                    {
                        // If the Read method times out, it throws an exception, 
                        // which is expected for Keep-Alive streams because the 
                        // connection isn't terminated.
                        try
                        {
                            Thread.Sleep(100);
                            bytesRead = respStream.Read(byteData, 0, byteData.Length);
                        }
                        catch (Exception)
                        {
                            bytesRead = 0;
                        }

                        // Zero bytes indicates the connection has been closed 
                        // by the server.
                        if (bytesRead == 0)
                        {
                            break;
                        }

                        int byteUsed, charUsed;
                        bool completed = false;
                        totalBytes += bytesRead;
                        UTF8decoder.Convert(
                            byteData,
                            0,
                            bytesRead,
                            charData,
                            0,
                            bytesRead,
                            true,
                            out byteUsed,
                            out charUsed,
                            out completed);
                        page = page + new String(charData, 0, charUsed);

                        // Display page download status.
                        //Debug.Print("Bytes Read Now: " + bytesRead + " Total: " + totalBytes);
                    }

                    Debug.Print("--- Total bytes downloaded in message body : " + totalBytes + ", (" + url + ")");
                }

                // Display the page results.
                //Debug.Print(page);

                // Close the response stream.  For Keep-Alive streams, the 
                // stream will remain open and will be pushed into the unused 
                // stream list.
                //respStream.Close();
                resp.Close();
                
            }
        }
        /*
        /// <summary>
        /// Prints the HTTP Web page from the given URL and status data while 
        /// receiving the page.
        /// </summary>
        /// <param name="url">The URL of the page to print.</param>
        /// <param name="caCerts">The root CA certificates that are required for 
        /// validating a secure website (HTTPS).</param>
        public static void PrintHttpData(string url, X509Certificate[] caCerts)
        {
            // Create an HTTP Web request.
            HttpWebRequest request = HttpWebRequest.Create(url) as HttpWebRequest;

            // Assign the certificates. The value must not be null if the
            // connection is HTTPS.
            request.HttpsAuthentCerts = caCerts;

            // Set request.KeepAlive to use a persistent connection. 
            request.KeepAlive = true;

            // Get a response from the server.
            WebResponse resp = null;

            try
            {
                resp = request.GetResponse();
            }
            catch (Exception e)
            {
                Debug.Print("Exception in HttpWebRequest.GetResponse(): " + e.ToString());
            }

            // Get the network response stream to read the page data.
            if (resp != null)
            {
                Stream respStream = resp.GetResponseStream();
                string page = null;
                byte[] byteData = new byte[4096];
                char[] charData = new char[4096];
                int bytesRead = 0;
                Decoder UTF8decoder = System.Text.Encoding.UTF8.GetDecoder();
                int totalBytes = 0;

                // allow 5 seconds for reading the stream
                respStream.ReadTimeout = 5000;

                // If we know the content length, read exactly that amount of 
                // data; otherwise, read until there is nothing left to read.
                if (resp.ContentLength != -1)
                {
                    for (int dataRem = (int)resp.ContentLength; dataRem > 0;)
                    {
                        Thread.Sleep(500);
                        bytesRead = respStream.Read(byteData, 0, byteData.Length);
                        if (bytesRead == 0)
                        {
                            Debug.Print(
                                "Error: Received " + (resp.ContentLength - dataRem) + " Out of " + resp.ContentLength);
                            break;
                        }
                        dataRem -= bytesRead;

                        // Convert from bytes to chars, and add to the page 
                        // string.
                        int byteUsed, charUsed;
                        bool completed = false;
                        totalBytes += bytesRead;
                        UTF8decoder.Convert(
                            byteData,
                            0,
                            bytesRead,
                            charData,
                            0,
                            bytesRead,
                            true,
                            out byteUsed,
                            out charUsed,
                            out completed);
                        page = page + new String(charData, 0, charUsed);

                        // Display the page download status.
                        Debug.Print("Bytes Read Now: " + bytesRead + " Total: " + totalBytes);
                    }

                    page = new String(System.Text.Encoding.UTF8.GetChars(byteData));
                }
                else
                {
                    // Read until the end of the data is reached.
                    while (true)
                    {
                        // If the Read method times out, it throws an exception, 
                        // which is expected for Keep-Alive streams because the 
                        // connection isn't terminated.
                        try
                        {
                            Thread.Sleep(500);
                            bytesRead = respStream.Read(byteData, 0, byteData.Length);
                        }
                        catch (Exception)
                        {
                            bytesRead = 0;
                        }

                        // Zero bytes indicates the connection has been closed 
                        // by the server.
                        if (bytesRead == 0)
                        {
                            break;
                        }

                        int byteUsed, charUsed;
                        bool completed = false;
                        totalBytes += bytesRead;
                        UTF8decoder.Convert(
                            byteData,
                            0,
                            bytesRead,
                            charData,
                            0,
                            bytesRead,
                            true,
                            out byteUsed,
                            out charUsed,
                            out completed);
                        page = page + new String(charData, 0, charUsed);

                        // Display page download status.
                        Debug.Print("Bytes Read Now: " + bytesRead + " Total: " + totalBytes);
                    }

                    Debug.Print("Total bytes downloaded in message body : " + totalBytes);
                }

                // Display the page results.
                Debug.Print(page);

                // Close the response stream.  For Keep-Alive streams, the 
                // stream will remain open and will be pushed into the unused 
                // stream list.
                resp.Close();
            }
        }*/
    }

}
