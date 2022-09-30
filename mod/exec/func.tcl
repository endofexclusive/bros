# SPDX-License-Identifier: GPL-2.0
# Copyright 2021-2022 Martin Åberg

mod_base        SysBase
mod_basetype    ExecBase
mod_libname     exec.library
mod_prefix      exec
mod_header      stdarg.h
mod_header      stddef.h

mod_struct      Device
mod_struct      ExecBase
mod_struct      IORequest
mod_struct      IntList
mod_struct      IntLock
mod_struct      Interrupt
mod_struct      Library
mod_struct      List
mod_struct      MemHeader
mod_struct      MemList
mod_struct      MemRequest
mod_struct      Message
mod_struct      MsgPort
mod_struct      Mutex
mod_struct      Node
mod_struct      Resident
mod_struct      ResidentAuto
mod_struct      ResidentInfo
mod_struct      Segment
mod_struct      Task

# NOTE: Add new functions to the TOP of this file.

func_begin      CreateTask
func_return     "struct Task *"
func_param      "char *" name
func_param      "int" priority
func_param      "void (*init)(struct ExecBase *lib)" init isfunc
func_param      "void *" user
func_param      "struct List *" cleanup
func_param      "size_t" stacksize
func_short      "create and add task"
func_long      "
Allocate, initialize and add a task to the system.

The new task's cleanlist will be initialized and any memory
allocated during the creation process will be added to it.  In
addition, if cleanup is not a null pointer, then all nodes on
the cleanup list will be moved to task->cleanlist of the created
task; it is processed automatically at task removal.

The name buffer is not copied and the pointer is used as is.
Priority is the task priority and must be greater than
TASK_PRI_IDLE.

When the task starts, it will execute the init function and the
task is automatically removed with RemTask() when init returns.
The user argument is installed in task->user and is not
dereferenced by CreateTask().

CreateTask() allocates a stack buffer of size stacksize for the
created task and arranges for it to be deallocated on task
removal.

A task created with CreateTask() can be deleted by calling
RemTask().
"
func_result     "pointer to the added task"
func_see        "AddTask(), RemTask()"

func_begin      CreateMsgPort
func_return     "struct MsgPort *"
func_short      "create a message port"
func_long       "
Allocate memory for a message port, allocate a task signal
number, initialize the port, and associate the signal with the
port.

A task signals is a task property, so the created message port
can only be used by the task calling this function.

Use DeleteMsgPort() to deallocate the port and signal.
"
func_result     "
The new message port is returned. If memory or task signal could
not be allocated, then NULL is returned.
"
func_see        "DeleteMsgPort()"

func_begin      DeleteMsgPort
func_return     "void"
func_param      "struct MsgPort *" port
func_short      "delete a message port"
func_long       "
Delete a message port created with CreateMsgPort(). It will
deallocate the signal and the port structure.

The function returns immediately if port is NULL.
"
func_see        "CreateMsgPort()"

func_begin      CreateIORequest
func_return     "struct IORequest *"
func_param      "struct MsgPort *" port
func_param      "size_t" size
func_short      "create an IORequest"
func_long       "
Allocate and initialize memory for an IORequest, then associate
the port with the IORequest.  A memory block of size bytes is
allocated for the IORequest, allowing for extended requests. The
size argument must be at least the size of a struct IORequest.

Use DeleteIORequest() to deallocate the IORequest.

This function returns NULL immediately if port is NULL.
"
func_result     "
The new IORequest structure is returned. If port is NULL, or if
memory could not be allocated, then NULL is returned.
"
func_see        "CreateMsgPort(), DeleteMsgPort(), DeleteIORequest()"

func_begin      DeleteIORequest
func_return     "void"
func_param      "struct IORequest *" ior
func_short      "delete an IORequst"
func_long       "
Delete an IORequest created with CreateIORequest(). It will
deallocate ior structure but not the message port.

The function returns immediately if ior is NULL.
"
func_see        "CreateIORequest(), CreateMsgPort(),
DeleteMsgPort()"

func_begin      AddDevice
func_return     "void"
func_param      "struct Device *" device
func_short      "publish a device"
func_see        "RemDevice()"

func_begin      RemDevice
func_return     "void"
func_param      "struct Device *" device
func_short      "unpublish a device"
func_see        "AddDevice()"

