//*****************************************************************************
// Copyright (C) 2014 Cambridge Silicon Radio Ltd.
// $Header: /cvs/MICECore/Include/common/qio.h,v 1.2 2015/12/24 07:57:44 AV01854 Exp $
// $Change: 244499 $ $Date: 2015/12/24 07:57:44 $
//
/// @file
/// Stream I/O.
/// API and object for wrapping a particular stream I/O system so that the
/// Inferno system can treat it as a generic stream.
///
/// @ingroup IO
/// @defgroup IO IO Stream Input/Output (Files/TCP/FTP/USB/TTY/etc.)
//
//*****************************************************************************
#ifndef QIO_H
#define QIO_H 1

struct tag_qio;
typedef struct tag_qio* PQIO;

#if defined(HOST_BASED) || defined(Linux) || defined(Windows)
    #include <fcntl.h>
#else
    #ifdef O_RDONLY
        #if O_CREAT != 0x0004
            #warning Somebody is including a Host OS system header
            #undef O_RDONLY
            #undef O_WRONLY
            #undef O_RDWR
            #undef O_CREAT
            #undef O_TRUNC
        #endif
    #endif
    #define O_RDONLY    0x0001
    #define O_WRONLY    0x0002
    #define O_RDWR      0x0003
    #define O_CREAT     0x0004
    #define O_TRUNC     0x0008

    #define SEEK_SET    0
    #define SEEK_CUR    1
    #define SEEK_END    2
#endif

//--------------------------------------------------------------------------

#define QIO_POLL_READ           1
#define QIO_POLL_WRITE          2
#define QIO_POLL_EVENT          4

/// Determine if the stream is readable.
/// See the description of the QIOPOLL function type for details
///
#define QIOreadable(q, t, u)    ((q)->Poll(q, QIO_POLL_READ, t, u))

/// Determine if the stream is writeable
///
#define QIOwriteable(q, t, u)   ((q)->Poll(q, QIO_POLL_WRITE, t, u))

/// Read bytes from a stream.
/// See QIOREAD type for details
///
#define QIOread(q, b, n)        ((q)->Read(q, b, n))

/// Write bytes to a stream
/// See QIOWRITE type for details
///
#define QIOwrite(q, b, n)       ((q)->Write(q, b, n))

/// Seek to a position in the stream
/// See QIOSEEK type for details
///
#define QIOseek(q, o, w)        (((q)->Seek) ? (q)->Seek(q, o, w) : QIOEOF)

/// Close a stream
/// See QIOCLOSE for details
///
#define QIOclose(q)             ((q)->Close(q))

#define QIOEOF      (-1)

/// Determine if the stream is readable or writeable based on the what parameter
/// @returns > 0 if the stream is readable or writeable
///          = 0 if there is no data pending
///          < 0 on errors, or the stream is at end of file
/// Note that many qio implementations follow the paradigm of
/// posix where if a poll operation returns readable (> 0) and
/// a subsequent read returns 0 bytes of data, that is an
/// indication that the stream has been closed, for example the
/// other side of a TCP/IP connection has shutdown the socket.
/// Use -1 for toSeconds to block (not recommended). Use 0 for
/// both timeout parameters to return instantly
///
typedef int         (*PQIOPOLL)     (
                                    PQIO pqio,      ///< stream to poll
                                    int what,       ///< poll for read (1) or write (2)
                                    int toSecs,     ///< seconds to wait for results
                                    int toMicroSecs ///< microseconds to wait
                                    );

/// Read bytes from a stream non-blocking
/// @returns  number of bytes read into buffer or < 0 on error
/// or QIOEOF (-1) for end-of-file/stream
/// Note that reading 0 bytes after a succesful QIOPOLL is an
/// indication that the underlying stream has been closed or 
/// shut down, and should be treated as an end of stream condition
/// as well as getting a QIOEOF return.
/// Note that this call may return less than the number requested
/// as it is non-blocking.
///
typedef int         (*PQIOREAD)     (
                                    PQIO pqio,      ///< stream to read from
                                    unsigned char *buffer,  ///< buffer to read into
                                    int nToRead     ///< byte to read (maximum)
                                    );

