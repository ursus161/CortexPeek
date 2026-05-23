# CortexPeek

CortexPeek is an interactive command-line debugger for Linux x86-64 ELF binaries, built on top of the `ptrace` system call. It provides a GDB-style REPL that lets you trace, inspect, and control the execution of a target process without modifying it.

The name comes from "peeking into the cortex" of a running program.

---

## Features

### Process control
- **Launch and trace**: CortexPeek forks the target binary as a child process and attaches via `ptrace`. The child calls `PTRACE_TRACEME` before `execvp`, so the debugger has full control from the very first instruction.
- **Attach to a running process**: `Process::attach(pid)` connects to an already-running process via `PTRACE_ATTACH`.
- **Continue execution**: resumes the tracee until the next stop event (breakpoint, signal, or exit).
- **Single-step**: executes one instruction at a time using `PTRACE_SINGLESTEP`.

### Breakpoints
- Set a software breakpoint at any address or symbol name with `break <addr|symbol>`.
- Breakpoints work by overwriting the target byte with `0xCC` (INT3) and restoring the original byte when disabled.
- When a breakpoint fires, the debugger backs up `RIP`, disables the breakpoint, single-steps over the original instruction, then re-enables it. The traced program never sees the INT3.

### Register inspection
- `registers` dumps all general-purpose and instruction-pointer registers of the tracee using `PTRACE_GETREGS`.


### Disassembly
- `disasm [addr|symbol] [count]` disassembles instructions starting from an address or symbol.
- If the address falls inside a known function (resolved from the symbol table), the full function body is disassembled automatically.
- Backed by **libcapstone** for accurate x86-64 decoding.

### Symbol resolution
- At startup, CortexPeek parses the ELF symbol table of the target binary using `nm`, building a `name → address` map.
- All commands that accept an address also accept a symbol name (e.g., `break main`, `disasm factorial`).

### Event history
- A `HistoryObserver` records every debug event (breakpoint hit, single-step, signal, exit) in a fixed-size ring buffer.
- `events [count]` replays the last N events from that buffer.

### Command history
- Typed commands are stored in a `History<std::string>` ring buffer.
- `history [count]` lists recent commands.
- Pressing Enter on an empty prompt repeats the last command, like GDB.

### Aliases
- Short aliases are registered for common commands: `c` → `continue`, `s` → `step`, `br` → `break`, `regs` → `registers`, `dis` → `disasm`, `e` → `events`, `h` → `help`.

### Configuration
- Settings are loaded from `data/config.txt` at startup (key=value format).
- Configurable values: `command_history_size`, `event_history_size`, `disasm_buffer_size`, `disasm_default_count`.
- `config` prints all loaded values.
- Missing or unreadable config file falls back to hardcoded defaults without crashing.

---

## Design Patterns

### Command (`include/Command.hpp`, `src/Command.cpp`)

All debugger actions implement the abstract `Command` interface (`include/Command.hpp:28`):

```cpp
class Command {
public:
    virtual void        execute(DebuggerContext& ctx, const std::vector<std::string>& args) = 0;
    virtual std::string name() const = 0;
    virtual std::string help() const = 0;
};
```

Concrete commands (`ContinueCommand`, `StepCommand`, `BreakCommand`, `RegistersCommand`, `DisassembleCommand`, `EventsCommand`, `HistoryCommand`, `HelpCommand`, `CommandsCommand`, `ConfigCommand`) each live in their own class and override `execute`. The REPL only ever holds a `std::unique_ptr<Command>` returned by the factory and calls `execute` polymorphically (`src/main.cpp:121`).

### Factory (`include/CommandFactory.hpp`, `src/CommandFactory.cpp`)

`CommandFactory` decouples command creation from command use. Commands are registered at startup with a name, help string, and a `Creator` lambda (`std::function<std::unique_ptr<Command>()>`). At runtime, `factory.create("step")` invokes the stored lambda and returns a fresh `unique_ptr<Command>`:

```cpp
factory.registerCommand("step", "single-step one instruction",
    []() { return std::make_unique<StepCommand>(); });
```

The factory also manages aliases, resolving `"s"` to `"step"` before creation. Adding a new command requires only one `registerCommand` call; the REPL loop and help system pick it up automatically.

