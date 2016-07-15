//*****************************************************************************
//  Copyright (C) 2013 Cambridge Silicon Radio Ltd.
//  $Header: /mfp/MiceUiKey/MiceUiKey/bios.h,v 1.1 2016/05/30 03:48:51 AV00287 Exp $
//  $Change: 251312 $ $Date: 2016/05/30 03:48:51 $
//
/// @file
/// Basic Input Output Sytem.
/// BIOS defines an upper layer API to abstract low level chip/platform/host
/// specific operations from the application code.  BIOS services are used for
/// builds running on host systems as well as target cpus and in the boot-rom
/// code as well.
///
/// @author Brian Dodge <brian.dodge@csr.com>
///
/// @defgroup BIOS BIOS - Basic Input Output System
//
//*****************************************************************************
#ifndef BIOS_H
#define BIOS_H 1

#ifdef __ARMCC_VERSION
#define inline
#endif

/// @name Convenience
/// Convenience macros
///@{

/// macros to read/write registers and other non-memory spaces
///
#define WRREG_UINT32(p,c)   (*((volatile Uint32*)(p)) = ((Uint32)(c)))
#define WRREG_UINT16(p,c)   (*((volatile Uint16*)(p)) = ((Uint16)(c)))
#define WRREG_UINT8(p,c)    (*((volatile Uint8 *)(p)) = ((Uint8)(c)))
#define RDREG_UINT32(p)     (Uint32)(*(volatile Uint32*)(p))
#define RDREG_UINT16(p)     (Uint16)(*(volatile Uint16*)(p))
#define RDREG_UINT8(p)      (Uint8)(*(volatile Uint8*)(p))

/// macros to do the same, when the register/other spaces are 
/// memory mapped (virtual memory systems, like Linux)
/// These are never used for non-Quatro builds, so Uint32 assumption of 
/// address size is OK
///
#ifndef HOST_BASED
#define MAPREG_WR32(x, r, v)        \
    (*(volatile Uint32 *)((Uint8*)((x)->base) + (r - (Uint32)((x)->start))) = (v))
#define MAPREG_RD32(x, r)            \
    (*(volatile Uint32 *)((Uint8*)((x)->base) + (r - (Uint32)((x)->start))))
#endif

#define PSPRINTF BIOSlog

#ifdef __COVERITY__
# define ASSERT(_XX_) (assert(x);)
#else
//# define ASSERT(_XX_)                   \
//    do { if (!(_XX_)) { BIOSassertFail(#_XX_, __FILE__, __LINE__); }} while (0)
#endif

#define OneMEG     ((unsigned long)1024 * 1024)
#define sixteenMEG ((unsigned long)16* 1024 * 1024)
#define OneGig     ((unsigned long)OneMEG * 1024)

        
#ifdef Q4500
/// We use the "A to B request" to generate an interrupt event on the second cpu
///
#define LAUNCH_CPU2(launch_dst)                                         \
    WRREG_UINT32(0xe81ffffc, launch_dst);                               \
    WRREG_UINT32(EXMSK2B, RDREG_UINT32(EXMSK2B) | INT_B_A2BREQ);        \
    WRREG_UINT32(EXSETB,  INT_B_A2BREQ);                                \
    WRREG_UINT32(EXCLRB,  INT_B_A2BREQ);                                \
    WRREG_UINT32(EXMSK2B, RDREG_UINT32(EXMSK2B) & (~(INT_B_A2BREQ)));
#else
#define LAUNCH_CPU2(launch_dst)
#endif

///@}

/// @name Initialization
/// BIOS initialization API
///@{

/// Common initialization of the BIOS layer, Sets up serial ports, etc.
/// This is called by most instantiations of the BIOSinit() function
/// to do things needed by all bios code
///
void            BIOScommonInit      (void);

/// For LINUX only, this starts init.  We starts init after the inferno app starts which
/// gives the app at least the appearance of starting quicker
///
void            BIOSstartInitProc   (Uint32  delay_microseconds /**< wait this long before init */);

/// Initialize BIOS layer.  This function is called by the system boot code
/// and initializes the hardware, overlays, etc. and prepares the system to
/// run applications.  There is one BIOSinit function specific to each
/// platform.
///
void            BIOSinit            (void);

///@}

/// @name Interrupts
/// ICU support
///@{

/// pointer to an interrupt vector
///
typedef void (*PIRQHANDLER)(int irq);

/// Initialize the interrupt controller
///
void            BIOSintsInit        (void);

/// Clear an Interrupt.  This will clear the interrupt in the controller.
/// Note that, for some interrupt causes, such as coprocessor mailbox
/// interrupts, the underlying cause needs to be cleared or the interrupt 
/// will reassert after calling this function.
///
void            BIOSintClear        (Uint32 vector /**< interrupt to clear */);

