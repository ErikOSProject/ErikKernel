# ErikOS Interprocess Communication (IPC) System

ErikOS provides a message-passing based IPC mechanism structured around a set of well-defined system calls and conventions. The IPC model combines a lightweight argument stack interface with method-oriented invocation semantics, including support for broadcasting and dynamic service discovery.

## Overview

IPC in ErikOS revolves around the manipulation of argument stacks and the invocation of methods on remote processes via unique identifiers. Communication is asynchronous and follows a message-based paradigm, with each message encapsulating arguments and addressing metadata.

ErikOS is being designed to explore and test microkernel architecture in practice. This entails the architecture's modularity, performance, and fault tolerance. The kernel only contains a minimal system, while most drivers are to be implemented in the user-space. Some drivers are added to the kernel with build time switches in order to facilitate easier debugging of the kernel and early boot.

NB: This document serves as a WIP design specification for the IPC mechanism. The current state of the software may not fully represent this document and this document is subject to change.

## System Calls

The IPC system defines six core system calls:

| System Call | Description                                                                      |
| ----------- | -------------------------------------------------------------------------------- |
| `PUSH`      | Pushes an argument onto the IPC stack.                                           |
| `PEEK`      | Reads (but does not remove) the top argument from the stack.                     |
| `POP`       | Reads and removes the top argument from the stack.                               |
| `METHOD`    | Calls a method in a target process, triggering a handler thread.                 |
| `SIGNAL`    | Broadcasts a message to all listening processes.                                 |
| `TARGETED_SIGNAL` | Sends a signal to a specific process.                     |
| `EXIT`      | Terminates the current IPC handler thread (used after handling a METHOD/SIGNAL). |

### Stack Semantics

* Each process maintains an IPC argument stack.
* Arguments can be:

  * **Primitive:** A 64-bit integer.
  * **Array:** Represented as a pointer and a length pair.
* The kernel copies argument data from the caller’s address space to the receiver's during `METHOD` and `SIGNAL` calls.

---

## Method Invocation Model

### Identifiers

Method invocations are routed via three numerical identifiers:

* **PID:** Process Identifier
* **IID:** Interface Identifier
* **MID:** Method Identifier

### Syntax

```text
METHOD <PID> <IID> <MID>
```

Upon invoking `METHOD`, the kernel spawns a new thread in the target process to handle the request. The target process must register a method handler entry point via a kernel-provided API. The interface identifier is placed in `rdi`, the method identifier in `rsi`, and the caller's PID in `rdx` for the handler.

### SIGNAL Semantics

Unlike `METHOD`, `SIGNAL` sends the message to **all** listening processes. Each
listener receives the signal in a newly created thread. Signals are identified by
an interface (IID) and a signal identifier (SID):

```text
SIGNAL <IID> <SID>
```

The kernel spawns a handler thread in every process that has registered an entry
point. The interface and signal identifiers are passed to the handler through the
`rdi` and `rsi` registers, respectively, and the caller's PID is provided in `rdx`.
The `SIGNAL` call returns immediately after creating these threads; each handler
executes asynchronously and cleans up its own argument stack.

### TARGETED_SIGNAL Semantics

`TARGETED_SIGNAL` delivers a signal to a single destination process identified
by its PID while carrying the same IID/SID tuple as `SIGNAL`.

```text
TARGETED_SIGNAL <PID> <IID> <SID>
```

Only the specified process receives the signal. The handler arguments match
`SIGNAL`: `rdi` holds the interface ID, `rsi` the signal ID, and `rdx` the
sender's PID. Like `SIGNAL`, this syscall returns immediately without waiting for
the handler to finish.

---

## Name Services

ErikOS uses dedicated name services for dynamic interface and method resolution.

### PID 0 — Kernel Internal Services

* Reserved pseudo-PID: `0` (fi.erikinkinen.kernel)

### IID 0 — Local Name Service

* Interface: `fi.erikinkinen.LocalNameService`
* Purpose: Resolves interfaces and methods based on human-readable names.

| Call Pattern | Description                            | Fully Qualified Name                            |
| ------------ | -------------------------------------- | ----------------------------------------------- |
| `(*, 0, 0)`  | Resolves IID from a string name.       | `fi.erikinkinen.LocalNameService.FindInterface` |
| `(*, 0, 1)`  | Resolves MID from a string and an IID. | `fi.erikinkinen.LocalNameService.FindMethod`    |

### IID 1 — Global Name Service

* Interface: `fi.erikinkinen.GlobalNameService`