### Observer (`include/Observer.hpp`, `include/Observers.hpp`, `src/Observer.cpp`, `src/Observers.cpp`)

`Process` inherits from `DebugEventSource` (`include/Observer.hpp:23`), which holds a list of `weak_ptr<IDebugObserver>`. Whenever a relevant process event occurs, `notify(event)` is called and all live observers receive `onEvent`.

Two concrete observers are provided:

| Observer | File | Role |
|---|---|---|
| `LogObserver` | `include/Observers.hpp:5` | Prints each event to stdout immediately |
| `HistoryObserver` | `include/Observers.hpp:10` | Stores events in a `History<DebugEvent>` ring buffer |

Both are registered in `src/main.cpp:35-38`:

```cpp
auto logger  = std::make_shared<LogObserver>();
auto histObs = std::make_shared<HistoryObserver>(config.getSize("event_history_size", 50));
proc.subscribe(logger);
proc.subscribe(histObs);
```

Observers are stored as `weak_ptr` so the event source never prevents their destruction.

### Template + Concept (`include/History.hpp`)

`History<T>` is a generic fixed-size ring buffer constrained with a C++20 concept:

```cpp
template<std::movable T>
class History { ... };
```

`std::movable` requires a move constructor, move assignment, and destructor. This is enough because `push()` uses `std::move(item)` internally, so the class also works with move-only types. The same template is instantiated for both `std::string` (command history) and `DebugEvent` (event history).

### RAII (`Breakpoint`, `Disassembler`, `Process`)

All resource-owning classes acquire in the constructor and release in the destructor, with copy suppressed:

- `Breakpoint` (`include/Breakpoint.hpp`): writes `0xCC` into the tracee on `enable()` and restores the saved byte on `disable()`.
- `Disassembler` (`include/Disassembler.hpp`): opens a Capstone handle in the constructor and closes it in the destructor. Copy is deleted because the handle cannot be duplicated.
- `Process` (`include/Process.hpp`): owns the child PID. Copy is deleted, move is allowed.

---

## Dependencies

| Dependency | Purpose |
|---|---|
| `ptrace` (Linux kernel) | Process control and memory/register access |
| `libcapstone` | x86-64 disassembly |
| `nm` (binutils) | ELF symbol table parsing |
| C++23 compiler | `std::movable` concept, structured bindings, `std::optional` |

Install libcapstone on Debian/Ubuntu:

```bash
sudo apt install libcapstone-dev
```

---

## Building

```bash
make
```

This compiles all sources under `src/` into `build/`, links against `-lcapstone`, and produces the `cortexpeek` binary. Dependency files (`.d`) are generated automatically so incremental rebuilds work correctly.

```bash
make clean   # remove build artifacts and the binary
```

Requires `g++` with C++23 support (`-std=c++23`). Tested with GCC 13+.

---

## Usage

```
./cortexpeek <program> [args...]
```

CortexPeek launches `<program>` under trace and drops into an interactive prompt:

```
(cortexpeek) break main
breakpoint set at 0x401234
(cortexpeek) continue
[breakpoint] at 0x401234
(cortexpeek) registers
rip = 0x0000000000401234   rsp = 0x00007ffd...
...
(cortexpeek) disasm main 20
0x401234  55           push rbp
0x401235  48 89 e5     mov  rbp, rsp
...
(cortexpeek) step
[single-step]
(cortexpeek) events
[0] breakpoint
[1] single-step
(cortexpeek) quit
```

### Command reference

| Command | Alias | Description |
|---|---|---|
| `continue` | `c` | Resume execution |
| `step` | `s` | Single-step one instruction |
| `break <addr\|symbol>` | `br` | Set a breakpoint |
| `registers` | `regs` | Dump all registers |
| `disasm [addr] [count]` | `dis` | Disassemble instructions |
| `events [count]` | `e` | Show debug event history |
| `history [count]` | | Show command history |
| `config` | | Show loaded configuration |
| `commands` | | List all commands and aliases |
| `help` | `h` | Show help |
| `quit` / `q` | | Exit |

An empty input repeats the previous command.

---

## Configuration

Edit `data/config.txt` to tune defaults:

```
command_history_size=100
event_history_size=50
disasm_buffer_size=65536
disasm_default_count=10
```

The file is optional. Missing keys fall back to the hardcoded defaults shown above.