/// Set Interrupt.  Force an interrupt.
///
void            BIOSintSet          (Uint32 vector /**< interrupt to set */);

/// Enable Interrupt.  Sets the proper enable in the controller's enable mask
///
void            BIOSintEnable       (Uint32 vector /**< interrupt to enable */);

/// Disable Interrupt.  Disables the interrupt via the controller's enable mask
///
void            BIOSintDisable      (Uint32 vector /**< interrupt to disable */);

/// Turn CPU interrupts off.  This disables all interrupts, and thus should
/// be used only by critical system code.  
/// Code should save the state of interrupts and use it to restore the
/// state with BIOSintsRestore() so that functions which turn off interrupts
/// can call other functions which might also turn off interrupts.
/// @code
///     unsigned long mask;
///
///     // disable interrupts
///     //
///     mask = BIOSintsOff();
///
///     // perform critical function
///     //
///
///     // restore interrupts
///     //
///     BIOSintsRestore(mask);
/// @endcode
/// @returns the on/off state of interrupts prior to calling this function
///
unsigned long   BIOSintsOff         (void);

/// Turn interrupts on (Enable CPU interrupts) unconditionally.  This 
/// function is deprecated and may be removed in new releases.
/// Code should call BIOSintsRestore() instead with the previous state
/// as returned by BIOSintsOff()
///
void            BIOSintsOn          (void);

/// Restore CPU interrupts on/off state to the state as save in a
/// call to BIOSintsOff()
///
void            BIOSintsRestore     (unsigned long mask /**< mask returned from BIOSintsOff() */);

/// Register an Interrupt vector.  Replaces the interrupt handler function for an interrupt.
///
void            BIOSintRegister     (
                                    Uint32 vector,      ///< [in] interrupt to register handler for
                                    PIRQHANDLER handler ///< [in] Handler function
                                    );

/// Install a specific CPU interrupt vector table into BIOS.  This replaces the table
/// built into BIOS.  This is used by operating systems to take over the interrupt
/// system entirely and should not be used generally.  The format of the table is
/// specific to the CPU and system. 
///
void            BIOSinstallVectors  (unsigned long** table /**< [in] new vector table */);

///@}

/// @name Timers
/// Hardware timers and RTC (Real Time Clock) support
///@{

/// This is an opaque type which represents a single hardware timer instance
/// Timers are used by Allocation with BIOStimerAlloc(), Setting with BIOStimerSet()
/// and releasing with BIOStimerRelease().
///
typedef int     HHWTIMER;

/// Allocate a hardware timer resource. 
/// @returns a handle for the timer, or NULL if no resources available
///
HHWTIMER        BIOStimerAlloc      (void);

/// Release (free) a hardware timer resource.
/// @returns non-0 on error (no such timer handle)
///
int             BIOStimerRelease    (HHWTIMER htimer /**< [in] timer to release, allocated with BIOStimerAlloc() */);

/// Set a timer.  The hardware timer is set to count a specific number of system clock tics
/// and is set to run once (periodic is 0) or coninuouslt (periodic is non-0).
/// Each timer the counter reaches the value ticks, the callback function pirqCallback is
/// called.  The irq in the callback will be the specific timer irq that initiated the
/// callback. If there is a need to access the timer by handle in the callback a function
/// could be added to provide an HHWTIMER given an irq.
/// @returns non-0 on error (no such timer)
///
int             BIOStimerSet        (
                                    HHWTIMER htimer,    ///< [in] handle to timer to set
                                    Uint32   ticks,     ///< [in] timer tick count (system clock based)
                                    int      periodic,  ///< [in] 0 = one-shot, 1 = continuous
                                    PIRQHANDLER pirqCallback    ///< [in] function to call on timer done
                                    );
                                    
/// Clear a timer.  Turns off the timer
/// @returns non-0 on error (no such timer)
///
int             BIOStimerClear      (HHWTIMER htimer /**< [in] handle to timer to clear */);

/// high res timer (counter) support
///

/// Initialize the high resolution timer.  This is normally called
/// by BIOS startup.  Sets the 64 bit high resultion timer counting
/// microseconds.  the sys_frequency parameter specifies the system
/// clock MHz/
///
void            BIOShrtInit         (Uint32 sysFrequency /**< [in] system clock MHz */);

/// @returns the current value of the counter which is counting system
/// clocks ticks
///
Uint64          BIOShrtSysCount     (void);

/// @returns the current microsecond counter
///
Uint64          BIOShrtMicroCount   (void);

/// @returns the current microsecond counter / 1000, in 32 bits
///
Uint32          BIOShrtMilliCount   (void);