func_begin      OpenDevice
func_return     "int"
func_param      "const char *" name
func_param      "int" unitnum
func_param      "struct IORequest *" ior
func_short      "open a device"
func_see        "CloseDevice()"

func_begin      CloseDevice
func_return     "void"
func_param      "struct IORequest *" ior
func_short      "close a device"
func_long       "
Close a device opened with OpenDevice.
"
func_see        "OpenDevice()"

func_begin      DoIO
func_return     "int"
func_param      "struct IORequest *" ior
func_short      "perform synchronous I/O"
func_long       "
Send an I/O request to a device and wait for completion.  It is
guaranteed that ior will not be on the IORequest port when this
function returns.
"
func_result     "ior.error"
func_see        "SendIO()"

func_begin      SendIO
func_return     "void"
func_param      "struct IORequest *" ior
func_short      "perform asynchronous I/O"
func_long       "
Send an I/O request to a device.

This function will not wait for the request to complete. You can
wait for completion with WaitIO(), which will also remove the
message from the IORequest port.

It is also possible to wait for completion with Wait(),
WaitMsg() or WaitPort().

Progress of a request can be checked with CheckIO() or aborted
with AbortIO().
"
func_see        "AbortIO(), CheckIO(), DoIO(), Wait(), WaitIO()"

func_begin      CheckIO
func_return     "int"
func_param      "struct IORequest *" ior
func_short      "check if an I/O request has completed"
func_long       "
Determine if an I/O request has completed.
This function will not Wait().
"
func_result     "1 if the I/O request has completed, else 0"
func_see        "AbortIO(), SendIO()"

func_begin      WaitIO
func_return     "int"
func_param      "struct IORequest *" ior
func_short      "wait for an I/O request to complete"
func_long      "
This function will wait for ior to complete. If it was on the
ior message port, then it will be removed from the port.
"
func_result     "ior.error"
func_see        "SendIO()"

func_begin      AbortIO
func_return     "void"
func_param      "struct IORequest *" ior
func_short      "abort an I/O request"
func_long       "
Attempt to abort an in-progress I/O request.

It can be used after ior has been scheduled with SendIO() but
before removed with WaitIO(ior):
- SendIO(ior)
- CheckIO(ior) (optional)
- AbortIO(ior)
- CheckIO(ior) (optional)
- WaitIO(ior)
"
func_see        "CheckIO(), SendIO(), WaitIO()"

func_begin      MakeLibrary
func_return     "struct Library *"
func_param      "const struct ResidentInfo *" rinfo
func_param      "const struct ResidentAuto *" rauto
func_param      "struct Segment *" segment
func_short      "construct a library"
func_long       "
This function allocates storage for a Library and initializes it
according to the parameters rinfo and rauto.

MakeFunctions will be called to initialize the library function
entry points from rauto.optable. Rauto.opsize should be the
nuber of bytes in rauto.optable. The rinfo fields will be
installed in the Library node. Finally, rauto.f() will be called
to do library specific initialization.

This function can be used to create Library and Device nodes.
To publish the node to the system, use AddLibrary() or
AddDevice().

Segment is passed to rauto->f() and is not interpreted by
MakeLibrary().
"
func_result     "Pointer to the library or NULL on failure"
func_see        "AddDevice(), AddLibrary(), MakeFunctions()"

func_begin      SetFunction
func_return     "int"
func_param      "struct Library *" library
func_param      "int" negoffset
func_param      "void (*newfunc)(void)" newfunc isfunc
func_param      "void (**oldfunc)(void)" oldfunc isfunc
func_short      "set a library function"
func_long       "
Library offsets are typically defined in <libname/offsets.h>

Library is the library for which the function newfunction,
identified by negoffset, is set. If the oldfunc pointer is not
NULL, then a pointer to the previous function written to
oldfunc.
"
func_result     "0"
func_see        "MakeFunctions(), MakeLibrary()"

func_begin      MakeFunctions
func_return     "void *"
func_param      "struct Library *" library
func_param      "const void *" optable
func_param      "int" opsize
func_short      "initialize library entry points"
func_long       "
MakeFunctions() initializes the library function entry points
from rauto.optable and opsize.

The library argument is the target library and optable is the
table of function pointers for the library.  Opsize indicates
the number of bytes in optable.
"
func_result     "
pointer to where the optable was installed or NULL on failure
"

func_begin      AddLibrary
func_return     "void"
func_param      "struct Library *" library
func_short      "publish a library"
func_see        "RemLibrary()"