/// Write bytes to a stream non-blocking
/// @returns number of bytes from buffer written or < 0 on errors.
/// Note that less then the bytes presented may be written, including
/// 0 bytes since this is a non-blocking call.  Do not implement 
/// blocking I/O by looping on read or write.  Inferno depends on
/// asynchronous I/O to work properly.
///
typedef int         (*PQIOWRITE)    (
                                    PQIO pqio,      ///< stream to write from
                                    unsigned char *buffer,  ///< buffer to write from
                                    int nToWrite    ///< bytes to write (maximum)
                                    );

/// Seek to offset bytes from a specified position in the stream.
/// @retuns the ending offset in the stream, or < 0 on errors.
/// Some stream types such as TCP/IP connections aren't seekable and
/// therefore this member function would be NULL and the QIOseek macro
/// would return QIOEOF (-1).  To create a/ seekable stream from a
/// non-seekable stream you can use QIOMEMPREBUFFEREDcreate
/// to wrap the stream in a memory-backed buffered QIO
///
typedef int         (*PQIOSEEK)     (
                                    PQIO pqio,      ///< stream to seek in
                                    long offset,    ///< offset to seek to
                                    int fromWhere   ///< starting position (see posix lseek)
                                    );

/// Close a stream.  This will close underlying stream objects such as 
/// file handles or sockets, and delete the qio object.
/// @return < 0 on errors, 0 for OK
///
typedef int         (*PQIOCLOSE)    (PQIO pqio /**< stream to close */);

/// QIO object
///
typedef struct tag_qio
{
    void           *priv;           ///< user data, holds context for implementation
    PQIOCLOSE       Close;          ///< close function
    PQIOPOLL        Poll;           ///< poll funciton
    PQIOREAD        Read;           ///< read function
    PQIOWRITE       Write;          ///< write function
    PQIOSEEK        Seek;           ///< seek function
}
QIO;

struct tag_netconnection;
struct tag_usbdconnection;

#ifndef NETTYPES_H
typedef void *SOCKET_TYPE;
#else
#define SOCKET_TYPE SOCKET
#endif

/// Create a NULL QIO.  This QIO is used to drop stream bytes into the bit-bucket
/// @returns pointer to QIO or NULL on failure
///
PQIO        QIONULLcreate           (void);

/// Add a file to a the memory-backed file sytem.
/// @returns non-0 on error
///
int         QIOMEMaddFile           (
                                    const char* name,       ///< [in] name of file to add
                                    Uint8* pbytes,          ///< [in] data area for file data
                                    Uint32 nRoom,           ///< [in] how large the data area is
                                    Uint32 nValid           ///< [in] how many bytes in room are valid
                                    );
/// Remove a file to a the memory-backed file sytem.
/// @returns non-0 on error
///
int         QIOMEMremoveFile        (const char* name /**< name of file to remove */);

/// Set the contents of a file in the memory-backed file list.
/// @returns non-0 on error
///
int         QIOMEMsetFileContents   (
                                    const char* name,       ///< [in] name of file to add
                                    Uint8* pbytes,          ///< [in] data area for file data
                                    Uint32 nRoom,           ///< [in] how large the data area is
                                    Uint32 nValid           ///< [in] how many bytes in room are valid
                                    );

/// Retrieve the data for a file in the memory-backed file list.
/// @returns a pointer to the data or NULL on failure
///
Uint8*      QIOMEMbytesFromName     (
                                    const char* name,       ///< [in] name of file to add
                                    Uint32* nRoom,          ///< [out] how large the data area is
                                    Uint32* nValid          ///< [out] how many bytes in room are valid
                                    );

/// Create a buffered-stream QIO.  This is a simple FIFO buffer to put between
/// two asynchronous threads, one producing and one consuming bytes in a stream
/// @returns pointer to QIO or NULL on failure
///
PQIO        QIOSTREAMcreate         (
                                    int iosize              ///< [in] how large a buffer to allocate
                                    );

/// Set a flag to indicate EOF to the reader of the stream QIO.  This lets a 
/// producer thread end the stream without destroying the actual object, since
/// the consumer thread probably owns the object
/// @returns non-0 on error (bad qio)
///
int         QIOSTREAMsetEOF         (PQIO pqio /**< stream qio to set EOF on */);

/// Clear any set EOF flag in a stream QIO so the reader can read new data written to
/// the QIO after an EOF condition set.  This allows multiple files to be transferred
/// on a single stream object
/// @returns non-0 on error (bad qio)
///
int         QIOSTREAMclearEOF       (PQIO pqio /**< stream qio to clear EOF on */);