/// Delay for a minimum microseconds.  Note that this is blocking,
/// and does not take into consideration threading or interrupts.
/// The only way to insure the delay is accurate is to disable CPU
/// interrupts during the call which is not recommended.
/// To get very accurate timing delays, a coprocessor like flexRISC
/// or Cortex M3 should be used.
///
void            BIOShrtMicroSleep   (Uint32 microseconds /**< [in] sleep count */);

/// @returns non-0 on error, and the current system time in seconds
/// in the tm parameter.  An OS may set the real-time-clock to "Unix"
/// time for example.  Note that this call reads the hardare directly
/// and may delay for hardware access.  The BIOStime() function returns
/// a cached count which is updated on the rtc seconds interrupt and
/// should be the normal function used to retrieve real time seconds.
/// 
///
int             BIOSgetRTCtime      (unsigned long *tm /**< [out] seconds count */);

/// Set the real-time-clock to tm seconds.
/// @returns non-0 on error
///
int             BIOSsetRTCtime      (unsigned long tm /**< [in] time, in seconds */);

/// Get the cached seconds count.  The seconds count is updated on
/// the real-time-clock tick interrupt.  The call is the preferred
/// method of reading the real-time-clock seconds count, as accessing
/// the rtc hardware might be quite slow.  See BIOSgetRTCtime().
/// @returns non-0 on error
///
int             BIOStime            (unsigned long *now /** < [out] time in seconds */);

/// Initialize the Real-Time-Clock.  This sets up rtc interrupt
/// handling and initializes the rtc cache.  This function is 
/// called from BIOSinit() and shouldn't need to be used directly.
/// This function should be called each time the system clock
/// frequency is changes, e.g. for enter/leave powersave
/// @returns non-0 on error
///
int             BIOStimeInit        (Uint32 sysclock /** < [in] system clock frequency Hz */);

/// The RTC h/w provides two 32 bit non-volatile registers.  These
/// are typically used to hold the system ethernet MAC addresses.
///
/// Get the value of an rtc non-volatile register
/// @returns non-0 on error
///
int             BIOSgetNVint        (
                                    int which,      ///< [in] which register (0 or 1)
                                    Uint32 *value   ///< [out] the value stored
                                    );
/// Get the value of an rtc non-volatile register
/// @returns non-0 on error
///
int             BIOSsetNVint        (
                                    int which,      ///< [in] which register (0 or 1)
                                    Uint32 value    ///< [in] the value to store
                                    );

///@}

/// @name Memory
/// Memory size, managment and cache support
///@{

/// @returns the the size, in bytes, of RAM installed in the system
///
unsigned long   BIOSmemRAMsize      (void);

/// @returns the starting address of RAM.  Typically this is 0, but can be
/// non-0 for systems linked to run only in a scratch-pad area for example
///
unsigned long   BIOSmemRAMaddr      (void);

/// @returns the starting address of the RAM region marked uncached.  The
/// BIOSmemMMUinit() function typically splits RAM into two regions, one
/// marked uncached.  The uncached region being the bulk of RAM and used
/// for image memory accessed from DSP and h/w units.
///
unsigned long   BIOSmemUncachedRAMaddr(void);

/// @returns the size of (NOR) ROM installed
///
unsigned long   BIOSmemROMsize(void);

/*unsigned long BIOSmemROMaddr      (void);*/
#define BIOSmemROMaddr() BIOSmemUncachedROMaddr()

/// @returns the starting address of ROM in the system
///
unsigned long   BIOSmemUncachedROMaddr(void);

#if defined(Q4500) || defined(Q5300) || defined(Q5500)
/// @returns the size of ROM
///
unsigned long   BIOSmemUncachedROMsize(void);

/// @returns the starting address of application space in ROM
///
unsigned long   BIOSmemUncachedROMappl(void);
#endif

/// Initialize the Memory Management Unit (MMU) on the CPU
/// For RTOS systems, the MMU is typically mapped 1:1 address
/// wise, and most RAM is marked uncached for use in imaging
/// with the cached region used for system heap and stack.
/// This is called from BIOSinit() and shouldn't be called
/// directly in most cases
///
void            BIOSmemMMUinit      (void);

/// @returns the enable state of the MMU.  If 0, the MMU
/// is disabled, and therefore data-caching.  If 1, the MMU
/// is active and applications may have to take actions to
/// insure data views are coherent between DMA engines and
/// the CPU
///
int             BIOSmemMMUenabled   (void);

/// @return the data cache setting for the address parameter.
/// 0 if the address is marked uncached, or non-0 if it is
/// in the cached region
///
int             BIOSmemIsCached     (void* where /**< [in] address to query cachable for */);