func_begin      RemLibrary
func_return     "void"
func_param      "struct Library *" library
func_short      "unpublish a library"
func_see        "AddLibrary()"

func_begin      OpenLibrary
func_return     "struct Library *"
func_param      "const char *" name
func_param      "int" version
func_short      "open a library"
func_see        "CloseLibrary()"

func_begin      CloseLibrary
func_return     "void"
func_param      "struct Library *" library
func_short      "close a library"
func_long       "
Close a library opened with OpenLibrary.
"
func_see        "OpenLibrary()"

func_begin      AddIntServer
func_return     "void"
func_param      "struct Interrupt *" interrupt
func_param      "int" intnum
func_short      "register interrupt handler"
func_long       "
Add interrupt to the chain of interrupt handlers for interrupt
intnum. Interrupt intnum will be enabled (unmasked).

The Interrupt.code() field must be initialized before calling
this function. The Interrupt.code() entry will be called with
Interrupt.data as argument.

Note that the definition of intnum is port specific.
"
func_see        "RemIntServer()"

func_begin      RemIntServer
func_return     "void"
func_param      "struct Interrupt *" interrupt
func_param      "int" intnum
func_short      "unregister interrupt handler"
func_long       "
Remove interrupt from the chain of interrupt handlers for
interrupt intnum. Interrupt intnum will be disabled (masked)
when the last handler is unregistered.
"
func_see        "AddIntServer()"

func_begin      FindResident
func_return     "struct Resident *"
func_param      "const char *" name
func_short      "find a resident module by name"
func_long       "
Search the system lists for a resident module with name name.
"
func_result     "the Resident structure, or NULL if not found"
func_see        "InitResident()"

func_begin      InitResident
func_return     "void *"
func_param      "const struct Resident *" resident
func_param      "struct Segment *" segment
func_short      "initialize a resident module"
func_see        "FindResident(), InitCode()"

func_begin      InitCode
func_return     "void"
func_param      "int" level
func_short      "initialize all resident modules by level"
func_long      "
Perform InitResident() on all modules with level level.

Levels are defined in <exec/resident.h>.
"
func_see        "InitResident()"

func_begin      PutMsg
func_return     "void"
func_param      "struct MsgPort *" port
func_param      "struct Message *" message
func_env        {isr}
func_short      "send a message"
func_long       "
Send a message to a message port. This performs asynchronous
message passing. It will signal the port task with the port
signal number.
"
func_see        "GetMsg(), ReplyMsg(), WaitMsg(), WaitPort()"

func_begin      GetMsg
func_return     "struct Message *"
func_param      "struct MsgPort *" port
func_env        {isr}
func_short      "remove a message from a message port"
func_long       "
Remove and message from a message port.

This function will never wait. It will RemHead() a Message from
the message port.
"
func_result     "the removed message or NULL if none"
func_see        "PutMsg(), ReplyMsg(), WaitMsg(), WaitPort()"

func_begin      ReplyMsg
func_return     "int"
func_param      "struct Message *" message
func_env        {isr}
func_short      "reply to a message"
func_long       "
ReplyMsg() replies a message by sending it its reply port.  The
message shall not be accessed after it has been replied.  The
message node type is set to NT_REPLYMSG before it is actually
sent.

If the reply port field of the message is NULL, then the message
is marked as NT_FREEMSG and no message is sent.
"
func_result     "1 if the message sent, else 0"
func_see        "GetMsg(), PutMsg(), WaitMsg(), WaitPort()"

func_begin      WaitPort
func_return     "struct Message *"
func_param      "struct MsgPort *" port
func_short      "wait for message port to be non-empty"
func_long       "
Wait for the port to contain at least one message.  If there is
a message on the port, then the function returns immediately.
Else it will Wait() on the ports signal bit until the port has
at least one message.

After this function returns, GetMsg() is guaranteed to return
non-NULL. This function never removes a message.
"
func_result     "pointer to the head of the port message queue"
func_see        "GetMsg(), PutMsg(), Wait(), WaitMsg()"

func_begin      WaitMsg
func_return     "void"
func_param      "struct Message *" message
func_short      "wait for message reply"
func_long       "
If the message has already been replied, then remove the message
from the reply port and return immediately. If the message has
not been replied yet then Wait() for the message to arrive on its
reply port and Remove() it.
"
func_see        "GetMsg(), ReplyMsg(), Wait(), WaitPort()"