/// Get the EOF state of a stream QIO for waiting to add data while reader is getting previous file
/// @returns non-0 on error (bad qio)
///
int         QIOSTREAMatEOF          (
                                    PQIO pqio,              ///< [in] stream qio to get state of
                                    int *eofstate           ///< [out] eof state of qio (non-0 == eof)
                                    );

/// Set the EOF state of a stream QIO, and wait for the EOF state to be reset
/// by a different reader thread.
/// @returns non-0 on error (bad qio, timeout, errors)
///
int         QIOSTREAMsyncAtEOF      (
                                    PQIO pqio,              ///< [in] stream qio to get state of
                                    int tos,                ///< [in] timeout sec 
                                    int tous                ///< [in] timeout usec 
                                    );

/// Create a memory-backed QIO.  This is a chunk of memory that can be accessed as
/// a QIO object to provide read/write/seek access as if the object was a file.  A
/// QIOMEM object can grow (to maxRoom) as data is written.  Other QIO types can
/// use a QIOMEM underneath for data buffering.  The HTTP QIO is an example.
/// If there is no initial data, a data area of size nRoom is allocated.  If the
/// maxRoom parameter is NULL, the memory area is limited to the initial size.
/// @returns pointer to QIO or NULL on failure
///
PQIO        QIOMEMcreate            (
                                    Uint8* pbytes,          ///< [in] data area for file data, may be NULL
                                    Uint32 nRoom,           ///< [in] how large the data area is.
                                    Uint32 maxRoom,         ///< [in] how large the memory region is allowed to grow. may be 0
                                    Uint32 nValid,          ///< [in] how many bytes in room are valid
                                    int flags               ///< [in] create flags
                                    );

/// Create a pre-bufferred memory-backed QIO from a stream
/// This provides a QIOMEM based QIO which is filled from the stream qio passed in
/// The stream QIO is read as needed by the user, but kept in memory so that
/// stream becomes back-seekable.  This is needed for reading jpeg and tiff images
/// from streams for example.
/// @returns pointer to QIO or NULL on failure
///
PQIO        QIOPREBUFFEREDcreate(
                                    PQIO pqio_stream,       ///< [in] stream to base memory backing on
                                    int  initial_size,      ///< [in] bytes count to allocate for initial size
                                    int  max_size,          ///< [in] maximum read/alloc size of buffer
                                    Uint8 *preread,         ///< [in] bytes to insert intially into memory buffer
                                    int prereadcount        ///< [in] how many pre read bytes to insert
                                    );

/// Create a memory backed QIO on top of a buffer in the memory-backed file list
/// @returns NULL on failure
///
PQIO        QIOMEMcreateFromName    (
                                    const char* name,       ///< [in] name of previously added file
                                    int flags               ///< [in] create flags
                                    );

/// Create a file based QIO stream.  This is an upper level function which
/// calls more specific creation functions depending on the system and
/// the stream name.
/// If the stream is an ftp or http URI, then the appropriate functions
/// are called to open a network stream if those protocols are configured
/// @returns QIO pointer, or NULL on failure
///
PQIO        QIOFILEcreate           (
                                    const char* stream,     ///< [in] stream name / uri
                                    int flags               ///< [in] creation flags
                                    );

/// Create an FTP based stream stream.  The direction of transfer is based on
/// first use (read or write) and can't be changed once a transfer is started.
/// FTP QIOs are not seeakable.
/// @returns QIO pointer or NULL on failure
///
PQIO        QIOFTPFILEcreate        (
                                    const char* stream,     ///< [in] ftp uri
                                    const char *user,       ///< [in] user name for authentication, may be NULL
                                    const char *pass        ///< [in] password for authentication, may be NULL
                                    );

/// Create an HTTP based stream (read only).  This creates a QIO which is suitable
/// to be used to do an HTTP GET by calling QIOHTTPFILEtransfer().  Underneath
/// the QIO is a memory based QIO which buffers the http data before it is ready to read.
/// This allows the retreival of large files like JPEG, TIFF, and PDF that need to be seekable
/// The HTTP transfer is started by a separate call so the internal HTTP object can be
/// modified (headers added, etc.) by calling QIOHTTPFILEgetHTTP().
/// @returns QIO pointer or NULL on failure
///
PQIO        QIOHTTPFILEcreate       (
                                    const char *stream,     ///< [in] http uri
                                    Uint32 minbuf,          ///< [in] starting size of http data buffer
                                    Uint32 maxbuf,          ///< [in] maximum suze of http data buffer
                                    int mode                ///< [in] must be O_RDONLY
                                    );