/// Start the MMU unit.  This begins caching and translations.  This
/// should not be called directly, as it doesn't insure the data 
/// cache will reflect the contents of memory properly.  Applications
/// should call BIOSdcacheEnable() to (re)start the MMU/
///
void            BIOSmemMMUstart     (
                                    /// [in] used to indicate cached is shared by multiple CPUs
                                    int SMPmode,
                                    
                                    /// [in] used to set allocate-on-write for the L2 cached
                                    int L2AllocOnWrite
                                    );

/// Stop the MMU unit.  Applications should call BIOSdcacheDisable() instead, to
/// insure memory contents are properly updated
///
void            BIOSmemMMUstop      (void);

/// @returns the size of the initial RAM heap setup by boot code
///
unsigned long   BIOSmemHEAPsize     (void);

/// @returns the starting address of the RAM heap setup by boot code
///
unsigned long   BIOSmemHEAPaddr     (void);

/// Perform a CPU memory synchronization function to insure the
/// CPU has completed any memory operations that are in its 
/// pipeline.  This is done before cache operations for example.
/// This is not needed for most application  use.
///
void            BIOSsync            (void);

/// @returns an uncached RAM address for barrier errata workaround
///
inline unsigned long *BIOSGetAnUncachedRamAddress(void);

/// Enable the instruction cache
///
void            BIOSicacheEnable    (void);

/// Disable the instruction cache
///
void            BIOSicacheDisable   (void);

/// @returns non-0 if the instruction cache is enabled
///
int             BIOSicacheEnabled   (void);

/// Invalidate (mark entries as not-valid) the entire instruction
/// cache.  This should be called after any code content is
/// modified in RAM and before it might be executed by the CPU.
///
void            BIOSicacheInvalidate(void);

/// Enable the data cache
///
void            BIOSdcacheEnable    (void);

/// Disable the data cache
///
void            BIOSdcacheDisable   (void);

/// @returns non-0 if the data cache is enabled
///
int             BIOSdcacheEnabled   (void);

/// Invalidate (mark entries as not-valid) the entire data
/// cache.  This is used before reading memory that a
/// coprocessor or DMA engine had written to.  Most applications
/// should instead use the BIOSdcacheInvalidateRegion() to mark
/// specific regions for invalidation for performance
///
void            BIOSdcacheInvalidate(void);


/// Invalidate (mark entries as not-valid) a specific region
/// of the data cache.  This is used before reading memory that a
/// coprocessor or DMA engine had written to.  Invalidating a
/// region is preferred over using BIOSdcacheInvalidate() for
/// performance reasons
///
void            BIOSdcacheInvalidateRegion(
                                    void *addr,         ///< [in] starting address of region
                                    unsigned long size  ///< [in] region size, in bytes
                                    );
                                    
/// Flush data cache.  This should be done after writing memory
/// from the CPU when the data cache is enabled.  For performance
/// reasons, BIOSdcacheFlushRegion() should be used instead
/// if the region is known
///
void            BIOSdcacheFlush     (void);

/// Flush a specific region of the data cache.
///
void            BIOSdcacheFlushRegion(
                                    void *addr,         ///< [in] starting address of region
                                    unsigned long size  ///< [in] size of region in bytes
                                    );

void            BIOSdcacheInvalidateRegionByIndex(
                                    void *addr,
                                    unsigned long index
                                    );

/// Start Level-1 cache statistics measurement
///
void            BIOSL1cacheStatsStart(void);

/// Stop Level-1 cache statistics measurement
///
void            BIOSL1cacheStatsStop (void);

/// Read Level-1 cache statistics
///
void            BIOSL1cacheStatsRead(
                                    unsigned *dcache_misses,///< [out] dcache miss count
                                    unsigned *icache_misses ///< [out] icache miss count
                                    );
                                
/// Start Level-2 cache.  The level 2 cache memory is shared by
/// the cache and scratch-pad memory
///
void            BIOSL2cacheStart    (void);

/// Stop Level-2 cache
///
void            BIOSL2cacheStop     (void);

/// Invalidate a region of level-2 cache.
///
void            BIOSL2cacheInvalidateRegion(
                                    void *addr,             ///< [in] starting address of region
                                    unsigned long size      ///< [in] size of region in bytes
                                    );

void            BIOSL2cacheInvalidateRegionByIndex(
                                    void *addr,
                                    unsigned long size
                                    );
/// Flush a region of L-2 cache.  For all Quatro SOCs with an L2
/// cache, it is used in write through mode, so this operation
/// is not needed.  If it where, this function should take region
/// parameters
///
void            BIOSL2cacheFlushRegion(void);

/// @returns state of L2 cache.  non-0 if enabled, 0 if not
///
int             BIOSL2cacheEnabled(void);