func_begin      Alert
func_return     "void"
func_param      "unsigned long" why
func_env        {isr}
func_short      "announce a system alert"
func_long       "
Announce a system alert. The wait o announce the alert is
unspecified, but will by default put out a message describing
the alert using RawPutChar().

See <exec/alerts.h> for the why argument.

This function will not return if the argument contains the mask
AT_DeadEnd.
"

func_begin      AddTask
func_return     "struct Task *"
func_param      "struct Task *" task
func_short      "add task to a scheduler"
func_long       "
The caller must initialize the following task fields before
call:
- node.name
- node.type (NT_TASK or NT_PROCESS)
- node.pri (must be greater than TASK_PRI_IDLE)
- init
- splower
- spupper
- cleanlist (shall be a proper list, so at least do NewList())

The task fields splower and spupper should define a stack buffer
with size of at least ExecBase.minstack. AddTask() may change
the task fields splower and spupper. It will not touch the user
field.

A more user friendly way to create tasks is to use the
CreateTask() function.
"
func_result     "Pointer to the task on success, else NULL"
func_see        "CreateTask(), RemTask(), SetTaskPri()"

func_begin      RemTask
func_return     "void"
func_short      "remove a task"
func_long       "
This function requests the calling task to be removed from the
scheduler at an appropriate time. Any operations in the task
cleanlist will be processed when the task is no longer
executing. There is no resource tracking for tasks, so make sure
to deallocate any resources before calling this function.

RemTask() never returns.
"
func_see        "AddTask(), CreateTask()"

func_begin      SetTaskPri
func_return     "int"
func_param      "struct Task *" task
func_param      "int" priority
func_short      "set and get task priority"
func_long       "
Set a new task priority. This may trig a task reschedule.

The new priority must be greater than TASK_PRI_IDLE
"
func_result     "Previous priority"
func_see        "AddTask(), CreateTask()"

func_begin      FindTask
func_return     "struct Task *"
func_short      "get a pointer to the current task"
func_long       "
Return a pointer to the current task. It is not allowed to call
FindTask() from interrupt context.
"
func_result     "The current task"

func_begin      AllocSignal
func_return     "int"
func_param      "int" signum
func_short      "allocate a task signal"
func_long       "
Allocate a signal for the current task from its signal number
pool. Each task can have at least 16 signals.

Signal numbers are task private. You can not share your signal
numbers with another tasks. The other task must allocate its own
signals.

Signum is the signal number to allocate. If signum is -1 then
the implementation will select a free signal if any.

A signal can be deallocated with FreeSignal().
"
func_result     "The signal number or -1 if allocation failed"
func_see        "FreeSignal(), Signal(), Wait()"

func_begin      FreeSignal
func_return     "void"
func_param      "int" signum
func_short      "deallocate a task signal"
func_long       "
Give a signal number allocated with AllocSignal() back to the
task signal number pool.
"
func_see        "AllocSignal()"

func_begin      ClearSignal
func_return     "unsigned int"
func_param      "unsigned int" sigmask
func_short      "clear a set of signals"
func_long       "
Remove signal numbers in sigmask from the set of received
signals in calling task. The task can still receive the signals.
All old received signal mask is returned. By giving a 0
argument, set set of all received signals are returned.
"
func_result     "Full set of old received signals"
func_see        "AllocSignal(), Signal(), Wait()"

func_begin      Signal
func_return     "void"
func_param      "struct Task *" task
func_param      "unsigned int" sigmask
func_env        {isr}
func_short      "signal a task"
func_long       "
Signal() posts a set (mask) of signals to a task. If the target
task is currently Wait()ing for any of the signals in sigmask,
then it will become ready. If the task is currently not waiting
for any of the signals, then they will be recorded in the task
and can be Wait()ed on later.
"
func_see        "ClearSignal(), Wait()"

func_begin      Wait
func_return     "unsigned int"
func_param      "unsigned int" sigmask
func_short      "wait for signals"

func_begin      RawDoFmt
func_return     "void"
func_param      "void (*put)(void *arg, int c)" put isfunc
func_param      "void *" arg
func_param      "const char *" fmt
func_param      "va_list" ap
func_env        {isr}
func_short      "convert data to characters"
func_long       "
This function calls the put() function with characters converted
according format string and an argument list. The format
language is a subset of the C standard library string formatting
functions.