| Call Pattern | Description                                    | Fully Qualified Name                                     |
| ------------ | ---------------------------------------------- | -------------------------------------------------------- |
| `(0, 1, 0)`  | Finds PID by service name.                     | `fi.erikinkinen.GlobalNameService.FindDestination`       |
| `(0, 1, 1)`  | Registers a service name with its entry point. | `fi.erikinkinen.GlobalNameService.RegisterDestination`   |
| `(0, 1, 2)`  | Unregisters a service.                         | `fi.erikinkinen.GlobalNameService.UnregisterDestination` |

---

## Handler Threads and Entry Points

When a `METHOD` or `SIGNAL` call is received, the kernel automatically creates a new thread within the target process. This thread executes a user-defined handler function whose address must be registered via a kernel API.

* Each process may set this **entry point** via a method call to the kernel.
* The handler accesses the IPC argument stack using `POP` and `PEEK`.

---

## Example Flow

1. **Service Registration**
   A process registers a global service:

   ```text
   PUSH "my.service.name"
   PUSH handler_entry_address
   METHOD 0 1 1  ; RegisterDestination
   ```

2. **Client Service Discovery**
   A client resolves a PID for the service:

   ```text
   PUSH "my.service.name"
   METHOD 0 1 0  ; FindDestination
   => Result PID is now on top of the stack
   ```

3. **Method Invocation**
   The client pushes arguments and invokes:

   ```text
   PUSH 42
   PUSH "hello"
   METHOD <pid> <iid> <mid>
   ```

---

## Low-Level Syscall Interface

ErikOS IPC system calls are implemented through a well-defined syscall interface using enumerated types and structured arguments.

### Syscall Types

The `enum syscall_type` defines all IPC-related syscall operations available to user processes:

```c
enum syscall_type {
    SYSCALL_EXIT,    // Terminates the current handler thread
    SYSCALL_METHOD,  // Invokes a method in a target process
    SYSCALL_SIGNAL,  // Broadcasts a message to all listeners
    SYSCALL_TARGETED_SIGNAL, // Sends a signal to a single process
    SYSCALL_PUSH,    // Pushes an argument onto the IPC stack
    SYSCALL_PEEK,    // Peeks at the top argument of the IPC stack
    SYSCALL_POP,     // Pops (reads and removes) the top argument
};
```

These values are typically passed as the first argument in a syscall dispatch function, or used as selectors when invoking a unified syscall entry point (e.g., `sys_ipc(enum syscall_type, ...)`).

---

### IPC Argument Representation

IPC arguments passed via `PUSH` and manipulated via `PEEK` and `POP` are wrapped in the following structure:

```c
enum syscall_param_type {
    SYSCALL_PARAM_ARRAY,     // Arbitrary byte sequence (e.g., string or struct)
    SYSCALL_PARAM_PRIMITIVE, // 64-bit value
};

struct syscall_param {
    enum syscall_param_type type;
    size_t size;             // Size in bytes (required for arrays)
    union {
        uint64_t value;      // Used if type == SYSCALL_PARAM_PRIMITIVE
        void *array;         // Pointer to array data (copied by kernel)
    };
};
```

#### Parameter Semantics

* For `SYSCALL_PUSH`, the user process constructs a `syscall_param` and passes it to the kernel. The kernel copies any array data from userspace into a secure internal buffer.
* For `SYSCALL_PEEK` and `SYSCALL_POP`, the kernel fills a `syscall_param` structure with data from the top of the argument stack.
* The `size` field must always be valid for `SYSCALL_PARAM_ARRAY`.

---

### Method Call Structure

`SYSCALL_METHOD` takes a `syscall_method_data` structure to identify the destination:

```c
struct syscall_method_data {
    uint64_t pid;       // Target process ID
    uint64_t interface; // Interface ID (IID)
    uint64_t method;    // Method ID (MID)
};
```

This structure fully identifies the invocation target, and it must be accompanied by any arguments pushed beforehand via `SYSCALL_PUSH`.

---

### Syscall Dispatch Example

Here’s a conceptual example of how a user-level library might issue a method call:

```c
// Step 1: Push arguments
struct syscall_param arg1 = {
    .type = SYSCALL_PARAM_PRIMITIVE,
    .value = 42
};
syscall(SYSCALL_PUSH, &arg1, NULL);

const char *str = "hello";
struct syscall_param arg2 = {
    .type = SYSCALL_PARAM_ARRAY,
    .size = strlen(str),
    .array = (void *)str
};
syscall(SYSCALL_PUSH, &arg2, NULL);

// Step 2: Method call
struct syscall_method_data method = {
    .pid = 123,
    .interface = 1,
    .method = 5
};
syscall(SYSCALL_METHOD, NULL, &method);
```

---

### Kernel Behavior

* All data passed via pointers (e.g., `.array`) is copied to avoid security vulnerabilities and ensure inter-process isolation.
* The handler thread in the destination process receives the arguments on its own IPC stack and must consume them using `SYSCALL_POP`.