/// L2 cache statistics structure
///
typedef struct tag_l23stats
{
    unsigned long Cycles;
    unsigned long Hits;
    unsigned long Misses;
    unsigned long Waits;
    unsigned long MultipleHits;
    unsigned long MultipleHitsDetected;
}
L2CacheStats;

/// Start L2 cache statistics gathering
///
void            BIOSL2cacheStatsStart(int CountHits);

/// Stop L2 cache statisitcs gathering
///
void            BIOSL2cacheStatsStop (void);

/// Read Lt cache statistics
///
void            BIOSL2cacheStats    (L2CacheStats *results /**< [out] L2 statistics */);

/// Flash type to use is a run time decision.  These are the types of flash
/// memory that can be found on Quatro board.  Sometimes there are all three
///
typedef enum
{
    eFlashParallel, ///< NOR flash
    eFlashNAND,     ///< NAND flash
    eFlashSPI,      ///< SPI flash
    eFlashEMMC      ///< eMMC
}
FLASHTYPE;

/// Call this function before accessing NAND flash
///
void            NANDFlashInit       (void);
void            NANDFlashInitialInvalidBlock(void);

/// write to SPI flash
/// @returns non-0 on error
//
int             BIOSwriteSerialFlash(
                                    Uint8* destAddress,     ///< [in] destination address
                                    Uint8* sourceAddress,   ///< [in] source address
                                    Uint32 lengthBytes,     ///< [in] source byte count
                                    int verifyImage         ///< [in] non-0 to check written contents vs. source
                                    );
/// read from SPI flash
/// @returns non-0 on error
//
int             BIOSreadSerialFlash(
                                    Uint8* dest,     ///< [in] destination address
                                    Uint8* address,  ///< [in] source address
                                    Uint32 length    ///< [in] source byte count
                                    );
/// write to Parallel NOR flash
/// @returns non-0 on error
//
int             BIOSwriteParallelFlash(
                                    Uint8* destAddress,     ///< [in] destination address
                                    Uint8* sourceAddress,   ///< [in] source address
                                    Uint32 lengthBytes,     ///< [in] source byte count
                                    int verifyImage         ///< [in] non-0 to check written contents vs. source
                                    );
/// read from parallel NOR flash
/// @returns non-0 on error
//
int             BIOSreadParallelFlash(
                                    Uint8* destAddress,
                                    Uint8* sourceAddress,
                                    Uint32 lengthBytes
                                    );
/// write to NAND flash
/// @returns non-0 on error
//
int             BIOSwriteNandFlash (
                                    Uint8* destAddress,     ///< [in] destination address
                                    Uint8* sourceAddress,   ///< [in] source address
                                    Uint32 lengthBytes,     ///< [in] source byte count
                                    int verifyImage         ///< [in] non-0 to check written contents vs. source
                                   );
/// read from NAND flash
/// @returns non-0 on error
//
int             BIOSreadNandFlash(
                                    Uint8* destAddress,     ///< [in] destination address
                                    Uint8* sourceAddress,   ///< [in] source address
                                    Uint32 lengthBytes      ///< [in] source byte count
                                 );
#if defined(Q5500) || defined(Q53XXDVT)
/// write to eMMC flash
/// @returns non-0 on error
//
int             BIOSwriteEMMCFlash (
                                    Uint8* destAddress,     ///< [in] destination address
                                    Uint8* sourceAddress,   ///< [in] source address
                                    Uint32 lengthBytes,     ///< [in] source byte count
                                    int verifyImage         ///< [in] non-0 to check written contents vs. source
                                   );
/// read from eMMC flash
/// @returns non-0 on error
//
int             BIOSreadEMMCFlash (
                                    Uint8* destAddress,     ///< [in] destination address
                                    Uint8* sourceAddress,   ///< [in] source address
                                    Uint32 lengthBytes      ///< [in] source byte count
                                 );
#endif
/// read from flash, selecting which read to call based on which
/// @returns non-0 on error
//
int             BIOSreadFlash(
                                    FLASHTYPE which,        ///< [in] flash type to read
                                    Uint8* dest,     ///< [in] destination address
                                    Uint8* source,   ///< [in] source address
                                    Uint32 length    ///< [in] source byte count
                             );
/// write to flash, selecting which read to call based on which
/// @returns non-0 on error
//
int             BIOSwriteFlash(
                                    FLASHTYPE which,        ///< [in] flash type to write
                                    Uint8* dest,     ///< [in] destination address
                                    Uint8* source,   ///< [in] source address
                                    Uint32 length,   ///< [in] source byte count
                                    int verifyIt     ///< [in] non-0 to check written contents vs. source
                              );