The arg parameter is given as argument to put() on all
invocations. Put() is supplied by the user and could for example
write the characters to a file or a string buffer.

Note that the RawDoFmt function may be patched to allow
additional conversions but it can be expected that the below
conversions are always be available.

Each conversion specification begins with the character % and
ends with a conversion character. Between the % and the
conversion character there may be, in order:

- Flags:
  - '-': Left adjustment of the converted argument.
  - '+': Always print the number with a sign.
  - ' ': If first character is not a sign then prefix with
         space.
  - '0': Pad number with leading zeros.
- Minimum field width. The field will be wider if necessary. If
  the converted argument has fewer characters than the field
  width, it will be padded on the left, or right if left
  adjustement has been requested.
- A length modifier:
  - 'l', specifying that a following 'd', 'o', 'u', 'x', or 'X'
    conversion specifier applies to a long int or unsigned long
    int argument.

 Conversion    Argument
 character     type            Formatted as
------------------------------------------------------------
  d            int             signed decimal
  o            unsigned int    unsigned octal
  u            unsigned int    unsigned decimal
  x, X         unsigned int    unsigned hexadecimal
  c            int             single character
  s            char *          string
  p            void *          pointer
  %            -               %. No argument is converted.
------------------------------------------------------------
"

func_begin      MemSet
func_return     "void *"
func_param      "void *" dst
func_param      "int" c
func_param      "size_t" len
func_short      "write a value to a byte string"
func_long       "
Write len bytes of value c to memory pointed to by dst.

This function has the same interface as the C standard library
function memset().
"
func_result     "The original value of dst"
func_see        "MemSet()"

func_begin      MemMove
func_return     "void *"
func_param      "void *" dst
func_param      "const void *" src
func_param      "size_t" len
func_short      "copy bytes"
func_long       "
Copy len bytes from src to dst. The src and dst buffers may
overlap.

This function has the same interface as the C standard library
function memmove().
"
func_result     "The original value of dst"
func_see        "MemMove()"

func_begin      InitMemHeader
func_return     "struct MemHeader *"
func_param      "void *" base
func_param      "size_t" numbytes
func_short      "initialize memory header"
func_long       "
Initialize a private memory header preparing it to be used with
Allocate(), Deallocate() or AddMemHeader(). The input parameters
base and numbytes can have any alignment and InitMemHeader()
will do alignment calculations as needed.

Base is a pointer to the memory for the MemHeader and its data
and numbytes is the number of bytes available at base.

Use AddMemHeader() to make the memory header public.
"
func_result     "
An initialized MemHeader on succss, or NULL if numbytes is too
small
"
func_see        "AddMemHeader(), Allocate(), Deallocate(), RemMemHeader()"

func_begin      Allocate
func_return     "void *"
func_param      "struct MemHeader *" mh
func_param      "size_t" numbytes
func_short      "allocate from a memory header"
func_long       "
Allocate memory from a (private) MemHeader. The allocated memory
will have the same alignment as the MemAlign type. The allocated
memory should be deallocated with Deallocate().

Numbytes is the number of bytes to allocate from the MemHeader
mh.  Numbytes may be 0, in which case the function behaves as if
numbytes were 1.
"
func_result     "pointer to allocated memory, or NULL on failure"
func_see        "AllocMem(), AllocVec(), Deallocate()"

func_begin      Deallocate
func_return     "void"
func_param      "struct MemHeader *" mh
func_param      "void *" ptr
func_param      "size_t" numbytes
func_short      "deallocate to a memory header"
func_long       "
Deallocate to a memory header. The arguments mh, ptr and
numbytes should be the same as in Allocate().

ptr shall not be a null pointer.
"
func_see        "Allocate(), FreeMem(), FreeVec()"

func_begin      AddMemHeader
func_return     "void"
func_param      "struct MemHeader *" mh
func_param      "const char *" name
func_param      "int" attr
func_param      "int" pri
func_short      "publish a memory header"
func_long       "
This function publishes a memory header and makes it available
to AllocMem() and AllocVec(). There is no specific meaning to
name but it is available to anyone probing the memory lists.

AllocMem() and AllocVec will search the public memory headers in
priority order for a MemHeader with matching attributes (attr)
and which can satisfy the requested size.
"
func_see        "AllocMem(), AllocVec(), FreeMem(), FreeVec()"