/// Start an HTTP transfer on a stream opened with QIOHTTPFILEcreate().
/// The transfer happens in the calling threads context, and the return
/// is 0 if the HTTP request encountered no process errors.  The HTTP transfer
/// result itself (the code) is placed in the rcode parameter.
/// @returns non-0 on socket/parameter errors, 0 on basic success, check rcode for HTTP status
///
int         QIOHTTPFILEtransfer     (
                                    PQIO pqio,              ///< [in] QIO created with QIOHTTPFILEcreate()
                                    const char *user,       ///< [in] user name for authentication, may be NULL
                                    const char *pass,       ///< [in] password for authentication, may be NULL
                                    int *rcode              ///< [out] set to the HTTP return code of the transfer
                                    );

#if 0 // this is private to includers of net/incl/http.h
/// Get the underlying HTTP object for an QIOHTTPFILE object.  This can be used to
/// add headers, etc.  Do not delete this object
/// @returns the HTTP handle, or NULL (for invalid QIOs)
///
PHHTTP      QIOHTTPFILEgetHTTP      (PQIO pqio /**< [in] QIO to get http object from */);
#endif

/// Create a TCP/IP network stream QIO based on a connection callback for
/// a service registered via NetserverAddService()
/// @returns NULL on failure
///
PQIO        QIOTCPcreate            (/*struct tag_netconnection**/int pnc /**< [in] net connection */); //kevin

/// Create a TCP/IP network stream QIO from an open network socket
/// @returns NULL on failure
///
PQIO        QIOTCPcreateFromSOCKET  (/*SOCKET_TYPE*/int sock /**< [in] socket to wrap in QIO */);//kevin

/// Create a TCP/IP network stream QIO from connecting to a
/// remote server at hostname:port, waiting perhaps for the
/// connection to be accepted.
/// @returns NULL on failure
///
PQIO        QIOTCPcreateByConnection(
                                    const char *hostname,   ///< [in] host name to connect to
                                    unsigned short port,    ///< [in] port to connect to
                                    int tos,                ///< [in] connect timeout, seconds
                                    int tous                ///< [in] connect timeout, microseconds
                                    );

/// Given a TCP/IP QIO, return it's internal socket handle
/// @returns socket handle or INVALIDSOCKET on failure
///
/*SOCKET_TYPE*/int QIOTCPgetSocket         (PQIO pqio /**< [in] stream to fetch socket for */);//kevin

/// Set the internal socket handle into a TCP/IP QIO
///
void        QIOTCPsetSocket         (
                                    PQIO pqio,              ///< [in] stream to set socket in
                                    /*SOCKET_TYPE*/int sock        ///< [in] socket to set
                                    ); //kevin
#if IN_SSL
/// Wrap the incoming QIO with an SSL wrapper.
/// SSL negotiation needs to being at the server so a parameter is used
/// to indicate if the SSL connection on the QIO is used as a client (typical) or server.
/// Typically this is used on a TCP/IP QIO, but doesn't technically have to be
/// since the implementation marshalls i/o properly to the SSL layer from the QIO
/// @returns NULL on failure
///
PQIO        QIOSSLcreate            (
                                    PQIO pqio,              ///< [in] stream to wrap with SSL
                                    int isClient            ///< [in] non-0 if this is a client side
                                    );


/// Create an SSL QIO based in a TCP/IP socket. See also QIOSSLcreate()
/// @returns NULL on failure
///
PQIO        QIOSSLcreateFromSOCKET  (
                                    SOCKET_TYPE sock,       ///< [in] socket to wrap
                                    int isClient            ///< [in] non-0 if this is a client side
                                    );

/// Initalize the SSL layer.  This is called each time an SSL wrapper is made
/// and QIOSSLepilog() each time an SSL wrapped QIO is closed, so this doesn't
/// have to be called by applications.  Some code, like the HTTP server, which
/// typically serves multiple threads simulataneously will call this to insure
/// the SSL layer is initialized properly only once, since the prolog is not
/// thread safe
/// @returns non-0 on error
///
int         QIOSSLprolog            (void);