/// Description of a memory mapping of physical to virtual 
/// addresses. The BIOSmmap() function is used to set the
/// virtual base.
///
typedef struct tag_mmap_descriptor
{
    volatile Uint32 *start;  ///< start address (physical) of block
    volatile Uint32 *end;    ///< end   address (physical) of block
    volatile Uint32 *base;   ///< memory mapped (virtual) base address
}
MMAPDESC, *PMMAPDESC;

/// Map an array of MMAPDESC descriptors setting the base member to
/// the virtual address of the start member.
/// @returns non-0 on error
///
int             BIOSmmap            (
                                    int file,               ///< [in] opened driver for mmap operation (0 for non-Linux)
                                    PMMAPDESC maps,         ///< [in/out] array of descriptors to map
                                    int num                 ///< [in] number of descriptors in array to map
                                    );
/// Unmap mappings done by BIOSmmap()
///
void            BIOSmunmap          (
                                    PMMAPDESC pMaps,        ///< [in] array of descriptors to unmap
                                    int nMappings           ///< [in] number of descriptors in array to unmap
                                    );

///@}

/// @name UART
/// Serial port, logging, and debug support
///@{

/// Setup the serial port(s) to use for debugging and set to baud rate
/// This also sets up structures to manage all serial ports on a Quatro SOC 
/// and is typically called by BIOScommonInit() and doesn't need to be called
/// by an application
///
void            BIOSserialInit      (Uint32 defaultbaud /**< baudrate to use */);

/// Get the ordinal for the port BIOS will use for debug output
/// @returns the debug serial port number
///
int             BIOSserialGetDebugPort(void);

/// Setup a particular serial port
/// @returns non-0 on error
///
int             BIOSserialSetupEx   (
                                    int    portnum,         ///< [in] port to setup
                                    Uint8 *txiobuf,         ///< [in] transmit ring buffer, or NULL to keep current buffer
                                    int    txiosize,        ///< [in] size in bytes of tx buffer, if txiobuf non-NULL
                                    Uint8 *rxiobuf,         ///< [in] receive ring buffer, or NULL to keep current buffer
                                    int    rxiosize,        ///< [in] size in bytes of rx buffer if rxiobuf non-NULL
                                    Uint32 baud,            ///< [in] baudrate to use
                                    int    bits,            ///< [in] bit count
                                    int    stopbits,        ///< [in] stop bit count
                                    int    parity,          ///< [in] parity 0=none, 1=odd, 2=even
                                    int    flow,            ///< [in] non-0 enable hw flow control
                                    int    pollmode         ///< [in] non-0 to use polled mode instead of interrupts
                                    );

/// Setup a particular serial port like BIOSserialSetupEx, but using
/// default sizes and built in buffers, for compatibility with older code
/// @returns non-0 on error
///
int             BIOSserialSetup     (
                                    int    portnum,         ///< [in] port to setup
                                    Uint32 baud,            ///< [in] baudrate to use
                                    int    bits,            ///< [in] bit count
                                    int    stopbits,        ///< [in] stop bit count
                                    int    parity,          ///< [in] parity 0=none, 1=odd, 2=even
                                    int    flow             ///< [in] non-0 enable hw flow control
                                    );

/// Receive a byte on serial port port. This reads a character from the ring
/// buffer for the port managed by BIOS
/// @returns read byte, or -1 if no bytes or bad port num
///
int             BIOSserialRX        (int portnum /**< port to read from */);

/// Determine if there are any bytes avaiable to read in a serial port's buffer
/// @returns > 0 if there are, -1 on error (bad port), or 0 if no bytes available
///
int             BIOSserialRXpoll    (int portnum /**< port to poll */);

/// Set a callback function for a serial port rx char interrupt.  This function will
/// be called in an interrupt context (ISR) and can be used, for example, to
/// send a mail message to a polling task waiting for keyboard input.
/// This function is called after the normal port isr is handled so any byte(s)
/// received via the interrupt will already be in the port's rx buffer.
/// Replaces any existing callback if one already set
/// @returns non-0 on error (no such port)
///
int             BIOSserialRXsetISRcallback(
                                    int portnum,                ///< [in] port to install callback on
                                    PIRQHANDLER pirqCallback    ///< [in] function to call on rx char isr
                                    );

/// Transmit a byte on serial port.  The byte is placed in the transmit ring
/// buffer and the UART trasmit is started if not active.
/// @returns non-0 on error (ring buffer full, no such port)
///
int             BIOSserialTX        (
                                    int portnum,            ///< port to send on
                                    char c                  ///< byte to send
                                    );

/// Determine if a byte can be sent to a serial port. 
/// @returns the number of bytes that can be written to the port without dropping any
/// which is 0 or 1 for polled-mode and the room in the tx ring buffer in interrupt mode,
/// or -1 on error (bad port)
///
int             BIOSserialTXpoll    (int portnum /**< port to poll */);