func_begin      AllocMem
func_return     "void *"
func_param      "size_t" numbytes
func_param      "int" attr
func_short      "allocate memory"
func_long       "
Allocate memory from a published memory header.  The allocated
memory will have the same alignment as the MemAlign type. The
allocated memory should be deallocated with FreeMem().

Numbytes it the number of bytes to allocate. This may be 0, in
which case the function behaves as if numbytes were 1.

Attributes are a mask of:
  * requirements:
    * MEMF_DMA:   memory which you can do DMA with
    * MEMF_EXEC:  memory which can hold CPU instructions
    * MEMF_ANY:   no specific requirements
  * options:
    * MEMF_CLEAR: initialize the memory to zero.
"
func_result     "pointer to allocated memory, or NULL on failure"
func_see        "AllocVec(), FreeMem(), FreeVec()"

func_begin      FreeMem
func_return     "void"
func_param      "void *" ptr
func_param      "size_t" numbytes
func_short      "deallocate memory"
func_long       "
Deallocate back to the published memory header. The argument
numbytes shall be the same as given to AllocMem() when the
memory was allocated.

If ptr is a null pointer, no action will occur.
"
func_see        "AllocMem(), AllocVec(), FreeVec()"

func_begin      AllocVec
func_return     "void *"
func_param      "size_t" numbytes
func_param      "int" attr
func_short      "allocate memory and remember size"
func_long       "
Same as AllocMem() but keeps track of number of bytes allocated.
Memory returned by this function must be deallocated with
FreeVec().

Numbytes is the number of bytes to allocate. It may be 0, in
which case the function behaves as if numbytes were 1.
Attributes work the same as for AllocMem()
"
func_result     "pointer to allocated memory, or NULL on failure"
func_see        "AllocMem(), FreeMem(), FreeVec()"

func_begin      FreeVec
func_return     "void"
func_param      "void *" ptr
func_short      "deallocate memory with remembered size"
func_long       "
This function is used to deallocate memory allocated with
AllocVec().

If ptr is a null pointer, no action will occur.
"
func_see        "AllocMem(), AllocVec(), FreeMem()"

func_begin      AvailMem
func_return     "size_t"
func_param      "int" attr
func_short      "query published memory headers"
func_long       "
Query published memory headers.  The function scans the
published memory headers matching attr and sums the number of
bytes of free storage on these.  Only memory headers with
attributes matching attr will be queried.
"
func_result     "
number of byte matching attr at some point in time
"
func_see        "AllocMem()"

func_begin      AllocEntry
func_return     "struct MemList *"
func_param      "const struct MemRequest *" req
func_param      "int" numentries
func_short      "allocate memory blocks"
func_long       "
Allocate multiple blocks of memory from published memory
header(s).

Req is an array of MemRequest which each describes a block of
memory to allocate. Each array element contains attributes and
size for the block, which is the same as the arguments to
AllocMem(). Numentries indicates the number of entries in req.
"
func_result     "description of allocated memory or NULL"
func_see        "AllocMem(), FreeEntry()"

func_begin      FreeEntry
func_return     "void"
func_param      "struct MemList *" ml
func_short      "deallocate memory blocks"
func_long       "
Deallocate all memory blocks allocated by AllocEntry().  The
MemList itself will also be deallocated.

Ml is the MemList returned by AllocEntry(). It may be NULL, in
which case no action will occur.
"
func_see        "AllocEntry()"

func_begin      InitMutex
func_return     "void"
func_param      "struct Mutex *" mutex
func_short      "initialize an Mutex"
func_long       "
A Mutex must be initialized before first use. This function
wlll do the initialization.
"
func_see        "ObtainMutex(), ReleaseMutex()"

func_begin      ObtainMutex
func_return     "void"
func_param      "struct Mutex *" mutex
func_short      "obtain an IntLock"
func_long       "
Gain exclusive access to an invariant protected by a Mutex.
When this function returns, the calling context has exclusive
access to the state protected by the mutex.  An obtained Mutex
must be released with a call to ReleaseMutex().

If Mutex is available, then this function will return
immediately; if the Mutex can not be obtained immediately, then
the task will Wait() on SIGF_SINGLE until it can be obtained
before it returns. Each Mutex maintains a list of waiting tasks
and multiple tasks requesting the same Mutex are served in FIFO
order. Each Mutex also maintains a nest count for the current
owner and it is permitted for the same task to obtain the same
Mutex multiple times without releasing it first.