/// Bring down/clean SSL layer. The pro/epilog functions are reference counted
/// so only the last call to the epilog will actual destroy any resources. See
/// QIOSSLprolog()
///
int         QIOSSLepilog            (void);

#endif
/// Create a QIO stream based on a connection on the USB device port
/// in a callback from a service registered with USBDserverAddService()
/// @returns NULL on failure
///
PQIO        QIOUSBDcreate           (struct tag_usbdconnection* pusbdc);

/// Create a QIO stream on a serial port
/// @returns NULL on failure
///
PQIO        QIOTTYcreate            (
                                    int port,               ///< [in] which serial port
                                    int baud,               ///< [in] baud rate to use
                                    int bits,               ///< [in] bits
                                    int stops,              ///< [in] stop bits
                                    int parity,             ///< [in] parity 0-none, 1-odd, 2-even
                                    int flow                ///< [in] flow control (h/w)
                                    );

/// Create a QIO on a UDP (IP datagram) connection to a remote server
/// Note that UDP is connectionless in the IP sense and the connection
/// in the function name is the connection of the QIO to the SOCKET
/// that gets created
/// @returns NULL on failure
///
PQIO        QIOUDPcreateByConnection(
                                    const char* hostname,  ///< [in] ip addr or host name of server
                                    unsigned short port,   ///< [in] server port num
                                    int tos,               ///< [in] timeout sec 
                                    int tous               ///< [in] timeout usec 
                                    );

/// Create a UDP server QIO for the given port number
/// If join_mcast is non-0, the ip address for the multicast group must be provided
/// @returns NULL on failure
///
PQIO        QIOUDPserverCreate      (
                                    unsigned short port,    ///< [in] port number of the UDP server  
                                    int port_sacred,        ///< [in] must bind to this port ?
                                    int join_mcast,         ///< [in] whether to join multicast group
                                    unsigned long ip,       ///< [in] ipv4 address of multicast group
                                    int ttl,                ///< [in] time to live  
                                    int enable_bcast        ///< [in] whether enable broacast
                                    );


/// Begin an image in a PDF stream creation filter.  This sets the dimensions and
/// format of the image being added to a PDF filter.  See QIOPDFcreate
/// @returns non-0 on errors
///
int         QIOPDFbeginImage        (
                                    PQIO pqio,              ///< [in] QIO to write PDF output to
                                    const char *filter,     ///< [in] compression (in PDF terms, i.e. DCTDecode)
                                    int w,                  ///< [in] image width
                                    int h,                  ///< [in] image height
                                    int d,                  ///< [in] image depth
                                    int s,                  ///< [in] image stride
                                    int f,                  ///< [in] image format
                                    int xres,               ///< [in] x res of image (can be 0)
                                    int yres,               ///< [in] x res of image (can be 0)
                                    int clipx,              ///< [in] limit pdf output to this max width, can be 0
                                    int clipy               ///< [in] limit pdf output to this max height, can be 0
                                    );

/// Create a PDF creation stream. This QIO is a write-only stream filter which takes a
/// QIO destination, and builds a PDF wrapper around image data written to it.  Use
/// QIOPDFbeginImage to setup image dimensions, then write normally
/// @returns NULL on errors
///
PQIO        QIOPDFcreate            (
                                    PQIO pqioDst,           ///< [in] qio to write PDF data to
                                    int  takeOwnership      ///< [in] non-0 means QIOPDF now owns input qio (to delete on close)
                                    );
                                
/// how long to wait (millisecs) for bytes inside a single read/poll for
/// async stream reading.  Since print pipe steps read streams in a
/// pipe-loop they shouldn't block the loop for more than a typical DSP
/// or h/w band processing time which is in the low-milliseconds range
///
#define PDL_READ_DWELL  10

/// Since IO is done in a pipeline loop we don't want to block the pipe
/// waiting for data. This structure holds the needed incremental state
/// to handle long (many minutes) timeouts. This also provides a 
/// physical memory buffer so that i/o bytes can be passed to DSP, etc.
/// and a virtual mapping for host cpu access.  Its really just a
/// ring buffer, but with the ability to fill a small physical buffer
/// from the ring.
///
typedef struct tag_async_pdl_io
{
    PQIO    pqio;               ///< stream used to fill this context (can change)
    Uint8  *iobuf;              ///< io bytes, ring buffer
    int     iohead;             ///< head index
    int     iotail;             ///< tail index
    int     iocnt;              ///< valid bytes in buf now
    int     iosize;             ///< buffer allocated size
    int     tosecs;             ///< seconds after tostart when request times out
    Uint32  tostart;            ///< when, in system seconds, the last byte was read, or initialized
}
PDLIOCTX, *PPDLIOCTX;