/// Force the flusing of a serial port.  Blocks until all bytes in the tx
/// ring buffer are transmitted (interrupt mode) or until the transmit register
/// is empty (polled mode).  While blocking, this will block all other 
/// code except the interrupt handler.
/// @returns 0 if buffer emptied, non-0 on errors
///
int             BIOSserialTXflush   (int portnum /**< port to flush */);

/// @returns non-0 if the system is configured to NOT print any boot messages
/// or other low-level/early information in order to speed up application start
///
int             BIOSisQuietBoot     (void);

/// Print a character on the debug console.  Typically this function
/// calls BIOSputSerialChar()
///
void            BIOSputChar         (char c /**< char to print */);

/// Get a character from the debug console.  This call is always
/// non-blocking
/// @returns character from console or -1 if none available
///
int             BIOSgetChar         (void);

/// Write an ASCIIZ string to the debug console.  This function
/// also expands LF (0xA) to CR-LF (0xD,0xA)
///
void            BIOSputStr          (const char* s);

/// Put a character on the debug serial port. This limits debug
/// output to the specified debug serial port.  The BIOSputChar()
/// function may redirect to other ports (USB/Network/etc.) 
/// This function is usually non-blocking, and will overrun previous
/// characters if the serial port can't transmit the chars faster than
/// they are added.  If messages are lost, this function can be made blocking
/// by setting the build manifest BIOS_SERIAL_POLL non 0.
/// @returns non-0 on error.
///
int             BIOSserialPutChar   (char c);

/// Get a character from thw debug serial port.  This functions 
/// is always non-blocking
/// @returns char from port, or -1 of none available
///
int             BIOSserialGetChar   (void);

#ifdef _MSC_VER
    // microsoft compiler
#if (_MSC_VER >= 1500) /* 2005 VC introduced variadic macros */
    #define BIOSerror(...) BIOSlogError(__FUNCTION__, __VA_ARGS__)
#else   /* VC++ 6.0 and earlier can't handle it so just use older funcs */
    #define BIOSerror BIOSlog
#endif
#else
    // gcc, etc.
    #define BIOSerror(...) BIOSlogError(__FUNCTION__, ## __VA_ARGS__)
#endif

/// Print a log message on the debug console
///
void            BIOSlog             (const char* format, ...);

/// Print an error message on the debug console.  Code should use
/// the BIOSerror macro instead.
///
void            BIOSlogError        (const char *funcname, const char* format, ...);

/// Print an ASSERT message on the debug console, used but the ASSERT macro
///
void            BIOSassertFail      (const char* exp, const char* file, int line);

/// snprintf replacement, wrapper function for BIOSvsnprintf
///
int             BIOSsnprintf        (char * buffer, int n, const char *format, ...);

///@}

/// @name PLL
/// System clocks
///@{

/// Initilize the system clocks and PLL.  This is called by early initialization
/// and shouldn't be called from applications
///
void            BIOSpllInit(void);

/// @returns the CPU clock frequency for the current CPU
///
Uint32          BIOSgetCPUfrequency (void);

/// @returns the A7 CPU clock frequency
///
Uint32          BIOSgetA7frequency (void);

/// @returns the A15 CPU clock frequency
///
Uint32          BIOSgetA15frequency (void);

/// @returns the System clock frequency.  This is the clock used to for 
/// timers, etc. 
///
Uint32          BIOSgetSYSfrequency (void);

/// @returns the MFP block's clock frequency.
///
Uint32          BIOSgetMFPfrequency (void);

/// @returns the PCIe clock frequency.
///
Uint32          BIOSgetPCIfrequency (void);

/// @returns the Cache Coherent Interconnect clock frequency.
///
Uint32          BIOSgetCCIfrequency (void);

/// @returns the Graphics Processing Unit's clock frequency.
///
Uint32          BIOSgetGPUfrequency (void);

/// @returns the first Image Processing block's clock frequency.
///
Uint32          BIOSgetIPM1frequency (void);

/// @returns the second Image Processing block's clock frequency.
///
Uint32          BIOSgetIPM2frequency (void);

/// @returns the DDR clock
///
Uint32          BIOSgetDDRfrequency (void);

/// @returns the DSP clock
///
Uint32          BIOSgetDSPfrequency (void);

///@}

/// @name Overlay
/// System overlays
///@{

/// Setup the system overlays to their default (as built) values
/// This is called by BIOSinit()
///
void            BIOSoverlayDefaults (void);

///@}

/// @name Firmware
/// Firmware management functions
///@{

/// Load a firmware application image from flash into RAM
/// and start running it
/// @returns non-0 on error
///
int             BIOSfirmwareLoad    (
                                    FLASHTYPE whence,    ///< what flash to read from
                                    Uint8* destAddress,     ///< where to load the image in RAM
                                    Uint8 *sourceAddress    ///< where to get the image in flash
                                    );