The Mutex must be release with ReleaseMutex() the same number of
times as it is obtained. It is permitted for a task to hold
different Mutex obtained at the same time.

This function will not disable interrupt processing or task
switches. A Mutex can not be obtained in interrupt context or
while holding an IntLock.
"
func_see        "InitIntLock(), ObtainMutex(), ReleaseIntLock()"

func_begin      ReleaseMutex
func_return     "void"
func_param      "struct Mutex *" mutex
func_short      "release a Mutex"
func_long       "
This funcion releases a Mutex which was obtained with
ObtainMutex(). If this was the final release and if there are
other tasks waiting for the mutex, then the first task will be
unblocked.
"
func_see        "InitMutex(), ObtainMutex(), ReleaseIntLock()"

func_begin      InitIntLock
func_return     "void"
func_param      "struct IntLock *" lock
func_short      "initialize an IntLock"
func_long       "
An IntLock must be initialized before first use. This function
wlll do the initialization.
"
func_see        "ObtainIntLock(), ReleaseIntLock()"

func_begin      ObtainIntLock
func_return     "void"
func_param      "struct IntLock *" lock
func_env        {isr}
func_short      "obtain an IntLock"
func_long       "
Gain exclusive access to an invariant protected by an IntLock.
When this function returns, the calling context has exclusive
access to whatever is protected by lock.

This function will disable interrupt processing on the local
CPU, which will prevent local task switches and interrupt
handlers to occur. An obtained IntLock must be released, by the
same CPU, and with the same lock argument, with a call to
ReleaseIntLock().

It is not permitted to obtain the same lock multiple times
without releasing it first. That is, a single lock does not
nest. It is however permitted to have different IntLocks
obtained at the same time, but watch out for deadlocks if you
work with multiple locks.

ObtainIntLock() may be called from task or interrupt context.
Only a limited number of system function may be called while the
lock is held. The list functions are safe for example.

IntLocks are typically implemented as a spin lock on SMP systems
and are useful for protecting invariants which must be accessed
in interrupt context. For managing data invariants whcih are
accessed in task context only, the Wait() based Mutex interface
may be a better choice.
"
func_see        "InitIntLock(), ObtainMutex(), ReleaseIntLock()"

func_begin      ReleaseIntLock
func_return     "void"
func_param      "struct IntLock *" lock
func_env        {isr}
func_short      "release an IntLock"
func_long       "
This funcion releases an IntLock which was obtained with
ObtainIntLock(). It allows other contextes to obtain the lock.
Please see the description of ObtainIntLock() for
considerations.
"
func_see        "InitIntLock(), ObtainIntLock(), ReleaseMutex()"

func_begin      SyncInstructions
func_return     "void"
func_param      "const void *" begin
func_param      "size_t" size
func_short      "synchronize caches after code change"
func_long      {
Synchronize caches after code change. You should call this
function after writing instructions to memory, for example as a
result of loading a program, but before using the instruction in
the current task or announcing the instructions to other tasks.
(Your task could be switched to another CPU at any time.)

The model for this function is that it broadcasts a request to
invalidate the specified memory region in the local instruction
caches of all CPUs, and that it returns only after all such
invalidations have been performed.

Begin is a pointer to the beginning of the code region to
synchronize and size is the size of the region.
}

func_begin      NewList
func_return     "void"
func_param      "struct List *" list
func_env        {isr}
func_short      "initialize a list"
func_long       "
A list must be initialized before use. Note that merely setting
the list members to 0 will not initialize it.
"
func_see        "All list functions"

func_begin      GetHead
func_return     "struct Node *"
func_param      "const struct List *" list
func_env        {isr}
func_short      "get the head node of a list"
func_long       "
This function returns the list head node. The head node is not
removed from the list.
"
func_result     "Head node or NULL if list is empty"
func_see        "GetTail(), RemHead()"

func_begin      GetTail
func_return     "struct Node *"
func_param      "const struct List *" list
func_env        {isr}
func_short      "get a pointer to the tail node of a list"
func_long       "
This function returns the list tail node. The tail node is not
removed from the list.
"
func_result     "Tail node or NULL if list is empty"
func_see        "GetHead(), RemTail()"