/// Allocate and initialize a PDL stream reading context to enable long
/// timeouts reading streams in pipe loops.  The toSecs parameter is used
/// to set final timeout time from the time this function is called.
/// Any bytes read from the stream resets the end time to this many seconds
/// after the current time. Setting a stream with PDLreadSetStream() also
/// resets the timeout timer.
/// @returns a context for buffered pdl i/o
///
PPDLIOCTX PDLcreateReadContext (
                                PQIO        pqio,     ///< [in] stream to read from
                                int         iosize,   ///< [in] size of ring buffer to allocate
                                int         toSecs    ///< [in] how many seconds to wait to fill buffer
                                );

/// Delete a PDL read context. Both the qio and buffer passed in to the 
/// create call PDLcreateReadContext are not altered closed or freed.                                
/// @returns non-0 on error
///
int     PDLdestroyReadContext   (PPDLIOCTX prx /**< read context to destroy */);

/// Incrementally read bytes from the stream into a buffer context
/// that was created and initialized by PDLcreateReadContext().
/// Returns the address of the first unread byte in both physical
/// and virtual address spaces as well as the number of bytes available.
/// Note that the PDL read context always garantees that the 
/// read data presented is contiguous, since h/w blocks need 
/// contiguous physical buffers.  Don't make the buffer too big
/// in PDLcreateReadContext() if you do a lot of single-byte stuff
/// since the data might be moved each time
/// @returns < 0 for eof, > 0 for timeout
///
int     PDLreadUpdate           (
                                PPDLIOCTX   prx,    ///< [in] context with io state
                                int        *count   ///< [out] count of data read, can be NULL
                                );
                        
/// Read bytes from prx into buffer, only up to bytes available in read context
/// non-blocking, no timeout check
/// @returns < 0 for errors, >= 0 bytes read into buffer
///
int     PDLreadBytes           (
                                PPDLIOCTX   prx,    ///< [in] context with io state
                                Uint8      *buffer, ///< [out] buffer to fill
                                int         count   ///< [in] bytes to read (max)
                                );

/// Mark consumed bytes in the input buffer (advance tail)
/// @returns non-0 for errors
///
int     PDLreadConsume          (
                                PPDLIOCTX   prx,    ///< [in] context with io state
                                int         consume ///< [in] number of input bytes to eat
                                );

/// Set a new stream into a read context.  resets timeouts but leaves current ring alone
/// @returns non-0 for errors
///
int     PDLreadSetStream        (
                                PPDLIOCTX   prx,    ///< [in] context with io state
                                PQIO        pqui    ///< [in] stream to read
                                );

/// Set a new stream timeout into a read context
/// @returns non-0 for errors
///
int     PDLreadSetTimeout       (
                                PPDLIOCTX   prx,    ///< [in] context with io state
                                int         timeout ///< [in] new timeout in seconds
                                );

/// Get the current stream from a read context
/// @returns non-0 for errors (no stream)
///
int     PDLreadGetStream        (
                                PPDLIOCTX   prx,    ///< [in] context with io state
                                PQIO        *ppqui  ///< [out] stream that pctx is using
                                );

/// Create an I/O buffer set (physical and virtual mapping)
/// for use in a PDLs that need a physical buffer read context (for example)
/// @returns the physical address created, and the virtial mapping
/// and actual size allocated which can be larger than requested
/// which should be used to call PDLdestroyIObuffer()
/// @returns NULL on failure
///
Uint8   *PDLcreateIObuffer      (
                                int         size,   ///< [in] how big a buffer
                                int        *actualSize, ///< [out] how big was made
                                Uint8     **pva_iobuf   ///< [out] the virtual address
                                );

/// free an IO buffer-set created with PDLcreateIObuffer()
/// @returns non-0 on error
///                                
int     PDLdestroyIObuffer      (
                                Uint8      *ppa,    /// [in] the physical address alloced
                                Uint8      *pva,    /// [in] the virtual address alloced
                                int         size    /// [in] the actual size from the alloc
                                );
#endif