/// Poll the USB device port for a firmware image download and if detected,
/// read teh firmware image and burn into flash of type whence
/// If the firmware is executable, run it after burning it
///
void            BIOSfirmwareUpgradePollUSB(FLASHTYPE whence);

/// Copy one region of ram to another, possibly overlapped, and possible containing
/// the code that is doing the copying.
///
void            BIOSRamImageCopy    (
                                    Uint32* dst,            ///< where to put the image
                                    Uint32* src,            ///< where to get the image
                                    Uint32 len              ///< length of image in bytes
                                    );
/// Init the USB device system, here as a convenience.  This should be moved
/// to the USB headers
///
void            USBprolog           (void);

/// De-init the USB device system
///
void            USBepilog           (void);

///@}

/// @name Misc
/// Miscellaneous BIOS functions
///@{
void            BIOSsataInit        (int use_external_clock);

void            BIOSpciemem         (unsigned int *base, unsigned int *size);
void            BIOSprobePCIE       (void);

unsigned        ARMsvc              (int service, unsigned parm);
unsigned        ROMsvc              (int service, ...);

/// @returns non-0 if the system booted from SPI flash
//
int             BIOSisSPIFlash      (void);

/// @returns non-0 if the system booted from XSPI flash
//
int             BIOSisXSPIFlash     (void);

/// @returns non-0 if the system booted from NAND flash
//
int             BIOSisNANDFlash     (void);
#if defined(Q5500)
/// @returns non-0 if the system booted from eMMC flash
//
int             BIOSisEMMCFlash      (void);
#endif
/// Write a value to the LEDs on a Quatro board.
//
void            BIOSwriteLEDs       (unsigned char value /**< [in] value to write */);

/// @returns the ordinal number of the CPU the code is running on.
//
int             BIOSgetCPU          (void);

/// @returns non-0 if the CPU calling the function is a Cortex A7.
///
int             BIOSisCPUtypeA7     (void);

/// @returns non-0 if the CPU calling the function is a Cortex A15.
///
int             BIOSisCPUtypeA15     (void);

/// @returns non-0 if a second CPU of multi-core SOCs is running.
///
int             BIOSisCPU1running   (void);

/// Get the system configured MAC address.  The MAC is typically
/// stored in one of the non-volatile registers in the Real-time-clock
/// and read with BIOSgetNVint()
/// @returns non-0 on error
///
int             BIOSgetMACaddress   (Uint8 *macaddr /**< [out] configured MAC address */);

/// Save a MAC address in non-volatile storage
/// @returns non-0 on error
///
int             BIOSsaveMACaddress  (Uint8 *macaddr /**< [in] MAC address to set */);

/// Set a MAC address into the ethernet LAN controller.
/// @returns non-0 on error
///
int             BIOSsetMACaddress   (Uint8 *macaddr); /* set in controller */

/// Create a psuedo-randim MAC address
///
void            BIOScreateMACaddress(
                                    /// [out] the generated MAX
                                    Uint8 *macaddr,
                                    /// [in] non-0 to indicate this is being called because of
                                    /// some error in the existing MAC address
                                    int err
                                    );

// CP15 control register access
unsigned int    BIOSGetCp15ControlReg(void);
void            BIOSSetCp15ControlReg(unsigned int);

unsigned int    BIOSGetCp15L2CTLReg (void);
void            BIOSSetCp15L2CTLReg (unsigned int value);

// Performance counter access - no-ops on some processors (e.g. ARM9)
unsigned int    BIOSGetCp15PMNCReg  (void);
void            BIOSSetCp15PMNCReg  (unsigned int value);
unsigned int    BIOSGetCp15CCReg    (void);
void            BIOSSetCp15CCReg    (unsigned int value);
unsigned int    BIOSGetCp15PMNCReg  (void);
void            BIOSSetCp15PMN0Reg  (unsigned int value);
unsigned int    BIOSGetCp15PMN0Reg  (void);
void            BIOSSetCp15PMN1Reg  (unsigned int value);
unsigned int    BIOSGetCp15PMN1Reg  (void);
void            BIOSSetCp15PMN2Reg  (unsigned int value);
unsigned int    BIOSGetCp15PMN2Reg  (void);
void            BIOSSetCp15PMN3Reg  (unsigned int value);
unsigned int    BIOSGetCp15PMN3Reg  (void);

// Modem Data Pump
void            BIOSEnableMdpClocks (void);
void            BIOSDisableMdpClocks(void);

///@}

// ARMv7 Counter Virtual Offset
void               BIOSSetCp15CntVOff  (unsigned long long value);
unsigned long long BIOSGetCp15CntVOff  (void);


#endif