func_begin      Insert
func_return     "void"
func_param      "struct List *" list
func_param      "struct Node *" node
func_param      "struct Node *" pred
func_env        {isr}
func_short      "insert a node into a list"
func_long       "
Insert node into list after the pred node. The insert node will
be become the successor of pred; pred will become the
predecessor of the insert node.  If pred is NULL, then node will
be added to the head of list.

In other words, node is the node to insert and pred is the node
before the insert node.
"
func_result     "Tail node or NULL if list is empty"
func_see        "AddHead(), AddTail(), Enqueue()"

func_begin      Enqueue
func_return     "void"
func_param      "struct List *" list
func_param      "struct Node *" node
func_env        {isr}
func_short      "insert a node on list in priority order"
func_long       "
Insert node into list in front of the first list node with lower
priority. Node priority is given by the node pri field.

If all nodes were ordered in priority order before the call,
then the same property holds after the call.  This function can
be used to build a priority queue.
"
func_see        "AddHead(), AddTail()"

func_begin      Remove
func_return     "void"
func_param      "struct Node *" node
func_env        {isr}
func_short      "remove a node from a list"
func_long       "
Remove the given node from the list it is currently on. It is
only allowed to remove a node which is currently on a list: this
is not checked.

Note that you don't need to specify which list the node is on.
"
func_see        "AddHead(), AddTail(), Enqueue(), RemHead(), RemTail()"

func_begin      AddHead
func_return     "void"
func_param      "struct List *" list
func_param      "struct Node *" node
func_env        {isr}
func_short      "insert a node at the head of a list"
func_long       "
Add node to the head of list.
"
func_see        "AddTail(), RemHead(), RemTail()"

func_begin      AddTail
func_return     "void"
func_param      "struct List *" list
func_param      "struct Node *" node
func_env        {isr}
func_short      "append a node to the tail of a list"
func_long       "
Add node to the tail of list.
"
func_see        "AddHead(), RemHead(), RemTail()"

func_begin      RemHead
func_return     "struct Node *"
func_param      "struct List *" list
func_env        {isr}
func_short      "remove the head node from a list"
func_long       "
Remove the head node of list and return it.
"
func_result     "Removed head node or NULL if list is empty"
func_see        "AddTail(), RemHead(), RemTail()"

func_begin      RemTail
func_return     "struct Node *"
func_param      "struct List *" list
func_env        {isr}
func_short      "remove the tail node from a list"
func_long       "
Remove the tail node of list and return it.
"
func_result     "Removed tail node or NULL if list is empty"
func_see        "AddTail(), RemHead(), RemTail()"

func_begin      FindName
func_return     "struct Node *"
func_param      "const struct List *" list
func_param      "const char *" name
func_env        {isr}
func_short      "find node in list by name"
func_long       "
Return the first node in list with the given name. It is
possible continue searching for the same or another name after a
found node, by calling the function again with the found node as
the list argument.
"
func_result     "A node with name name or NULL if not found"

func_begin      RawIOInit
func_return     "void"
func_short      "initialize debug character interface"
func_see        "RawMayGetChar(), RawPutChar()"

func_begin      RawPutChar
func_return     "void"
func_param      "int" c
func_short      "output a character to the debug interface"
func_long       "
Output the character c to the debug interface.  Output is
typically unbuffered and the function may or may not block until
the character has been emitted on the character debug interface.
"
func_see        "RawIOInit(), RawMayGetChar()"

func_begin      RawMayGetChar
func_return     "int"
func_short      "get a character from the debug interface"
func_result     "the next character as unsigned char, or -1 if none."
func_see        "RawIOInit(), RawPutChar()"

func_begin      Reschedule
func_return     "void"
func_env        {isr}
func_short      "request task reschedule"
func_long       "
This function requests the task scheduler to run immediately or
at a future time. It may cause a task switch. Reschedule() is a
low-level operation and there is in general no need for direct
use in applications. It may be callable from interrupt context.

Reschedule() is similar to Switch() but may make the actual
action pending rather than performed immediately. It is called
for example by AddTask(), SetTaskPri() and Signal(). Can be
called from interrupt context.
"
func_see        "Switch()"

func_begin      Switch
func_return     "void"
func_short      "request immediate tasks reschedule"
func_long       "
Request the task scheduler to reschedule and perform task switch
immediately. Switch() is a low-level operation and there is in
general no need for direct use in applications.

Reschedule is called by Wait() and RemTask(). Must be called
from task context.
"
func_see        "Reschedule()"

# NOTE: Add new functions to the TOP of this file.

